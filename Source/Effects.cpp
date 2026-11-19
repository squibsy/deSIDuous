#include "Effects.h"

namespace
{
    // Judo chorus timings, after the bucket-brigade chorus of a classic 1980s analogue polysynth (an MN3009-type circuit).
    struct ChorusMode { double rateHz, minMs, maxMs; };
    constexpr ChorusMode chorusI     { 0.513, 1.66, 5.35 };
    constexpr ChorusMode chorusII    { 0.863, 1.66, 5.35 };
    constexpr ChorusMode chorusBoth  { 9.75,  3.30, 3.70 };   // I + II together (the fast, shallow setting)

    constexpr double maxDelaySeconds = 4.0;
}

const juce::StringArray& Effects::divisionNames()
{
    static const juce::StringArray names { "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/1" };
    return names;
}

double Effects::divisionBeats (int index)
{
    static constexpr double beats[] = { 0.125, 1.0 / 6.0, 0.25, 0.375, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0, 4.0 };
    return beats[juce::jlimit (0, (int) std::size (beats) - 1, index)];
}

float Effects::Line::read (double delaySamples) const
{
    const int size = (int) data.size();
    double pos = (double) write - 1.0 - delaySamples;
    while (pos < 0.0) pos += size;
    const int i0 = (int) pos;
    const int i1 = i0 + 1 == size ? 0 : i0 + 1;
    const float frac = (float) (pos - i0);
    return data[(size_t) i0] + (data[(size_t) i1] - data[(size_t) i0]) * frac;
}

//==============================================================================
void Effects::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    chorusLine.setSize ((int) (sr * 0.01) + 4);
    for (auto& t : chorusTone) t.setCutoff (9000.0, sr);   // BBD clock filtering
    for (auto& l : delayLine) l.setSize ((int) (sr * maxDelaySeconds) + 4);
    for (auto& d : delayDamp) d.setCutoff (5500.0, sr);     // darker repeats

    chorusAmount.reset (sr, 0.03);
    delayMix.reset (sr, 0.05);
    delayFeedback.reset (sr, 0.05);
    reverbMix.reset (sr, 0.05);

    reverb.setSampleRate (sr);
    wet.setSize (2, juce::jmax (1, maxBlockSize));
    reset();
}

void Effects::reset()
{
    chorusLine.clear();
    for (auto& l : delayLine) l.clear();
    for (auto& t : chorusTone) t.z = 0.0f;
    for (auto& d : delayDamp) d.z = 0.0f;
    reverb.reset();
    // Start from zero: the targets are only known at the next process() call, so
    // anything that is switched on fades in over a few ms instead of a stale setting
    // fading out.
    chorusAmount.setCurrentAndTargetValue (0.0f);
    delayMix.setCurrentAndTargetValue (0.0f);
    reverbMix.setCurrentAndTargetValue (0.0f);
    delayActive = reverbActive = false;
}

void Effects::process (juce::AudioBuffer<float>& buffer, const FxSettings& s)
{
    const int n = buffer.getNumSamples();
    const int channels = juce::jmin (2, buffer.getNumChannels());
    if (channels == 0 || n == 0)
        return;
    if (wet.getNumSamples() < n)
        wet.setSize (2, n, false, false, true);   // host sent a bigger block than promised

    float* l = buffer.getWritePointer (0);
    float* r = channels > 1 ? buffer.getWritePointer (1) : nullptr;

    processChorus (l, r, n, s);
    processDelay (l, r, n, s);
    processReverb (l, r, n, channels, s);
}

//==============================================================================
// Judo chorus: the signal goes through one delay line whose length is swept by a
// triangle LFO. The left output reads it at centre + sweep and the right at
// centre - sweep, so the two sides move in opposite directions, which is where the
// the wide stereo comes from.
void Effects::processChorus (float* l, float* r, int n, const FxSettings& s)
{
    const bool on = s.chorus1 || s.chorus2;
    chorusAmount.setTargetValue (on ? 1.0f : 0.0f);

    const auto& mode = (s.chorus1 && s.chorus2) ? chorusBoth : (s.chorus2 ? chorusII : chorusI);
    const double targetCentre = (mode.minMs + mode.maxMs) * 0.0005;
    const double targetHalf = (mode.maxMs - mode.minMs) * 0.0005;
    const double k = 1.0 - std::exp (-1.0 / (0.05 * sr));   // 50 ms glide between modes

    const bool active = on || chorusAmount.isSmoothing() || chorusAmount.getCurrentValue() > 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float dry = r != nullptr ? 0.5f * (l[i] + r[i]) : l[i];
        chorusLine.push (dry);
        if (! active)
            continue;

        chorusCentre += (targetCentre - chorusCentre) * k;
        chorusHalf += (targetHalf - chorusHalf) * k;
        chorusRate += (mode.rateHz - chorusRate) * k;
        chorusPhase += chorusRate / sr;
        chorusPhase -= std::floor (chorusPhase);
        const double tri = 1.0 - 4.0 * std::abs (chorusPhase - 0.5);

        const float wetL = chorusTone[0].process (chorusLine.read ((chorusCentre + chorusHalf * tri) * sr));
        const float wetR = chorusTone[1].process (chorusLine.read ((chorusCentre - chorusHalf * tri) * sr));

        // roughly level-matched: dry and wet summed, about -3 dB each at full amount
        const float a = chorusAmount.getNextValue();
        const float dryGain = 1.0f - 0.29f * a, wetGain = 0.71f * a;
        if (r != nullptr)
        {
            l[i] = l[i] * dryGain + wetL * wetGain;
            r[i] = r[i] * dryGain + wetR * wetGain;
        }
        else
        {
            l[i] = l[i] * dryGain + 0.5f * (wetL + wetR) * wetGain;
        }
    }
}

//==============================================================================
void Effects::processDelay (float* l, float* r, int n, const FxSettings& s)
{
    delayMix.setTargetValue (s.delayOn ? s.delayMix : 0.0f);
    delayFeedback.setTargetValue (juce::jlimit (0.0f, 0.95f, s.delayFeedback));

    const double seconds = s.delaySync ? divisionBeats (s.delayDivision) * 60.0 / juce::jlimit (20.0, 400.0, s.bpm)
                                       : s.delayMs * 0.001;
    const double target = juce::jlimit (1.0, (double) delayLine[0].data.size() - 3.0, seconds * sr);

    if (! s.delayOn && ! delayMix.isSmoothing())
    {
        if (delayActive)
        {
            for (auto& line : delayLine) line.clear();
            delayActive = false;
        }
        return;
    }
    if (! delayActive)
    {
        delayActive = true;
        delaySamples = target;   // start at the right time instead of gliding to it
    }

    const double k = 1.0 - std::exp (-1.0 / (0.08 * sr));   // time changes glide like tape
    for (int i = 0; i < n; ++i)
    {
        delaySamples += (target - delaySamples) * k;
        const float mix = delayMix.getNextValue();
        const float fb = delayFeedback.getNextValue();

        const float yl = delayLine[0].read (delaySamples);
        delayLine[0].push (l[i] + fb * delayDamp[0].process (yl));
        l[i] = l[i] * (1.0f - mix) + yl * mix;

        if (r != nullptr)
        {
            const float yr = delayLine[1].read (delaySamples);
            delayLine[1].push (r[i] + fb * delayDamp[1].process (yr));
            r[i] = r[i] * (1.0f - mix) + yr * mix;
        }
    }
}

//==============================================================================
void Effects::processReverb (float* l, float* r, int n, int numChannels, const FxSettings& s)
{
    reverbMix.setTargetValue (s.reverbOn ? s.reverbMix : 0.0f);

    if (! s.reverbOn && ! reverbMix.isSmoothing())
    {
        if (reverbActive)
        {
            reverb.reset();
            reverbActive = false;
        }
        return;
    }
    reverbActive = true;

    juce::Reverb::Parameters p;
    p.roomSize = juce::jlimit (0.0f, 1.0f, s.reverbSize);
    p.damping = juce::jlimit (0.0f, 1.0f, s.reverbDamping);
    p.wetLevel = 1.0f / 3.0f;   // juce::Reverb scales wet by 3: this is unity
    p.dryLevel = 0.0f;
    p.width = 1.0f;
    p.freezeMode = 0.0f;
    reverb.setParameters (p);

    float* wl = wet.getWritePointer (0);
    float* wr = wet.getWritePointer (1);
    juce::FloatVectorOperations::copy (wl, l, n);
    if (numChannels > 1)
    {
        juce::FloatVectorOperations::copy (wr, r, n);
        reverb.processStereo (wl, wr, n);
    }
    else
    {
        reverb.processMono (wl, n);
    }

    for (int i = 0; i < n; ++i)
    {
        const float mix = reverbMix.getNextValue();
        l[i] = l[i] * (1.0f - mix) + wl[i] * mix;
        if (numChannels > 1)
            r[i] = r[i] * (1.0f - mix) + wr[i] * mix;
    }
}
