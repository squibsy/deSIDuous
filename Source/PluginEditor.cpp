#include "PluginEditor.h"
#include "Presets.h"
#include "BinaryData.h"

namespace ui
{
namespace
{
    template <typename Comp>
    void gridLayout (juce::Rectangle<int> area, int cols, int rows, std::initializer_list<Comp*> items)
    {
        const int w = area.getWidth() / cols, h = area.getHeight() / rows;
        int i = 0;
        for (auto* c : items)
        {
            if (c != nullptr)
                c->setBounds (area.getX() + (i % cols) * w, area.getY() + (i / cols) * h, w, h);
            ++i;
        }
    }

    juce::Colour voiceColour (int v) { return v == 0 ? C64::voice1 : (v == 1 ? C64::voice2 : C64::voice3); }
}

//==============================================================================
Knob::Knob (APVTS& s, const juce::String& id, const juce::String& name, const juce::String& tip)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 14);
    slider.setTooltip (tip);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (SidLookAndFeel::monoFont (11.0f));
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);

    attachment = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, r.getWidth(), 14);
    slider.setBounds (r);
}

Toggle::Toggle (APVTS& s, const juce::String& id, const juce::String& name, const juce::String& tip)
    : juce::TextButton (name)
{
    setClickingTogglesState (true);
    setTooltip (tip);
    attachment = std::make_unique<APVTS::ButtonAttachment> (s, id, *this);
}

Choice::Choice (APVTS& s, const juce::String& id, const juce::String& name, const juce::String& tip)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
        box.addItemList (p->choices, 1);
    box.setTooltip (tip);
    addAndMakeVisible (box);

    label.setText (name, juce::dontSendNotification);
    label.setFont (SidLookAndFeel::monoFont (11.0f));
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);

    attachment = std::make_unique<APVTS::ComboBoxAttachment> (s, id, box);
}

void Choice::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    box.setBounds (r.removeFromTop (juce::jmin (22, r.getHeight())));
}

Nibble::Nibble (APVTS& s, const juce::String& id, const juce::String& name, const juce::String& tip)
{
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    slider.setTooltip (tip);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setFont (SidLookAndFeel::monoFont (12.0f));
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);

    value.setFont (SidLookAndFeel::monoFont (12.0f));
    value.setJustificationType (juce::Justification::centred);
    value.setColour (juce::Label::textColourId, C64::textDim);
    addAndMakeVisible (value);

    attachment = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
    slider.onValueChange = [this] {
        value.setText (juce::String::toHexString (juce::roundToInt (slider.getValue())).toUpperCase(), juce::dontSendNotification);
    };
    slider.onValueChange();
}

void Nibble::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    value.setBounds (r.removeFromBottom (14));
    slider.setBounds (r);
}

//==============================================================================
Panel::Panel (const juce::String& t, juce::Colour a) : title (t), accent (a)
{
    getProperties().set ("accent", (juce::int64) a.getARGB());
}

void Panel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (C64::panel);
    g.fillRect (r);
    g.setColour (C64::panelEdge.withAlpha (0.8f));
    g.drawRect (r, 1.0f);

    auto header = getLocalBounds().removeFromTop (20);
    g.setColour (accent);
    g.fillRect (header.removeFromLeft (4).reduced (0, 4));
    g.setFont (SidLookAndFeel::monoFont (13.0f));
    g.drawText (title, header.withTrimmedLeft (6), juce::Justification::centredLeft);

    if (subtitle.isNotEmpty())
    {
        g.setColour (C64::textDim);
        g.setFont (SidLookAndFeel::monoFont (11.0f, false));
        g.drawText (subtitle, header.reduced (8, 0), juce::Justification::centredRight);
    }
}

//==============================================================================
VoicePanel::VoicePanel (APVTS& st, int v)
    : Panel ("VOICE " + juce::String (v + 1), voiceColour (v)),
      on       (st, Params::vid (v, Params::on),    "ON",   "Voice gate enable. A voice that is off keeps its oscillator running, so it can still be a sync / ring source."),
      tri      (st, Params::vid (v, Params::tri),   "TRI",  "Triangle waveform (control bit 4)"),
      saw      (st, Params::vid (v, Params::saw),   "SAW",  "Sawtooth waveform (control bit 5)"),
      pulse    (st, Params::vid (v, Params::pulse), "PUL",  "Pulse waveform (control bit 6). Width set by PW."),
      noise    (st, Params::vid (v, Params::noise), "NOI",  "Noise waveform (control bit 7). Combining noise with other waves can lock the LFSR, as on real chips."),
      ring     (st, Params::vid (v, Params::ring),  "RING", "Ring modulation (bit 2): replaces this voice's triangle with ring-modulated output. Needs TRI. Source: voice " + juce::String ((v + 2) % 3 + 1)),
      sync     (st, Params::vid (v, Params::oscSync),  "SYNC", "Hard sync (bit 1): resets this oscillator on the source oscillator's cycle. Source: voice " + juce::String ((v + 2) % 3 + 1)),
      test     (st, Params::vid (v, Params::test),  "TEST", "Test bit (bit 3): holds the oscillator at zero and resets noise."),
      filt     (st, Params::vid (v, Params::filt),  "FILT", "Route this voice through the SID filter ($D417)"),
      keyTrack (st, Params::vid (v, Params::keyTrack), "KEY", "Key tracking. Off = fixed pitch (C-4 + coarse), useful for drums."),
      arp      (st, Params::vid (v, Params::arp),   "ARP",  "Apply the arpeggio to this voice"),
      hitWave  (st, Params::vid (v, Params::hitWave), "HIT WAVE", "Waveform for the first moments of the note (classic SID drum / pluck trick)"),
      coarse   (st, Params::vid (v, Params::coarse),   "COARSE", "Semitone offset"),
      fine     (st, Params::vid (v, Params::fine),     "FINE",   "Fine tune in cents"),
      pw       (st, Params::vid (v, Params::pw),       "PW",     "12-bit pulse width. PWout = PWn / 40.95 %"),
      pwmRate  (st, Params::vid (v, Params::pwmRate),  "PWM HZ", "Pulse width sweep rate (triangle)"),
      pwmDepth (st, Params::vid (v, Params::pwmDepth), "PWM AMT","Pulse width sweep depth"),
      hitTime  (st, Params::vid (v, Params::hitTime),  "HIT MS", "How long the hit waveform plays"),
      sweep    (st, Params::vid (v, Params::sweep),    "SWEEP",  "Pitch sweep start offset in semitones; glides to the note"),
      sweepTime(st, Params::vid (v, Params::sweepTime),"SWP MS", "Pitch sweep time"),
      a (st, Params::vid (v, Params::attack),  "A", "Attack rate: 2 ms .. 8 s"),
      d (st, Params::vid (v, Params::decay),   "D", "Decay rate: 6 ms .. 24 s"),
      s (st, Params::vid (v, Params::sustain), "S", "Sustain level 0..15"),
      r (st, Params::vid (v, Params::release), "R", "Release rate: 6 ms .. 24 s")
{
    subtitle = "SYNC/RING SRC V" + juce::String ((v + 2) % 3 + 1);
    for (juce::Component* c : std::initializer_list<juce::Component*> { &on, &tri, &saw, &pulse, &noise, &ring, &sync, &test, &filt, &keyTrack, &arp, &hitWave,
                                                                        &coarse, &fine, &pw, &pwmRate, &pwmDepth, &hitTime, &sweep, &sweepTime, &a, &d, &s, &r })
        addAndMakeVisible (c);
}

void VoicePanel::resized()
{
    auto area = content();

    auto row1 = area.removeFromTop (24);
    gridLayout<juce::Component> (row1, 8, 1, { &on, &tri, &saw, &pulse, &noise, &ring, &sync, &test });
    area.removeFromTop (4);

    auto row2 = area.removeFromTop (38);
    auto flagArea = row2.removeFromLeft (row2.getWidth() / 2);
    gridLayout<juce::Component> (flagArea.withTrimmedTop (14).withHeight (24), 3, 1, { &filt, &keyTrack, &arp });
    hitWave.setBounds (row2.withTrimmedLeft (6));
    area.removeFromTop (4);

    auto adsr = area.removeFromRight (area.getWidth() * 34 / 100);
    gridLayout<juce::Component> (adsr, 4, 1, { &a, &d, &s, &r });
    gridLayout<juce::Component> (area, 4, 2, { &coarse, &fine, &pw, &pwmDepth, &pwmRate, &hitTime, &sweep, &sweepTime });
}

//==============================================================================
FilterPanel::FilterPanel (APVTS& st)
    : Panel ("FILTER", C64::accent),
      lp (st, Params::lowPass, "LP", "Low pass (12 dB/oct), $D418 bit 4"),
      bp (st, Params::bandPass, "BP", "Band pass (6 dB/oct), $D418 bit 5"),
      hp (st, Params::highPass, "HP", "High pass (12 dB/oct), $D418 bit 6"),
      v3off (st, Params::voice3Off, "3OFF", "Disconnect voice 3 from the output (only when not routed through the filter), $D418 bit 7"),
      cutoff (st, Params::cutoff, "CUTOFF", "11-bit filter cutoff register FC ($D415/$D416). The 6581 curve is strongly non-linear."),
      res (st, Params::resonance, "RES", "4-bit resonance ($D417 bits 4-7)"),
      envAmt (st, Params::fEnvAmount, "ENV AMT", "Filter envelope amount in cutoff register units"),
      keyTrack (st, Params::fKeyTrack, "KEY TRK", "Cutoff follows the played note"),
      velocity (st, Params::fVelocity, "VEL>CUT", "Velocity to cutoff"),
      attack (st, Params::fAttack, "ATTACK", "Filter envelope attack"),
      decay (st, Params::fDecay, "DECAY", "Filter envelope decay"),
      sustain (st, Params::fSustain, "SUSTAIN", "Filter envelope sustain"),
      release (st, Params::fRelease, "RELEASE", "Filter envelope release")
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &lp, &bp, &hp, &v3off, &cutoff, &res, &envAmt, &keyTrack, &velocity, &attack, &decay, &sustain, &release })
        addAndMakeVisible (c);
}

void FilterPanel::resized()
{
    auto area = content();
    gridLayout<juce::Component> (area.removeFromTop (24), 4, 1, { &lp, &bp, &hp, &v3off });
    area.removeFromTop (6);
    gridLayout<juce::Component> (area, 5, 2, { &cutoff, &res, &envAmt, &keyTrack, &velocity, &attack, &decay, &sustain, &release });
}

//==============================================================================
ModPanel::ModPanel (APVTS& st)
    : Panel ("LFO / ARP / GLIDE", C64::lightBlue.brighter (0.4f)),
      shape (st, Params::lfoShape, "LFO SHAPE"),
      arpMode (st, Params::arpMode, "ARP MODE", "Chord: each note cycles a chord shape. Held Notes: cycles through the keys you hold. Langton: an ant walking a grid plays the chord's notes, turning right to play and left to rest."),
      arpChord (st, Params::arpChord, "CHORD"),
      arpRule (st, Params::arpRule, "ANT RULE", "Langton arp rule: one letter per cell colour. R plays a note, L rests. RL is the classic Langton's ant."),
      rate (st, Params::lfoRate, "LFO HZ"),
      vib (st, Params::lfoVib, "VIBRATO", "LFO to pitch in cents (mod wheel adds up to 100 cents)"),
      delay (st, Params::lfoDelay, "VIB DLY", "Vibrato fade-in delay"),
      toCut (st, Params::lfoCutoff, "LFO>CUT"),
      toPw (st, Params::lfoPW, "LFO>PW"),
      arpSpeed (st, Params::arpSpeed, "ARP SPD", "Arpeggio step length in video frames (50 Hz PAL / 60 Hz NTSC), like a tracker"),
      glide (st, Params::glide, "GLIDE"),
      bend (st, Params::bendRange, "BEND"),
      legato (st, Params::legato, "LEGATO", "Mono legato: overlapping notes glide without retriggering the envelope")
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &shape, &arpMode, &arpChord, &arpRule, &rate, &vib, &delay, &toCut, &toPw, &arpSpeed, &glide, &bend, &legato })
        addAndMakeVisible (c);
}

void ModPanel::resized()
{
    auto area = content();
    auto top = area.removeFromTop (38);
    gridLayout<juce::Component> (top, 4, 1, { &shape, &arpMode, &arpChord, &arpRule });
    for (auto* c : { &shape, &arpMode, &arpChord, &arpRule })
        c->setBounds (c->getBounds().reduced (3, 0));
    area.removeFromTop (6);
    auto legatoRow = area.removeFromBottom (24);
    const auto legatoBounds = legatoRow.removeFromRight (90).reduced (2, 0);
    legato.setBounds (legatoBounds);
    // the second row of knobs has two empty cells on the right; the ANT VIEW button goes in the one above LEGATO
    antArea = { legatoBounds.getX(), legatoBounds.getY() - 28, legatoBounds.getWidth(), 22 };
    gridLayout<juce::Component> (area, 5, 2, { &rate, &vib, &delay, &toCut, &toPw, &arpSpeed, &glide, &bend });
}

//==============================================================================
GlobalPanel::GlobalPanel (APVTS& st)
    : Panel ("CHIP / OUTPUT", C64::cyan),
      poly (st, Params::polyphony, "POLY", "Number of SID chips used in Layered Poly mode (each note gets a whole 3-voice chip)"),
      volume (st, Params::volume, "VOLUME", "4-bit master volume register ($D418 bits 0-3)"),
      velVol (st, Params::velVolume, "VEL>VOL", "Velocity to the volume register"),
      outGain (st, Params::outGain, "OUTPUT"),
      bias (st, Params::filterBias, "FLT BIAS", "Filter curve: real chips vary a lot. 0.5 = typical"),
      hardRestart (st, Params::hardRestart, "HARD RST", "Hard restart: clears ADSR and gate before each note to avoid the SID envelope delay bug"),
      phaseReset (st, Params::phaseReset, "OSC RESET", "Pulse the test bit before each note so the oscillator starts from a known phase"),
      combined (st, Params::combined, "COMBINED WAVES", "Strength of combined waveforms (chips differ)"),
      quality (st, Params::quality, "RESAMPLING")
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &poly, &volume, &velVol, &outGain, &bias, &hardRestart, &phaseReset, &combined, &quality })
        addAndMakeVisible (c);
}

void GlobalPanel::resized()
{
    auto area = content();
    auto top = area.removeFromTop (38);
    combined.setBounds (top.removeFromLeft (top.getWidth() / 2).reduced (3, 0));
    quality.setBounds (top.reduced (3, 0));
    area.removeFromTop (6);
    auto bottom = area.removeFromBottom (24);
    phaseReset.setBounds (bottom.removeFromRight (100).reduced (2, 0));
    gridLayout<juce::Component> (area, 3, 2, { &poly, &volume, &velVol, &outGain, &bias, &hardRestart });
}

//==============================================================================
ChorusPanel::ChorusPanel (APVTS& st)
    : Panel ("JUDO CHORUS", C64::lightBlue.brighter (0.6f)),
      one (st, Params::chorus1, "I", "Chorus I: slow, gentle sweep (0.5 Hz)"),
      two (st, Params::chorus2, "II", "Chorus II: deeper, faster sweep (0.86 Hz). Press both for the fast I+II vibrato.")
{
    addAndMakeVisible (one);
    addAndMakeVisible (two);
    for (auto* b : { &one, &two })
        b->onStateChange = [this] { repaint(); };
}

void ChorusPanel::resized()
{
    auto area = content().withTrimmedTop (6);
    auto buttons = area.removeFromTop (30);
    one.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2).reduced (3, 0));
    two.setBounds (buttons.reduced (3, 0));
}

void ChorusPanel::paint (juce::Graphics& g)
{
    Panel::paint (g);
    const bool a = one.getToggleState(), b = two.getToggleState();
    const juce::String mode = a && b ? "I + II" : a ? "I" : b ? "II" : "OFF";
    auto area = content().withTrimmedTop (44).reduced (3, 0);
    g.setColour (a || b ? C64::accent : C64::textDim);
    g.setFont (SidLookAndFeel::monoFont (13.0f));
    g.drawText ("MODE " + mode, area.removeFromTop (20), juce::Justification::centredLeft);
    g.setColour (C64::textDim);
    g.setFont (SidLookAndFeel::monoFont (11.0f, false));
    g.drawText ("Press both for I+II", area.removeFromTop (18), juce::Justification::centredLeft);
}

//==============================================================================
DelayPanel::DelayPanel (APVTS& st)
    : Panel ("DELAY", C64::yellow),
      on (st, Params::delayOn, "ON"),
      sync (st, Params::delaySync, "SYNC", "Lock the delay time to the host tempo"),
      division (st, Params::delayDivision, "TIME", "Delay time as a note length at the host tempo (T = triplet, D = dotted)"),
      time (st, Params::delayTime, "TIME"),
      feedback (st, Params::delayFeedback, "FEEDBACK", "How much of each echo is fed back: more feedback, more repeats"),
      mix (st, Params::delayMix, "MIX", "Balance of dry signal and echoes")
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &on, &sync, &division, &time, &feedback, &mix })
        addAndMakeVisible (c);
    sync.onStateChange = [this] {
        division.setVisible (sync.getToggleState());
        time.setVisible (! sync.getToggleState());
    };
    sync.onStateChange();
}

void DelayPanel::resized()
{
    auto area = content();
    auto buttons = area.removeFromLeft (64);
    on.setBounds (buttons.removeFromTop (buttons.getHeight() / 2).reduced (2, 3));
    sync.setBounds (buttons.reduced (2, 3));
    area.removeFromLeft (6);
    auto slot = area.removeFromLeft (area.getWidth() / 3);
    time.setBounds (slot);
    division.setBounds (slot.withSizeKeepingCentre (slot.getWidth() - 6, 38));
    gridLayout<juce::Component> (area, 2, 1, { &feedback, &mix });
}

//==============================================================================
ReverbPanel::ReverbPanel (APVTS& st)
    : Panel ("REVERB", C64::lightGreen),
      on (st, Params::reverbOn, "ON"),
      size (st, Params::reverbSize, "SIZE", "Room size: bigger rooms ring longer"),
      damping (st, Params::reverbDamping, "DAMP", "High-frequency damping: more damping, darker tail"),
      mix (st, Params::reverbMix, "MIX", "Balance of dry signal and reverb")
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &on, &size, &damping, &mix })
        addAndMakeVisible (c);
}

void ReverbPanel::resized()
{
    auto area = content();
    auto buttons = area.removeFromLeft (64);
    on.setBounds (buttons.withSizeKeepingCentre (buttons.getWidth(), 30).reduced (2, 3));
    area.removeFromLeft (6);
    gridLayout<juce::Component> (area, 3, 1, { &size, &damping, &mix });
}

//==============================================================================
AntView::AntView (SidEngine& e, APVTS& s) : engine (e), state (s)
{
    setInterceptsMouseClicks (true, false);   // the controls underneath stay covered while it's open
}

void AntView::visibilityChanged()
{
    if (isVisible())
    {
        timerCallback();
        startTimerHz (30);
    }
    else
    {
        stopTimer();
    }
}

void AntView::timerCallback()
{
    for (int i = 0; i < SidEngine::antCells; ++i)
        cells[(size_t) i] = engine.monitorAnt[(size_t) i].load (std::memory_order_relaxed);
    pos = engine.monitorAntPos.load (std::memory_order_relaxed);
    step = engine.monitorAntStep.load (std::memory_order_relaxed);
    offset = engine.monitorAntOffset.load (std::memory_order_relaxed);
    sounding = engine.monitorAntSounding.load (std::memory_order_relaxed);
    repaint();
}

void AntView::paint (juce::Graphics& g)
{
    g.fillAll (C64::background);

    static const juce::Colour palette[] = { C64::blue, C64::lightBlue, C64::cyan, C64::lightGreen, C64::yellow, C64::lightRed,
                                            C64::purple, C64::orange, C64::grey, C64::voice1, C64::voice3, C64::white };
    constexpr int n = 32;

    auto area = getLocalBounds().reduced (16);
    const int side = juce::jmin (area.getHeight(), area.getWidth() * 3 / 5) / n * n;
    auto gridArea = area.removeFromLeft (side).withSizeKeepingCentre (side, side);
    const float cell = (float) side / (float) n;

    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x)
        {
            g.setColour (palette[cells[(size_t) (y * n + x)] % std::size (palette)]);
            g.fillRect (gridArea.getX() + x * cell, gridArea.getY() + y * cell, cell, cell);
        }

    g.setColour (C64::background.withAlpha (0.35f));
    for (int i = 1; i < n; ++i)
    {
        g.drawVerticalLine (gridArea.getX() + (int) (i * cell), (float) gridArea.getY(), (float) gridArea.getBottom());
        g.drawHorizontalLine (gridArea.getY() + (int) (i * cell), (float) gridArea.getX(), (float) gridArea.getRight());
    }
    g.setColour (C64::panelEdge);
    g.drawRect (gridArea.expanded (2), 2);

    if (pos >= 0)
    {
        const auto r = juce::Rectangle<float> (gridArea.getX() + (float) (pos % n) * cell, gridArea.getY() + (float) (pos / n) * cell, cell, cell);
        g.setColour (sounding ? C64::white : C64::textDim);
        g.fillRect (r.reduced (1.0f));
        g.setColour (C64::accent);
        g.drawRect (r.expanded (2.0f), 2.0f);
    }

    // ---- info column ----
    area.removeFromLeft (28);
    const int mode = juce::roundToInt (state.getRawParameterValue (Params::arpMode)->load());
    const int ruleIndex = juce::jlimit (0, Params::numAntRules - 1, juce::roundToInt (state.getRawParameterValue (Params::arpRule)->load()));
    const int chordIndex = juce::jlimit (0, Params::numChords - 1, juce::roundToInt (state.getRawParameterValue (Params::arpChord)->load()));
    const bool langton = mode == (int) Params::ArpMode::Langton;

    g.setColour (C64::text);
    g.setFont (SidLookAndFeel::monoFont (26.0f));
    g.drawText ("LANGTON ARP", area.removeFromTop (40), juce::Justification::centredLeft);
    area.removeFromTop (8);

    auto line = [&] (const juce::String& label, const juce::String& value, juce::Colour c) {
        auto row = area.removeFromTop (26);
        g.setFont (SidLookAndFeel::monoFont (15.0f, false));
        g.setColour (C64::textDim);
        g.drawText (label, row.removeFromLeft (110), juce::Justification::centredLeft);
        g.setColour (c);
        g.drawText (value, row, juce::Justification::centredLeft);
    };
    line ("RULE", Params::antRules[ruleIndex], C64::text);
    line ("CHORD", Params::chords[chordIndex].name, C64::text);
    line ("STEP", pos >= 0 ? juce::String (step) : "-", C64::text);
    line ("NOTE", pos >= 0 ? "+" + juce::String (offset) + " st" : "-", C64::text);
    line ("STATE", pos < 0 ? "IDLE" : (sounding ? "PLAYING" : "RESTING"), sounding ? C64::accent : C64::textDim);

    area.removeFromTop (18);
    g.setFont (SidLookAndFeel::monoFont (13.0f, false));
    g.setColour (langton ? C64::textDim : C64::accent);
    const juce::String hint = ! langton ? "Set ARP MODE to Langton, then hold a note."
                            : pos < 0   ? "Hold a note to start the ant."
                                        : "A right turn plays a note, a left turn rests. Pitch follows the ant's diagonal position, on the notes of the chord.";
    g.drawFittedText (hint, area.removeFromTop (60), juce::Justification::topLeft, 3);

    g.setColour (C64::textDim);
    g.drawFittedText ("Each cell colour is one letter of the rule. Showing chip " + juce::String (engine.monitorChip.load() + 1)
                          + ", the one playing the most recent note.",
                      area.removeFromTop (40), juce::Justification::topLeft, 2);
}

//==============================================================================
Scope::Scope (SidEngine& e) : engine (e) { startTimerHz (30); }

void Scope::timerCallback()
{
    auto& fifo = engine.scopeFifo;
    const auto rd = fifo.read (fifo.getNumReady());
    auto copy = [this] (int start, int n) {
        for (int i = 0; i < n; ++i)
        {
            ring[(size_t) writePos] = engine.scopeData[(size_t) (start + i)];
            writePos = (writePos + 1) % (int) ring.size();
        }
    };
    copy (rd.startIndex1, rd.blockSize1);
    copy (rd.startIndex2, rd.blockSize2);
    repaint();
}

void Scope::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (C64::background);
    g.fillRect (r);
    g.setColour (C64::panelEdge.withAlpha (0.35f));
    g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());

    const int n = (int) ring.size();
    const int window = juce::jmin (1024, n / 2);

    // find a rising zero crossing for a stable display
    int start = (writePos - window - 1 + n) % n;
    for (int i = 0; i < n / 2 - window; ++i)
    {
        const int idx = (writePos - window - 1 - i + 2 * n) % n;
        if (ring[(size_t) idx] <= 0.0f && ring[(size_t) ((idx + 1) % n)] > 0.0f) { start = idx; break; }
    }

    juce::Path p;
    for (int i = 0; i < window; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) (window - 1);
        const float y = r.getCentreY() - juce::jlimit (-1.0f, 1.0f, ring[(size_t) ((start + i) % n)] * 1.4f) * r.getHeight() * 0.46f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (C64::lightGreen);
    g.strokePath (p, juce::PathStrokeType (1.5f));
}

//==============================================================================
RegisterView::RegisterView (SidEngine& e) : engine (e) { startTimerHz (20); }

void RegisterView::timerCallback()
{
    bool changed = false;
    for (int i = 0; i < SidEngine::numRegs; ++i)
    {
        const auto v = engine.monitorRegs[(size_t) i].load (std::memory_order_relaxed);
        changed = changed || v != regs[(size_t) i];
        regs[(size_t) i] = v;
    }
    const int c = engine.monitorChip.load(), a = engine.activeChipCount.load();
    changed = changed || c != chip || a != active;
    chip = c; active = a;
    if (changed) repaint();
}

void RegisterView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    g.setFont (SidLookAndFeel::monoFont (12.0f, false));

    g.setColour (C64::textDim);
    g.drawText ("CHIP " + juce::String (chip + 1) + "   ACTIVE CHIPS " + juce::String (active), r.removeFromTop (16), juce::Justification::centredLeft);

    const int cols = 5, rows = 5;
    const int w = r.getWidth() / cols, h = r.getHeight() / rows;
    for (int i = 0; i < SidEngine::numRegs; ++i)
    {
        const int voice = i < 21 ? i / 7 : 3;
        const auto cell = juce::Rectangle<int> (r.getX() + (i / rows) * w, r.getY() + (i % rows) * h, w, h);
        g.setColour (voice == 0 ? C64::voice1 : voice == 1 ? C64::voice2 : voice == 2 ? C64::voice3 : C64::accent);
        g.drawText (juce::String::toHexString (0xd400 + i).toUpperCase(), cell.withWidth (w / 2 + 6), juce::Justification::centredLeft);
        g.setColour (C64::text);
        g.drawText (juce::String::toHexString (regs[(size_t) i]).toUpperCase().paddedLeft ('0', 2), cell.withTrimmedLeft (w / 2 + 6), juce::Justification::centredLeft);
    }
}
}

//==============================================================================
DesiduousEditor::DesiduousEditor (DesiduousProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      chip (p.state, Params::chipModel, "CHIP", "MOS 6581: warm, distorted, non-linear filter. MOS 8580: cleaner, linear filter, stronger combined waveforms."),
      mode (p.state, Params::voiceMode, "VOICE MODE", "Layered Poly: each note plays all 3 voices on its own chip. Paraphonic: one chip, one note per voice. MIDI Ch 1-3: channel N drives voice N."),
      clock (p.state, Params::clock, "CLOCK"),
      filter (p.state), mod (p.state), global (p.state),
      chorus (p.state), delay (p.state), reverb (p.state),
      antView (p.engine, p.state),
      scopePanel ("OSCILLOSCOPE", C64::lightGreen), regPanel ("REGISTERS $D400-$D418", C64::accent),
      scope (p.engine), registers (p.engine),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    const auto& presets = getFactoryPresets();
    // One submenu per category; the General MIDI families are grouped under their own submenu
    juce::StringArray categories;
    for (const auto& preset : presets)
        categories.addIfNotAlreadyThere (preset.category);

    auto* root = presetBox.getRootMenu();
    juce::PopupMenu generalMidi;
    for (const auto& cat : categories)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < (int) presets.size(); ++i)
            if (presets[(size_t) i].category == cat)
                sub.addItem (i + 1, presets[(size_t) i].name);

        if (cat.startsWith ("GM "))
            generalMidi.addSubMenu (cat.fromFirstOccurrenceOf ("GM ", false, false), sub);
        else
            root->addSubMenu (cat, sub);
    }
    if (generalMidi.getNumItems() > 0)
    {
        root->addSeparator();
        root->addSubMenu ("General MIDI", generalMidi);
    }
    presetBox.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { choosePreset (presetBox.getSelectedId() - 1); };
    prevButton.onClick = [this] { choosePreset (juce::jmax (0, processor.getCurrentProgram() - 1)); };
    nextButton.onClick = [this] { choosePreset (juce::jmin (processor.getNumPrograms() - 1, processor.getCurrentProgram() + 1)); };

    addAndMakeVisible (presetBox);
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    addAndMakeVisible (chip);
    addAndMakeVisible (mode);
    addAndMakeVisible (clock);

    for (int v = 0; v < 3; ++v)
    {
        voices[v] = std::make_unique<ui::VoicePanel> (p.state, v);
        addAndMakeVisible (*voices[v]);
    }
    addAndMakeVisible (filter);
    addAndMakeVisible (mod);
    addAndMakeVisible (global);
    addAndMakeVisible (scopePanel);
    addAndMakeVisible (regPanel);
    scopePanel.addAndMakeVisible (scope);
    regPanel.addAndMakeVisible (registers);

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, C64::text);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, C64::blue);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, C64::background);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, C64::accent);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, C64::lightBlue.withAlpha (0.5f));
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, C64::panel);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, C64::text);
    keyboard.setLowestVisibleKey (36);
    addAndMakeVisible (keyboard);

    addAndMakeVisible (chorus);
    addAndMakeVisible (delay);
    addAndMakeVisible (reverb);

    // the scope and register monitor share one panel
    scopeButton.setTooltip ("Switch between the oscilloscope and the SID register monitor");
    scopeButton.onClick = [this] {
        processor.showRegisters = ! processor.showRegisters;
        updateScopeMode();
    };
    addAndMakeVisible (scopeButton);
    updateScopeMode();

    antButton.setClickingTogglesState (true);
    antButton.setTooltip ("Show the Langton arp's ant grid over the controls");
    antButton.setToggleState (processor.showAntView, juce::dontSendNotification);
    antButton.onClick = [this] {
        processor.showAntView = antButton.getToggleState();
        antView.setVisible (processor.showAntView);
        if (processor.showAntView)
        {
            antView.toFront (false);
            antButton.toFront (false);   // the button now sits inside the area the ant view covers, so it has to stay on top
        }
    };
    addAndMakeVisible (antButton);
    addChildComponent (antView);
    antView.setVisible (processor.showAntView);
    antButton.toFront (false);
    updateAntButton();

    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    setSize (1184, 770 - 54 + headerHeight);
    startTimerHz (10);
}

void DesiduousEditor::updateScopeMode()
{
    scopePanel.setVisible (! processor.showRegisters);
    regPanel.setVisible (processor.showRegisters);
    scopeButton.setButtonText (processor.showRegisters ? "SCOPE" : "REGS");
    scopeButton.toFront (false);
    if (antView.isVisible())
    {
        antView.toFront (false);
        antButton.toFront (false);
    }
}

void DesiduousEditor::updateAntButton()
{
    // the ant grid belongs to the Langton arp, so the button only works while ARP MODE is Langton
    bool langton = false;
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (processor.state.getParameter (Params::arpMode)))
        langton = p->getCurrentChoiceName() == "Langton";

    if (antButton.isEnabled() != langton)
    {
        antButton.setEnabled (langton);
        antButton.setTooltip (langton ? "Show the Langton arp's ant grid over the controls"
                                      : "Available when ARP MODE is set to Langton");
    }

    if (! langton && (processor.showAntView || antView.isVisible()))   // leaving Langton closes the view
    {
        processor.showAntView = false;
        antButton.setToggleState (false, juce::dontSendNotification);
        antView.setVisible (false);
    }
}

DesiduousEditor::~DesiduousEditor()
{
    setLookAndFeel (nullptr);
}

void DesiduousEditor::choosePreset (int index)
{
    if (index < 0 || index >= processor.getNumPrograms())
        return;
    processor.setCurrentProgram (index);
    processor.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
    presetBox.setSelectedId (index + 1, juce::dontSendNotification);
}

void DesiduousEditor::timerCallback()
{
    const int id = processor.getCurrentProgram() + 1;
    if (presetBox.getSelectedId() != id)
        presetBox.setSelectedId (id, juce::dontSendNotification);
    updateAntButton();
}

void DesiduousEditor::paint (juce::Graphics& g)
{
    g.fillAll (C64::background);

    // header bar in the logo's own night-black, with the logo on the left
    auto headerArea = getLocalBounds().removeFromTop (headerHeight);
    g.setColour (Logo::night);
    g.fillRect (headerArea);

    // logo on the left with a one-line caption underneath
    auto logoColumn = headerArea.withTrimmedLeft (12).withWidth (logoWidth).withTrimmedBottom (4);
    auto caption = logoColumn.removeFromBottom (18);
    auto logoArea = logoColumn.withTrimmedTop (6);
    if (logo.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImageWithin (logo, logoArea.getX(), logoArea.getY(), logoArea.getWidth(), logoArea.getHeight(),
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }

    g.setFont (SidLookAndFeel::monoFont (11.0f, false));
    g.setColour (Logo::cyan);
    g.drawText ("MOS 6581/8580 SID SYNTH", caption, juce::Justification::centredLeft);
    g.setColour (Logo::blue);
    g.drawText ("v" JucePlugin_VersionString, caption, juce::Justification::centredRight);

    // cyan / blue split line under the header, echoing the logo's two-tone lettering
    auto line = headerArea.removeFromBottom (4);
    g.setColour (Logo::cyan);
    g.fillRect (line.removeFromTop (2));
    g.setColour (Logo::blue);
    g.fillRect (line.translated (6, 0));
}

void DesiduousEditor::resized()
{
    const int gap = 8;
    auto area = getLocalBounds().reduced (gap, 0);

    auto header = area.removeFromTop (headerHeight).withTrimmedBottom (4);
    header = header.withSizeKeepingCentre (header.getWidth(), 40);
    header.removeFromLeft (4 + logoWidth + 16);
    auto presetArea = header.removeFromLeft (300);
    prevButton.setBounds (presetArea.removeFromLeft (28).withTrimmedTop (14).withHeight (22));
    nextButton.setBounds (presetArea.removeFromRight (28).withTrimmedTop (14).withHeight (22));
    presetBox.setBounds (presetArea.reduced (4, 0).withTrimmedTop (14).withHeight (22));
    header.removeFromLeft (8);
    chip.setBounds (header.removeFromLeft (110).reduced (3, 0));
    mode.setBounds (header.removeFromLeft (175).reduced (3, 0));
    clock.setBounds (header.removeFromLeft (130).reduced (3, 0));

    area.removeFromTop (gap);
    const int colW = (area.getWidth() - 2 * gap) / 3;

    auto row = area.removeFromTop (250);
    for (int v = 0; v < 3; ++v)
    {
        voices[v]->setBounds (row.removeFromLeft (colW));
        row.removeFromLeft (gap);
    }

    area.removeFromTop (gap);
    row = area.removeFromTop (230);
    filter.setBounds (row.removeFromLeft (colW)); row.removeFromLeft (gap);
    mod.setBounds (row.removeFromLeft (colW));    row.removeFromLeft (gap);
    antButton.setBounds (mod.antSlot().translated (mod.getX(), mod.getY()));   // above LEGATO in the arp section
    global.setBounds (row);

    // effects share the bottom row with the scope; the scope panel flips to the registers
    area.removeFromTop (gap);
    row = area.removeFromTop (130);
    chorus.setBounds (row.removeFromLeft (150)); row.removeFromLeft (gap);
    delay.setBounds (row.removeFromLeft (340));  row.removeFromLeft (gap);
    reverb.setBounds (row.removeFromLeft (262)); row.removeFromLeft (gap);
    scopePanel.setBounds (row);
    regPanel.setBounds (row);
    scope.setBounds (scopePanel.content());
    registers.setBounds (regPanel.content());
    scopeButton.setBounds (row.getRight() - 70, row.getY() + 2, 64, 16);

    area.removeFromTop (gap);
    keyboard.setBounds (area.removeFromTop (70));

    // the ant view covers everything between the header and the keyboard
    antView.setBounds (getLocalBounds().withTop (headerHeight).withBottom (keyboard.getY() - gap / 2));
}
