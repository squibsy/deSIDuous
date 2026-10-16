#include "SidLookAndFeel.h"

SidLookAndFeel::SidLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, C64::background);
    setColour (juce::Label::textColourId, C64::text);
    setColour (juce::Slider::textBoxTextColourId, C64::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, C64::lightBlue);
    setColour (juce::ComboBox::backgroundColourId, C64::blue);
    setColour (juce::ComboBox::textColourId, C64::text);
    setColour (juce::ComboBox::outlineColourId, C64::panelEdge);
    setColour (juce::ComboBox::arrowColourId, C64::text);
    setColour (juce::PopupMenu::backgroundColourId, C64::blue);
    setColour (juce::PopupMenu::textColourId, C64::text);
    setColour (juce::PopupMenu::headerTextColourId, C64::accent);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, C64::lightBlue);
    setColour (juce::PopupMenu::highlightedTextColourId, C64::white);
    setColour (juce::TextButton::textColourOffId, C64::textDim);
    setColour (juce::TextButton::textColourOnId, C64::background);
    setColour (juce::TooltipWindow::backgroundColourId, C64::blue);
    setColour (juce::TooltipWindow::textColourId, C64::text);
    setColour (juce::TooltipWindow::outlineColourId, C64::panelEdge);
    setColour (juce::TextEditor::backgroundColourId, C64::blue);
    setColour (juce::TextEditor::textColourId, C64::text);
    setColour (juce::TextEditor::highlightColourId, C64::lightBlue);
    setColour (juce::CaretComponent::caretColourId, C64::accent);
}

juce::Font SidLookAndFeel::monoFont (float size, bool bold)
{
    return juce::Font (juce::FontOptions ("Consolas", size, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Colour SidLookAndFeel::accentOf (const juce::Component& c, juce::Colour fallback)
{
    for (auto* p = &c; p != nullptr; p = p->getParentComponent())
        if (p->getProperties().contains ("accent"))
            return juce::Colour ((juce::uint32) (juce::int64) p->getProperties()["accent"]);
    return fallback;
}

juce::Label* SidLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (monoFont (11.0f, false));
    l->setColour (juce::Label::textColourId, C64::textDim);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setJustificationType (juce::Justification::centred);
    return l;
}

void SidLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float start, float end, juce::Slider& s)
{
    const auto accent = accentOf (s);
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const float track = juce::jmax (2.5f, radius * 0.16f);

    juce::Path bg;
    bg.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, start, end, true);
    g.setColour (C64::background);
    g.strokePath (bg, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    // bipolar parameters fill from the centre
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float from = bipolar ? start + (float) s.valueToProportionOfLength (0.0) * (end - start) : start;

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setColour (s.isEnabled() ? accent : C64::grey);
    g.strokePath (arc, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    const float knobR = radius - track * 2.2f;
    g.setColour (C64::blue);
    g.fillEllipse (juce::Rectangle<float> (knobR * 2, knobR * 2).withCentre (centre));
    g.setColour (C64::panelEdge);
    g.drawEllipse (juce::Rectangle<float> (knobR * 2, knobR * 2).withCentre (centre), 1.0f);

    const juce::Point<float> tip = centre.getPointOnCircumference (knobR * 0.85f, angle);
    g.setColour (C64::text);
    g.drawLine ({ centre.getPointOnCircumference (knobR * 0.2f, angle), tip }, 2.0f);
}

void SidLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                                       juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, minPos, maxPos, style, s);
        return;
    }

    // Segmented "block character" bar: one segment per register step (0..15)
    const auto accent = accentOf (s);
    const auto area = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (juce::jmax (2.0f, w * 0.28f), 1.0f);
    const int steps = juce::roundToInt (s.getMaximum() - s.getMinimum());
    const int value = juce::roundToInt (s.getValue() - s.getMinimum());
    const float segH = area.getHeight() / (float) (steps + 1);

    for (int i = 0; i <= steps; ++i)
    {
        auto seg = juce::Rectangle<float> (area.getX(), area.getBottom() - (i + 1) * segH, area.getWidth(), segH).reduced (0.0f, 1.0f);
        g.setColour (i <= value ? accent.withMultipliedBrightness (0.65f + 0.35f * (float) i / (float) steps) : C64::background);
        g.fillRect (seg);
    }
}

void SidLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    const auto accent = accentOf (b);
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();

    g.setColour (on ? accent : C64::background);
    g.fillRect (r);
    g.setColour (on ? accent.brighter (0.3f) : (over ? C64::text : C64::panelEdge));
    g.drawRect (r, 1.0f);
    if (down)
    {
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.fillRect (r);
    }
}

void SidLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.getToggleState() ? C64::background : C64::text);
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (2, 0), juce::Justification::centred, 1);
}

void SidLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<int> (w, h).toFloat().reduced (0.5f);
    g.setColour (C64::blue);
    g.fillRect (r);
    g.setColour (box.hasKeyboardFocus (false) ? C64::text : C64::panelEdge);
    g.drawRect (r, 1.0f);

    juce::Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax, ay - 3.0f, ax + 8.0f, ay - 3.0f, ax + 4.0f, ay + 3.0f);
    g.setColour (C64::text);
    g.fillPath (arrow);
}
