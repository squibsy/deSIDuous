#include "Params.h"

namespace Params
{
juce::StringArray waveComboNames()
{
    // Index = waveform nibble (control register bits 4..7)
    return { "Off", "Tri", "Saw", "Tri+Saw", "Pulse", "Tri+Pulse", "Saw+Pulse", "Tri+Saw+Pulse",
             "Noise", "Noise+Tri", "Noise+Saw", "Noise+Tri+Saw", "Noise+Pulse", "Noise+Tri+Pulse",
             "Noise+Saw+Pulse", "All Four" };
}

namespace
{
    using APF = juce::AudioParameterFloat;
    using API = juce::AudioParameterInt;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;
    using Attr = juce::AudioParameterFloatAttributes;

    juce::ParameterID pid (const juce::String& s) { return { s, 1 }; }

    juce::NormalisableRange<float> skewed (float lo, float hi, float centre)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::String msText (float v, int)
    {
        return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " s" : juce::String (juce::roundToInt (v)) + " ms";
    }

    juce::String hex3 (int v, int) { return "$" + juce::String::toHexString (v).toUpperCase().paddedLeft ('0', 3); }
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ------------------------------------------------------------------ voices
    for (int v = 0; v < 3; ++v)
    {
        const juce::String n = "V" + juce::String (v + 1) + " ";
        auto group = std::make_unique<juce::AudioProcessorParameterGroup> ("voice" + juce::String (v + 1), "Voice " + juce::String (v + 1), "|");

        group->addChild (std::make_unique<APB> (pid (vid (v, on)),    n + "On",       v == 0));
        group->addChild (std::make_unique<APB> (pid (vid (v, tri)),   n + "Triangle", false));
        group->addChild (std::make_unique<APB> (pid (vid (v, saw)),   n + "Sawtooth", v == 0));
        group->addChild (std::make_unique<APB> (pid (vid (v, pulse)), n + "Pulse",    v != 0));
        group->addChild (std::make_unique<APB> (pid (vid (v, noise)), n + "Noise",    false));
        group->addChild (std::make_unique<APB> (pid (vid (v, ring)),  n + "Ring Mod", false));
        group->addChild (std::make_unique<APB> (pid (vid (v, oscSync)),  n + "Hard Sync", false));
        group->addChild (std::make_unique<APB> (pid (vid (v, test)),  n + "Test Bit", false));
        group->addChild (std::make_unique<APB> (pid (vid (v, filt)),  n + "Filter Route", false));

        group->addChild (std::make_unique<API> (pid (vid (v, coarse)), n + "Coarse", -48, 48, 0,
                                                juce::AudioParameterIntAttributes().withLabel ("st")));
        group->addChild (std::make_unique<APF> (pid (vid (v, fine)), n + "Fine", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f,
                                                Attr().withLabel ("ct")));
        group->addChild (std::make_unique<API> (pid (vid (v, pw)), n + "Pulse Width", 0, 4095, 2048,
                                                juce::AudioParameterIntAttributes().withStringFromValueFunction (
                                                    [] (int x, int) { return juce::String (x / 40.95f, 1) + "%"; })));
        group->addChild (std::make_unique<APF> (pid (vid (v, pwmRate)), n + "PWM Rate", skewed (0.0f, 20.0f, 2.0f), 1.0f,
                                                Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2) + " Hz"; })));
        group->addChild (std::make_unique<API> (pid (vid (v, pwmDepth)), n + "PWM Depth", 0, 2047, 0));

        auto adsrText = [] (const float* table) {
            return juce::AudioParameterIntAttributes().withStringFromValueFunction (
                [table] (int x, int) { return juce::String::toHexString (x).toUpperCase() + "  " + msText (table[x], 0); });
        };
        group->addChild (std::make_unique<API> (pid (vid (v, attack)),  n + "Attack",  0, 15, 0, adsrText (attackMs)));
        group->addChild (std::make_unique<API> (pid (vid (v, decay)),   n + "Decay",   0, 15, 9, adsrText (decayMs)));
        group->addChild (std::make_unique<API> (pid (vid (v, sustain)), n + "Sustain", 0, 15, 10,
                                                juce::AudioParameterIntAttributes().withStringFromValueFunction (
                                                    [] (int x, int) { return juce::String::toHexString (x).toUpperCase() + "  " + juce::String (juce::roundToInt (x * 100.0f / 15.0f)) + "%"; })));
        group->addChild (std::make_unique<API> (pid (vid (v, release)), n + "Release", 0, 15, 8, adsrText (decayMs)));

        group->addChild (std::make_unique<APC> (pid (vid (v, hitWave)), n + "Hit Wave", waveComboNames(), 0));
        group->addChild (std::make_unique<APF> (pid (vid (v, hitTime)), n + "Hit Time", skewed (0.0f, 250.0f, 30.0f), 20.0f,
                                                Attr().withStringFromValueFunction (msText)));
        group->addChild (std::make_unique<APF> (pid (vid (v, sweep)), n + "Pitch Sweep", juce::NormalisableRange<float> (-48.0f, 48.0f, 0.1f), 0.0f,
                                                Attr().withLabel ("st")));
        group->addChild (std::make_unique<APF> (pid (vid (v, sweepTime)), n + "Sweep Time", skewed (1.0f, 3000.0f, 200.0f), 150.0f,
                                                Attr().withStringFromValueFunction (msText)));
        group->addChild (std::make_unique<APB> (pid (vid (v, keyTrack)), n + "Key Track", true));
        group->addChild (std::make_unique<APB> (pid (vid (v, arp)), n + "Arp", true));

        layout.add (std::move (group));
    }

    // ------------------------------------------------------------------ global
    {
        auto g = std::make_unique<juce::AudioProcessorParameterGroup> ("global", "Global", "|");
        g->addChild (std::make_unique<APC> (pid (chipModel), "Chip Model", juce::StringArray { "MOS 6581", "MOS 8580" }, 0));
        g->addChild (std::make_unique<APC> (pid (voiceMode), "Voice Mode", juce::StringArray { "Layered Poly", "Paraphonic (3 notes)", "MIDI Ch 1-3" }, 0));
        g->addChild (std::make_unique<API> (pid (polyphony), "Polyphony", 1, 8, 4));
        g->addChild (std::make_unique<APC> (pid (clock), "Clock", juce::StringArray { "PAL 985248 Hz", "NTSC 1022730 Hz" }, 0));
        g->addChild (std::make_unique<APC> (pid (combined), "Combined Waves", juce::StringArray { "Average", "Weak", "Strong" }, 0));
        g->addChild (std::make_unique<APF> (pid (filterBias), "Filter Bias", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        g->addChild (std::make_unique<API> (pid (volume), "SID Volume", 0, 15, 15));
        g->addChild (std::make_unique<APF> (pid (velVolume), "Velocity > Volume", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));
        g->addChild (std::make_unique<APF> (pid (outGain), "Output Gain", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
                                            Attr().withLabel ("dB")));
        g->addChild (std::make_unique<APF> (pid (glide), "Glide", skewed (0.0f, 2000.0f, 150.0f), 0.0f,
                                            Attr().withStringFromValueFunction (msText)));
        g->addChild (std::make_unique<APB> (pid (legato), "Legato", false));
        g->addChild (std::make_unique<API> (pid (bendRange), "Bend Range", 0, 24, 2,
                                            juce::AudioParameterIntAttributes().withLabel ("st")));
        g->addChild (std::make_unique<APF> (pid (hardRestart), "Hard Restart", juce::NormalisableRange<float> (0.0f, 40.0f, 0.1f), 0.0f,
                                            Attr().withStringFromValueFunction (msText)));
        g->addChild (std::make_unique<APB> (pid (phaseReset), "Osc Reset On Note", true));
        g->addChild (std::make_unique<APC> (pid (quality), "Resampling", juce::StringArray { "High (Sinc)", "Fast (Decimate)" }, 0));
        layout.add (std::move (g));
    }

    // ------------------------------------------------------------------ filter
    {
        auto g = std::make_unique<juce::AudioProcessorParameterGroup> ("filter", "Filter", "|");
        g->addChild (std::make_unique<API> (pid (cutoff), "Cutoff", 0, 2047, 1024,
                                            juce::AudioParameterIntAttributes().withStringFromValueFunction (hex3)));
        g->addChild (std::make_unique<API> (pid (resonance), "Resonance", 0, 15, 4));
        g->addChild (std::make_unique<APB> (pid (lowPass),  "Low Pass",  true));
        g->addChild (std::make_unique<APB> (pid (bandPass), "Band Pass", false));
        g->addChild (std::make_unique<APB> (pid (highPass), "High Pass", false));
        g->addChild (std::make_unique<APB> (pid (voice3Off), "Voice 3 Off", false));
        g->addChild (std::make_unique<API> (pid (fEnvAmount), "Filter Env Amount", -2047, 2047, 0));
        g->addChild (std::make_unique<APF> (pid (fAttack),  "Filter Env Attack",  skewed (0.0f, 5000.0f, 300.0f), 2.0f,
                                            Attr().withStringFromValueFunction (msText)));
        g->addChild (std::make_unique<APF> (pid (fDecay),   "Filter Env Decay",   skewed (1.0f, 10000.0f, 500.0f), 300.0f,
                                            Attr().withStringFromValueFunction (msText)));
        g->addChild (std::make_unique<APF> (pid (fSustain), "Filter Env Sustain", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        g->addChild (std::make_unique<APF> (pid (fRelease), "Filter Env Release", skewed (1.0f, 10000.0f, 500.0f), 300.0f,
                                            Attr().withStringFromValueFunction (msText)));
        g->addChild (std::make_unique<APF> (pid (fKeyTrack), "Filter Key Track", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        g->addChild (std::make_unique<API> (pid (fVelocity), "Velocity > Cutoff", -2047, 2047, 0));
        layout.add (std::move (g));
    }

    // ------------------------------------------------------------------ modulation
    {
        auto g = std::make_unique<juce::AudioProcessorParameterGroup> ("mod", "Modulation", "|");
        g->addChild (std::make_unique<APF> (pid (lfoRate), "LFO Rate", skewed (0.05f, 30.0f, 5.0f), 5.5f, Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2) + " Hz"; })));
        g->addChild (std::make_unique<APC> (pid (lfoShape), "LFO Shape", juce::StringArray { "Triangle", "Sine", "Square", "Saw Down", "Random" }, 0));
        g->addChild (std::make_unique<APF> (pid (lfoVib), "Vibrato Depth", skewed (0.0f, 1200.0f, 60.0f), 0.0f, Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " ct"; })));
        g->addChild (std::make_unique<APF> (pid (lfoDelay), "Vibrato Delay", skewed (0.0f, 3000.0f, 400.0f), 0.0f,
                                            Attr().withStringFromValueFunction (msText)));
        g->addChild (std::make_unique<API> (pid (lfoCutoff), "LFO > Cutoff", 0, 1024, 0));
        g->addChild (std::make_unique<API> (pid (lfoPW), "LFO > Pulse Width", 0, 2047, 0));

        juce::StringArray chordNames;
        for (auto& c : chords) chordNames.add (c.name);
        g->addChild (std::make_unique<APC> (pid (arpMode), "Arp Mode", juce::StringArray { "Off", "Chord", "Held Notes" }, 0));
        g->addChild (std::make_unique<APC> (pid (arpChord), "Arp Chord", chordNames, 1));
        g->addChild (std::make_unique<API> (pid (arpSpeed), "Arp Speed", 1, 16, 1,
                                            juce::AudioParameterIntAttributes().withStringFromValueFunction (
                                                [] (int x, int) { return juce::String (x) + (x == 1 ? " frame" : " frames"); })));
        layout.add (std::move (g));
    }

    return layout;
}
}
