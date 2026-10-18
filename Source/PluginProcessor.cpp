#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

DesiduousProcessor::DesiduousProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "deSIDuous", Params::createLayout())
{
    auto get = [this] (const juce::String& id) {
        auto* p = state.getRawParameterValue (id);
        jassert (p != nullptr);
        return p;
    };

    using namespace Params;
    for (int v = 0; v < 3; ++v)
    {
        auto& x = vp[v];
        x.on = get (vid (v, on));       x.tri = get (vid (v, tri));     x.saw = get (vid (v, saw));
        x.pulse = get (vid (v, pulse)); x.noise = get (vid (v, noise)); x.ring = get (vid (v, ring));
        x.sync = get (vid (v, oscSync));   x.test = get (vid (v, test));   x.filt = get (vid (v, filt));
        x.keyTrack = get (vid (v, keyTrack)); x.arp = get (vid (v, arp));
        x.coarse = get (vid (v, coarse)); x.fine = get (vid (v, fine)); x.pw = get (vid (v, pw));
        x.pwmRate = get (vid (v, pwmRate)); x.pwmDepth = get (vid (v, pwmDepth));
        x.a = get (vid (v, attack)); x.d = get (vid (v, decay)); x.s = get (vid (v, sustain)); x.r = get (vid (v, release));
        x.hitWave = get (vid (v, hitWave)); x.hitTime = get (vid (v, hitTime));
        x.sweep = get (vid (v, sweep)); x.sweepTime = get (vid (v, sweepTime));
    }

    chipModel = get (Params::chipModel);   voiceMode = get (Params::voiceMode); polyphony = get (Params::polyphony);
    clock = get (Params::clock);           combined = get (Params::combined);   filterBias = get (Params::filterBias);
    volume = get (Params::volume);         velVolume = get (Params::velVolume); outGain = get (Params::outGain);
    glide = get (Params::glide);           legato = get (Params::legato);       bendRange = get (Params::bendRange);
    hardRestart = get (Params::hardRestart); phaseReset = get (Params::phaseReset); quality = get (Params::quality);
    cutoff = get (Params::cutoff);         resonance = get (Params::resonance);
    lp = get (Params::lowPass);            bp = get (Params::bandPass);         hp = get (Params::highPass);
    v3off = get (Params::voice3Off);       fEnvAmount = get (Params::fEnvAmount);
    fAttack = get (Params::fAttack);       fDecay = get (Params::fDecay);       fSustain = get (Params::fSustain);
    fRelease = get (Params::fRelease);     fKeyTrack = get (Params::fKeyTrack); fVelocity = get (Params::fVelocity);
    lfoRate = get (Params::lfoRate);       lfoShape = get (Params::lfoShape);   lfoVib = get (Params::lfoVib);
    lfoDelay = get (Params::lfoDelay);     lfoCutoff = get (Params::lfoCutoff); lfoPW = get (Params::lfoPW);
    arpMode = get (Params::arpMode);       arpChord = get (Params::arpChord);   arpSpeed = get (Params::arpSpeed);

    state.addParameterListener (Params::clock, this);
    state.addParameterListener (Params::quality, this);

    // Start on the first real preset rather than the bare init patch
    setCurrentProgram (1);
}

DesiduousProcessor::~DesiduousProcessor()
{
    cancelPendingUpdate();
    state.removeParameterListener (Params::clock, this);
    state.removeParameterListener (Params::quality, this);
}

bool DesiduousProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

EngineSettings DesiduousProcessor::snapshot() const
{
    auto b = [] (const std::atomic<float>* p) { return p->load() > 0.5f; };
    auto i = [] (const std::atomic<float>* p) { return juce::roundToInt (p->load()); };
    auto f = [] (const std::atomic<float>* p) { return p->load(); };

    EngineSettings s;
    for (int v = 0; v < 3; ++v)
    {
        auto& o = s.voice[v];
        auto& x = vp[v];
        o.on = b (x.on); o.tri = b (x.tri); o.saw = b (x.saw); o.pulse = b (x.pulse); o.noise = b (x.noise);
        o.ring = b (x.ring); o.sync = b (x.sync); o.test = b (x.test); o.filt = b (x.filt);
        o.keyTrack = b (x.keyTrack); o.arp = b (x.arp);
        o.coarse = i (x.coarse); o.fine = f (x.fine); o.pw = i (x.pw); o.pwmRate = f (x.pwmRate); o.pwmDepth = i (x.pwmDepth);
        o.a = i (x.a); o.d = i (x.d); o.s = i (x.s); o.r = i (x.r);
        o.hitWave = i (x.hitWave); o.hitTimeMs = f (x.hitTime); o.sweep = f (x.sweep); o.sweepTimeMs = f (x.sweepTime);
    }

    s.chipModel = i (chipModel); s.voiceMode = i (voiceMode); s.polyphony = i (polyphony); s.clock = i (clock);
    s.combined = i (combined); s.filterBias = f (filterBias); s.volume = i (volume); s.velVolume = f (velVolume);
    s.outGainDb = f (outGain); s.glideMs = f (glide); s.legato = b (legato); s.bendRange = i (bendRange);
    s.hardRestartMs = f (hardRestart); s.phaseReset = b (phaseReset); s.quality = i (quality);
    s.cutoff = i (cutoff); s.resonance = i (resonance); s.lp = b (lp); s.bp = b (bp); s.hp = b (hp); s.v3off = b (v3off);
    s.fEnvAmount = i (fEnvAmount); s.fAttackMs = f (fAttack); s.fDecayMs = f (fDecay); s.fSustain = f (fSustain);
    s.fReleaseMs = f (fRelease); s.fKeyTrack = f (fKeyTrack); s.fVelocity = i (fVelocity);
    s.lfoRate = f (lfoRate); s.lfoShape = i (lfoShape); s.lfoVibCents = f (lfoVib); s.lfoDelayMs = f (lfoDelay);
    s.lfoCutoff = i (lfoCutoff); s.lfoPW = i (lfoPW);
    s.arpMode = i (arpMode); s.arpChord = i (arpChord); s.arpSpeed = i (arpSpeed);
    return s;
}

void DesiduousProcessor::prepareToPlay (double sampleRate, int)
{
    if (sampleRate != currentSampleRate || engine.needsRebuild (snapshot()))
    {
        currentSampleRate = sampleRate;
        engine.rebuildChips (sampleRate, snapshot());
    }
    keyboardState.reset();
}

void DesiduousProcessor::parameterChanged (const juce::String&, float)
{
    triggerAsyncUpdate();   // rebuild chips on the message thread (resampler tables are expensive)
}

void DesiduousProcessor::handleAsyncUpdate()
{
    const auto s = snapshot();
    if (currentSampleRate > 0.0 && engine.needsRebuild (s))
        engine.rebuildChips (currentSampleRate, s);
}

void DesiduousProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    engine.process (buffer, midi, snapshot());
}

//==============================================================================
int DesiduousProcessor::getNumPrograms()
{
    return (int) getFactoryPresets().size();
}

void DesiduousProcessor::setCurrentProgram (int index)
{
    const auto& presets = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;

    currentProgram = index;
    applyPreset (state, presets[(size_t) index]);
}

const juce::String DesiduousProcessor::getProgramName (int index)
{
    const auto& presets = getFactoryPresets();
    return juce::isPositiveAndBelow (index, (int) presets.size()) ? presets[(size_t) index].displayName() : juce::String();
}

void DesiduousProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto tree = state.copyState();
    tree.setProperty ("program", currentProgram, nullptr);
    if (auto xml = tree.createXml())
        copyXmlToBinary (*xml, destData);
}

void DesiduousProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = tree.getProperty ("program", 0);
            state.replaceState (tree);
        }
}

juce::AudioProcessorEditor* DesiduousProcessor::createEditor()
{
    return new DesiduousEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DesiduousProcessor();
}
