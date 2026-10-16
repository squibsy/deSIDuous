#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

#include "Params.h"

namespace reSIDfp { class SID; }

//==============================================================================
// Plain snapshot of all parameter values, taken once per audio block.
struct VoiceSettings
{
    bool on = true, tri = false, saw = true, pulse = false, noise = false;
    bool ring = false, sync = false, test = false, filt = true, keyTrack = true, arp = true;
    int coarse = 0;
    float fine = 0.0f;
    int pw = 2048;
    float pwmRate = 1.0f;
    int pwmDepth = 0;
    int a = 0, d = 9, s = 10, r = 8;
    int hitWave = 0;
    float hitTimeMs = 20.0f;
    float sweep = 0.0f, sweepTimeMs = 150.0f;

    int waveNibble() const { return (tri ? 1 : 0) | (saw ? 2 : 0) | (pulse ? 4 : 0) | (noise ? 8 : 0); }
};

struct EngineSettings
{
    VoiceSettings voice[3];

    int chipModel = 0;          // 0 = 6581, 1 = 8580
    int voiceMode = 0;          // Params::VoiceMode
    int polyphony = 6;
    int clock = 0;              // 0 = PAL, 1 = NTSC
    int combined = 0;           // 0 = average, 1 = weak, 2 = strong
    float filterBias = 0.5f;
    int volume = 15;
    float velVolume = 0.3f;
    float outGainDb = 0.0f;
    float glideMs = 0.0f;
    bool legato = false;
    int bendRange = 2;
    float hardRestartMs = 0.0f;
    bool phaseReset = true;
    int quality = 0;            // 0 = sinc resampling, 1 = decimate

    int cutoff = 1024, resonance = 4;
    bool lp = true, bp = false, hp = false, v3off = false;
    int fEnvAmount = 0;
    float fAttackMs = 2.0f, fDecayMs = 300.0f, fSustain = 0.0f, fReleaseMs = 300.0f;
    float fKeyTrack = 0.0f;
    int fVelocity = 0;

    float lfoRate = 5.5f;
    int lfoShape = 0;
    float lfoVibCents = 0.0f, lfoDelayMs = 0.0f;
    int lfoCutoff = 0, lfoPW = 0;

    int arpMode = 0, arpChord = 1, arpSpeed = 1;

    double clockHz() const { return clock == 0 ? 985248.0 : 1022730.0; }
    double frameHz() const { return clock == 0 ? 50.0 : 60.0; }
};

//==============================================================================
// One emulated SID chip: reSIDfp core + register shadow + output FIFO.
class SidChip
{
public:
    SidChip();
    ~SidChip();

    // Heavy (builds resampler FIR tables): call off the audio thread.
    void setSampling (double clockHz, double sampleRate, int quality);

    // Cheap enough for the audio thread.
    void setModel (int chipModel, int combined, float filterBias);

    void reset();
    void write (int reg, uint8_t value);
    uint8_t reg (int r) const { return regs[(size_t) r]; }

    // Adds n samples of this chip's output to dest.
    void render (float* dest, int n, float gain);

private:
    std::unique_ptr<reSIDfp::SID> sid;
    std::array<uint8_t, 0x19> regs {};
    std::vector<short> tmp;
    std::vector<float> fifo;
    int fifoRead = 0, fifoCount = 0;
    double cyclesPerSample = 22.0;
    int model = -1, combinedMode = -1;
    float bias = -1.0f;

    void pushFifo (const short* s, int n);
};

//==============================================================================
class SidEngine
{
public:
    static constexpr int maxChips = 8;
    static constexpr int numRegs = 0x19;

    SidEngine();
    ~SidEngine();

    // Called from the message thread (prepareToPlay or when clock/quality change).
    void rebuildChips (double sampleRate, const EngineSettings& s);
    bool needsRebuild (const EngineSettings& s) const { return s.clock != builtClock || s.quality != builtQuality; }

    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const EngineSettings& s);
    void allNotesOff();

    // ---- UI taps (read from the message thread) ----
    std::array<std::atomic<uint8_t>, numRegs> monitorRegs {};
    std::atomic<int> monitorChip { 0 };
    std::atomic<int> activeChipCount { 0 };

    static constexpr int scopeSize = 4096;
    juce::AbstractFifo scopeFifo { scopeSize };
    std::array<float, scopeSize> scopeData {};

private:
    struct VoiceRT
    {
        int note = -1;          // note being played (pre-offsets)
        bool keyDown = false;   // logical gate (note held)
        bool gatedInChip = false;
        bool startPending = false;
        bool releaseAfterStart = false;
        int pendingTicks = 0;
        float velocity = 1.0f;
        double pitch = 60.0, target = 60.0;
        double tOn = 0.0, tOff = 1.0e9;
        double pwmPhase = 0.0;
        bool hasPlayed = false;
        uint64_t order = 0;
        int stack[16] {}; int stackSize = 0;   // MIDI-channel mode note stack
    };

    struct FilterEnv
    {
        enum Stage { Idle, Attack, Decay, Sustain, Release } stage = Idle;
        double level = 0.0;
        void trigger() { stage = Attack; }
        void release() { if (stage != Idle) stage = Release; }
        void advance (double dt, const EngineSettings& s);
    };

    struct ChipRT
    {
        std::unique_ptr<SidChip> chip;
        VoiceRT v[3];
        FilterEnv fenv;
        float velocity = 1.0f;
        int lastNote = 60;
        uint64_t order = 0;
        bool active = false;
        double idleTime = 0.0;
        int arpStep = 0;
        double arpTimer = 0.0;
    };

    std::array<ChipRT, maxChips> chips;
    juce::SpinLock chipLock;

    double sampleRate = 44100.0;
    int builtClock = -1, builtQuality = -1;
    int tickSamples = 44, samplesToTick = 0;
    uint64_t orderCounter = 0;
    int lastVoiceMode = -1, lastPoly = -1, lastArpMode = -1;

    // Global modulation state
    double lfoPhase = 0.0, lfoValue = 0.0, lfoRandom = 0.0;
    float modWheel = 0.0f, pitchBend = 0.0f;
    bool sustainPedal = false;
    std::array<bool, 128> keyHeld {}, sustained {};
    std::vector<int> monoStack;   // layered mono / held-note arp
    float dcIn = 0.0f, dcOut = 0.0f, dcCoeff = 0.9987f;

    void handleMidi (const juce::MidiMessage& m, const EngineSettings& s);
    void noteOn (int channel, int note, float vel, const EngineSettings& s);
    void noteOff (int channel, int note, const EngineSettings& s);
    void releaseNote (int channel, int note, const EngineSettings& s);

    void triggerChip (int c, int note, float vel, bool retrigger, const EngineSettings& s);
    void triggerVoice (ChipRT& c, int vi, int note, float vel, bool retrigger, const EngineSettings& s);
    void releaseVoice (VoiceRT& v);
    void releaseChip (ChipRT& c);

    void tick (double dt, const EngineSettings& s);
    void updateChip (ChipRT& c, double dt, const EngineSettings& s, bool advance);
    int chipsInUse (const EngineSettings& s) const;
    void resetAllVoices();
    void publishMonitor();
};
