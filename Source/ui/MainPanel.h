#pragma once
#include "ChoiceBox.h"
#include "Knob.h"
#include "Spectrogram.h"
#include "TransferCurveDisplay.h"
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
          detailKnob (apvts, Parameters::detail),
          outputKnob (apvts, Parameters::outputGain),
          mixKnob (apvts, Parameters::mix),
          shapeBox (apvts, Parameters::shape),
          oversamplingBox (apvts, Parameters::oversampling),
          bypassAttachment (apvts, Parameters::bypass.getParamID(), bypassButton),
          curveDisplay (apvts)
    {
        for (auto* knob : { &inputKnob, &thresholdKnob, &softnessKnob, &detailKnob, &outputKnob, &mixKnob })
            addAndMakeVisible (knob);

        detailKnob.setArcColour (Palette::detail);

        addAndMakeVisible (shapeBox);
        addAndMakeVisible (oversamplingBox);
        addAndMakeVisible (bypassButton);
        // The curve over the spectrogram
        addAndMakeVisible (spectrogram);
        addAndMakeVisible (curveDisplay);
        setSize (width, height);
    }

    // Called once per display frame with the samples that arrived since the last one
    void showSamples (const ClipTap::Frame* frames, int numFrames, double sampleRate) { spectrogram.update (frames, numFrames, sampleRate); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (16);
        auto controls = bounds.removeFromBottom (170);
        bounds.removeFromBottom (16);

        spectrogram.setBounds (bounds);
        curveDisplay.setBounds (bounds);

        auto options = controls.removeFromRight (140).withSizeKeepingCentre (140, 156);
        shapeBox.setBounds (options.removeFromTop (52));
        options.removeFromTop (12);
        oversamplingBox.setBounds (options.removeFromTop (52));
        options.removeFromTop (12);
        bypassButton.setBounds (options.removeFromTop (28));

        const auto knobWidth = controls.getWidth() / 6;
        for (auto* knob : { &inputKnob, &thresholdKnob, &softnessKnob, &detailKnob, &outputKnob, &mixKnob })
            knob->setBounds (controls.removeFromLeft (knobWidth).reduced (6, 0));
    }

private:
    Knob inputKnob, thresholdKnob, softnessKnob, detailKnob, outputKnob, mixKnob;
    ChoiceBox shapeBox, oversamplingBox;
    juce::ToggleButton bypassButton { "Bypass" };
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;

    Spectrogram spectrogram;
    TransferCurveDisplay curveDisplay;
};
