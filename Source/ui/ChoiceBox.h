#pragma once
#include <JuceHeader.h>

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

        label.setText (parameter.getName (32), juce::dontSendNotification);
        label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        label.setJustificationType (juce::Justification::centred);

        addAndMakeVisible (label);
        addAndMakeVisible (box);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        label.setBounds (bounds.removeFromTop (24));
        box.setBounds (bounds.removeFromTop (28));
    }

private:
    juce::Label label;
    juce::ComboBox box;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};
