#pragma once
#include "Knob.h"
#include "../Parameters.h"

// Every control, laid out at a fixed size. The editor scales it to fit the window.
class MainPanel : public juce::Component
{
public:
    static constexpr int width = 800;
    static constexpr int height = 500;

    explicit MainPanel (juce::AudioProcessorValueTreeState& apvts)
        : inputKnob (apvts, Parameters::inputGain),
          thresholdKnob (apvts, Parameters::threshold),
          softnessKnob (apvts, Parameters::softness),
          outputKnob (apvts, Parameters::outputGain),
          mixKnob (apvts, Parameters::mix),
          bypassAttachment (apvts, Parameters::bypass.getParamID(), bypassButton)
    {
        for (auto* knob : { &inputKnob, &thresholdKnob, &softnessKnob, &outputKnob, &mixKnob })
            addAndMakeVisible (knob);

        // The attachment selects the current choice when it's created, so the items have to exist first
        const auto& oversampling = *dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (Parameters::oversampling.getParamID()));
        oversamplingBox.addItemList (oversampling.choices, 1);
        oversamplingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, Parameters::oversampling.getParamID(), oversamplingBox);
        oversamplingLabel.setText (oversampling.getName (32), juce::dontSendNotification);
        oversamplingLabel.setJustificationType (juce::Justification::centred);

        addAndMakeVisible (oversamplingLabel);
        addAndMakeVisible (oversamplingBox);
        addAndMakeVisible (bypassButton);
        setSize (width, height);
    }

    void paint (juce::Graphics& g) override
    {
        const auto background = getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId);
        g.fillAll (background);

        g.setColour (background.darker (0.3f));
        for (const auto& area : { waveformArea, curveArea })
            g.fillRoundedRectangle (area.toFloat(), 6.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (16);
        auto controls = bounds.removeFromBottom (170);
        bounds.removeFromBottom (16);

        curveArea = bounds.removeFromRight (bounds.getHeight());
        bounds.removeFromRight (16);
        waveformArea = bounds;

        auto options = controls.removeFromRight (140).withSizeKeepingCentre (140, 100);
        oversamplingLabel.setBounds (options.removeFromTop (24));
        oversamplingBox.setBounds (options.removeFromTop (28));
        options.removeFromTop (20);
        bypassButton.setBounds (options.removeFromTop (28));

        const auto knobWidth = controls.getWidth() / 5;
        for (auto* knob : { &inputKnob, &thresholdKnob, &softnessKnob, &outputKnob, &mixKnob })
            knob->setBounds (controls.removeFromLeft (knobWidth).reduced (6, 0));
    }

private:
    Knob inputKnob, thresholdKnob, softnessKnob, outputKnob, mixKnob;
    juce::Label oversamplingLabel;
    juce::ComboBox oversamplingBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oversamplingAttachment;
    juce::ToggleButton bypassButton { "Bypass" };
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;

    // Kept clear for the waveform and transfer curve displays
    juce::Rectangle<int> waveformArea, curveArea;
};
