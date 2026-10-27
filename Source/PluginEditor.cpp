#include "PluginEditor.h"

ClipAudioProcessorEditor::ClipAudioProcessorEditor (ClipAudioProcessor& p)
    : AudioProcessorEditor (&p), clipProcessor (p), panel (p.apvts)
{
    clipProcessor.tap.discard();
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

void ClipAudioProcessorEditor::drainSamples()
{
    const auto numSamples = clipProcessor.tap.pop (newSamples.data(), (int) newSamples.size());
    panel.showSamples (newSamples.data(), numSamples, clipProcessor.tap.sampleRate);
}

void ClipAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Palette::background);
}

// Scaling the whole panel keeps text, strokes and hit areas in proportion at any size.
// Hosts don't always keep the aspect ratio (macOS applies it to a window frame including its title bar),
// so the panel is fitted to whichever side is tighter and centred.
void ClipAudioProcessorEditor::resized()
{
    clipProcessor.editorWidth = getWidth();

    const auto scale = std::min ((float) getWidth() / defaultWidth, (float) getHeight() / defaultHeight);
    panel.setTransform (juce::AffineTransform::scale (scale).translated (((float) getWidth() - defaultWidth * scale) / 2.0f,
                                                                         ((float) getHeight() - defaultHeight * scale) / 2.0f));
}
