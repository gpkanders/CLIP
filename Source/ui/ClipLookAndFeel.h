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

    // The anodised panel, top to bottom
    inline const juce::Colour panelTop { 0xff1c1e22 };
    inline const juce::Colour panelMiddle { 0xff141518 };
    inline const juce::Colour panelBottom { 0xff0f1013 };
    inline const juce::Colour silkscreen { 0xffc9ccd2 }; // control names printed on the panel
    inline const juce::Colour rule { 0xff2c2f35 }; // section header lines
    inline const juce::Colour engraving { 0xff08090a }; // section dividers, edges in shadow
    inline const juce::Colour bezel { 0xff07080a };
    inline const juce::Colour screwLight { 0xff5a5e66 };
    inline const juce::Colour screwDark { 0xff16171a };

    // Machined knobs, lit from above
    inline const juce::Colour knurlLight { 0xff2d3035 };
    inline const juce::Colour knurlDark { 0xff17191c };
    inline const juce::Colour capLight { 0xff3b3f46 };
    inline const juce::Colour cap { 0xff202327 };
    inline const juce::Colour pointer { 0xffeef0f3 };
    inline const juce::Colour tickMajor { 0xffa3a9b3 };
    inline const juce::Colour tickMinor { 0xff5b616b };
    inline const juce::Colour readout { 0xff06080a }; // behind the lit value under each knob
}

class ClipLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ClipLookAndFeel();
    ~ClipLookAndFeel() override;

    // A font from the embedded typefaces. JUCE only asks the default LookAndFeel for typefaces,
    // so everything that draws text goes through this instead.
    static juce::Font font (float height, bool bold = false);

    // For the labels inside the display
    static juce::Font displayFont (float height);

    // For the values under the knobs
    static juce::Font readoutFont (float height);

    juce::Font getLabelFont (juce::Label& label) override
    {
        if (isReadout (label))
            return readoutFont (13.0f);
        return font (label.getFont().getHeight(), label.getFont().isBold()).withExtraKerningFactor (label.getFont().getExtraKerningFactor());
    }

    void drawLabel (juce::Graphics&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox& box) override { return font (std::min (16.0f, (float) box.getHeight() * 0.6f)); }
    juce::Font getPopupMenuFont() override { return font (15.0f); }

    // Set on a slider to draw it as a rotary switch
    static constexpr const char* rotarySwitchProperty = "rotarySwitch";

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

private:
    // A slider's text box
    static bool isReadout (juce::Label& label) { return dynamic_cast<juce::Slider*> (label.getParentComponent()) != nullptr; }

    struct Typefaces;
    juce::SharedResourcePointer<Typefaces> typefaces;
};
