#pragma once

// Small builder DSL used by the factory preset files.
// Values are plain parameter units (register values, ms, semitones); see Params.cpp.

#include "Presets.h"
#include "Params.h"

namespace PresetDSL
{
    using namespace Params;

    // Waveform nibbles for the "hit" wave (control register bits 4..7)
    constexpr int T = 1, S = 2, P = 4, N = 8;

    // Chord indices into Params::chords
    enum { cOctave, cMajor, cMinor, cMaj7, cMin7, cDom7, cSus4, cPower, cDim, cTwoOct, cMinAdd9, cFifths };

    enum { m6581 = 0, m8580 = 1 };
    enum { lfoTri, lfoSine, lfoSquare, lfoSaw, lfoRandom };

    struct B
    {
        Preset p;
        B (const char* cat, const char* name) { p.category = cat; p.name = name; }

        B& set (const juce::String& id, float v) { p.values.emplace_back (id, v); return *this; }
        B& vset (int v, const char* id, float x) { return set (vid (v, id), x); }

        B& chip (int m)              { return set (chipModel, (float) m); }
        B& poly (int n)              { return set (polyphony, (float) n); }
        B& mode (VoiceMode m)        { return set (voiceMode, (float) (int) m); }
        B& cws (int c)               { return set (combined, (float) c); }
        B& bias (float b)            { return set (filterBias, b); }
        B& gain (float db)           { return set (outGain, db); }
        B& vel (float amt)           { return set (velVolume, amt); }
        B& glide (float ms, bool leg) { return set (Params::glide, ms).set (legato, leg ? 1.0f : 0.0f); }
        B& hr (float ms)             { return set (hardRestart, ms); }
        B& noReset()                 { return set (phaseReset, 0.0f); }
        B& bend (int st)             { return set (bendRange, (float) st); }

        // Voice (0-based index)
        B& on (int v, bool b = true) { return vset (v, Params::on, b ? 1.0f : 0.0f); }
        B& wave (int v, bool t, bool s, bool pl, bool n)
        {
            return vset (v, tri, t ? 1.0f : 0.0f).vset (v, saw, s ? 1.0f : 0.0f)
                  .vset (v, pulse, pl ? 1.0f : 0.0f).vset (v, noise, n ? 1.0f : 0.0f);
        }
        B& adsr (int v, int a, int d, int s, int r)
        {
            return vset (v, attack, (float) a).vset (v, decay, (float) d).vset (v, sustain, (float) s).vset (v, release, (float) r);
        }
        B& pulseW (int v, int width, int depth = 0, float rate = 1.0f)
        {
            return vset (v, pw, (float) width).vset (v, pwmDepth, (float) depth).vset (v, pwmRate, rate);
        }
        B& tune (int v, int semis, float cents = 0.0f) { return vset (v, coarse, (float) semis).vset (v, fine, cents); }
        B& ringMod (int v)          { return vset (v, ring, 1.0f); }
        B& hardSync (int v)         { return vset (v, oscSync, 1.0f); }
        B& filt (int v)             { return vset (v, Params::filt, 1.0f); }
        B& hit (int v, int nibble, float ms) { return vset (v, hitWave, (float) nibble).vset (v, hitTime, ms); }
        B& sweepFrom (int v, float st, float ms) { return vset (v, sweep, st).vset (v, sweepTime, ms); }
        B& fixedPitch (int v)       { return vset (v, keyTrack, 0.0f); }
        B& noArp (int v)            { return vset (v, arp, 0.0f); }

        // Filter
        B& filter (int cut, int res, bool lp, bool bp, bool hp)
        {
            return set (cutoff, (float) cut).set (resonance, (float) res)
                  .set (lowPass, lp ? 1.0f : 0.0f).set (bandPass, bp ? 1.0f : 0.0f).set (highPass, hp ? 1.0f : 0.0f);
        }
        B& fenv (int amount, float a, float d, float s, float r)
        {
            return set (fEnvAmount, (float) amount).set (fAttack, a).set (fDecay, d).set (fSustain, s).set (fRelease, r);
        }
        B& fkey (float k)           { return set (fKeyTrack, k); }
        B& fvel (int amt)           { return set (fVelocity, (float) amt); }
        B& mute3()                  { return set (voice3Off, 1.0f); }

        // Modulation
        B& lfo (float rate, int shape) { return set (lfoRate, rate).set (lfoShape, (float) shape); }
        B& vibrato (float cents, float delayMs) { return set (lfoVib, cents).set (lfoDelay, delayMs); }
        B& lfoToCut (int amt)       { return set (lfoCutoff, (float) amt); }
        B& lfoToPw (int amt)        { return set (lfoPW, (float) amt); }
        B& chordArp (int chord, int frames) { return set (arpMode, 1.0f).set (arpChord, (float) chord).set (arpSpeed, (float) frames); }
        B& heldArp (int frames)     { return set (arpMode, 2.0f).set (arpSpeed, (float) frames); }


        // Compact voice setup: waveform letters ("T", "S", "P", "N" in any combination)
        // plus ADSR. Switches the voice on.
        B& voice (int v, const char* waves, int a, int d, int s, int r)
        {
            const juce::String w (waves);
            return on (v).wave (v, w.containsChar ('T'), w.containsChar ('S'), w.containsChar ('P'), w.containsChar ('N'))
                         .adsr (v, a, d, s, r);
        }
        B& filtAll()                { return filt (0).filt (1).filt (2); }

        operator Preset() const { return p; }
    };
}

// Defined in PresetsExtra.cpp and PresetsGM.cpp
void addExtraPresets (std::vector<Preset>& list);
void addGeneralMidiPresets (std::vector<Preset>& list);
