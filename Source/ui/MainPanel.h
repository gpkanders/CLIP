#pragma once
#include "DisplayGlass.h"
#include "Faceplate.h"
#include "Knob.h"
#include "RotarySwitch.h"
#include "Spectrum.h"
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
          shapeSwitch (apvts, Parameters::shape),
          oversamplingSwitch (apvts, Parameters::oversampling),
          bypassAttachment (apvts, Parameters::bypass.getParamID(), bypassButton),
          curveDisplay (apvts)
    {
        addAndMakeVisible (faceplate);

        for (auto* knob : { &inputKnob, &thresholdKnob, &softnessKnob, &detailKnob, &outputKnob, &mixKnob })
            addAndMakeVisible (knob);

        detailKnob.setPointerColour (Palette::detail);

        addAndMakeVisible (shapeSwitch);
        addAndMakeVisible (oversamplingSwitch);
        addAndMakeVisible (bypassButton);
        // The curve over the spectrum
        addAndMakeVisible (spectrum);
        addAndMakeVisible (curveDisplay);
        addAndMakeVisible (glass);
        setSize (width, height);
    }

    // Called once per display frame with the samples that arrived since the last one
    void showSamples (const ClipTap::Frame* frames, int numFrames, double sampleRate)
    {
        spectrum.update (frames, numFrames, sampleRate);

        auto peak = 0.0f;
        for (auto i = 0; i < numFrames; ++i)
            peak = std::max (peak, std::abs (frames[i].input));
        curveDisplay.showPeak (peak);
    }

    void resized() override
    {
        faceplate.setBounds (getLocalBounds());

        const juce::Rectangle bezel (24, 46, 752, 246);
        spectrum.setBounds (bezel.reduced (4));
        curveDisplay.setBounds (bezel.reduced (4));
        glass.setBounds (bezel.reduced (4));
        bypassButton.setBounds (676, 10, 92, 26);

        // Each section's width follows how many controls it holds
        const std::pair<const char*, std::vector<juce::Component*>> layout[] {
            { "INPUT", { &inputKnob } },
            { "CLIP", { &thresholdKnob, &softnessKnob, &detailKnob, &shapeSwitch } },
            { "OUTPUT", { &outputKnob, &mixKnob } },
            { "QUALITY", { &oversamplingSwitch } },
        };

        auto row = juce::Rectangle (24, 306, 752, 178);
        const auto controlWidth = row.getWidth() / 8;
        std::vector<Faceplate::Section> sections;

        for (const auto& [name, controls] : layout)
        {
            auto section = row.removeFromLeft (controlWidth * (int) controls.size());
            sections.push_back ({ name, section });

            // Below the section header
            section.removeFromTop (26);
            for (auto* control : controls)
                control->setBounds (section.removeFromLeft (controlWidth));
        }

        faceplate.setLayout (bezel, std::move (sections));
    }

private:
    Faceplate faceplate;
    Knob inputKnob, thresholdKnob, softnessKnob, detailKnob, outputKnob, mixKnob;
    RotarySwitch shapeSwitch, oversamplingSwitch;
    juce::ToggleButton bypassButton { "Bypass" };
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;

    Spectrum spectrum;
    TransferCurveDisplay curveDisplay;
    DisplayGlass glass;
};
