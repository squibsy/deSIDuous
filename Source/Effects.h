#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

// Plain snapshot of the effect parameters, taken once per audio block.
struct FxSettings
{
    bool chorus1 = false, chorus2 = false;
    bool delayOn = false, delaySync = true;
    int delayDivision = 5;          // index into Effects::divisionNames()
    float delayMs = 350.0f, delayFeedback = 0.35f, delayMix = 0.25f;
    bool reverbOn = false;
    float reverbSize = 0.5f, reverbDamping = 0.5f, reverbMix = 0.2f;
    double bpm = 120.0;
};

//==============================================================================
// Post-chip effects: Judo chorus, then delay, then reverb.
class Effects
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const FxSettings& s);

    static const juce::StringArray& divisionNames();
    static double divisionBeats (int index);

private:
    // Circular buffer with a fractional (linear interpolated) read.
    struct Line
    {
        std::vector<float> data;
        int write = 0;
        void setSize (int n) { data.assign ((size_t) juce::jmax (4, n), 0.0f); write = 0; }
        void clear() { std::fill (data.begin(), data.end(), 0.0f); }
        void push (float x) { data[(size_t) write] = x; if (++write == (int) data.size()) write = 0; }
        float read (double delaySamples) const;   // delay measured back from the last push
    };

    struct OnePole
    {
        float z = 0.0f, a = 0.0f;
        void setCutoff (double hz, double sr) { a = (float) std::exp (-juce::MathConstants<double>::twoPi * hz / sr); }
        float process (float x) { z = x + a * (z - x); return z; }
    };

    void processChorus (float* l, float* r, int n, const FxSettings& s);
    void processDelay (float* l, float* r, int n, const FxSettings& s);
    void processReverb (float* l, float* r, int n, int numChannels, const FxSettings& s);

    double sr = 44100.0;

    // chorus: one bucket-brigade line read at two mirrored taps (left / right)
    Line chorusLine;
    OnePole chorusTone[2];
    double chorusPhase = 0.0;
    double chorusCentre = 0.0035, chorusHalf = 0.00185, chorusRate = 0.513;   // seconds, seconds, Hz (smoothed)
    juce::SmoothedValue<float> chorusAmount;

    // delay
    Line delayLine[2];
    OnePole delayDamp[2];
    double delaySamples = 0.0;
    juce::SmoothedValue<float> delayMix, delayFeedback;
    bool delayActive = false;

    // reverb
    juce::Reverb reverb;
    juce::SmoothedValue<float> reverbMix;
    bool reverbActive = false;
    juce::AudioBuffer<float> wet;
};
