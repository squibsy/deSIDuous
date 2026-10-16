#include "SidEngine.h"

#include "SID.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr int fifoSize = 8192;
    constexpr int fifoMask = fifoSize - 1;
    constexpr float chipGain = 1.0f;

    // Register offsets per voice (MOS 6581 data sheet)
    constexpr int regFreqLo = 0, regFreqHi = 1, regPwLo = 2, regPwHi = 3, regControl = 4, regAD = 5, regSR = 6;
    constexpr int regFcLo = 0x15, regFcHi = 0x16, regResFilt = 0x17, regModeVol = 0x18;

    // Control register bits
    constexpr uint8_t bitGate = 0x01, bitSync = 0x02, bitRing = 0x04, bitTest = 0x08;
}

//==============================================================================
SidChip::SidChip()
    : sid (std::make_unique<reSIDfp::SID>())
{
    tmp.resize (fifoSize);
    fifo.resize (fifoSize);

    // Pre-build the combined-waveform tables for every model/strength
    // (they are cached globally by reSIDfp) so later switches are cheap.
    for (auto cws : { reSIDfp::AVERAGE, reSIDfp::WEAK, reSIDfp::STRONG })
        for (auto m : { reSIDfp::MOS6581, reSIDfp::MOS8580 })
        {
            sid->setCombinedWaveforms (cws);
            sid->setChipModel (m);
        }
}

SidChip::~SidChip() = default;

void SidChip::setSampling (double clockHz, double sampleRate, int quality)
{
    sid->setSamplingParameters (clockHz, quality == 0 ? reSIDfp::RESAMPLE : reSIDfp::DECIMATE, sampleRate);
    cyclesPerSample = clockHz / sampleRate;
    reset();

    // Let the C64 output stage (1.6 Hz high-pass in reSIDfp) settle with the volume
    // register up, so the first note does not start with a DC thump.
    write (0x18, 0x0f);
    for (int i = 0; i < 20; ++i)
        sid->clock ((unsigned int) (clockHz / 40.0), tmp.data());
    fifoRead = 0;
    fifoCount = 0;
}

void SidChip::setModel (int chipModel, int combined, float filterBias)
{
    if (chipModel != model || combined != combinedMode)
    {
        static constexpr reSIDfp::CombinedWaveforms cwsTable[] = { reSIDfp::AVERAGE, reSIDfp::WEAK, reSIDfp::STRONG };
        sid->setCombinedWaveforms (cwsTable[juce::jlimit (0, 2, combined)]);
        sid->setChipModel (chipModel == 0 ? reSIDfp::MOS6581 : reSIDfp::MOS8580);
        model = chipModel;
        combinedMode = combined;
    }

    if (std::abs (filterBias - bias) > 0.002f)
    {
        sid->setFilter6581Curve (filterBias);
        sid->setFilter8580Curve (filterBias);
        bias = filterBias;
    }
}

void SidChip::reset()
{
    sid->reset();
    regs.fill (0);
    fifoRead = 0;
    fifoCount = 0;
}

void SidChip::write (int r, uint8_t value)
{
    if (regs[(size_t) r] != value)
    {
        regs[(size_t) r] = value;
        sid->write (r, value);
    }
}

void SidChip::pushFifo (const short* s, int n)
{
    for (int i = 0; i < n && fifoCount < fifoSize; ++i)
    {
        fifo[(size_t) ((fifoRead + fifoCount) & fifoMask)] = s[i] * (1.0f / 32768.0f);
        ++fifoCount;
    }
}

void SidChip::render (float* dest, int n, float gain)
{
    // Clock the chip on demand until enough output samples exist. Driving by demand
    // (instead of a fixed cycle count) keeps us locked to reSIDfp's own resampler phase.
    const int target = n + 2;
    for (int guard = 0; fifoCount < target && guard < 64; ++guard)
    {
        const int want = target - fifoCount;
        const auto cycles = (unsigned int) (want * cyclesPerSample) + 1;
        const int produced = sid->clock (cycles, tmp.data());
        pushFifo (tmp.data(), produced);
    }

    const int avail = std::min (n, fifoCount);
    for (int i = 0; i < avail; ++i)
    {
        dest[i] += gain * fifo[(size_t) fifoRead];
        fifoRead = (fifoRead + 1) & fifoMask;
    }
    fifoCount -= avail;
}

//==============================================================================
void SidEngine::FilterEnv::advance (double dt, const EngineSettings& s)
{
    switch (stage)
    {
        case Attack:
            level += dt / std::max (0.001, s.fAttackMs * 0.001);
            if (level >= 1.0) { level = 1.0; stage = Decay; }
            break;
        case Decay:
        {
            const double sus = s.fSustain;
            level = sus + (level - sus) * std::exp (-dt * 5.0 / std::max (0.001, s.fDecayMs * 0.001));
            if (std::abs (level - sus) < 0.001) { level = sus; stage = Sustain; }
            break;
        }
        case Sustain:
            level = s.fSustain;
            break;
        case Release:
            level *= std::exp (-dt * 5.0 / std::max (0.001, s.fReleaseMs * 0.001));
            if (level < 0.0005) { level = 0.0; stage = Idle; }
            break;
        case Idle:
        default:
            level = 0.0;
            break;
    }
}

//==============================================================================
SidEngine::SidEngine()
{
    monoStack.reserve (128);
}

SidEngine::~SidEngine() = default;

void SidEngine::rebuildChips (double newSampleRate, const EngineSettings& s)
{
    std::array<std::unique_ptr<SidChip>, maxChips> fresh;
    for (auto& c : fresh)
    {
        c = std::make_unique<SidChip>();
        c->setModel (s.chipModel, s.combined, s.filterBias);
        c->setSampling (s.clockHz(), newSampleRate, s.quality);
    }

    {
        const juce::SpinLock::ScopedLockType lock (chipLock);
        for (int i = 0; i < maxChips; ++i)
            std::swap (chips[(size_t) i].chip, fresh[(size_t) i]);

        sampleRate = newSampleRate;
        tickSamples = std::max (1, juce::roundToInt (sampleRate / 1000.0));
        dcCoeff = (float) std::exp (-juce::MathConstants<double>::twoPi * 10.0 / sampleRate);
        dcIn = dcOut = 0.0f;
        samplesToTick = 0;
        builtClock = s.clock;
        builtQuality = s.quality;
        resetAllVoices();
        for (auto& c : chips)
        {
            c.active = false;
            for (auto& v : c.v) v = VoiceRT();
        }
    }
    // old chips are destroyed here, outside the lock
}

void SidEngine::resetAllVoices()
{
    for (auto& c : chips)
    {
        for (auto& v : c.v)
        {
            if (v.keyDown || v.startPending)
                releaseVoice (v);
            v.stackSize = 0;
        }
        c.fenv.release();
    }
    monoStack.clear();
    keyHeld.fill (false);
    sustained.fill (false);
}

void SidEngine::allNotesOff()
{
    const juce::SpinLock::ScopedLockType lock (chipLock);
    resetAllVoices();
}

int SidEngine::chipsInUse (const EngineSettings& s) const
{
    if (s.voiceMode != (int) Params::VoiceMode::LayeredPoly)
        return 1;
    if (s.arpMode == (int) Params::ArpMode::HeldNotes)
        return 1;
    return juce::jlimit (1, maxChips, s.polyphony);
}

//==============================================================================
void SidEngine::process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const EngineSettings& s)
{
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    const juce::SpinLock::ScopedTryLockType lock (chipLock);
    if (! lock.isLocked() || chips[0].chip == nullptr)
        return;

    // Structural changes: release everything so no voice is left hanging
    if (s.voiceMode != lastVoiceMode || s.polyphony != lastPoly || s.arpMode != lastArpMode)
    {
        if (lastVoiceMode >= 0)
            resetAllVoices();
        lastVoiceMode = s.voiceMode;
        lastPoly = s.polyphony;
        lastArpMode = s.arpMode;
    }

    for (auto& c : chips)
        c.chip->setModel (s.chipModel, s.combined, s.filterBias);

    const float gain = chipGain * juce::Decibels::decibelsToGain (s.outGainDb);
    const double tickDt = tickSamples / sampleRate;
    float* outL = buffer.getWritePointer (0);

    auto midiIt = midi.cbegin();
    int pos = 0;

    while (pos < numSamples)
    {
        while (midiIt != midi.cend() && (*midiIt).samplePosition <= pos)
        {
            handleMidi ((*midiIt).getMessage(), s);
            ++midiIt;
        }

        if (samplesToTick <= 0)
        {
            tick (tickDt, s);
            samplesToTick += tickSamples;
        }

        int end = std::min (numSamples, pos + samplesToTick);
        if (midiIt != midi.cend())
            end = std::min (end, std::max (pos + 1, (*midiIt).samplePosition));

        const int len = end - pos;
        int activeCount = 0;
        for (auto& c : chips)
        {
            if (c.active)
            {
                c.chip->render (outL + pos, len, gain);
                ++activeCount;
            }
        }
        activeChipCount.store (activeCount, std::memory_order_relaxed);

        pos = end;
        samplesToTick -= len;
    }

    // remaining events at the very end of the block
    for (; midiIt != midi.cend(); ++midiIt)
        handleMidi ((*midiIt).getMessage(), s);

    // 10 Hz DC blocker: SID outputs carry large DC steps (envelope x DAC offset,
    // 6581 volume register) which the real C64 only removes very slowly.
    for (int i = 0; i < numSamples; ++i)
    {
        const float x = outL[i];
        dcOut = x - dcIn + dcCoeff * dcOut;
        dcIn = x;
        outL[i] = dcOut;
    }

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);

    // Feed the oscilloscope
    {
        const auto scope = scopeFifo.write (numSamples);
        if (scope.blockSize1 > 0) std::copy (outL, outL + scope.blockSize1, scopeData.begin() + scope.startIndex1);
        if (scope.blockSize2 > 0) std::copy (outL + scope.blockSize1, outL + scope.blockSize1 + scope.blockSize2, scopeData.begin() + scope.startIndex2);
    }

    publishMonitor();
}

void SidEngine::publishMonitor()
{
    const int c = juce::jlimit (0, maxChips - 1, monitorChip.load (std::memory_order_relaxed));
    for (int r = 0; r < numRegs; ++r)
        monitorRegs[(size_t) r].store (chips[(size_t) c].chip->reg (r), std::memory_order_relaxed);
}

//==============================================================================
void SidEngine::handleMidi (const juce::MidiMessage& m, const EngineSettings& s)
{
    if (m.isNoteOn())
    {
        noteOn (m.getChannel(), m.getNoteNumber(), m.getFloatVelocity(), s);
    }
    else if (m.isNoteOff())
    {
        noteOff (m.getChannel(), m.getNoteNumber(), s);
    }
    else if (m.isPitchWheel())
    {
        pitchBend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    }
    else if (m.isController())
    {
        const int cc = m.getControllerNumber();
        const int val = m.getControllerValue();
        if (cc == 1)
        {
            modWheel = (float) val / 127.0f;
        }
        else if (cc == 64)
        {
            const bool down = val >= 64;
            if (sustainPedal && ! down)
            {
                sustainPedal = false;
                for (int n = 0; n < 128; ++n)
                    if (sustained[(size_t) n])
                    {
                        sustained[(size_t) n] = false;
                        if (! keyHeld[(size_t) n])
                            releaseNote (m.getChannel(), n, s);
                    }
            }
            sustainPedal = down;
        }
        else if (cc == 120 || cc == 123)
        {
            resetAllVoices();
        }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        resetAllVoices();
    }
}

void SidEngine::noteOn (int channel, int note, float vel, const EngineSettings& s)
{
    keyHeld[(size_t) note] = true;
    sustained[(size_t) note] = false;

    const auto mode = (Params::VoiceMode) s.voiceMode;

    if (mode == Params::VoiceMode::LayeredPoly)
    {
        const bool heldArp = s.arpMode == (int) Params::ArpMode::HeldNotes;

        if (heldArp || s.polyphony <= 1)
        {
            const bool wasHeld = ! monoStack.empty();
            monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
            monoStack.push_back (note);

            if (heldArp)
            {
                if (! wasHeld)
                    triggerChip (0, note, vel, true, s);
            }
            else
            {
                triggerChip (0, note, vel, ! (wasHeld && s.legato), s);
            }
            return;
        }

        const int n = chipsInUse (s);
        int best = -1;

        // 1. same note already sounding on a chip: reuse it
        for (int c = 0; c < n && best < 0; ++c)
            if (chips[(size_t) c].lastNote == note && chips[(size_t) c].active)
                best = c;

        // 2. oldest chip that is not held
        if (best < 0)
        {
            uint64_t oldest = UINT64_MAX;
            for (int c = 0; c < n; ++c)
            {
                auto& ch = chips[(size_t) c];
                bool held = false;
                for (auto& v : ch.v) held = held || v.keyDown || v.startPending;
                if (! held && ch.order < oldest) { oldest = ch.order; best = c; }
            }
        }

        // 3. steal the oldest held chip
        if (best < 0)
        {
            uint64_t oldest = UINT64_MAX;
            for (int c = 0; c < n; ++c)
                if (chips[(size_t) c].order < oldest) { oldest = chips[(size_t) c].order; best = c; }
        }

        triggerChip (best, note, vel, true, s);
    }
    else if (mode == Params::VoiceMode::Paraphonic)
    {
        auto& c = chips[0];
        int best = -1;
        uint64_t oldest = UINT64_MAX;

        for (int vi = 0; vi < 3; ++vi)
        {
            auto& v = c.v[vi];
            if (! s.voice[vi].on) continue;
            if (! v.keyDown && ! v.startPending && v.order < oldest) { oldest = v.order; best = vi; }
        }
        if (best < 0)
            for (int vi = 0; vi < 3; ++vi)
                if (s.voice[vi].on && c.v[vi].order < oldest) { oldest = c.v[vi].order; best = vi; }
        if (best < 0)
            return;

        c.velocity = vel;
        c.lastNote = note;
        c.active = true;
        c.idleTime = 0.0;
        c.fenv.trigger();
        c.arpStep = 0;
        c.arpTimer = 0.0;
        triggerVoice (c, best, note, vel, true, s);
        c.v[best].order = ++orderCounter;
        monitorChip.store (0);
    }
    else // MIDI channels 1-3 -> voices 1-3
    {
        const int vi = channel - 1;
        if (vi < 0 || vi > 2 || ! s.voice[vi].on)
            return;

        auto& c = chips[0];
        auto& v = c.v[vi];
        const bool wasHeld = v.stackSize > 0;

        int k = 0;
        for (int i = 0; i < v.stackSize; ++i)
            if (v.stack[i] != note) v.stack[k++] = v.stack[i];
        v.stackSize = k;
        if (v.stackSize < 16) v.stack[v.stackSize++] = note;

        c.velocity = vel;
        c.lastNote = note;
        c.active = true;
        c.idleTime = 0.0;
        if (! (wasHeld && s.legato))
        {
            c.fenv.trigger();
            c.arpStep = 0;
            c.arpTimer = 0.0;
        }
        triggerVoice (c, vi, note, vel, ! (wasHeld && s.legato), s);
        monitorChip.store (0);
    }
}

void SidEngine::noteOff (int channel, int note, const EngineSettings& s)
{
    keyHeld[(size_t) note] = false;

    if (sustainPedal && s.voiceMode != (int) Params::VoiceMode::MidiChannels)
    {
        sustained[(size_t) note] = true;
        return;
    }
    releaseNote (channel, note, s);
}

void SidEngine::releaseNote (int channel, int note, const EngineSettings& s)
{
    const auto mode = (Params::VoiceMode) s.voiceMode;

    if (mode == Params::VoiceMode::LayeredPoly)
    {
        const bool heldArp = s.arpMode == (int) Params::ArpMode::HeldNotes;

        if (heldArp || s.polyphony <= 1)
        {
            monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
            auto& c = chips[0];

            if (monoStack.empty())
                releaseChip (c);
            else if (! heldArp && c.lastNote == note)
                triggerChip (0, monoStack.back(), c.velocity, ! s.legato, s);
            return;
        }

        for (int i = 0; i < maxChips; ++i)
        {
            auto& c = chips[(size_t) i];
            if (c.lastNote != note) continue;
            bool held = false;
            for (auto& v : c.v) held = held || v.keyDown || v.startPending;
            if (held) releaseChip (c);
        }
    }
    else if (mode == Params::VoiceMode::Paraphonic)
    {
        auto& c = chips[0];
        bool anyHeld = false;
        for (auto& v : c.v)
        {
            if (v.note == note && (v.keyDown || v.startPending))
                releaseVoice (v);
            anyHeld = anyHeld || v.keyDown;
        }
        if (! anyHeld)
            c.fenv.release();
    }
    else
    {
        const int vi = channel - 1;
        if (vi < 0 || vi > 2)
            return;

        auto& c = chips[0];
        auto& v = c.v[vi];

        int k = 0;
        for (int i = 0; i < v.stackSize; ++i)
            if (v.stack[i] != note) v.stack[k++] = v.stack[i];
        v.stackSize = k;

        if (v.note == note)
        {
            if (v.stackSize > 0)
                triggerVoice (c, vi, v.stack[v.stackSize - 1], v.velocity, ! s.legato, s);
            else if (v.keyDown || v.startPending)
                releaseVoice (v);
        }

        bool anyHeld = false;
        for (auto& vv : c.v) anyHeld = anyHeld || vv.keyDown;
        if (! anyHeld)
            c.fenv.release();
    }
}

//==============================================================================
void SidEngine::triggerChip (int ci, int note, float vel, bool retrigger, const EngineSettings& s)
{
    auto& c = chips[(size_t) ci];
    c.velocity = vel;
    c.lastNote = note;
    c.order = ++orderCounter;
    c.active = true;
    c.idleTime = 0.0;

    if (retrigger)
    {
        c.fenv.trigger();
        c.arpStep = 0;
        c.arpTimer = 0.0;
    }

    for (int vi = 0; vi < 3; ++vi)
        triggerVoice (c, vi, note, vel, retrigger, s);

    monitorChip.store (ci);
    updateChip (c, 0.0, s, false);
}

void SidEngine::triggerVoice (ChipRT& c, int vi, int note, float vel, bool retrigger, const EngineSettings& s)
{
    auto& v = c.v[vi];
    v.note = note;
    v.target = note;
    if (! v.hasPlayed || s.glideMs <= 0.0f)
        v.pitch = note;
    v.hasPlayed = true;
    v.velocity = vel;

    // A voice that is switched off still follows the note: its oscillator keeps
    // running and can act as the sync / ring-mod source for its neighbour.
    if (! s.voice[vi].on)
    {
        v.keyDown = false;
        return;
    }

    v.keyDown = true;
    v.releaseAfterStart = false;

    if (retrigger || ! v.gatedInChip)
    {
        const int hrTicks = (int) std::ceil (s.hardRestartMs);   // ticks are 1 ms
        v.pendingTicks = (v.gatedInChip ? 1 : 0) + hrTicks + (s.phaseReset ? 1 : 0);
        v.startPending = true;
    }
    juce::ignoreUnused (c);
}

void SidEngine::releaseVoice (VoiceRT& v)
{
    if (v.startPending)
        v.releaseAfterStart = true;
    v.keyDown = false;
}

void SidEngine::releaseChip (ChipRT& c)
{
    for (auto& v : c.v)
        if (v.keyDown || v.startPending)
            releaseVoice (v);
    c.fenv.release();
}

//==============================================================================
void SidEngine::tick (double dt, const EngineSettings& s)
{
    // ---- global LFO ----
    lfoPhase += s.lfoRate * dt;
    if (lfoPhase >= 1.0)
    {
        lfoPhase -= std::floor (lfoPhase);
        lfoRandom = juce::Random::getSystemRandom().nextDouble() * 2.0 - 1.0;
    }
    switch (s.lfoShape)
    {
        case 1:  lfoValue = std::sin (lfoPhase * juce::MathConstants<double>::twoPi); break;
        case 2:  lfoValue = lfoPhase < 0.5 ? 1.0 : -1.0; break;
        case 3:  lfoValue = 1.0 - 2.0 * lfoPhase; break;
        case 4:  lfoValue = lfoRandom; break;
        default: lfoValue = 1.0 - 4.0 * std::abs (lfoPhase - 0.5); break;
    }

    const int inUse = chipsInUse (s);

    // ---- held-note arpeggio (layered mode, chip 0) ----
    if (s.voiceMode == (int) Params::VoiceMode::LayeredPoly
        && s.arpMode == (int) Params::ArpMode::HeldNotes && ! monoStack.empty())
    {
        std::array<int, 128> sorted {};
        const int n = std::min ((int) monoStack.size(), 128);
        std::copy (monoStack.begin(), monoStack.begin() + n, sorted.begin());
        std::sort (sorted.begin(), sorted.begin() + n);

        auto& c = chips[0];
        for (int vi = 0; vi < 3; ++vi)
        {
            const int note = s.voice[vi].arp ? sorted[(size_t) (c.arpStep % n)] : sorted[0];
            c.v[vi].note = note;
            c.v[vi].target = note;
        }
        c.lastNote = sorted[0];
    }

    for (int i = 0; i < maxChips; ++i)
    {
        auto& c = chips[(size_t) i];

        // chips beyond the current polyphony are released and left to die out
        if (i >= inUse)
            releaseChip (c);

        if (c.active)
            updateChip (c, dt, s, true);
    }
}

void SidEngine::updateChip (ChipRT& c, double dt, const EngineSettings& s, bool advance)
{
    auto& chip = *c.chip;
    const double clockHz = s.clockHz();

    if (advance)
    {
        c.fenv.advance (dt, s);

        c.arpTimer += dt;
        const double step = s.arpSpeed / s.frameHz();
        while (c.arpTimer >= step)
        {
            c.arpTimer -= step;
            ++c.arpStep;
        }
    }

    const auto& chord = Params::chords[juce::jlimit (0, Params::numChords - 1, s.arpChord)];
    const bool chordArp = s.arpMode == (int) Params::ArpMode::Chord;

    bool anyHeld = false;
    float maxReleaseMs = 0.0f;

    for (int vi = 0; vi < 3; ++vi)
    {
        const auto& p = s.voice[vi];
        auto& v = c.v[vi];
        const int base = vi * 7;

        // ---- gate sequencing (retrigger gap, hard restart, oscillator reset) ----
        if (advance && v.pendingTicks > 0)
            --v.pendingTicks;

        bool gate = false;
        bool testPulse = false;
        bool hardRestart = false;

        if (v.startPending)
        {
            if (v.pendingTicks > 0)
            {
                testPulse = s.phaseReset && v.pendingTicks == 1;
                hardRestart = s.hardRestartMs > 0.0f;
            }
            else
            {
                v.startPending = false;
                v.tOn = 0.0;
                gate = true;
                if (v.releaseAfterStart)
                {
                    v.releaseAfterStart = false;
                    v.keyDown = false;
                }
            }
        }
        else
        {
            gate = v.keyDown;
        }

        if (gate && ! v.gatedInChip)
            v.tOff = 0.0;

        // ---- timing ----
        if (advance)
        {
            if (gate || v.gatedInChip) v.tOn += dt;
            if (! gate) v.tOff += dt;

            if (s.glideMs > 0.0f)
                v.pitch += (v.target - v.pitch) * (1.0 - std::exp (-dt * 3.0 / (s.glideMs * 0.001)));
            else
                v.pitch = v.target;

            v.pwmPhase += p.pwmRate * dt;
            v.pwmPhase -= std::floor (v.pwmPhase);
        }
        if (s.glideMs <= 0.0f)
            v.pitch = v.target;

        const double tOnMs = v.tOn * 1000.0;

        // ---- pitch ----
        double semis = (p.keyTrack ? v.pitch : 60.0) + p.coarse + p.fine * 0.01 + pitchBend * s.bendRange;

        if (chordArp && p.arp)
            semis += chord.notes[c.arpStep % chord.count];

        if (p.sweep != 0.0f)
        {
            const double x = std::min (1.0, tOnMs / std::max (1.0f, p.sweepTimeMs));
            semis += p.sweep * (1.0 - x) * (1.0 - x);
        }

        const double vibCents = s.lfoVibCents + modWheel * 100.0;
        if (vibCents > 0.0)
        {
            double ramp = 1.0;
            if (s.lfoDelayMs > 0.0f)
                ramp = juce::jlimit (0.0, 1.0, (tOnMs - s.lfoDelayMs) / 250.0);
            semis += lfoValue * vibCents * 0.01 * ramp;
        }

        const double hz = 440.0 * std::pow (2.0, (semis - 69.0) / 12.0);
        const int freq = juce::jlimit (0, 65535, (int) std::lround (hz * 16777216.0 / clockHz));   // Fn = Fout * 2^24 / Fclk

        // ---- pulse width ----
        const double pwmTri = 1.0 - 4.0 * std::abs (v.pwmPhase - 0.5);
        const int pw = juce::jlimit (0, 4095, (int) std::lround (p.pw + p.pwmDepth * pwmTri + s.lfoPW * lfoValue));

        // ---- control register ----
        int nibble = p.waveNibble();
        if (p.hitWave > 0 && tOnMs < p.hitTimeMs)
            nibble = p.hitWave;

        uint8_t control = 0;
        if (p.on)
        {
            control = (uint8_t) (nibble << 4);
            if (p.ring) control |= bitRing;
            if (p.sync) control |= bitSync;
            if (p.test || testPulse) control |= bitTest;
            if (gate) control |= bitGate;
        }

        chip.write (base + regFreqLo, (uint8_t) (freq & 0xff));
        chip.write (base + regFreqHi, (uint8_t) (freq >> 8));
        chip.write (base + regPwLo,   (uint8_t) (pw & 0xff));
        chip.write (base + regPwHi,   (uint8_t) (pw >> 8));

        if (hardRestart)
        {
            chip.write (base + regAD, 0x00);
            chip.write (base + regSR, 0x00);
        }
        else
        {
            chip.write (base + regAD, (uint8_t) ((p.a << 4) | p.d));
            chip.write (base + regSR, (uint8_t) ((p.s << 4) | p.r));
        }
        chip.write (base + regControl, control);

        v.gatedInChip = gate;
        anyHeld = anyHeld || gate || v.startPending;
        if (p.on)
            maxReleaseMs = std::max (maxReleaseMs, Params::decayMs[p.r]);
    }

    // ---- filter & volume ----
    const double keyScale = std::pow (2.0, s.fKeyTrack * (c.lastNote - 60) / 12.0);
    const double cut = s.cutoff * keyScale
                     + s.fEnvAmount * c.fenv.level
                     + s.fVelocity * c.velocity
                     + s.lfoCutoff * lfoValue;
    const int fc = juce::jlimit (0, 2047, (int) std::lround (cut));

    uint8_t filt = 0;
    for (int vi = 0; vi < 3; ++vi)
        if (s.voice[vi].filt) filt |= (uint8_t) (1 << vi);

    const int vol = juce::jlimit (0, 15, (int) std::lround (s.volume * (1.0 - s.velVolume + s.velVolume * c.velocity)));
    uint8_t modeVol = (uint8_t) vol;
    if (s.lp)    modeVol |= 0x10;
    if (s.bp)    modeVol |= 0x20;
    if (s.hp)    modeVol |= 0x40;
    if (s.v3off) modeVol |= 0x80;

    chip.write (regFcLo, (uint8_t) (fc & 0x07));
    chip.write (regFcHi, (uint8_t) (fc >> 3));
    chip.write (regResFilt, (uint8_t) ((juce::jlimit (0, 15, s.resonance) << 4) | filt));
    chip.write (regModeVol, modeVol);

    // ---- idle detection: stop clocking chips that have fully released ----
    if (advance)
    {
        if (anyHeld)
        {
            c.idleTime = 0.0;
        }
        else
        {
            c.idleTime += dt;
            if (c.idleTime * 1000.0 > maxReleaseMs * 1.25 + 250.0)
                c.active = false;
        }
    }
}
