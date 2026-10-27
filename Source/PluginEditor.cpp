#include "PluginEditor.h"

// Placeholder UI: generic sliders until the custom editor lands
ClipAudioProcessorEditor::ClipAudioProcessorEditor (ClipAudioProcessor& p)
    : AudioProcessorEditor (&p), genericEditor (p)
{
    addAndMakeVisible (genericEditor);
    setSize (480, 320);
}

void ClipAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void ClipAudioProcessorEditor::resized()
{
    genericEditor.setBounds (getLocalBounds());
}
