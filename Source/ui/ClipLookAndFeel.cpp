#include "ClipLookAndFeel.h"
#include "BinaryData.h"

// Loaded once and shared by every open editor
struct ClipLookAndFeel::Typefaces
{
    juce::Typeface::Ptr regular = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowRegular_ttf, BinaryData::BarlowRegular_ttfSize);
    juce::Typeface::Ptr bold = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowSemiBold_ttf, BinaryData::BarlowSemiBold_ttfSize);
};

ClipLookAndFeel::ClipLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Palette::background);
    setColour (juce::Label::textColourId, Palette::text);
    setColour (juce::Label::textWhenEditingColourId, Palette::text);
    setColour (juce::Label::backgroundWhenEditingColourId, Palette::knob);
    setColour (juce::Label::outlineWhenEditingColourId, Palette::accent);
    setColour (juce::TextEditor::highlightColourId, Palette::accent.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, Palette::accent);

    setColour (juce::Slider::rotarySliderFillColourId, Palette::accent);
    setColour (juce::Slider::textBoxTextColourId, Palette::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, Palette::accent.withAlpha (0.4f));

    setColour (juce::ComboBox::backgroundColourId, Palette::knob);
    setColour (juce::ComboBox::outlineColourId, Palette::track);
    setColour (juce::ComboBox::focusedOutlineColourId, Palette::accent);
    setColour (juce::ComboBox::textColourId, Palette::text);
    setColour (juce::ComboBox::arrowColourId, Palette::textDim);
    setColour (juce::PopupMenu::backgroundColourId, Palette::knob);
    setColour (juce::PopupMenu::textColourId, Palette::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Palette::accent.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, Palette::text);

    setColour (juce::ToggleButton::textColourId, Palette::text);
    setColour (juce::ToggleButton::tickColourId, Palette::accent);
    setColour (juce::ToggleButton::tickDisabledColourId, Palette::textDim);
}

ClipLookAndFeel::~ClipLookAndFeel() = default;

juce::Font ClipLookAndFeel::font (float height, bool bold)
{
    const juce::SharedResourcePointer<Typefaces> shared;
    return juce::Font (juce::FontOptions (bold ? shared->bold : shared->regular).withHeight (height));
}

// LookAndFeel_V4's toggle, drawing its text with the embedded font
void ClipLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    const auto fontSize = std::min (15.0f, (float) button.getHeight() * 0.75f);
    const auto tickWidth = fontSize * 1.1f;

    drawTickBox (g, button, 4.0f, ((float) button.getHeight() - tickWidth) * 0.5f, tickWidth, tickWidth,
                 button.getToggleState(), button.isEnabled(), shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    g.setColour (button.findColour (juce::ToggleButton::textColourId));
    g.setFont (font (fontSize));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().withTrimmedLeft (juce::roundToInt (tickWidth) + 10).withTrimmedRight (2),
                      juce::Justification::centredLeft, 10);
}

// A track arc with the value filled in, around a knob with a pointer. Ranges that cross 0 (the gains)
// fill from 0, so the arc shows boost and cut.
void ClipLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                        float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle (x, y, width, height).toFloat().reduced (4.0f);
    const auto centre = bounds.getCentre();
    const auto arcRadius = std::min (bounds.getWidth(), bounds.getHeight()) / 2.0f - 2.0f;
    const auto bodyRadius = arcRadius - 8.0f;

    const auto angleAt = [&] (float proportion) { return rotaryStartAngle + proportion * (rotaryEndAngle - rotaryStartAngle); };
    const auto bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const auto valueAngle = angleAt (sliderPos);
    const auto originAngle = angleAt (bipolar ? (float) slider.valueToProportionOfLength (0.0) : 0.0f);

    const auto strokeArc = [&] (float from, float to, juce::Colour colour) {
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, from, to, true);
        g.setColour (colour);
        g.strokePath (arc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };

    strokeArc (rotaryStartAngle, rotaryEndAngle, Palette::track);
    strokeArc (std::min (originAngle, valueAngle), std::max (originAngle, valueAngle), slider.findColour (juce::Slider::rotarySliderFillColourId));

    g.setColour (Palette::knob);
    g.fillEllipse (juce::Rectangle (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre));

    g.setColour (Palette::text);
    g.drawLine ({ centre.getPointOnCircumference (bodyRadius * 0.35f, valueAngle), centre.getPointOnCircumference (bodyRadius * 0.85f, valueAngle) }, 2.5f);
}
