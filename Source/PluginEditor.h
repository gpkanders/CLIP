#pragma once
#include "PluginProcessor.h"

class ClipAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ClipAudioProcessorEditor (ClipAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::GenericAudioProcessorEditor genericEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipAudioProcessorEditor)
};
