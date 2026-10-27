#pragma once
#include <JuceHeader.h>

// Every colour in the plugin
namespace Palette
{
    inline const juce::Colour background { 0xff16181d };
    inline const juce::Colour display { 0xff0d0f13 };
    inline const juce::Colour knob { 0xff262a32 };
    inline const juce::Colour track { 0xff31353e };
    inline const juce::Colour text { 0xffe4e6eb };
    inline const juce::Colour textDim { 0xff8b919d };
    inline const juce::Colour accent { 0xff4cc9e0 }; // output signal, values, the curve
    inline const juce::Colour clip { 0xffff5a36 }; // the part of the input that gets clipped
    inline const juce::Colour detail { 0xffffc94a }; // what Detail keeps on the clipped peaks
}

class ClipLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ClipLookAndFeel();
    ~ClipLookAndFeel() override;

    // A font from the embedded typefaces. JUCE only asks the default LookAndFeel for typefaces,
    // so everything that draws text goes through this instead.
    static juce::Font font (float height, bool bold = false);

    juce::Font getLabelFont (juce::Label& label) override { return font (label.getFont().getHeight(), label.getFont().isBold()); }
    juce::Font getComboBoxFont (juce::ComboBox& box) override { return font (std::min (16.0f, (float) box.getHeight() * 0.6f)); }
    juce::Font getPopupMenuFont() override { return font (15.0f); }

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

private:
    struct Typefaces;
    juce::SharedResourcePointer<Typefaces> typefaces;
};
