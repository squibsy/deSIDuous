#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SidEngine.h"

class SIDSynthProcessor : public juce::AudioProcessor,
                          private juce::AudioProcessorValueTreeState::Listener,
                          private juce::AsyncUpdater
{
public:
    SIDSynthProcessor();
    ~SIDSynthProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState state;
    SidEngine engine;
    juce::MidiKeyboardState keyboardState;   // on-screen keyboard

private:
    struct VoicePtrs
    {
        std::atomic<float> *on, *tri, *saw, *pulse, *noise, *ring, *sync, *test, *filt, *keyTrack, *arp;
        std::atomic<float> *coarse, *fine, *pw, *pwmRate, *pwmDepth, *a, *d, *s, *r, *hitWave, *hitTime, *sweep, *sweepTime;
    };
    VoicePtrs vp[3] {};
    std::atomic<float> *chipModel, *voiceMode, *polyphony, *clock, *combined, *filterBias, *volume, *velVolume, *outGain,
                       *glide, *legato, *bendRange, *hardRestart, *phaseReset, *quality,
                       *cutoff, *resonance, *lp, *bp, *hp, *v3off, *fEnvAmount, *fAttack, *fDecay, *fSustain, *fRelease,
                       *fKeyTrack, *fVelocity, *lfoRate, *lfoShape, *lfoVib, *lfoDelay, *lfoCutoff, *lfoPW,
                       *arpMode, *arpChord, *arpSpeed;

    EngineSettings snapshot() const;
    void parameterChanged (const juce::String& id, float) override;
    void handleAsyncUpdate() override;

    double currentSampleRate = 0.0;
    int currentProgram = 0;
    std::atomic<bool> rebuilding { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SIDSynthProcessor)
};
