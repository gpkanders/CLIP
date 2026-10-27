#pragma once
#include "PluginProcessor.h"

class ClipAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ClipAudioProcessorEditor (ClipAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void drainPeaks();

    ClipAudioProcessor& clipProcessor;
    juce::GenericAudioProcessorEditor genericEditor;

    std::array<PeakFifo::Frame, PeakFifo::capacity> newPeaks;
    int numNewPeaks = 0;
    juce::VBlankAttachment vblank { this, [this] { drainPeaks(); } };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipAudioProcessorEditor)
};
