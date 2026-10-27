#pragma once
#include <JuceHeader.h>

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

        label.setText (parameter.getName (32), juce::dontSendNotification);
        label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        label.setJustificationType (juce::Justification::centred);

        addAndMakeVisible (label);
        addAndMakeVisible (slider);
    }

    void setArcColour (juce::Colour colour) { slider.setColour (juce::Slider::rotarySliderFillColourId, colour); }

    void resized() override
    {
        auto bounds = getLocalBounds();
        label.setBounds (bounds.removeFromTop (24));
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, bounds.getWidth() - 16, 22);
        slider.setBounds (bounds);
    }

private:
    juce::Label label;
    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};
