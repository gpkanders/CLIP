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
        previousInput.fill (0.0f);
    }

    // ADAA averages neighbouring samples below the knee, which rolls off the top octave
    // unless the clipper runs oversampled
    void setAntialiasing (bool shouldAntialias) { antialiasing = shouldAntialias; }

    void setParameters (float inputGainDb, float newThresholdDb, float newSoftness, float outputGainDb)
    {
        inputGain.setTargetValue (juce::Decibels::decibelsToGain (inputGainDb));
        thresholdDb.setTargetValue (newThresholdDb);
        softness.setTargetValue (newSoftness);
        outputGain.setTargetValue (juce::Decibels::decibelsToGain (outputGainDb));
    }

    void process (juce::dsp::AudioBlock<float> block)
    {
        jassert (block.getNumChannels() <= previousInput.size());

        for (size_t i = 0; i < block.getNumSamples(); ++i)
        {
            const auto in = inputGain.getNextValue();
            const TransferCurves::Knee knee (thresholdDb.getNextValue(), softness.getNextValue());
            const auto out = outputGain.getNextValue();

            for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
            {
                auto* samples = block.getChannelPointer (ch);
                const auto x = samples[i] * in;
                samples[i] = (antialiasing ? TransferCurves::clipAntialiased (x, previousInput[ch], knee) : TransferCurves::clip (x, knee)) * out;
                previousInput[ch] = x;
            }
        }
    }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inputGain;
    juce::SmoothedValue<float> thresholdDb;
    juce::SmoothedValue<float> softness;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain;
    std::array<float, 2> previousInput {};
    bool antialiasing = false;
};
