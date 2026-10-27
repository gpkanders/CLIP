#include "PluginEditor.h"

ClipAudioProcessorEditor::ClipAudioProcessorEditor (ClipAudioProcessor& p)
    : AudioProcessorEditor (&p), clipProcessor (p), panel (p.apvts)
{
    clipProcessor.peakFifo.discard();
    addAndMakeVisible (panel);

    // After adding the panel, so the change reaches its sliders, which copy colours into their text boxes
    setLookAndFeel (&lookAndFeel);

    // Read the saved width first: setResizeLimits resizes straight away, and resized() saves the width
    const auto minWidth = defaultWidth * 3 / 4, maxWidth = defaultWidth * 2;
    const auto width = clipProcessor.editorWidth > 0 ? juce::jlimit (minWidth, maxWidth, clipProcessor.editorWidth.load()) : defaultWidth;

    setResizable (true, true);
    setResizeLimits (minWidth, defaultHeight * 3 / 4, maxWidth, defaultHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) defaultWidth / defaultHeight);
    setSize (width, width * defaultHeight / defaultWidth);
}

ClipAudioProcessorEditor::~ClipAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void ClipAudioProcessorEditor::drainPeaks()
{
    const auto numPeaks = clipProcessor.peakFifo.pop (newPeaks.data(), (int) newPeaks.size());
    panel.showPeaks (newPeaks.data(), numPeaks);
}

// Scaling the whole panel keeps text, strokes and hit areas in proportion at any size
void ClipAudioProcessorEditor::resized()
{
    clipProcessor.editorWidth = getWidth();
    panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / defaultWidth));
}
