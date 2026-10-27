#pragma once
#include "TransferCurves.h"

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

    void process (juce::AudioBuffer<float>& buffer)
    {
        auto* const* channels = buffer.getArrayOfWritePointers();

        for (auto i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto in = inputGain.getNextValue();
            const auto threshold = thresholdDb.getNextValue();
            const auto soft = softness.getNextValue();
            const auto out = outputGain.getNextValue();

            for (auto ch = 0; ch < buffer.getNumChannels(); ++ch)
                channels[ch][i] = TransferCurves::clip (channels[ch][i] * in, threshold, soft) * out;
        }
    }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inputGain;
    juce::SmoothedValue<float> thresholdDb;
    juce::SmoothedValue<float> softness;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain;
};
