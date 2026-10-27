#pragma once
#include "PluginProcessor.h"
#include "ui/ClipLookAndFeel.h"
#include "ui/MainPanel.h"

class ClipAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    static constexpr int defaultWidth = MainPanel::width;
    static constexpr int defaultHeight = MainPanel::height;

    explicit ClipAudioProcessorEditor (ClipAudioProcessor&);
    ~ClipAudioProcessorEditor() override;

    void resized() override;

private:
    void drainPeaks();

    ClipAudioProcessor& clipProcessor;
    ClipLookAndFeel lookAndFeel;
    MainPanel panel;

    std::array<PeakFifo::Frame, PeakFifo::capacity> newPeaks;
    juce::VBlankAttachment vblank { this, [this] { drainPeaks(); } };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipAudioProcessorEditor)
};
