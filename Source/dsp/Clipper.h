#pragma once
#include "TransferCurves.h"

class Clipper
{
public:
    void setParameters (float inputGainDb, float newThresholdDb, float newSoftness, float outputGainDb)
    {
        inputGain = juce::Decibels::decibelsToGain (inputGainDb);
        thresholdDb = newThresholdDb;
        softness = newSoftness;
        outputGain = juce::Decibels::decibelsToGain (outputGainDb);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        for (auto ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* samples = buffer.getWritePointer (ch);

            for (auto i = 0; i < buffer.getNumSamples(); ++i)
                samples[i] = TransferCurves::clip (samples[i] * inputGain, thresholdDb, softness) * outputGain;
        }
    }

private:
    float inputGain = 1.0f;
    float thresholdDb = 0.0f;
    float softness = 0.0f;
    float outputGain = 1.0f;
};
