#include "Presets.h"
#include "Params.h"

namespace
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
        B& hardSync (int v)         { return vset (v, sync, 1.0f); }
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

        operator Preset() const { return p; }
    };

    std::vector<Preset> build()
    {
        std::vector<Preset> list;
        auto add = [&list] (const B& b) { list.push_back (b.p); };

        // ================================================================ INIT
        add (B ("Init", "Init Saw").gain (6.5f));

        // ================================================================ BASS
        add (B ("Bass", "Hubbard Pulse Bass").gain (6.5f).chip (m6581).poly (1).hr (2)
                 .wave (0, false, false, true, false).pulseW (0, 1100, 450, 0.6f).adsr (0, 0, 8, 9, 4).filt (0)
                 .filter (620, 9, true, false, false).fenv (650, 1, 320, 0.15f, 200));

        add (B ("Bass", "Sync Growl").gain (8.5f).chip (m6581).poly (1).hr (2)
                 .wave (0, false, true, false, false).hardSync (0).tune (0, 12).sweepFrom (0, 9, 420).adsr (0, 0, 10, 8, 3).filt (0)
                 .on (2, false).wave (2, true, false, false, false)
                 .filter (900, 6, true, false, false).fenv (450, 1, 500, 0.0f, 300));

        add (B ("Bass", "Galway Fat Saw").gain (-1.0f).chip (m6581).poly (4)
                 .wave (0, false, true, false, false).adsr (0, 0, 9, 10, 3).filt (0)
                 .on (1).wave (1, false, true, false, false).tune (1, 0, 9).adsr (1, 0, 9, 10, 3).filt (1)
                 .filter (560, 10, true, false, false).fenv (800, 1, 260, 0.2f, 200).fvel (200));

        add (B ("Bass", "8580 Acid Line").gain (6.0f).chip (m8580).poly (1).glide (70, true).hr (1)
                 .wave (0, false, true, false, false).adsr (0, 0, 9, 12, 2).filt (0)
                 .filter (140, 14, true, false, false).fenv (1300, 1, 220, 0.0f, 120).fvel (450));

        add (B ("Bass", "Triangle Sub").gain (6.5f).chip (m8580).poly (1).hr (2)
                 .wave (0, true, false, false, false).tune (0, -12).adsr (0, 0, 0, 15, 4));

        add (B ("Bass", "Ring Thump").gain (5.5f).chip (m6581).poly (1).hr (2)
                 .wave (0, true, false, false, false).ringMod (0).adsr (0, 0, 8, 0, 0)
                 .on (1).wave (1, false, false, true, false).pulseW (1, 800, 300, 0.5f).adsr (1, 0, 9, 5, 3).filt (1)
                 .on (2, false).tune (2, 7, 4)
                 .filter (1000, 4, true, false, false).fenv (300, 1, 200, 0.0f, 200));

        add (B ("Bass", "Plucked Pulse").gain (2.0f).chip (m8580).poly (4)
                 .wave (0, false, false, true, false).pulseW (0, 420).sweepFrom (0, 12, 25).adsr (0, 0, 7, 4, 6).filt (0)
                 .filter (380, 8, true, false, false).fenv (1000, 1, 150, 0.0f, 150));

        add (B ("Bass", "Wobble").gain (0.0f).chip (m8580).poly (1).hr (2)
                 .wave (0, false, true, false, false).adsr (0, 0, 0, 15, 3).filt (0)
                 .on (1).wave (1, false, false, true, false).tune (1, -12).pulseW (1, 2048).adsr (1, 0, 0, 15, 3).filt (1)
                 .filter (300, 12, true, false, false).lfo (4.0f, lfoSine).lfoToCut (480));

        add (B ("Bass", "Noise Snap Bass").gain (8.0f).chip (m6581).poly (2).hr (2)
                 .wave (0, false, true, false, false).hit (0, N, 8).sweepFrom (0, 5, 40).adsr (0, 0, 8, 7, 3).filt (0)
                 .filter (700, 7, true, false, false).fenv (700, 1, 180, 0.1f, 150));

        // ================================================================ LEAD
        add (B ("Lead", "Monty Pulse Lead").gain (3.5f).chip (m6581).poly (1).glide (35, true).hr (2)
                 .wave (0, false, false, true, false).pulseW (0, 2048, 1400, 0.7f).adsr (0, 0, 9, 11, 6)
                 .lfo (5.8f, lfoTri).vibrato (35, 350));

        add (B ("Lead", "Sync Scream").gain (7.0f).chip (m6581).poly (1).glide (50, true).hr (2)
                 .wave (0, false, true, false, false).hardSync (0).tune (0, 19).sweepFrom (0, 12, 800).adsr (0, 1, 9, 12, 6)
                 .on (2, false)
                 .lfo (5.5f, lfoTri).vibrato (25, 400));

        add (B ("Lead", "Tri+Saw Lead").gain (3.0f).chip (m8580).poly (1).glide (40, true)
                 .wave (0, true, true, false, false).adsr (0, 0, 9, 12, 6)
                 .on (1).wave (1, false, false, true, false).pulseW (1, 3000, 500, 0.4f).tune (1, 12, 5).adsr (1, 0, 9, 8, 6)
                 .lfo (5.2f, lfoTri).vibrato (20, 300));

        add (B ("Lead", "Saw+Pulse Snarl").gain (6.5f).chip (m8580).poly (2)
                 .wave (0, false, true, true, false).pulseW (0, 1500, 800, 0.4f).adsr (0, 0, 9, 12, 5).filt (0)
                 .filter (1400, 6, true, false, false).fenv (300, 1, 400, 0.4f, 300)
                 .lfo (5.6f, lfoTri).vibrato (30, 300));

        add (B ("Lead", "Chip Square").gain (2.5f).chip (m6581).poly (1).hr (2)
                 .wave (0, false, false, true, false).pulseW (0, 2048).adsr (0, 0, 6, 12, 4)
                 .lfo (6.0f, lfoTri).vibrato (18, 250));

        add (B ("Lead", "Sync Sweep").gain (2.0f).chip (m6581).poly (1).hr (2)
                 .wave (0, false, false, true, false).hardSync (0).pulseW (0, 1800).tune (0, 24).sweepFrom (0, -12, 600).adsr (0, 0, 9, 13, 5)
                 .on (2, false)
                 .lfo (5.0f, lfoTri).vibrato (20, 500));

        add (B ("Lead", "Ring Mod Lead").gain (3.5f).chip (m8580).poly (1).glide (25, true)
                 .wave (0, true, false, false, false).ringMod (0).adsr (0, 0, 9, 10, 6)
                 .on (1).wave (1, false, false, true, false).pulseW (1, 1024, 400, 0.3f).adsr (1, 0, 9, 9, 5)
                 .on (2, false).tune (2, 12)
                 .lfo (5.5f, lfoTri).vibrato (25, 350));

        add (B ("Lead", "Breathy Flute").gain (8.0f).chip (m8580).poly (1).glide (30, true)
                 .wave (0, true, false, false, false).adsr (0, 3, 6, 12, 6)
                 .on (1).wave (1, false, false, false, true).tune (1, 24).adsr (1, 2, 4, 2, 2).filt (1)
                 .filter (1500, 2, false, false, true)
                 .lfo (5.0f, lfoSine).vibrato (25, 450));

        add (B ("Lead", "Portamento Pulse").gain (0.5f).chip (m8580).poly (1).glide (180, true)
                 .wave (0, false, false, true, false).pulseW (0, 700, 300, 0.25f).adsr (0, 0, 9, 13, 6)
                 .on (1).wave (1, false, false, true, false).pulseW (1, 2400).tune (1, 12, 6).adsr (1, 0, 9, 9, 6)
                 .lfo (5.5f, lfoTri).vibrato (15, 600));

        add (B ("Lead", "Fuzz Lead 6581").gain (2.0f).chip (m6581).poly (1).glide (20, true).hr (2)
                 .wave (0, false, true, false, false).adsr (0, 0, 9, 13, 5).filt (0)
                 .on (1).wave (1, false, true, false, false).tune (1, 0, 12).adsr (1, 0, 9, 13, 5).filt (1)
                 .filter (1100, 11, true, false, false).bias (0.7f)
                 .lfo (5.8f, lfoTri).vibrato (30, 300));

        // ================================================================ PAD
        add (B ("Pad", "8580 String Machine").gain (-4.0f).chip (m8580).poly (6)
                 .wave (0, false, true, false, false).tune (0, 0, -8).adsr (0, 10, 9, 12, 10).filt (0)
                 .on (1).wave (1, false, true, false, false).tune (1, 0, 8).adsr (1, 10, 9, 12, 10).filt (1)
                 .on (2).wave (2, false, false, true, false).pulseW (2, 2400, 900, 0.3f).adsr (2, 10, 9, 12, 10).filt (2)
                 .filter (650, 3, true, false, false).fenv (300, 800, 2000, 0.5f, 1500)
                 .lfo (4.5f, lfoSine).vibrato (8, 600));

        add (B ("Pad", "Breathing PWM").gain (-3.5f).chip (m6581).poly (6)
                 .wave (0, false, false, true, false).pulseW (0, 2048, 1500, 0.13f).adsr (0, 11, 9, 13, 11).filt (0)
                 .on (1).wave (1, false, false, true, false).pulseW (1, 2048, 1500, 0.21f).tune (1, 12, 4).adsr (1, 11, 9, 13, 11).filt (1)
                 .on (2).wave (2, false, false, true, false).pulseW (2, 2048, 1500, 0.29f).tune (2, 7, -4).adsr (2, 11, 9, 13, 11).filt (2)
                 .filter (900, 6, true, false, false).lfo (0.15f, lfoSine).lfoToCut (120));

        add (B ("Pad", "Dark 6581").gain (-2.0f).chip (m6581).poly (6).bias (0.35f)
                 .wave (0, false, true, false, false).tune (0, 0, -6).adsr (0, 12, 0, 15, 11).filt (0)
                 .on (1).wave (1, false, true, false, false).tune (1, 0, 6).adsr (1, 12, 0, 15, 11).filt (1)
                 .filter (480, 12, true, false, false));

        add (B ("Pad", "Glass Triangles").gain (-4.0f).chip (m8580).poly (6)
                 .wave (0, true, false, false, false).adsr (0, 9, 10, 9, 11)
                 .on (1).wave (1, true, false, false, false).tune (1, 12, 5).adsr (1, 9, 10, 9, 11)
                 .on (2).wave (2, true, false, false, false).tune (2, 19, -5).adsr (2, 9, 10, 9, 11)
                 .lfo (4.0f, lfoSine).vibrato (6, 800));

        add (B ("Pad", "Band Pass Choir").gain (7.5f).chip (m8580).poly (6)
                 .wave (0, false, false, true, false).pulseW (0, 1000, 600, 0.5f).adsr (0, 11, 9, 12, 10).filt (0)
                 .on (1).wave (1, false, true, false, false).tune (1, 0, 7).adsr (1, 11, 9, 12, 10).filt (1)
                 .filter (600, 10, false, true, false).lfo (0.3f, lfoSine).lfoToCut (80));

        add (B ("Pad", "Slow Sweep").gain (-2.0f).chip (m8580).poly (6)
                 .wave (0, false, true, false, false).tune (0, 0, -5).adsr (0, 10, 9, 14, 11).filt (0)
                 .on (1).wave (1, false, true, false, false).tune (1, 0, 5).adsr (1, 10, 9, 14, 11).filt (1)
                 .on (2).wave (2, false, true, false, false).tune (2, -12).adsr (2, 10, 9, 14, 11).filt (2)
                 .filter (200, 10, true, false, false).fenv (1200, 3000, 4000, 0.3f, 2000));

        add (B ("Pad", "Ring Halo").gain (1.0f).chip (m8580).poly (6)
                 .wave (0, true, false, false, false).ringMod (0).adsr (0, 11, 10, 10, 11)
                 .on (1).wave (1, true, false, false, false).tune (1, 12, 3).adsr (1, 11, 10, 10, 11)
                 .on (2, false).tune (2, 19, 7)
                 .lfo (3.0f, lfoSine).vibrato (5, 0));

        // ================================================================ ARP / CHORD
        add (B ("Arp", "Major Chord 50Hz").gain (2.5f).chip (m6581).poly (4)
                 .wave (0, false, false, true, false).pulseW (0, 1600, 600, 0.4f).adsr (0, 0, 9, 10, 6)
                 .chordArp (cMajor, 1));

        add (B ("Arp", "Minor Chord").gain (3.5f).chip (m6581).poly (4)
                 .wave (0, false, false, true, false).pulseW (0, 1400, 700, 0.35f).adsr (0, 0, 9, 10, 6)
                 .chordArp (cMinor, 2));

        add (B ("Arp", "Octave Bounce Bass").gain (8.0f).chip (m6581).poly (1).hr (2)
                 .wave (0, false, false, true, false).pulseW (0, 900, 300, 0.5f).adsr (0, 0, 8, 8, 3).filt (0)
                 .filter (700, 8, true, false, false).fenv (500, 1, 250, 0.1f, 200)
                 .chordArp (cOctave, 2));

        add (B ("Arp", "Held Note Arpeggiator").gain (3.5f).chip (m8580)
                 .wave (0, false, false, true, false).pulseW (0, 1400, 500, 0.4f).adsr (0, 0, 8, 9, 5)
                 .on (1).wave (1, true, false, false, false).tune (1, -12).adsr (1, 0, 9, 12, 5).noArp (1)
                 .heldArp (3));

        add (B ("Arp", "Maj7 Shimmer").gain (1.5f).chip (m8580).poly (4)
                 .wave (0, false, true, false, false).adsr (0, 2, 9, 11, 9).filt (0)
                 .on (1).wave (1, false, false, true, false).pulseW (1, 2600).tune (1, 12, 6).adsr (1, 2, 9, 9, 9).filt (1)
                 .filter (1200, 5, true, false, false)
                 .chordArp (cMaj7, 1));

        add (B ("Arp", "Hubbard Chord Stab").gain (9.0f).chip (m6581).poly (4).hr (2)
                 .wave (0, false, true, false, false).adsr (0, 0, 7, 0, 6).filt (0)
                 .filter (900, 9, true, false, false).fenv (700, 1, 200, 0.0f, 200)
                 .chordArp (cMin7, 1));

        add (B ("Arp", "Power Fifths").gain (6.0f).chip (m8580).poly (4)
                 .wave (0, false, true, true, false).pulseW (0, 1800, 400, 0.3f).adsr (0, 0, 9, 11, 5)
                 .chordArp (cPower, 1));

        add (B ("Arp", "Diminished Tension").gain (2.5f).chip (m6581).poly (4)
                 .wave (0, false, false, true, false).pulseW (0, 1000, 900, 0.8f).adsr (0, 0, 9, 9, 6)
                 .chordArp (cDim, 2));

        add (B ("Arp", "Sus4 Sparkle").gain (4.5f).chip (m8580).poly (4)
                 .wave (0, true, false, false, false).adsr (0, 0, 10, 6, 9)
                 .on (1).wave (1, false, false, true, false).pulseW (1, 3200).tune (1, 12).adsr (1, 0, 8, 0, 8).noArp (1)
                 .chordArp (cSus4, 1));

        // ================================================================ KEYS
        add (B ("Keys", "Ring Bell").gain (1.0f).chip (m8580).poly (6)
                 .wave (0, true, false, false, false).ringMod (0).adsr (0, 0, 11, 0, 11)
                 .on (1).wave (1, true, false, false, false).tune (1, 12).adsr (1, 0, 10, 0, 10)
                 .on (2, false).tune (2, 19, 27));

        add (B ("Keys", "Sync Clav").gain (5.5f).chip (m6581).poly (6)
                 .wave (0, false, false, true, false).hardSync (0).pulseW (0, 700).tune (0, 12).sweepFrom (0, 12, 60).adsr (0, 0, 7, 0, 5).filt (0)
                 .on (2, false)
                 .filter (1600, 4, true, false, false));

        add (B ("Keys", "Triangle E-Piano").gain (1.5f).chip (m8580).poly (6)
                 .wave (0, true, false, false, false).adsr (0, 0, 11, 4, 9)
                 .on (1).wave (1, true, false, false, false).tune (1, 12).adsr (1, 0, 8, 0, 8)
                 .on (2).wave (2, false, false, true, false).pulseW (2, 300).tune (2, 24).adsr (2, 0, 4, 0, 4));

        add (B ("Keys", "Drawbar Organ").gain (-2.0f).chip (m8580).poly (6)
                 .wave (0, true, false, false, false).adsr (0, 0, 0, 15, 2)
                 .on (1).wave (1, true, false, false, false).tune (1, 12).adsr (1, 0, 0, 15, 2)
                 .on (2).wave (2, true, false, false, false).tune (2, 19).adsr (2, 0, 0, 15, 2)
                 .lfo (6.5f, lfoSine).vibrato (10, 0));

        add (B ("Keys", "Pulse Harpsichord").gain (1.5f).chip (m8580).poly (6)
                 .wave (0, false, true, true, false).pulseW (0, 600).adsr (0, 0, 9, 0, 8)
                 .on (1).wave (1, false, false, true, false).pulseW (1, 400).tune (1, 12).adsr (1, 0, 8, 0, 7));

        add (B ("Keys", "Music Box").gain (3.0f).chip (m8580).poly (6)
                 .wave (0, true, false, false, false).tune (0, 24).adsr (0, 0, 10, 0, 9)
                 .on (1).wave (1, true, false, false, false).ringMod (1).tune (1, 36).adsr (1, 0, 8, 0, 8));

        add (B ("Keys", "Filter Pluck").gain (-3.0f).chip (m6581).poly (6)
                 .wave (0, false, true, false, false).adsr (0, 0, 9, 3, 8).filt (0)
                 .on (1).wave (1, false, false, true, false).pulseW (1, 1200).tune (1, 0, 7).adsr (1, 0, 9, 3, 8).filt (1)
                 .filter (500, 11, true, false, false).fenv (1100, 1, 250, 0.0f, 300).fkey (0.5f).fvel (300));

        // ================================================================ DRUMS
        add (B ("Drums", "Kick").gain (8.0f).chip (m6581).poly (4).vel (0.5f)
                 .wave (0, false, false, true, false).pulseW (0, 1700).hit (0, N, 6).sweepFrom (0, 30, 90).tune (0, -24).fixedPitch (0).adsr (0, 0, 7, 0, 0));

        add (B ("Drums", "Snare").gain (5.5f).chip (m6581).poly (4).vel (0.5f)
                 .wave (0, false, false, false, true).hit (0, P, 18).tune (0, 24).fixedPitch (0).adsr (0, 0, 7, 0, 0)
                 .on (1).wave (1, true, false, false, false).tune (1, -5).sweepFrom (1, 7, 40).fixedPitch (1).adsr (1, 0, 5, 0, 0));

        add (B ("Drums", "Closed Hi-Hat").gain (3.0f).chip (m8580).poly (4).vel (0.5f)
                 .wave (0, false, false, false, true).tune (0, 48).fixedPitch (0).adsr (0, 0, 3, 0, 0).filt (0)
                 .filter (950, 2, false, false, true));

        add (B ("Drums", "Open Hi-Hat").gain (3.0f).chip (m8580).poly (4).vel (0.5f)
                 .wave (0, false, false, false, true).tune (0, 48).fixedPitch (0).adsr (0, 0, 9, 0, 8).filt (0)
                 .filter (900, 3, false, false, true));

        add (B ("Drums", "Electro Tom").gain (9.5f).chip (m6581).poly (6).vel (0.5f)
                 .wave (0, true, false, false, false).hit (0, N, 8).sweepFrom (0, 12, 220).adsr (0, 0, 9, 0, 0));

        add (B ("Drums", "Zap Drum").gain (8.0f).chip (m6581).poly (6).vel (0.5f)
                 .wave (0, false, false, true, false).pulseW (0, 1200).sweepFrom (0, 36, 50).tune (0, -12).adsr (0, 0, 8, 0, 0));

        add (B ("Drums", "Hubbard Snare Roll").gain (11.0f).chip (m6581).poly (4).vel (0.4f)
                 .wave (0, false, false, false, true).hit (0, T, 12).sweepFrom (0, -12, 60).tune (0, 24).fixedPitch (0).adsr (0, 0, 6, 0, 0));

        // ================================================================ FX
        add (B ("FX", "Laser Zap").gain (4.0f).chip (m6581).poly (4)
                 .wave (0, false, false, true, false).pulseW (0, 1024).tune (0, 12).sweepFrom (0, 36, 300).adsr (0, 0, 9, 0, 0));

        add (B ("FX", "Explosion").gain (3.5f).chip (m6581).poly (2)
                 .wave (0, false, false, false, true).tune (0, -12).fixedPitch (0).sweepFrom (0, 12, 1200).adsr (0, 0, 11, 0, 11).filt (0)
                 .on (1).wave (1, false, false, false, true).tune (1, -24).fixedPitch (1).adsr (1, 0, 12, 0, 12).filt (1)
                 .filter (300, 6, true, false, false).fenv (1200, 1, 1200, 0.0f, 800));

        add (B ("FX", "Power Up").gain (2.0f).chip (m6581).poly (2)
                 .wave (0, false, false, true, false).pulseW (0, 800).sweepFrom (0, -24, 600).adsr (0, 0, 10, 0, 6)
                 .chordArp (cMajor, 1));

        add (B ("FX", "Coin Pickup").gain (6.0f).chip (m8580).poly (2)
                 .wave (0, false, false, true, false).pulseW (0, 2048).tune (0, 12).adsr (0, 0, 8, 0, 0)
                 .chordArp (cFifths, 4));

        add (B ("FX", "Siren").gain (0.5f).chip (m6581).poly (1)
                 .wave (0, false, false, true, false).pulseW (0, 2048).adsr (0, 0, 0, 15, 5)
                 .lfo (0.6f, lfoTri).vibrato (700, 0));

        add (B ("FX", "Robot Babble").gain (6.0f).chip (m8580).poly (1)
                 .wave (0, true, false, false, false).ringMod (0).adsr (0, 0, 0, 15, 6)
                 .on (2, false).tune (2, -17, 40)
                 .lfo (12.0f, lfoRandom).vibrato (300, 0));

        add (B ("FX", "Teleport").gain (10.5f).chip (m8580).poly (1)
                 .wave (0, true, true, false, false).hardSync (0).sweepFrom (0, -24, 1500).tune (0, 12).adsr (0, 0, 0, 15, 9).filt (0)
                 .on (2, false)
                 .filter (700, 12, false, true, false).lfo (3.0f, lfoSine).lfoToCut (400));

        add (B ("FX", "Noise Wind").gain (7.0f).chip (m8580).poly (2)
                 .wave (0, false, false, false, true).adsr (0, 12, 0, 15, 12).filt (0)
                 .filter (500, 13, false, true, false).lfo (0.2f, lfoSine).lfoToCut (350));

        add (B ("FX", "Alarm").gain (5.5f).chip (m6581).poly (1)
                 .wave (0, false, true, false, false).adsr (0, 0, 0, 15, 2)
                 .lfo (8.0f, lfoSquare).vibrato (500, 0));

        // ================================================================ WAVEFORM SHOWCASE
        add (B ("Wave", "Tri+Pulse (8580)").gain (6.0f).chip (m8580).poly (4)
                 .wave (0, true, false, true, false).pulseW (0, 2600, 900, 0.3f).adsr (0, 0, 9, 10, 6));

        add (B ("Wave", "Tri+Saw+Pulse (8580)").gain (5.0f).chip (m8580).poly (4)
                 .wave (0, true, true, true, false).pulseW (0, 2200, 700, 0.3f).adsr (0, 0, 9, 10, 6));

        add (B ("Wave", "Saw+Pulse (6581)").gain (12.0f).chip (m6581).poly (4).cws (2)
                 .wave (0, false, true, true, false).pulseW (0, 2000, 800, 0.3f).adsr (0, 0, 9, 10, 6));

        add (B ("Wave", "Tri+Saw (6581)").gain (12.0f).chip (m6581).poly (4).cws (2)
                 .wave (0, true, true, false, false).adsr (0, 0, 9, 10, 6));

        add (B ("Wave", "All Three Voices").gain (0.0f).chip (m6581).poly (4)
                 .wave (0, true, false, false, false).adsr (0, 0, 9, 10, 6)
                 .on (1).wave (1, false, true, false, false).tune (1, 12).adsr (1, 0, 9, 10, 6)
                 .on (2).wave (2, false, false, true, false).tune (2, 19).pulseW (2, 1000).adsr (2, 0, 9, 10, 6));

        return list;
    }
}

const std::vector<Preset>& getFactoryPresets()
{
    static const std::vector<Preset> presets = build();
    return presets;
}

void applyPreset (juce::AudioProcessorValueTreeState& state, const Preset& preset)
{
    for (auto* p : state.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (auto& [id, value] : preset.values)
        if (auto* rp = state.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
        else
            jassertfalse;   // unknown parameter ID in preset
}
