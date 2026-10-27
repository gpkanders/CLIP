#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

static const juce::Identifier editorWidthId { "editorWidth" };

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
    mix = apvts.getRawParameterValue (Parameters::mix.getParamID());
    oversampling = apvts.getRawParameterValue (Parameters::oversampling.getParamID());
    shape = apvts.getRawParameterValue (Parameters::shape.getParamID());

    startTimerHz (10);
}

void ClipAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    setChainParameters (bypass->get());
    chain.prepare (sampleRate, getTotalNumOutputChannels(), samplesPerBlock, (int) oversampling->load());

    inputDelay.setMaximumDelayInSamples (chain.getMaxLatencySamples());
    inputDelay.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) getTotalNumOutputChannels() });
    delayedInput.setSize (getTotalNumOutputChannels(), samplesPerBlock);
    peakFifo.prepare (sampleRate);

    pendingLatency = chain.getLatencySamples();
    setLatencySamples (pendingLatency);
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
    process (buffer, bypass->get());
}

// Hosts that bypass without the bypass parameter still get the delayed, crossfaded dry signal
void ClipAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    process (buffer, true);
}

void ClipAudioProcessor::process (juce::AudioBuffer<float>& buffer, bool bypassed)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    chain.setOversampling ((int) oversampling->load());
    setChainParameters (bypassed);

    juce::dsp::AudioBlock<float> block (buffer);
    auto delayed = juce::dsp::AudioBlock<float> (delayedInput).getSubBlock (0, block.getNumSamples());
    inputDelay.setDelay ((float) chain.getLatencySamples());
    inputDelay.process (juce::dsp::ProcessContextNonReplacing<float> (block, delayed));

    chain.process (block);
    peakFifo.push (delayed, block);

    pendingLatency = chain.getLatencySamples();
}

void ClipAudioProcessor::setChainParameters (bool bypassed)
{
    chain.setParameters (inputGain->load(), threshold->load(), softness->load(), outputGain->load(), mix->load(), bypassed);
    chain.setShape ((TransferCurves::Shape) (int) shape->load());
}

void ClipAudioProcessor::timerCallback()
{
    setLatencySamples (pendingLatency);
}

juce::AudioProcessorEditor* ClipAudioProcessor::createEditor()
{
    return new ClipAudioProcessorEditor (*this);
}

void ClipAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty (editorWidthId, editorWidth.load(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ClipAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        const auto state = juce::ValueTree::fromXml (*xml);
        editorWidth = (int) state.getProperty (editorWidthId, 0);
        apvts.replaceState (state);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClipAudioProcessor();
}
