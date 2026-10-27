#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

ClipAudioProcessor::ClipAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", Parameters::createLayout())
{
    bypass = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (Parameters::bypass.getParamID()));
    inputGain = apvts.getRawParameterValue (Parameters::inputGain.getParamID());
    threshold = apvts.getRawParameterValue (Parameters::threshold.getParamID());
    softness = apvts.getRawParameterValue (Parameters::softness.getParamID());
    outputGain = apvts.getRawParameterValue (Parameters::outputGain.getParamID());
}

void ClipAudioProcessor::prepareToPlay (double sampleRate, int)
{
    clipper.setParameters (inputGain->load(), threshold->load(), softness->load(), outputGain->load());
    clipper.prepare (sampleRate);
}

void ClipAudioProcessor::releaseResources()
{
}

bool ClipAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void ClipAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    clipper.setParameters (inputGain->load(), threshold->load(), softness->load(), outputGain->load());
    clipper.process (buffer);
}

juce::AudioProcessorEditor* ClipAudioProcessor::createEditor()
{
    return new ClipAudioProcessorEditor (*this);
}

void ClipAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ClipAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClipAudioProcessor();
}
