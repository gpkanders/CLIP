#include "PluginEditor.h"

ClipAudioProcessorEditor::ClipAudioProcessorEditor (ClipAudioProcessor& p)
    : AudioProcessorEditor (&p), clipProcessor (p), panel (p.apvts)
{
    clipProcessor.peakFifo.discard();
    addAndMakeVisible (panel);

    // Read the saved width first: setResizeLimits resizes straight away, and resized() saves the width
    const auto minWidth = defaultWidth * 3 / 4, maxWidth = defaultWidth * 2;
    const auto width = clipProcessor.editorWidth > 0 ? juce::jlimit (minWidth, maxWidth, clipProcessor.editorWidth.load()) : defaultWidth;

    setResizable (true, true);
    setResizeLimits (minWidth, defaultHeight * 3 / 4, maxWidth, defaultHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) defaultWidth / defaultHeight);
    setSize (width, width * defaultHeight / defaultWidth);
}

// Drains every frame so the FIFO keeps flowing while the editor is open; the waveform display will draw these
void ClipAudioProcessorEditor::drainPeaks()
{
    numNewPeaks = clipProcessor.peakFifo.pop (newPeaks.data(), (int) newPeaks.size());
}

// Scaling the whole panel keeps text, strokes and hit areas in proportion at any size
void ClipAudioProcessorEditor::resized()
{
    clipProcessor.editorWidth = getWidth();
    panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / defaultWidth));
}
