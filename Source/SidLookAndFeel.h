#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Commodore 64 palette (Pepto) plus a few derived UI shades
namespace C64
{
    const juce::Colour black      { 0xff000000 };
    const juce::Colour white      { 0xffffffff };
    const juce::Colour blue       { 0xff352879 };
    const juce::Colour lightBlue  { 0xff6c5eb5 };
    const juce::Colour cyan       { 0xff70a4b2 };
    const juce::Colour lightRed   { 0xff9a6759 };
    const juce::Colour lightGreen { 0xff9ad284 };
    const juce::Colour yellow     { 0xffb8c76f };
    const juce::Colour orange     { 0xff6f4f25 };
    const juce::Colour purple     { 0xff6f3d86 };
    const juce::Colour grey       { 0xff6c6c6c };

    const juce::Colour background { 0xff241b5c };
    const juce::Colour panel      { 0xff2e2470 };
    const juce::Colour panelEdge  { 0xff6c5eb5 };
    const juce::Colour text       { 0xffc4bcf5 };
    const juce::Colour textDim    { 0xff8d83cf };
    const juce::Colour voice1     { 0xffe08a78 };
    const juce::Colour voice2     { 0xff9ad284 };
    const juce::Colour voice3     { 0xff7fc4d6 };
    const juce::Colour accent     { 0xffd8e08a };
}

class SidLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SidLookAndFeel();

    static juce::Font monoFont (float size, bool bold = true);
    static juce::Colour accentOf (const juce::Component& c, juce::Colour fallback = C64::accent);

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;

    juce::Font getLabelFont (juce::Label&) override        { return monoFont (12.0f); }
    juce::Font getComboBoxFont (juce::ComboBox&) override  { return monoFont (12.5f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return monoFont (11.5f); }
    juce::Font getPopupMenuFont() override                  { return monoFont (13.0f, false); }
    juce::Label* createSliderTextBox (juce::Slider&) override;
};
