#pragma once
#include "PluginProcessor.h"
#include "ui/MainPanel.h"

class ClipAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    static constexpr int defaultWidth = MainPanel::width;
    static constexpr int defaultHeight = MainPanel::height;

    explicit ClipAudioProcessorEditor (ClipAudioProcessor&);

    void resized() override;

private:
    void drainPeaks();

    ClipAudioProcessor& clipProcessor;
    MainPanel panel;

    std::array<PeakFifo::Frame, PeakFifo::capacity> newPeaks;
    int numNewPeaks = 0;
    juce::VBlankAttachment vblank { this, [this] { drainPeaks(); } };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipAudioProcessorEditor)
};
