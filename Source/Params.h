#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Parameter IDs and value tables. Register ranges follow the MOS 6581/8580 data sheet:
//   FREQ 16 bit, PW 12 bit, ADSR 4 bit each, FC 11 bit, RES 4 bit, VOL 4 bit.
namespace Params
{
    // ---- Voice parameters: prefixed "v1_", "v2_", "v3_" ----
    inline juce::String vid (int voice, const char* name) { return "v" + juce::String (voice + 1) + "_" + name; }

    constexpr const char* on        = "on";
    constexpr const char* tri       = "tri";
    constexpr const char* saw       = "saw";
    constexpr const char* pulse     = "pulse";
    constexpr const char* noise     = "noise";
    constexpr const char* ring      = "ring";
    constexpr const char* sync      = "sync";
    constexpr const char* test      = "test";
    constexpr const char* filt      = "filt";
    constexpr const char* coarse    = "coarse";
    constexpr const char* fine      = "fine";
    constexpr const char* pw        = "pw";
    constexpr const char* pwmRate   = "pwmrate";
    constexpr const char* pwmDepth  = "pwmdepth";
    constexpr const char* attack    = "a";
    constexpr const char* decay     = "d";
    constexpr const char* sustain   = "s";
    constexpr const char* release   = "r";
    constexpr const char* hitWave   = "hitwave";
    constexpr const char* hitTime   = "hittime";
    constexpr const char* sweep     = "sweep";
    constexpr const char* sweepTime = "sweeptime";
    constexpr const char* keyTrack  = "keytrack";
    constexpr const char* arp       = "arp";

    // ---- Global ----
    constexpr const char* chipModel   = "chip";
    constexpr const char* voiceMode   = "voicemode";
    constexpr const char* polyphony   = "poly";
    constexpr const char* clock       = "clock";
    constexpr const char* combined    = "combined";
    constexpr const char* filterBias  = "filterbias";
    constexpr const char* volume      = "volume";
    constexpr const char* velVolume   = "velvol";
    constexpr const char* outGain     = "outgain";
    constexpr const char* glide       = "glide";
    constexpr const char* legato      = "legato";
    constexpr const char* bendRange   = "bendrange";
    constexpr const char* hardRestart = "hardrestart";
    constexpr const char* phaseReset  = "phasereset";
    constexpr const char* quality     = "quality";

    // ---- Filter ----
    constexpr const char* cutoff     = "cutoff";
    constexpr const char* resonance  = "res";
    constexpr const char* lowPass    = "lp";
    constexpr const char* bandPass   = "bp";
    constexpr const char* highPass   = "hp";
    constexpr const char* voice3Off  = "v3off";
    constexpr const char* fEnvAmount = "fenvamt";
    constexpr const char* fAttack    = "fenva";
    constexpr const char* fDecay     = "fenvd";
    constexpr const char* fSustain   = "fenvs";
    constexpr const char* fRelease   = "fenvr";
    constexpr const char* fKeyTrack  = "fkeytrack";
    constexpr const char* fVelocity  = "fvel";

    // ---- LFO / modulation ----
    constexpr const char* lfoRate   = "lforate";
    constexpr const char* lfoShape  = "lfoshape";
    constexpr const char* lfoVib    = "lfovib";
    constexpr const char* lfoDelay  = "lfodelay";
    constexpr const char* lfoCutoff = "lfocut";
    constexpr const char* lfoPW     = "lfopw";

    // ---- Arpeggio ----
    constexpr const char* arpMode  = "arpmode";
    constexpr const char* arpChord = "arpchord";
    constexpr const char* arpSpeed = "arpspeed";

    // Data sheet envelope rates (ms) for register values 0..15
    constexpr float attackMs[16]  = { 2, 8, 16, 24, 38, 56, 68, 80, 100, 250, 500, 800, 1000, 3000, 5000, 8000 };
    constexpr float decayMs[16]   = { 6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000 };

    // Waveform names for 4-bit control-register waveform nibble (bit0 TRI, bit1 SAW, bit2 PULSE, bit3 NOISE)
    juce::StringArray waveComboNames();

    // Arpeggio chord shapes (semitone offsets)
    struct Chord { const char* name; int notes[4]; int count; };
    constexpr Chord chords[] = {
        { "Octave",      { 0, 12 },         2 },
        { "Major",       { 0, 4, 7 },       3 },
        { "Minor",       { 0, 3, 7 },       3 },
        { "Major 7",     { 0, 4, 7, 11 },   4 },
        { "Minor 7",     { 0, 3, 7, 10 },   4 },
        { "Dom 7",       { 0, 4, 7, 10 },   4 },
        { "Sus 4",       { 0, 5, 7 },       3 },
        { "Power",       { 0, 7, 12 },      3 },
        { "Diminished",  { 0, 3, 6, 9 },    4 },
        { "Two Octaves", { 0, 12, 24 },     3 },
        { "Minor Add9",  { 0, 3, 7, 14 },   4 },
        { "Fifths",      { 0, 7 },          2 },
    };
    constexpr int numChords = (int) (sizeof (chords) / sizeof (chords[0]));

    enum class VoiceMode { LayeredPoly = 0, Paraphonic, MidiChannels };
    enum class ArpMode   { Off = 0, Chord, HeldNotes };

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
