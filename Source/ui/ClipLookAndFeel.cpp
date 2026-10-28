#include "ClipLookAndFeel.h"
#include "BinaryData.h"

// Loaded once and shared by every open editor
struct ClipLookAndFeel::Typefaces
{
    juce::Typeface::Ptr regular = juce::Typeface::createSystemTypefaceFor (BinaryData::ArchivoMedium_ttf, BinaryData::ArchivoMedium_ttfSize);
    juce::Typeface::Ptr bold = juce::Typeface::createSystemTypefaceFor (BinaryData::ArchivoBold_ttf, BinaryData::ArchivoBold_ttfSize);
    juce::Typeface::Ptr display = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowRegular_ttf, BinaryData::BarlowRegular_ttfSize);
    juce::Typeface::Ptr readout = juce::Typeface::createSystemTypefaceFor (BinaryData::ShareTechMonoRegular_ttf, BinaryData::ShareTechMonoRegular_ttfSize);
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

    setColour (juce::Slider::rotarySliderFillColourId, Palette::pointer);
    setColour (juce::Slider::textBoxTextColourId, Palette::accent);
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

    setColour (juce::ToggleButton::textColourId, Palette::silkscreen);
}

ClipLookAndFeel::~ClipLookAndFeel() = default;

juce::Font ClipLookAndFeel::font (float height, bool bold)
{
    const juce::SharedResourcePointer<Typefaces> shared;
    return juce::Font (juce::FontOptions (bold ? shared->bold : shared->regular).withHeight (height));
}

juce::Font ClipLookAndFeel::displayFont (float height)
{
    const juce::SharedResourcePointer<Typefaces> shared;
    return juce::Font (juce::FontOptions (shared->display).withHeight (height));
}

juce::Font ClipLookAndFeel::readoutFont (float height)
{
    const juce::SharedResourcePointer<Typefaces> shared;
    return juce::Font (juce::FontOptions (shared->readout).withHeight (height));
}

// Slider values sit in a small recessed window, lit in the accent colour. Other labels draw as usual.
void ClipLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (! isReadout (label))
    {
        LookAndFeel_V4::drawLabel (g, label);
        return;
    }

    const auto bounds = label.getLocalBounds().toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (Palette::readout);
    g.fillRoundedRectangle (bounds.withTrimmedBottom (1.0f), 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.fillRect (bounds.reduced (3.0f, 0.0f).withHeight (1.0f));

    if (label.isBeingEdited())
        return;

    const auto colour = label.findColour (juce::Label::textColourId);
    g.setFont (getLabelFont (label));

    // A soft glow, then the digits
    g.setColour (colour.withAlpha (0.12f));
    for (const auto offset : { juce::Point (-1.0f, 0.0f), juce::Point (1.0f, 0.0f), juce::Point (0.0f, -1.0f), juce::Point (0.0f, 1.0f) })
        g.drawText (label.getText(), bounds.translated (offset.x, offset.y), juce::Justification::centred, false);

    g.setColour (colour);
    g.drawText (label.getText(), bounds, juce::Justification::centred, false);
}

// A raised push button with an LED that lights while it's on
void ClipLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f, 2.0f);
    if (shouldDrawButtonAsDown)
        bounds.translate (0.0f, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (bounds.translated (0.0f, shouldDrawButtonAsDown ? 1.0f : 2.0f), 4.0f);
    g.setColour (Palette::engraving);
    g.fillRoundedRectangle (bounds, 4.0f);

    const auto face = bounds.reduced (1.0f);
    const auto brightness = shouldDrawButtonAsHighlighted && ! shouldDrawButtonAsDown ? 0.04f : 0.0f;
    g.setGradientFill (juce::ColourGradient::vertical (Palette::buttonTop.brighter (brightness), face.getY(), Palette::buttonBottom, face.getBottom()));
    g.fillRoundedRectangle (face, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.fillRect (face.reduced (3.0f, 0.0f).withHeight (1.0f));

    const auto text = button.getButtonText().toUpperCase();
    const auto textFont = font (9.5f, true).withExtraKerningFactor (0.16f);
    const auto ledSize = 7.0f, gap = 8.0f;
    const auto contentWidth = ledSize + gap + juce::GlyphArrangement::getStringWidth (textFont, text);
    auto content = face.withSizeKeepingCentre (contentWidth, face.getHeight());

    const auto led = content.removeFromLeft (ledSize).withSizeKeepingCentre (ledSize, ledSize);
    if (button.getToggleState())
    {
        g.setColour (Palette::led.withAlpha (0.25f));
        g.fillEllipse (led.expanded (3.0f));
        g.setColour (Palette::led);
        g.fillEllipse (led);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.fillEllipse (led.reduced (2.0f).translated (-0.5f, -0.5f));
    }
    else
    {
        g.setColour (Palette::ledOff);
        g.fillEllipse (led);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (led.reduced (0.5f), 1.0f);
    }

    g.setColour (button.findColour (juce::ToggleButton::textColourId));
    g.setFont (textFont);
    g.drawText (text, content.withTrimmedLeft (gap), juce::Justification::centredLeft, false);
}

// A machined knob with a knurled edge, inside a printed scale. The light comes from above, so the shading
// stays put while the pointer turns. A rotary switch is a smaller knob, with its choices printed by RotarySwitch.
void ClipLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                        float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    const auto centre = juce::Rectangle (x, y, width, height).toFloat().getCentre();
    const auto isSwitch = (bool) slider.getProperties()[rotarySwitchProperty];
    const auto scaleRadius = 41.0f, capRadius = isSwitch ? 20.0f : 28.0f, faceRadius = capRadius - 5.0f;
    const auto circle = [&] (float radius) { return juce::Rectangle (radius * 2.0f, radius * 2.0f).withCentre (centre); };

    // Major ticks at both ends and the middle
    constexpr auto numTicks = 11;
    for (auto i = 0; i < (isSwitch ? 0 : numTicks); ++i)
    {
        const auto major = i == 0 || i == numTicks / 2 || i == numTicks - 1;
        const auto angle = rotaryStartAngle + (float) i / (numTicks - 1) * (rotaryEndAngle - rotaryStartAngle);
        const auto tickWidth = major ? 2.0f : 1.5f;

        juce::Path tick;
        tick.addRectangle (-tickWidth / 2.0f, -scaleRadius, tickWidth, major ? 7.0f : 5.0f);
        g.setColour (major ? Palette::tickMajor : Palette::tickMinor);
        g.fillPath (tick, juce::AffineTransform::rotation (angle).translated (centre));
    }

    // Shadow, widening and fading below the knob
    for (auto i = 0; i < 4; ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.22f));
        g.fillEllipse (circle (capRadius + (float) i).translated (0.0f, 2.0f + (float) i * 1.5f));
    }

    constexpr auto numKnurls = 60;
    for (auto i = 0; i < numKnurls; ++i)
    {
        juce::Path knurl;
        const auto from = juce::MathConstants<float>::twoPi * (float) i / numKnurls;
        knurl.addPieSegment (circle (capRadius), from, from + juce::MathConstants<float>::twoPi / numKnurls, 0.0f);
        g.setColour (i % 2 == 0 ? Palette::knurlLight : Palette::knurlDark);
        g.fillPath (knurl);
    }

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (circle (faceRadius + 1.0f));
    g.setGradientFill ({ Palette::capLight, centre.translated (0.0f, -faceRadius * 0.8f), Palette::cap, centre.translated (0.0f, faceRadius * 0.6f), true });
    g.fillEllipse (circle (faceRadius));

    // Turned metal catches the light on opposite sides
    for (const auto [middle, colour] : { std::pair (0.35f, juce::Colours::white.withAlpha (0.06f)), std::pair (3.49f, juce::Colours::white.withAlpha (0.04f)),
                                         std::pair (1.92f, juce::Colours::black.withAlpha (0.12f)), std::pair (5.06f, juce::Colours::black.withAlpha (0.12f)) })
    {
        juce::Path sheen;
        sheen.addPieSegment (circle (faceRadius), middle - 0.45f, middle + 0.45f, 0.0f);
        g.setColour (colour);
        g.fillPath (sheen);
    }

    juce::Path rim;
    rim.addCentredArc (centre.x, centre.y, faceRadius - 0.5f, faceRadius - 0.5f, 0.0f, -1.2f, 1.2f, true);
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.strokePath (rim, juce::PathStrokeType (1.0f));

    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.0f, -(capRadius - 7.0f), 2.0f, capRadius * 0.46f, 1.0f);
    g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}
