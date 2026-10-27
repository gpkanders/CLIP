#pragma once
#include <JuceHeader.h>

// Parameter IDs are saved in sessions: never rename one after release, add new ones with a higher version hint
namespace Parameters
{
    inline const juce::ParameterID inputGain { "inputGain", 1 };
    inline const juce::ParameterID threshold { "threshold", 1 };
    inline const juce::ParameterID softness { "softness", 1 };
    inline const juce::ParameterID outputGain { "outputGain", 1 };
    inline const juce::ParameterID mix { "mix", 1 };
    inline const juce::ParameterID bypass { "bypass", 1 };

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using juce::AudioParameterFloat;
        using juce::AudioParameterFloatAttributes;

        const auto db = AudioParameterFloatAttributes().withLabel ("dB");
        const auto percent = AudioParameterFloatAttributes().withLabel ("%");

        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add (std::make_unique<AudioParameterFloat> (inputGain, "Input", juce::NormalisableRange (-24.0f, 24.0f, 0.01f), 0.0f, db));
        layout.add (std::make_unique<AudioParameterFloat> (threshold, "Threshold", juce::NormalisableRange (-36.0f, 0.0f, 0.01f), 0.0f, db));
        layout.add (std::make_unique<AudioParameterFloat> (softness, "Softness", juce::NormalisableRange (0.0f, 100.0f, 0.1f), 0.0f, percent));
        layout.add (std::make_unique<AudioParameterFloat> (outputGain, "Output", juce::NormalisableRange (-24.0f, 24.0f, 0.01f), 0.0f, db));
        layout.add (std::make_unique<AudioParameterFloat> (mix, "Mix", juce::NormalisableRange (0.0f, 100.0f, 0.1f), 100.0f, percent));
        layout.add (std::make_unique<juce::AudioParameterBool> (bypass, "Bypass", false));
        return layout;
    }
}
