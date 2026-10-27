#pragma once
#include "TransferCurves.h"
#include <juce_dsp/juce_dsp.h>

class Clipper
{
public:
    static constexpr double smoothingSeconds = 0.02;

    // Snaps to the current targets, so call setParameters first to start without a ramp
    void prepare (double sampleRate)
    {
        inputGain.reset (sampleRate, smoothingSeconds);
        thresholdDb.reset (sampleRate, smoothingSeconds);
        softness.reset (sampleRate, smoothingSeconds);
        outputGain.reset (sampleRate, smoothingSeconds);
    }

    void setParameters (float inputGainDb, float newThresholdDb, float newSoftness, float outputGainDb)
    {
        inputGain.setTargetValue (juce::Decibels::decibelsToGain (inputGainDb));
        thresholdDb.setTargetValue (newThresholdDb);
        softness.setTargetValue (newSoftness);
        outputGain.setTargetValue (juce::Decibels::decibelsToGain (outputGainDb));
    }

    void process (juce::dsp::AudioBlock<float> block)
    {
        for (size_t i = 0; i < block.getNumSamples(); ++i)
        {
            const auto in = inputGain.getNextValue();
            const auto threshold = thresholdDb.getNextValue();
            const auto soft = softness.getNextValue();
            const auto out = outputGain.getNextValue();

            for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
            {
                auto* samples = block.getChannelPointer (ch);
                samples[i] = TransferCurves::clip (samples[i] * in, threshold, soft) * out;
            }
        }
    }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inputGain;
    juce::SmoothedValue<float> thresholdDb;
    juce::SmoothedValue<float> softness;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain;
};
