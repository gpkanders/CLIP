#include "PluginEditor.h"

// Placeholder UI: generic sliders until the custom editor lands
ClipAudioProcessorEditor::ClipAudioProcessorEditor (ClipAudioProcessor& p)
    : AudioProcessorEditor (&p), clipProcessor (p), genericEditor (p)
{
    clipProcessor.peakFifo.discard();
    addAndMakeVisible (genericEditor);
    setSize (480, 320);
}

// Drains every frame so the FIFO keeps flowing while the editor is open; the waveform display will draw these
void ClipAudioProcessorEditor::drainPeaks()
{
    numNewPeaks = clipProcessor.peakFifo.pop (newPeaks.data(), (int) newPeaks.size());
}

void ClipAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void ClipAudioProcessorEditor::resized()
{
    genericEditor.setBounds (getLocalBounds());
}
