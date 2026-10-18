// Factory presets 129-256: SID interpretations of the 128 General MIDI instruments,
// numbered and named after the GM sound set. Levels are matched by PresetTrims.inc.
#include "PresetBuilder.h"

using namespace PresetDSL;

void addGeneralMidiPresets (std::vector<Preset>& list)
{
    auto add = [&list] (const B& b) { list.push_back (b.p); };

    // ================================================================ PIANO
    add (B ("GM Piano", "001 Acoustic Grand Piano").chip (m8580).poly (6).vel (0.5f)
             .voice (0, "P", 0, 10, 3, 8).pulseW (0, 1700).voice (1, "S", 0, 9, 2, 8).tune (1, 0, 5).filtAll()
             .filter (600, 2, true, false, false).fenv (900, 1, 1200, 0.15f, 600).fkey (0.6f).fvel (400));
    add (B ("GM Piano", "002 Bright Acoustic Piano").chip (m8580).poly (6).vel (0.5f)
             .voice (0, "P", 0, 10, 3, 8).pulseW (0, 1700).voice (1, "P", 0, 9, 2, 8).pulseW (1, 1200).tune (1, 0, 5).filtAll()
             .filter (900, 2, true, false, false).fenv (1000, 1, 1000, 0.2f, 600).fkey (0.6f).fvel (400));
    add (B ("GM Piano", "003 Electric Grand Piano").chip (m8580).poly (6).vel (0.4f)
             .voice (0, "P", 0, 10, 4, 8).pulseW (0, 1400).filt (0).voice (1, "T", 0, 8, 2, 8).tune (1, 12)
             .filter (800, 3, true, false, false).fenv (700, 1, 800, 0.2f, 500).fvel (300));
    add (B ("GM Piano", "004 Honky-tonk Piano").chip (m6581).poly (6).vel (0.4f)
             .voice (0, "P", 0, 10, 3, 8).pulseW (0, 1700).tune (0, 0, -14).voice (1, "P", 0, 10, 3, 8).pulseW (1, 1700).tune (1, 0, 14).filtAll()
             .filter (900, 2, true, false, false).fenv (600, 1, 900, 0.2f, 500));
    add (B ("GM Piano", "005 Electric Piano 1").chip (m8580).poly (6).vel (0.4f)
             .voice (0, "T", 0, 11, 5, 9).voice (1, "T", 0, 6, 0, 6).ringMod (1).tune (1, 24));
    add (B ("GM Piano", "006 Electric Piano 2").chip (m8580).poly (6).vel (0.4f)
             .voice (0, "T", 0, 10, 4, 9).voice (1, "T", 0, 7, 0, 7).ringMod (1).tune (1, 19));
    add (B ("GM Piano", "007 Harpsichord").chip (m8580).poly (6)
             .voice (0, "SP", 0, 9, 0, 7).pulseW (0, 600).voice (1, "P", 0, 8, 0, 6).pulseW (1, 400).tune (1, 12));
    add (B ("GM Piano", "008 Clavinet").chip (m6581).poly (6)
             .voice (0, "P", 0, 8, 3, 4).pulseW (0, 450).filt (0)
             .filter (1000, 10, false, true, false).fenv (500, 1, 120, 0.2f, 80).fvel (300));

    // ================================================================ CHROMATIC PERCUSSION
    add (B ("GM Chromatic", "009 Celesta").chip (m8580).poly (6)
             .voice (0, "T", 0, 9, 0, 9).tune (0, 12).voice (1, "T", 0, 7, 0, 7).tune (1, 24));
    add (B ("GM Chromatic", "010 Glockenspiel").chip (m8580).poly (6)
             .voice (0, "T", 0, 10, 0, 10).tune (0, 24).voice (1, "T", 0, 6, 0, 6).ringMod (1).tune (1, 36));
    add (B ("GM Chromatic", "011 Music Box").chip (m8580).poly (6)
             .voice (0, "T", 0, 9, 0, 9).tune (0, 12).hit (0, P, 4).voice (1, "T", 0, 7, 0, 7).ringMod (1).tune (1, 31));
    add (B ("GM Chromatic", "012 Vibraphone").chip (m8580).poly (6)
             .voice (0, "T", 0, 11, 2, 11).voice (1, "T", 0, 9, 0, 9).tune (1, 12).lfo (5.5f, lfoSine).vibrato (8, 0));
    add (B ("GM Chromatic", "013 Marimba").chip (m8580).poly (6)
             .voice (0, "T", 0, 7, 0, 6).voice (1, "T", 0, 4, 0, 3).tune (1, 12));
    add (B ("GM Chromatic", "014 Xylophone").chip (m8580).poly (6)
             .voice (0, "T", 0, 6, 0, 5).tune (0, 12).hit (0, N, 2).voice (1, "T", 0, 4, 0, 3).tune (1, 24));
    add (B ("GM Chromatic", "015 Tubular Bells").chip (m8580).poly (6)
             .voice (0, "T", 0, 12, 0, 12).ringMod (0).on (2, false).tune (2, 9, 30).voice (1, "T", 0, 11, 0, 11).tune (1, 12));
    add (B ("GM Chromatic", "016 Dulcimer").chip (m6581).poly (6)
             .voice (0, "P", 0, 9, 0, 8).pulseW (0, 900).tune (0, 0, -7).voice (1, "P", 0, 9, 0, 8).pulseW (1, 1200).tune (1, 0, 7).filtAll()
             .filter (1300, 3, true, false, false));

    // ================================================================ ORGAN
    add (B ("GM Organ", "017 Drawbar Organ").chip (m8580).poly (6)
             .voice (0, "T", 0, 0, 15, 1).voice (1, "T", 0, 0, 15, 1).tune (1, 12).voice (2, "T", 0, 0, 15, 1).tune (2, 19)
             .lfo (6.5f, lfoSine).vibrato (8, 0));
    add (B ("GM Organ", "018 Percussive Organ").chip (m8580).poly (6)
             .voice (0, "T", 0, 0, 15, 1).voice (1, "T", 0, 0, 15, 1).tune (1, 12).voice (2, "T", 0, 7, 0, 2).tune (2, 24));
    add (B ("GM Organ", "019 Rock Organ").chip (m6581).poly (6).bias (0.6f)
             .voice (0, "P", 0, 0, 15, 1).pulseW (0, 2048).voice (1, "T", 0, 0, 15, 1).tune (1, 12)
             .voice (2, "S", 0, 0, 15, 1).tune (2, 19).filtAll().filter (1400, 4, true, false, false)
             .lfo (6.8f, lfoSine).vibrato (12, 0));
    add (B ("GM Organ", "020 Church Organ").chip (m8580).poly (6)
             .voice (0, "P", 6, 0, 15, 6).pulseW (0, 2048).tune (0, -12).voice (1, "S", 6, 0, 15, 6)
             .voice (2, "T", 6, 0, 15, 6).tune (2, 12).filtAll().filter (1100, 2, true, false, false));
    add (B ("GM Organ", "021 Reed Organ").chip (m8580).poly (6)
             .voice (0, "P", 4, 0, 15, 4).pulseW (0, 700).voice (1, "P", 4, 0, 13, 4).pulseW (1, 900).tune (1, 12, 4).filtAll()
             .filter (800, 5, true, false, false));
    add (B ("GM Organ", "022 Accordion").chip (m8580).poly (6)
             .voice (0, "P", 3, 0, 15, 3).pulseW (0, 600).tune (0, 0, -10).voice (1, "P", 3, 0, 15, 3).pulseW (1, 600).tune (1, 0, 10)
             .voice (2, "P", 3, 0, 12, 3).pulseW (2, 1200).tune (2, 12).filtAll()
             .filter (1000, 3, true, false, false).lfo (5.0f, lfoSine).vibrato (6, 0));
    add (B ("GM Organ", "023 Harmonica").chip (m8580).poly (1).glide (15, true)
             .voice (0, "P", 3, 0, 14, 4).pulseW (0, 500).filt (0).filter (250, 4, false, true, false)
             .lfo (5.5f, lfoSine).vibrato (20, 300));
    add (B ("GM Organ", "024 Tango Accordion").chip (m6581).poly (6)
             .voice (0, "P", 3, 0, 15, 3).pulseW (0, 700).tune (0, 0, -16).voice (1, "P", 3, 0, 15, 3).pulseW (1, 700).tune (1, 0, 16)
             .voice (2, "P", 3, 0, 15, 3).pulseW (2, 400).tune (2, -12).filtAll().filter (900, 4, true, false, false));

    // ================================================================ GUITAR
    add (B ("GM Guitar", "025 Acoustic Guitar (nylon)").chip (m8580).poly (6).vel (0.4f)
             .voice (0, "P", 0, 9, 0, 7).pulseW (0, 1800).filt (0).voice (1, "T", 0, 10, 0, 8)
             .filter (500, 2, true, false, false).fenv (600, 1, 400, 0.0f, 300).fvel (200));
    add (B ("GM Guitar", "026 Acoustic Guitar (steel)").chip (m8580).poly (6).vel (0.4f)
             .voice (0, "S", 0, 10, 0, 8).voice (1, "P", 0, 9, 0, 7).pulseW (1, 900).tune (1, 0, 6).filtAll()
             .filter (700, 3, true, false, false).fenv (900, 1, 500, 0.1f, 300).fkey (0.5f));
    add (B ("GM Guitar", "027 Electric Guitar (jazz)").chip (m8580).poly (6)
             .voice (0, "P", 0, 10, 4, 6).pulseW (0, 1600).filt (0)
             .filter (450, 4, true, false, false).fenv (300, 1, 400, 0.2f, 200));
    add (B ("GM Guitar", "028 Electric Guitar (clean)").chip (m8580).poly (6)
             .voice (0, "P", 0, 10, 6, 6).pulseW (0, 1000).voice (1, "S", 0, 9, 4, 6).tune (1, 0, 4).filtAll()
             .filter (900, 3, true, false, false).fenv (400, 1, 300, 0.3f, 200));
    add (B ("GM Guitar", "029 Electric Guitar (muted)").chip (m8580).poly (6)
             .voice (0, "P", 0, 4, 0, 2).pulseW (0, 800).filt (0)
             .filter (600, 6, true, false, false).fenv (600, 1, 80, 0.0f, 60));
    add (B ("GM Guitar", "030 Overdriven Guitar").chip (m6581).poly (4).bias (0.7f)
             .voice (0, "SP", 0, 0, 14, 4).pulseW (0, 1400).voice (1, "S", 0, 0, 14, 4).tune (1, 0, 9).filtAll()
             .filter (1200, 10, true, false, false));
    add (B ("GM Guitar", "031 Distortion Guitar").chip (m6581).poly (4).bias (0.75f)
             .voice (0, "S", 0, 0, 15, 4).tune (0, 0, -10).voice (1, "S", 0, 0, 15, 4).tune (1, 0, 10)
             .voice (2, "S", 0, 0, 15, 4).tune (2, 12).filtAll().filter (1500, 13, true, false, false).vibrato (10, 500));
    add (B ("GM Guitar", "032 Guitar Harmonics").chip (m8580).poly (6)
             .voice (0, "T", 0, 10, 0, 9).tune (0, 12).voice (1, "T", 0, 8, 0, 8).ringMod (1).tune (1, 19));

    // ================================================================ BASS
    add (B ("GM Bass", "033 Acoustic Bass").chip (m8580).poly (2)
             .voice (0, "T", 0, 9, 0, 5).hit (0, P, 6));
    add (B ("GM Bass", "034 Electric Bass (finger)").chip (m8580).poly (2)
             .voice (0, "T", 0, 10, 6, 4).voice (1, "P", 0, 7, 2, 3).pulseW (1, 800).filt (1)
             .filter (500, 2, true, false, false).fenv (300, 1, 200, 0.0f, 100));
    add (B ("GM Bass", "035 Electric Bass (pick)").chip (m6581).poly (2)
             .voice (0, "S", 0, 10, 5, 3).filt (0).filter (600, 4, true, false, false).fenv (700, 1, 150, 0.1f, 100));
    add (B ("GM Bass", "036 Fretless Bass").chip (m8580).poly (1).glide (80, true)
             .voice (0, "T", 2, 0, 15, 4).voice (1, "P", 3, 10, 6, 4).pulseW (1, 1500).filt (1)
             .filter (400, 6, true, false, false).vibrato (8, 400));
    add (B ("GM Bass", "037 Slap Bass 1").chip (m6581).poly (2)
             .voice (0, "P", 0, 8, 5, 3).pulseW (0, 1200).hit (0, N, 4).sweepFrom (0, 7, 20).filt (0)
             .filter (1000, 8, true, false, false).fenv (900, 1, 100, 0.2f, 80));
    add (B ("GM Bass", "038 Slap Bass 2").chip (m8580).poly (2)
             .voice (0, "SP", 0, 8, 4, 3).pulseW (0, 1000).hit (0, N, 5).filt (0)
             .filter (1200, 10, true, false, false).fenv (800, 1, 90, 0.15f, 60));
    add (B ("GM Bass", "039 Synth Bass 1").chip (m8580).poly (1)
             .voice (0, "S", 0, 9, 8, 3).filt (0).filter (300, 12, true, false, false).fenv (1100, 1, 250, 0.1f, 150));
    add (B ("GM Bass", "040 Synth Bass 2").chip (m6581).poly (1)
             .voice (0, "P", 0, 0, 15, 3).pulseW (0, 1000, 500, 0.7f).voice (1, "P", 0, 0, 15, 3).pulseW (1, 2048).tune (1, -12, 5).filtAll()
             .filter (500, 9, true, false, false).fenv (600, 1, 300, 0.3f, 200));

    // ================================================================ STRINGS
    add (B ("GM Strings", "041 Violin").chip (m8580).poly (1).glide (30, true)
             .voice (0, "S", 5, 0, 15, 6).voice (1, "P", 6, 0, 13, 6).pulseW (1, 900).tune (1, 0, 5).filtAll()
             .filter (1100, 3, true, false, false).lfo (5.5f, lfoSine).vibrato (25, 400));
    add (B ("GM Strings", "042 Viola").chip (m8580).poly (1).glide (30, true)
             .voice (0, "S", 5, 0, 15, 6).voice (1, "P", 6, 0, 13, 6).pulseW (1, 1100).tune (1, 0, 5).filtAll()
             .filter (850, 3, true, false, false).lfo (5.2f, lfoSine).vibrato (22, 450));
    add (B ("GM Strings", "043 Cello").chip (m8580).poly (1).glide (40, true)
             .voice (0, "S", 6, 0, 15, 7).voice (1, "S", 6, 0, 15, 7).tune (1, 0, 6).filtAll()
             .filter (600, 3, true, false, false).lfo (5.0f, lfoSine).vibrato (20, 450));
    add (B ("GM Strings", "044 Contrabass").chip (m8580).poly (1).glide (40, true)
             .voice (0, "S", 6, 0, 15, 6).tune (0, -12).filt (0).filter (450, 4, true, false, false).vibrato (15, 500));
    add (B ("GM Strings", "045 Tremolo Strings").chip (m8580).poly (6)
             .voice (0, "S", 3, 0, 15, 6).tune (0, 0, -6).voice (1, "S", 3, 0, 15, 6).tune (1, 0, 6).filtAll()
             .filter (900, 3, true, false, false).lfo (9.0f, lfoTri).lfoToCut (250));
    add (B ("GM Strings", "046 Pizzicato Strings").chip (m8580).poly (6)
             .voice (0, "S", 0, 7, 0, 6).voice (1, "S", 0, 7, 0, 6).tune (1, 0, 7).filtAll()
             .filter (600, 3, true, false, false).fenv (600, 1, 150, 0.0f, 100));
    add (B ("GM Strings", "047 Orchestral Harp").chip (m8580).poly (6)
             .voice (0, "T", 0, 10, 0, 10).hit (0, P, 4).voice (1, "P", 0, 8, 0, 8).pulseW (1, 2400).tune (1, 12));
    add (B ("GM Strings", "048 Timpani").chip (m6581).poly (4)
             .voice (0, "T", 0, 10, 0, 10).hit (0, N, 10).sweepFrom (0, 3, 120).tune (0, -12).filt (0)
             .filter (500, 4, true, false, false));

    // ================================================================ ENSEMBLE
    add (B ("GM Ensemble", "049 String Ensemble 1").chip (m8580).poly (6)
             .voice (0, "S", 8, 0, 15, 9).tune (0, 0, -7).voice (1, "S", 8, 0, 15, 9).voice (2, "S", 8, 0, 15, 9).tune (2, 0, 7).filtAll()
             .filter (800, 2, true, false, false).lfo (4.5f, lfoSine).vibrato (6, 600));
    add (B ("GM Ensemble", "050 String Ensemble 2").chip (m8580).poly (6)
             .voice (0, "S", 11, 0, 15, 11).tune (0, 0, -7).voice (1, "S", 11, 0, 15, 11).voice (2, "S", 11, 0, 15, 11).tune (2, 0, 7).filtAll()
             .filter (600, 2, true, false, false).lfo (4.5f, lfoSine).vibrato (6, 800));
    add (B ("GM Ensemble", "051 Synth Strings 1").chip (m6581).poly (6)
             .voice (0, "P", 9, 0, 15, 10).pulseW (0, 2048, 1400, 0.35f).voice (1, "S", 9, 0, 15, 10).tune (1, 0, 8).filtAll()
             .filter (900, 4, true, false, false).fenv (400, 900, 2000, 0.5f, 1500));
    add (B ("GM Ensemble", "052 Synth Strings 2").chip (m8580).poly (6)
             .voice (0, "S", 10, 0, 15, 10).tune (0, 0, -10).voice (1, "S", 10, 0, 15, 10).tune (1, 0, 10)
             .voice (2, "P", 10, 0, 15, 10).pulseW (2, 1500, 1000, 0.2f).tune (2, 12).filtAll()
             .filter (1300, 3, true, false, false));
    add (B ("GM Ensemble", "053 Choir Aahs").chip (m8580).poly (6)
             .voice (0, "P", 9, 0, 15, 10).pulseW (0, 1000, 400, 0.3f).voice (1, "S", 9, 0, 15, 10).tune (1, 0, 6).filtAll()
             .filter (160, 9, false, true, false).lfo (5.0f, lfoSine).vibrato (10, 500));
    add (B ("GM Ensemble", "054 Voice Oohs").chip (m8580).poly (6)
             .voice (0, "T", 9, 0, 15, 10).voice (1, "T", 9, 0, 10, 10).tune (1, 12, 4).filtAll()
             .filter (500, 8, true, false, false).vibrato (8, 500));
    add (B ("GM Ensemble", "055 Synth Voice").chip (m8580).poly (6)
             .voice (0, "P", 6, 0, 15, 8).pulseW (0, 700, 500, 0.4f).filt (0)
             .filter (200, 12, false, true, false).lfo (0.3f, lfoSine).lfoToCut (40).vibrato (12, 400));
    add (B ("GM Ensemble", "056 Orchestra Hit").chip (m6581).poly (4)
             .voice (0, "S", 0, 8, 0, 7).hit (0, N, 8).voice (1, "S", 0, 8, 0, 7).tune (1, 7).voice (2, "S", 0, 8, 0, 7).tune (2, 12).filtAll()
             .filter (1200, 4, true, false, false).fenv (600, 1, 300, 0.0f, 200));

    // ================================================================ BRASS
    add (B ("GM Brass", "057 Trumpet").chip (m8580).poly (1).glide (25, true)
             .voice (0, "S", 3, 0, 15, 4).filt (0).filter (500, 4, true, false, false).fenv (900, 80, 400, 0.6f, 200).fvel (300)
             .lfo (5.5f, lfoSine).vibrato (15, 350));
    add (B ("GM Brass", "058 Trombone").chip (m8580).poly (1).glide (60, true)
             .voice (0, "S", 4, 0, 15, 4).filt (0).filter (380, 4, true, false, false).fenv (700, 120, 500, 0.55f, 250).fvel (250)
             .vibrato (12, 400));
    add (B ("GM Brass", "059 Tuba").chip (m8580).poly (1).glide (40, true)
             .voice (0, "S", 4, 0, 15, 4).filt (0).voice (1, "T", 4, 0, 15, 4).tune (1, -12)
             .filter (250, 5, true, false, false).fenv (500, 120, 400, 0.5f, 200));
    add (B ("GM Brass", "060 Muted Trumpet").chip (m8580).poly (1).glide (25, true)
             .voice (0, "S", 2, 0, 15, 4).filt (0).filter (350, 5, false, true, false).fenv (150, 60, 300, 0.6f, 200)
             .vibrato (12, 350));
    add (B ("GM Brass", "061 French Horn").chip (m8580).poly (4)
             .voice (0, "S", 5, 0, 15, 6).voice (1, "T", 5, 0, 15, 6).filtAll()
             .filter (330, 3, true, false, false).fenv (350, 200, 600, 0.6f, 300).vibrato (8, 500));
    add (B ("GM Brass", "062 Brass Section").chip (m8580).poly (6)
             .voice (0, "S", 4, 0, 15, 5).tune (0, 0, -8).voice (1, "S", 4, 0, 15, 5).tune (1, 0, 8)
             .voice (2, "S", 4, 0, 15, 5).tune (2, -12).filtAll()
             .filter (450, 4, true, false, false).fenv (900, 100, 500, 0.5f, 250).fvel (300));
    add (B ("GM Brass", "063 Synth Brass 1").chip (m6581).poly (6)
             .voice (0, "S", 2, 9, 12, 4).tune (0, 0, -10).voice (1, "S", 2, 9, 12, 4).tune (1, 0, 10).filtAll()
             .filter (300, 6, true, false, false).fenv (1100, 60, 400, 0.4f, 200));
    add (B ("GM Brass", "064 Synth Brass 2").chip (m8580).poly (6)
             .voice (0, "P", 5, 0, 15, 6).pulseW (0, 1500, 600, 0.4f).voice (1, "S", 5, 0, 15, 6).tune (1, 0, 7).filtAll()
             .filter (400, 8, true, false, false).fenv (800, 300, 900, 0.5f, 400));

    // ================================================================ REED
    add (B ("GM Reed", "065 Soprano Sax").chip (m8580).poly (1).glide (30, true)
             .voice (0, "P", 3, 0, 15, 4).pulseW (0, 900).filt (0).filter (250, 7, false, true, false)
             .fenv (300, 60, 300, 0.6f, 200).lfo (5.5f, lfoSine).vibrato (18, 300));
    add (B ("GM Reed", "066 Alto Sax").chip (m8580).poly (1).glide (30, true)
             .voice (0, "P", 3, 0, 15, 4).pulseW (0, 1100).filt (0).filter (200, 7, false, true, false)
             .fenv (300, 60, 300, 0.6f, 200).lfo (5.5f, lfoSine).vibrato (18, 300));
    add (B ("GM Reed", "067 Tenor Sax").chip (m8580).poly (1).glide (30, true)
             .voice (0, "SP", 3, 0, 15, 4).pulseW (0, 1300).filt (0).filter (250, 6, true, false, false)
             .fenv (300, 60, 300, 0.6f, 200).lfo (5.2f, lfoSine).vibrato (15, 350));
    add (B ("GM Reed", "068 Baritone Sax").chip (m8580).poly (1).glide (30, true)
             .voice (0, "SP", 3, 0, 15, 4).pulseW (0, 1500).filt (0).filter (180, 6, true, false, false)
             .fenv (250, 60, 300, 0.6f, 200).vibrato (10, 400));
    add (B ("GM Reed", "069 Oboe").chip (m8580).poly (1).glide (20, true)
             .voice (0, "P", 3, 0, 15, 4).pulseW (0, 350).filt (0).filter (300, 9, false, true, false).vibrato (15, 400));
    add (B ("GM Reed", "070 English Horn").chip (m8580).poly (1).glide (20, true)
             .voice (0, "P", 3, 0, 15, 4).pulseW (0, 450).filt (0).filter (230, 8, false, true, false).vibrato (12, 500));
    add (B ("GM Reed", "071 Bassoon").chip (m8580).poly (1).glide (20, true)
             .voice (0, "P", 3, 0, 15, 4).pulseW (0, 400).filt (0).filter (600, 6, true, false, false).vibrato (8, 500));
    add (B ("GM Reed", "072 Clarinet").chip (m8580).poly (1).glide (20, true)
             .voice (0, "P", 2, 0, 15, 4).pulseW (0, 2048).filt (0).filter (900, 2, true, false, false).vibrato (8, 500));

    // ================================================================ PIPE
    add (B ("GM Pipe", "073 Piccolo").chip (m8580).poly (1).glide (20, true)
             .voice (0, "T", 2, 0, 15, 4).tune (0, 12).voice (1, "N", 1, 3, 1, 1).tune (1, 36).filt (1)
             .filter (1600, 2, false, false, true).lfo (5.5f, lfoSine).vibrato (15, 300));
    add (B ("GM Pipe", "074 Flute").chip (m8580).poly (1).glide (25, true)
             .voice (0, "T", 3, 0, 15, 5).voice (1, "N", 2, 4, 2, 2).tune (1, 24).filt (1)
             .filter (1500, 2, false, false, true).lfo (5.0f, lfoSine).vibrato (20, 400));
    add (B ("GM Pipe", "075 Recorder").chip (m8580).poly (1)
             .voice (0, "T", 2, 0, 15, 3).voice (1, "P", 2, 0, 4, 3).pulseW (1, 300).tune (1, 12));
    add (B ("GM Pipe", "076 Pan Flute").chip (m8580).poly (2)
             .voice (0, "T", 3, 0, 14, 5).voice (1, "N", 2, 5, 3, 3).tune (1, 20).filt (1)
             .filter (900, 10, false, true, false).vibrato (10, 500));
    add (B ("GM Pipe", "077 Blown Bottle").chip (m8580).poly (2)
             .voice (0, "T", 5, 0, 13, 5).voice (1, "N", 5, 0, 6, 4).tune (1, 12).filt (1)
             .filter (45, 14, false, true, false).fkey (1.0f));
    add (B ("GM Pipe", "078 Shakuhachi").chip (m8580).poly (1)
             .voice (0, "T", 4, 0, 15, 5).sweepFrom (0, -1, 200).voice (1, "N", 3, 6, 4, 3).tune (1, 24).filt (1)
             .filter (1000, 8, false, true, false).lfo (4.5f, lfoSine).vibrato (25, 600));
    add (B ("GM Pipe", "079 Whistle").chip (m8580).poly (1).glide (40, true)
             .voice (0, "T", 2, 0, 15, 3).tune (0, 24).lfo (6.0f, lfoSine).vibrato (20, 250));
    add (B ("GM Pipe", "080 Ocarina").chip (m8580).poly (2)
             .voice (0, "T", 2, 0, 15, 4).tune (0, 12).vibrato (8, 400));

    // ================================================================ SYNTH LEAD
    add (B ("GM Synth Lead", "081 Lead 1 (square)").chip (m6581).poly (1).glide (20, true)
             .voice (0, "P", 0, 0, 15, 4).pulseW (0, 2048).vibrato (10, 400));
    add (B ("GM Synth Lead", "082 Lead 2 (sawtooth)").chip (m8580).poly (1).glide (20, true)
             .voice (0, "S", 0, 0, 15, 4).tune (0, 0, -5).voice (1, "S", 0, 0, 15, 4).tune (1, 0, 5).filtAll()
             .filter (1400, 4, true, false, false));
    add (B ("GM Synth Lead", "083 Lead 3 (calliope)").chip (m8580).poly (1)
             .voice (0, "TP", 1, 0, 15, 3).pulseW (0, 2600).voice (1, "T", 1, 0, 15, 3).tune (1, 12).vibrato (18, 200));
    add (B ("GM Synth Lead", "084 Lead 4 (chiff)").chip (m8580).poly (1)
             .voice (0, "P", 0, 0, 15, 4).pulseW (0, 1500).hit (0, N, 12).voice (1, "T", 0, 0, 15, 4).tune (1, 12));
    add (B ("GM Synth Lead", "085 Lead 5 (charang)").chip (m6581).poly (1).bias (0.7f)
             .voice (0, "SP", 0, 0, 15, 4).pulseW (0, 1200).voice (1, "S", 0, 0, 15, 4).tune (1, 0, 8).filtAll()
             .filter (1300, 11, true, false, false));
    add (B ("GM Synth Lead", "086 Lead 6 (voice)").chip (m8580).poly (1).glide (30, true)
             .voice (0, "P", 4, 0, 15, 5).pulseW (0, 900, 400, 0.4f).filt (0).filter (180, 12, false, true, false)
             .vibrato (20, 400));
    add (B ("GM Synth Lead", "087 Lead 7 (fifths)").chip (m8580).poly (1)
             .voice (0, "S", 0, 0, 15, 4).voice (1, "S", 0, 0, 15, 4).tune (1, 7).filtAll()
             .filter (1200, 5, true, false, false));
    add (B ("GM Synth Lead", "088 Lead 8 (bass + lead)").chip (m8580).poly (1).glide (15, true)
             .voice (0, "S", 0, 0, 15, 4).voice (1, "P", 0, 0, 15, 4).pulseW (1, 1200).tune (1, -12).filtAll()
             .filter (900, 6, true, false, false).fenv (400, 1, 300, 0.3f, 200));

    // ================================================================ SYNTH PAD
    add (B ("GM Synth Pad", "089 Pad 1 (new age)").chip (m8580).poly (6)
             .voice (0, "T", 8, 0, 15, 10).voice (1, "T", 0, 10, 4, 10).ringMod (1).tune (1, 19));
    add (B ("GM Synth Pad", "090 Pad 2 (warm)").chip (m8580).poly (6)
             .voice (0, "S", 10, 0, 15, 11).tune (0, 0, -6).voice (1, "S", 10, 0, 15, 11).tune (1, 0, 6).filtAll()
             .filter (350, 3, true, false, false));
    add (B ("GM Synth Pad", "091 Pad 3 (polysynth)").chip (m6581).poly (6)
             .voice (0, "S", 2, 9, 11, 9).tune (0, 0, -8).voice (1, "P", 2, 9, 11, 9).pulseW (1, 1600, 700, 0.4f).tune (1, 0, 8).filtAll()
             .filter (400, 6, true, false, false).fenv (800, 5, 800, 0.3f, 600));
    add (B ("GM Synth Pad", "092 Pad 4 (choir)").chip (m8580).poly (6)
             .voice (0, "P", 10, 0, 15, 11).pulseW (0, 1000, 500, 0.3f).voice (1, "T", 10, 0, 15, 11).tune (1, 12).filtAll()
             .filter (160, 10, false, true, false).vibrato (8, 600));
    add (B ("GM Synth Pad", "093 Pad 5 (bowed)").chip (m8580).poly (6)
             .voice (0, "S", 11, 0, 15, 10).filt (0).filter (140, 9, false, true, false).lfo (0.25f, lfoSine).lfoToCut (50));
    add (B ("GM Synth Pad", "094 Pad 6 (metallic)").chip (m8580).poly (6)
             .voice (0, "S", 9, 0, 15, 10).hardSync (0).tune (0, 19).voice (1, "T", 9, 0, 13, 10).ringMod (1).tune (1, 12)
             .on (2, false).tune (2, 7));
    add (B ("GM Synth Pad", "095 Pad 7 (halo)").chip (m8580).poly (6)
             .voice (0, "T", 11, 0, 14, 11).ringMod (0).on (2, false).tune (2, 12, 6)
             .voice (1, "T", 12, 0, 10, 11).tune (1, 24).vibrato (5, 0));
    add (B ("GM Synth Pad", "096 Pad 8 (sweep)").chip (m8580).poly (6)
             .voice (0, "S", 9, 0, 15, 10).tune (0, 0, -6).voice (1, "S", 9, 0, 15, 10).tune (1, 0, 6).filtAll()
             .filter (150, 11, true, false, false).fenv (1300, 2500, 3000, 0.2f, 2000));

    // ================================================================ SYNTH FX
    add (B ("GM Synth FX", "097 FX 1 (rain)").chip (m8580).poly (2)
             .voice (0, "N", 9, 0, 15, 10).filt (0).filter (500, 6, false, false, true).lfo (9.0f, lfoRandom).lfoToCut (300));
    add (B ("GM Synth FX", "098 FX 2 (soundtrack)").chip (m8580).poly (6)
             .voice (0, "S", 12, 0, 15, 12).tune (0, 0, -8).voice (1, "P", 12, 0, 15, 12).pulseW (1, 2048, 1000, 0.15f).tune (1, 7).filtAll()
             .filter (250, 9, true, false, false).fenv (1000, 4000, 4000, 0.4f, 3000));
    add (B ("GM Synth FX", "099 FX 3 (crystal)").chip (m8580).poly (4)
             .voice (0, "T", 0, 10, 0, 10).tune (0, 12).voice (1, "T", 0, 8, 0, 8).ringMod (1).tune (1, 24).chordArp (cOctave, 2));
    add (B ("GM Synth FX", "100 FX 4 (atmosphere)").chip (m8580).poly (6)
             .voice (0, "T", 10, 0, 15, 11).voice (1, "N", 11, 0, 5, 11).tune (1, 30).filt (1)
             .filter (1000, 10, false, true, false).lfo (0.2f, lfoSine).lfoToCut (300));
    add (B ("GM Synth FX", "101 FX 5 (brightness)").chip (m8580).poly (6)
             .voice (0, "S", 8, 0, 15, 10).voice (1, "SP", 8, 0, 13, 10).pulseW (1, 2048).tune (1, 12).filtAll()
             .filter (400, 4, false, false, true));
    add (B ("GM Synth FX", "102 FX 6 (goblins)").chip (m6581).poly (2)
             .voice (0, "P", 6, 0, 15, 9).pulseW (0, 1000).filt (0).filter (600, 12, false, true, false)
             .lfo (7.0f, lfoRandom).vibrato (250, 0).lfoToCut (300));
    add (B ("GM Synth FX", "103 FX 7 (echoes)").chip (m8580).poly (4)
             .voice (0, "P", 0, 9, 6, 10).pulseW (0, 1600).voice (1, "T", 6, 9, 4, 10).tune (1, 12).chordArp (cOctave, 4));
    add (B ("GM Synth FX", "104 FX 8 (sci-fi)").chip (m8580).poly (2)
             .voice (0, "T", 6, 0, 15, 9).ringMod (0).on (2, false).tune (2, 5).lfo (3.0f, lfoRandom).vibrato (120, 0));

    // ================================================================ ETHNIC
    add (B ("GM Ethnic", "105 Sitar").chip (m8580).poly (4)
             .voice (0, "S", 0, 10, 3, 8).hardSync (0).tune (0, 12).sweepFrom (0, 0.6f, 300).on (2, false).filt (0)
             .filter (300, 8, false, true, false).vibrato (10, 600));
    add (B ("GM Ethnic", "106 Banjo").chip (m8580).poly (6)
             .voice (0, "P", 0, 7, 0, 5).pulseW (0, 500).voice (1, "P", 0, 6, 0, 4).pulseW (1, 700).tune (1, 12, 6));
    add (B ("GM Ethnic", "107 Shamisen").chip (m8580).poly (4)
             .voice (0, "P", 0, 6, 0, 4).pulseW (0, 600).hit (0, N, 3).filt (0).filter (300, 4, false, false, true));
    add (B ("GM Ethnic", "108 Koto").chip (m8580).poly (6)
             .voice (0, "TP", 0, 9, 0, 8).pulseW (0, 2400).sweepFrom (0, 0.5f, 80).voice (1, "T", 0, 8, 0, 7).tune (1, 12));
    add (B ("GM Ethnic", "109 Kalimba").chip (m8580).poly (6)
             .voice (0, "T", 0, 8, 0, 7).hit (0, P, 3).voice (1, "T", 0, 5, 0, 5).ringMod (1).tune (1, 24));
    add (B ("GM Ethnic", "110 Bag pipe").chip (m6581).poly (1).glide (20, true)
             .voice (0, "P", 2, 0, 15, 3).pulseW (0, 500)
             .voice (1, "P", 2, 0, 15, 3).pulseW (1, 600).tune (1, -15).fixedPitch (1)
             .voice (2, "P", 2, 0, 13, 3).pulseW (2, 600).tune (2, -8).fixedPitch (2).filtAll()
             .filter (1100, 6, false, true, false));
    add (B ("GM Ethnic", "111 Fiddle").chip (m8580).poly (1).glide (30, true)
             .voice (0, "S", 2, 0, 15, 4).voice (1, "S", 2, 0, 13, 4).tune (1, 0, 9).filtAll()
             .filter (1300, 5, true, false, false).lfo (6.0f, lfoSine).vibrato (30, 250));
    add (B ("GM Ethnic", "112 Shanai").chip (m8580).poly (1).glide (50, true)
             .voice (0, "P", 2, 0, 15, 4).pulseW (0, 350).filt (0).filter (350, 12, false, true, false)
             .lfo (6.5f, lfoSine).vibrato (35, 200));

    // ================================================================ PERCUSSIVE
    add (B ("GM Percussive", "113 Tinkle Bell").chip (m8580).poly (6)
             .voice (0, "T", 0, 9, 0, 9).ringMod (0).tune (0, 24).on (2, false).tune (2, 31, 20));
    add (B ("GM Percussive", "114 Agogo").chip (m8580).poly (6)
             .voice (0, "T", 0, 6, 0, 6).tune (0, 12).hit (0, N, 2).voice (1, "T", 0, 5, 0, 5).ringMod (1).tune (1, 19));
    add (B ("GM Percussive", "115 Steel Drums").chip (m8580).poly (6)
             .voice (0, "T", 0, 9, 0, 8).voice (1, "T", 0, 8, 0, 7).ringMod (1).tune (1, 12, 20));
    add (B ("GM Percussive", "116 Woodblock").chip (m8580).poly (6)
             .voice (0, "P", 0, 4, 0, 2).pulseW (0, 1000).tune (0, 24).sweepFrom (0, 3, 15));
    add (B ("GM Percussive", "117 Taiko Drum").chip (m6581).poly (4).vel (0.5f)
             .voice (0, "T", 0, 9, 0, 8).tune (0, -12).hit (0, N, 12).sweepFrom (0, 7, 150).filt (0)
             .filter (400, 5, true, false, false));
    add (B ("GM Percussive", "118 Melodic Tom").chip (m6581).poly (6).vel (0.5f)
             .voice (0, "T", 0, 9, 0, 7).hit (0, N, 6).sweepFrom (0, 12, 200));
    add (B ("GM Percussive", "119 Synth Drum").chip (m6581).poly (6).vel (0.5f)
             .voice (0, "P", 0, 8, 0, 6).pulseW (0, 2048).sweepFrom (0, 24, 120).tune (0, -12));
    add (B ("GM Percussive", "120 Reverse Cymbal").chip (m8580).poly (2)
             .voice (0, "N", 12, 0, 15, 1).tune (0, 44).fixedPitch (0).filt (0).filter (500, 2, false, false, true));

    // ================================================================ SOUND EFFECTS
    add (B ("GM Sound FX", "121 Guitar Fret Noise").chip (m8580).poly (2)
             .voice (0, "N", 0, 4, 0, 3).tune (0, 20).sweepFrom (0, 12, 100).filt (0).filter (900, 10, false, true, false));
    add (B ("GM Sound FX", "122 Breath Noise").chip (m8580).poly (2)
             .voice (0, "N", 8, 0, 12, 8).tune (0, 24).filt (0).filter (1000, 6, false, true, false));
    add (B ("GM Sound FX", "123 Seashore").chip (m8580).poly (2)
             .voice (0, "N", 12, 0, 15, 12).tune (0, 12).fixedPitch (0).filt (0)
             .filter (500, 4, true, false, false).lfo (0.12f, lfoSine).lfoToCut (400));
    add (B ("GM Sound FX", "124 Bird Tweet").chip (m8580).poly (2)
             .voice (0, "T", 0, 6, 0, 4).tune (0, 36).sweepFrom (0, -7, 60).lfo (14.0f, lfoSine).vibrato (250, 0));
    add (B ("GM Sound FX", "125 Telephone Ring").chip (m8580).poly (1)
             .voice (0, "P", 0, 0, 15, 1).pulseW (0, 2048).tune (0, 24).lfo (16.0f, lfoSquare).vibrato (300, 0));
    add (B ("GM Sound FX", "126 Helicopter").chip (m6581).poly (1)
             .voice (0, "N", 6, 0, 15, 6).tune (0, -12).fixedPitch (0).filt (0)
             .filter (400, 10, true, false, false).lfo (12.0f, lfoSquare).lfoToCut (300));
    add (B ("GM Sound FX", "127 Applause").chip (m8580).poly (1)
             .voice (0, "N", 8, 0, 15, 10).tune (0, 36).fixedPitch (0).filt (0)
             .filter (1400, 3, false, true, false).lfo (18.0f, lfoRandom).lfoToCut (400));
    add (B ("GM Sound FX", "128 Gunshot").chip (m6581).poly (2)
             .voice (0, "N", 0, 8, 0, 0).tune (0, 6).fixedPitch (0).sweepFrom (0, 24, 60).filt (0)
             .filter (900, 3, true, false, false).fenv (900, 1, 200, 0.0f, 100));
}
