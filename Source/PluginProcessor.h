#pragma once
#include <JuceHeader.h>
#include "dsp/ClipChain.h"

class ClipAudioProcessor : public juce::AudioProcessor,
                           private juce::Timer
{
public:
    ClipAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override { return bypass; }

    juce::AudioProcessorValueTreeState apvts;

private:
    void process (juce::AudioBuffer<float>& buffer, bool bypassed);
    void setChainParameters (bool bypassed);
    void timerCallback() override;

    juce::AudioParameterBool* bypass = nullptr;
    std::atomic<float>* inputGain = nullptr;
    std::atomic<float>* threshold = nullptr;
    std::atomic<float>* softness = nullptr;
    std::atomic<float>* outputGain = nullptr;
    std::atomic<float>* mix = nullptr;
    std::atomic<float>* oversampling = nullptr;

    ClipChain chain;

    // Written on the audio thread, reported to the host from the message thread
    std::atomic<int> pendingLatency { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipAudioProcessor)
};
