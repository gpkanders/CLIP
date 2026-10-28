#pragma once
#include <JuceHeader.h>
#include "ClipLookAndFeel.h"

// Rotary slider bound to a parameter, with its name above and a readout with units below.
// Double click resets it to the parameter's default.
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::ParameterID& id)
        : attachment (apvts, id.getParamID(), slider)
    {
        const auto& parameter = *apvts.getParameter (id.getParamID());

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextValueSuffix (" " + parameter.getLabel());
        slider.setDoubleClickReturnValue (true, parameter.convertFrom0to1 (parameter.getDefaultValue()));

        label.setText (parameter.getName (32).toUpperCase(), juce::dontSendNotification);
        label.setFont (juce::FontOptions (9.5f, juce::Font::bold).withKerningFactor (0.12f));
        label.setColour (juce::Label::textColourId, Palette::silkscreen);
        label.setJustificationType (juce::Justification::centred);

        addAndMakeVisible (label);
        addAndMakeVisible (slider);
    }

    void setPointerColour (juce::Colour colour) { slider.setColour (juce::Slider::rotarySliderFillColourId, colour); }

    void resized() override
    {
        auto bounds = getLocalBounds();
        label.setBounds (bounds.removeFromTop (14));
        bounds.removeFromTop (4);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
        slider.setBounds (bounds.removeFromTop (106));
    }

private:
    juce::Label label;
    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};
