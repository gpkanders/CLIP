#pragma once
#include <JuceHeader.h>
#include "ClipLookAndFeel.h"

// Combo box bound to a choice parameter, with its name above
class ChoiceBox : public juce::Component
{
public:
    ChoiceBox (juce::AudioProcessorValueTreeState& apvts, const juce::ParameterID& id)
    {
        // The attachment selects the current choice when it's created, so the items have to exist first
        const auto& parameter = *dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id.getParamID()));
        box.addItemList (parameter.choices, 1);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, id.getParamID(), box);

        label.setText (parameter.getName (32).toUpperCase(), juce::dontSendNotification);
        label.setFont (juce::FontOptions (9.5f, juce::Font::bold).withKerningFactor (0.12f));
        label.setColour (juce::Label::textColourId, Palette::silkscreen);
        label.setJustificationType (juce::Justification::centred);

        addAndMakeVisible (label);
        addAndMakeVisible (box);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        label.setBounds (bounds.removeFromTop (14));
        bounds.removeFromTop (4);
        box.setBounds (bounds.removeFromTop (28).reduced (6, 0));
    }

private:
    juce::Label label;
    juce::ComboBox box;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};
