#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "SidLookAndFeel.h"

namespace ui
{
using APVTS = juce::AudioProcessorValueTreeState;

class Knob : public juce::Component
{
public:
    Knob (APVTS& s, const juce::String& id, const juce::String& name, const juce::String& tip = {});
    void resized() override;
    juce::Slider slider;
private:
    juce::Label label;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

class Toggle : public juce::TextButton
{
public:
    Toggle (APVTS& s, const juce::String& id, const juce::String& name, const juce::String& tip = {});
private:
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};

class Choice : public juce::Component
{
public:
    Choice (APVTS& s, const juce::String& id, const juce::String& name, const juce::String& tip = {});
    void resized() override;
    juce::ComboBox box;
private:
    juce::Label label;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};

// Vertical 0..15 register slider (attack, decay, sustain, release)
class Nibble : public juce::Component
{
public:
    Nibble (APVTS& s, const juce::String& id, const juce::String& name, const juce::String& tip = {});
    void resized() override;
    juce::Slider slider;
private:
    juce::Label label, value;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

class Panel : public juce::Component
{
public:
    Panel (const juce::String& title, juce::Colour accent);
    void paint (juce::Graphics&) override;
    juce::Rectangle<int> content() const { return getLocalBounds().reduced (8).withTrimmedTop (20); }
    juce::String subtitle;
protected:
    juce::String title;
    juce::Colour accent;
};

class VoicePanel : public Panel
{
public:
    VoicePanel (APVTS& s, int voiceIndex);
    void resized() override;
private:
    Toggle on, tri, saw, pulse, noise, ring, sync, test, filt, keyTrack, arp;
    Choice hitWave;
    Knob coarse, fine, pw, pwmRate, pwmDepth, hitTime, sweep, sweepTime;
    Nibble a, d, s, r;
};

class FilterPanel : public Panel
{
public:
    explicit FilterPanel (APVTS& s);
    void resized() override;
private:
    Toggle lp, bp, hp, v3off;
    Knob cutoff, res, envAmt, keyTrack, velocity, attack, decay, sustain, release;
};

class ModPanel : public Panel
{
public:
    explicit ModPanel (APVTS& s);
    void resized() override;
private:
    Choice shape, arpMode, arpChord;
    Knob rate, vib, delay, toCut, toPw, arpSpeed, glide, bend;
    Toggle legato;
};

class GlobalPanel : public Panel
{
public:
    explicit GlobalPanel (APVTS& s);
    void resized() override;
private:
    Knob poly, volume, velVol, outGain, bias, hardRestart;
    Toggle phaseReset;
    Choice combined, quality;
};

class Scope : public juce::Component, private juce::Timer
{
public:
    explicit Scope (SidEngine& e);
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    SidEngine& engine;
    std::array<float, 4096> ring {};
    int writePos = 0;
};

class RegisterView : public juce::Component, private juce::Timer
{
public:
    explicit RegisterView (SidEngine& e);
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    SidEngine& engine;
    std::array<uint8_t, SidEngine::numRegs> regs {};
    int chip = 0, active = 0;
};
}

//==============================================================================
class DesiduousEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DesiduousEditor (DesiduousProcessor&);
    ~DesiduousEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void choosePreset (int index);

    DesiduousProcessor& processor;
    static constexpr int headerHeight = 84;
    static constexpr int logoWidth = 340;   // logo is 6.4:1, drawn ~53 px tall

    SidLookAndFeel lnf;
    juce::Image logo;
    juce::TooltipWindow tooltips { this, 600 };

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    ui::Choice chip, mode, clock;

    std::unique_ptr<ui::VoicePanel> voices[3];
    ui::FilterPanel filter;
    ui::ModPanel mod;
    ui::GlobalPanel global;
    ui::Panel scopePanel, regPanel;
    ui::Scope scope;
    ui::RegisterView registers;
    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DesiduousEditor)
};
