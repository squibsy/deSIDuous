// Factory presets 65-128: more originals in the main categories.
// Levels are matched afterwards by the generated PresetTrims.inc.
#include "PresetBuilder.h"

using namespace PresetDSL;

void addExtraPresets (std::vector<Preset>& list)
{
    auto add = [&list] (const B& b) { list.push_back (b.p); };

    // ================================================================ BASS
    add (B ("Bass", "Rubber Pulse").chip (m8580).poly (1).glide (40, true)
             .voice (0, "P", 0, 7, 10, 3).pulseW (0, 600, 300, 0.8f).filt (0)
             .filter (350, 10, true, false, false).fenv (700, 1, 180, 0.2f, 150));
    add (B ("Bass", "Detuned Saw Stack").chip (m6581).poly (4)
             .voice (0, "S", 0, 9, 10, 3).voice (1, "S", 0, 9, 10, 3).tune (1, 0, 7).voice (2, "S", 0, 9, 10, 3).tune (2, -12).filtAll()
             .filter (500, 8, true, false, false).fenv (600, 1, 300, 0.2f, 200));
    add (B ("Bass", "Square Sub Octave").chip (m8580).poly (1).hr (2)
             .voice (0, "P", 0, 0, 15, 3).pulseW (0, 2048).voice (1, "T", 0, 0, 15, 3).tune (1, -12));
    add (B ("Bass", "Noise Slap").chip (m6581).poly (2)
             .voice (0, "P", 0, 8, 6, 3).pulseW (0, 800).hit (0, N, 5).sweepFrom (0, 5, 30).filt (0)
             .filter (900, 6, true, false, false).fenv (600, 1, 120, 0.0f, 100));
    add (B ("Bass", "Ring Sub").chip (m8580).poly (1)
             .voice (0, "T", 0, 9, 10, 3).ringMod (0).voice (1, "T", 0, 0, 15, 3).tune (1, -12)
             .on (2, false).tune (2, -12, 3));
    add (B ("Bass", "Sync Drop").chip (m6581).poly (1).hr (2)
             .voice (0, "S", 0, 9, 8, 3).hardSync (0).tune (0, 19).sweepFrom (0, -12, 250).filt (0).on (2, false)
             .filter (1200, 4, true, false, false));
    add (B ("Bass", "Fat 8580 Reese").chip (m8580).poly (2)
             .voice (0, "S", 2, 0, 15, 4).tune (0, 0, -12).voice (1, "S", 2, 0, 15, 4).tune (1, 0, 12).filtAll()
             .filter (450, 6, true, false, false).lfo (0.3f, lfoSine).lfoToCut (100));
    add (B ("Bass", "Plucky Tri Bass").chip (m8580).poly (2)
             .voice (0, "T", 0, 7, 4, 3).hit (0, P, 10).sweepFrom (0, 12, 15));

    // ================================================================ LEAD
    add (B ("Lead", "Square Vibrato Lead").chip (m6581).poly (1).glide (30, true)
             .voice (0, "P", 0, 0, 13, 5).pulseW (0, 2048).lfo (6.0f, lfoTri).vibrato (40, 200));
    add (B ("Lead", "Saw Unison Lead").chip (m8580).poly (1).glide (25, true)
             .voice (0, "S", 0, 9, 12, 5).tune (0, 0, -8).voice (1, "S", 0, 9, 12, 5).tune (1, 0, 8)
             .voice (2, "S", 0, 9, 12, 5).tune (2, 12).vibrato (15, 400));
    add (B ("Lead", "Thin Pulse Solo").chip (m6581).poly (1)
             .voice (0, "P", 0, 9, 11, 5).pulseW (0, 300, 200, 0.5f).lfo (5.5f, lfoTri).vibrato (25, 300));
    add (B ("Lead", "Ring Siren Lead").chip (m8580).poly (1).glide (60, true)
             .voice (0, "T", 0, 0, 15, 5).ringMod (0).on (2, false).tune (2, 7).lfo (5.0f, lfoTri).vibrato (30, 300));
    add (B ("Lead", "Sync Sweep Up").chip (m6581).poly (1)
             .voice (0, "S", 0, 0, 14, 5).hardSync (0).tune (0, 24).sweepFrom (0, -12, 900).on (2, false));
    add (B ("Lead", "Filtered Lead 6581").chip (m6581).poly (1).glide (20, true)
             .voice (0, "S", 0, 0, 14, 5).voice (1, "P", 0, 0, 14, 5).pulseW (1, 1500).tune (1, 0, 5).filtAll()
             .filter (700, 11, true, false, false).fenv (600, 5, 500, 0.4f, 300).vibrato (20, 300));
    add (B ("Lead", "Chiptune Arp Lead").chip (m6581).poly (1)
             .voice (0, "P", 0, 6, 12, 4).pulseW (0, 1024).chordArp (cOctave, 2));
    add (B ("Lead", "Bell Lead").chip (m8580).poly (2)
             .voice (0, "T", 0, 10, 6, 7).ringMod (0).voice (1, "T", 0, 8, 4, 6).tune (1, 12).on (2, false).tune (2, 19, 5));
    add (B ("Lead", "Hollow Pulse Lead").chip (m8580).poly (1).glide (25, true)
             .voice (0, "P", 0, 0, 14, 5).pulseW (0, 2048, 1200, 4.0f).filt (0).filter (200, 6, false, true, false));
    add (B ("Lead", "Screaming Fifths").chip (m6581).poly (1).glide (30, true).bias (0.65f)
             .voice (0, "S", 0, 0, 15, 5).voice (1, "S", 0, 0, 15, 5).tune (1, 7).filtAll()
             .filter (1300, 10, true, false, false).vibrato (25, 300));

    // ================================================================ PAD
    add (B ("Pad", "Warm Saw Pad").chip (m8580).poly (6)
             .voice (0, "S", 10, 0, 15, 11).tune (0, 0, -6).voice (1, "S", 10, 0, 15, 11).tune (1, 0, 6).filtAll()
             .filter (380, 3, true, false, false).lfo (0.2f, lfoSine).lfoToCut (60));
    add (B ("Pad", "Glassy PWM Pad").chip (m8580).poly (6)
             .voice (0, "P", 9, 9, 13, 11).pulseW (0, 2048, 1600, 0.25f).voice (1, "T", 9, 9, 13, 11).tune (1, 12));
    add (B ("Pad", "Ring Choir Pad").chip (m8580).poly (6)
             .voice (0, "T", 11, 9, 12, 11).ringMod (0).voice (1, "P", 11, 9, 12, 11).pulseW (1, 900, 600, 0.4f).filt (1)
             .on (2, false).tune (2, 5, 10).filter (160, 8, false, true, false));
    add (B ("Pad", "Dark Drone").chip (m6581).poly (4).bias (0.4f)
             .voice (0, "S", 12, 0, 15, 12).tune (0, -12).voice (1, "P", 12, 0, 15, 12).pulseW (1, 400, 300, 0.1f).tune (1, 7).filtAll()
             .filter (350, 12, true, false, false).lfo (0.07f, lfoTri).lfoToCut (150));
    add (B ("Pad", "Shimmer Arp Pad").chip (m8580).poly (6)
             .voice (0, "T", 8, 9, 12, 10).voice (1, "P", 10, 0, 13, 10).pulseW (1, 2600, 800, 0.3f).noArp (1)
             .chordArp (cTwoOct, 2));
    add (B ("Pad", "Noise Breath Pad").chip (m8580).poly (6)
             .voice (0, "P", 11, 0, 14, 11).pulseW (0, 1800, 900, 0.2f).voice (1, "N", 12, 0, 6, 11).tune (1, 24).filt (1)
             .filter (1400, 5, false, true, false));
    add (B ("Pad", "Sync Pad").chip (m8580).poly (6)
             .voice (0, "S", 10, 0, 14, 11).hardSync (0).tune (0, 12).voice (1, "S", 10, 0, 14, 11).tune (1, 0, 7).on (2, false).filtAll()
             .filter (700, 6, true, false, false).lfo (0.15f, lfoSine).lfoToCut (200));
    add (B ("Pad", "Bright Strings 6581").chip (m6581).poly (6)
             .voice (0, "S", 9, 9, 13, 10).tune (0, 0, -9).voice (1, "S", 9, 9, 13, 10).tune (1, 0, 9)
             .voice (2, "P", 9, 9, 13, 10).pulseW (2, 2200, 900, 0.4f).tune (2, 12).filtAll()
             .filter (1300, 2, true, false, false).fenv (300, 900, 2000, 0.5f, 1500).lfo (4.5f, lfoSine).vibrato (6, 600));

    // ================================================================ ARP
    add (B ("Arp", "Minor 7 Shimmer").chip (m8580).poly (4)
             .voice (0, "T", 0, 9, 10, 8).voice (1, "P", 0, 8, 6, 7).pulseW (1, 3000).tune (1, 12, 4).chordArp (cMin7, 1));
    add (B ("Arp", "Dom7 Stab").chip (m6581).poly (4)
             .voice (0, "S", 0, 6, 0, 6).filt (0).filter (1100, 8, true, false, false).fenv (600, 1, 150, 0.0f, 100)
             .chordArp (cDom7, 1));
    add (B ("Arp", "Octave Pulse Arp").chip (m6581).poly (4)
             .voice (0, "P", 0, 0, 14, 5).pulseW (0, 1600, 600, 0.5f).chordArp (cTwoOct, 3));
    add (B ("Arp", "Fifths Ping").chip (m8580).poly (4)
             .voice (0, "T", 0, 8, 0, 8).chordArp (cFifths, 2));
    add (B ("Arp", "MinAdd9 Dream").chip (m8580).poly (4)
             .voice (0, "T", 3, 9, 10, 10).voice (1, "P", 5, 9, 8, 10).pulseW (1, 2048).tune (1, 12).chordArp (cMinAdd9, 1));
    add (B ("Arp", "Held Arp Bass+Lead").chip (m8580)
             .voice (0, "P", 0, 7, 8, 4).pulseW (0, 1300).voice (1, "S", 0, 9, 12, 4).tune (1, -12).noArp (1).filt (1)
             .filter (700, 8, true, false, false).heldArp (2));
    add (B ("Arp", "Fast Sus Arp").chip (m6581).poly (4)
             .voice (0, "P", 0, 7, 9, 5).pulseW (0, 900).chordArp (cSus4, 1));
    add (B ("Arp", "Diminished Bells").chip (m8580).poly (4)
             .voice (0, "T", 0, 9, 4, 8).ringMod (0).on (2, false).tune (2, 12).chordArp (cDim, 1));

    // ================================================================ KEYS
    add (B ("Keys", "Pulse Piano").chip (m8580).poly (6).vel (0.5f)
             .voice (0, "P", 0, 10, 2, 8).pulseW (0, 1600).voice (1, "P", 0, 9, 2, 8).pulseW (1, 2400).tune (1, 0, 6).filtAll()
             .filter (1000, 2, true, false, false).fenv (700, 1, 900, 0.2f, 600).fkey (0.6f).fvel (300));
    add (B ("Keys", "Tine Keys").chip (m8580).poly (6)
             .voice (0, "T", 0, 11, 4, 9).voice (1, "T", 0, 6, 0, 6).ringMod (1).tune (1, 24));
    add (B ("Keys", "Organ Click").chip (m8580).poly (6)
             .voice (0, "T", 0, 0, 15, 2).voice (1, "T", 0, 0, 15, 2).tune (1, 12)
             .voice (2, "P", 0, 3, 0, 0).pulseW (2, 600).tune (2, 19));
    add (B ("Keys", "Clavinet").chip (m6581).poly (6)
             .voice (0, "P", 0, 8, 2, 5).pulseW (0, 500).filt (0)
             .filter (900, 10, false, true, false).fenv (600, 1, 150, 0.1f, 100).fvel (300));
    add (B ("Keys", "Vibes").chip (m8580).poly (6)
             .voice (0, "T", 0, 11, 0, 11).voice (1, "T", 0, 8, 0, 8).tune (1, 24, 3).lfo (5.0f, lfoSine).vibrato (6, 0));
    add (B ("Keys", "Toy Piano").chip (m8580).poly (6)
             .voice (0, "P", 0, 8, 0, 7).pulseW (0, 3000).tune (0, 12).voice (1, "T", 0, 6, 0, 6).ringMod (1).tune (1, 24));

    // ================================================================ DRUMS
    add (B ("Drums", "Deep Kick 8580").chip (m8580).poly (4).vel (0.5f)
             .voice (0, "T", 0, 8, 0, 0).hit (0, N, 4).sweepFrom (0, 36, 70).tune (0, -24).fixedPitch (0));
    add (B ("Drums", "Punch Kick 6581").chip (m6581).poly (4).vel (0.5f)
             .voice (0, "P", 0, 6, 0, 0).pulseW (0, 2048).hit (0, N, 8).sweepFrom (0, 24, 50).tune (0, -21).fixedPitch (0));
    add (B ("Drums", "Clap").chip (m8580).poly (4).vel (0.5f)
             .voice (0, "N", 0, 6, 0, 0).tune (0, 30).fixedPitch (0).voice (1, "N", 0, 4, 0, 0).tune (1, 20).fixedPitch (1));
    add (B ("Drums", "Rimshot").chip (m6581).poly (4).vel (0.5f)
             .voice (0, "P", 0, 3, 0, 0).pulseW (0, 1200).tune (0, 24).fixedPitch (0).hit (0, N, 4).sweepFrom (0, 12, 15));
    add (B ("Drums", "Cowbell").chip (m8580).poly (4).vel (0.5f)
             .voice (0, "P", 0, 6, 0, 0).pulseW (0, 2048).tune (0, 13).fixedPitch (0)
             .voice (1, "P", 0, 6, 0, 0).pulseW (1, 2048).tune (1, 20).fixedPitch (1).filtAll()
             .filter (120, 6, false, true, false));
    add (B ("Drums", "Crash Cymbal").chip (m8580).poly (4).vel (0.5f)
             .voice (0, "N", 0, 11, 0, 11).tune (0, 48).fixedPitch (0).filt (0).filter (1300, 2, false, false, true));
    add (B ("Drums", "Ride Cymbal").chip (m8580).poly (4).vel (0.5f)
             .voice (0, "N", 0, 9, 0, 9).tune (0, 48).fixedPitch (0).filt (0)
             .voice (1, "T", 0, 9, 0, 9).ringMod (1).tune (1, 45).fixedPitch (1)
             .filter (1500, 2, false, false, true));
    add (B ("Drums", "Low Tom 8580").chip (m8580).poly (4).vel (0.5f)
             .voice (0, "T", 0, 9, 0, 0).hit (0, N, 6).sweepFrom (0, 10, 250).tune (0, -12));
    add (B ("Drums", "Shaker").chip (m8580).poly (4).vel (0.5f)
             .voice (0, "N", 3, 4, 0, 0).tune (0, 44).fixedPitch (0).filt (0).filter (1600, 2, false, false, true));
    add (B ("Drums", "Noise Snare 8580").chip (m8580).poly (4).vel (0.5f)
             .voice (0, "N", 0, 7, 0, 0).tune (0, 30).fixedPitch (0).hit (0, P, 15)
             .voice (1, "T", 0, 4, 0, 0).tune (1, -2).fixedPitch (1).sweepFrom (1, 5, 30));

    // ================================================================ FX
    add (B ("FX", "Wind Gust").chip (m8580).poly (2)
             .voice (0, "N", 12, 0, 15, 12).filt (0).filter (400, 12, false, true, false).lfo (0.15f, lfoSine).lfoToCut (500));
    add (B ("FX", "Sonar Ping").chip (m8580).poly (2)
             .voice (0, "T", 0, 11, 0, 11).ringMod (0).tune (0, 12).on (2, false).tune (2, 12, 30));
    add (B ("FX", "Rising Alarm").chip (m6581).poly (1)
             .voice (0, "P", 0, 0, 15, 3).pulseW (0, 2048).sweepFrom (0, -24, 800).lfo (4.0f, lfoSquare).vibrato (300, 0));
    add (B ("FX", "Zapper").chip (m6581).poly (2)
             .voice (0, "S", 0, 8, 0, 0).hardSync (0).tune (0, 36).sweepFrom (0, 24, 200).on (2, false));
    add (B ("FX", "Space Drone").chip (m8580).poly (2)
             .voice (0, "T", 12, 0, 15, 12).ringMod (0).on (2, false).tune (2, -5, 20)
             .voice (1, "N", 12, 0, 4, 12).tune (1, 36).filt (1)
             .filter (600, 13, false, true, false).lfo (0.1f, lfoSine).lfoToCut (300));
    add (B ("FX", "Jump").chip (m6581).poly (2)
             .voice (0, "P", 0, 6, 0, 0).pulseW (0, 1024).sweepFrom (0, -12, 120));
    add (B ("FX", "Hit Damage").chip (m6581).poly (2)
             .voice (0, "N", 0, 7, 0, 0).tune (0, 24).fixedPitch (0).hit (0, P, 30).sweepFrom (0, 12, 200));
    add (B ("FX", "Warp Drive").chip (m8580).poly (1)
             .voice (0, "S", 10, 0, 15, 10).hardSync (0).tune (0, 12).on (2, false).lfo (0.3f, lfoSaw).vibrato (900, 0));

    // ================================================================ WAVE
    add (B ("Wave", "Pulse Hard Sync (6581)").chip (m6581).poly (4)
             .voice (0, "P", 0, 9, 10, 6).pulseW (0, 1600).hardSync (0).tune (0, 12).on (2, false));
    add (B ("Wave", "Tri+Pulse Width Sweep (8580)").chip (m8580).poly (4)
             .voice (0, "TP", 0, 0, 15, 5).pulseW (0, 2000, 1800, 0.15f));
    add (B ("Wave", "Saw+Pulse (8580)").chip (m8580).poly (4)
             .voice (0, "SP", 0, 9, 10, 6).pulseW (0, 2000, 800, 0.3f));
    add (B ("Wave", "Tri+Saw (8580)").chip (m8580).poly (4)
             .voice (0, "TS", 0, 9, 10, 6));
    add (B ("Wave", "Ring+Sync Combo").chip (m8580).poly (4)
             .voice (0, "T", 0, 9, 10, 6).ringMod (0).hardSync (0).tune (0, 12).on (2, false).tune (2, 7));
    add (B ("Wave", "Pulse Width Sweep").chip (m6581).poly (4)
             .voice (0, "P", 0, 0, 15, 5).pulseW (0, 100, 2000, 0.25f));
}
