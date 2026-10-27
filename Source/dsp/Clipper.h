#pragma once
#include "TransferCurves.h"
#include <juce_dsp/juce_dsp.h>

class Clipper
{
public:
    static constexpr double smoothingSeconds = 0.02;

    // Detail takes what the curve would remove from the band below this off before clipping, so the band above
    // rides on the clipped peaks instead of being flattened with them. The clip still holds the ceiling.
    static constexpr double detailCrossoverHz = 1000.0;

    // Snaps to the current targets, so call setParameters first to start without a ramp
    void prepare (double sampleRate)
    {
        inputGain.reset (sampleRate, smoothingSeconds);
        thresholdDb.reset (sampleRate, smoothingSeconds);
        softness.reset (sampleRate, smoothingSeconds);
        outputGain.reset (sampleRate, smoothingSeconds);
        shapeFade.reset (sampleRate, smoothingSeconds);
        detail.reset (sampleRate, smoothingSeconds);
        detailCoefficient = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * detailCrossoverHz / sampleRate));
        previousInput.fill (0.0f);
        lowBand.fill (0.0f);
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

    // Switching crossfades from the old shape over the smoothing time, so it doesn't click
    void setShape (TransferCurves::Shape newShape)
    {
        if (newShape == shape)
            return;

        previousShape = shape;
        shape = newShape;
        shapeFade.setCurrentAndTargetValue (0.0f);
        shapeFade.setTargetValue (1.0f);
    }

    void setDetail (float percent) { detail.setTargetValue (percent / 100.0f); }

    void process (juce::dsp::AudioBlock<float> block)
    {
        jassert (block.getNumChannels() <= previousInput.size());

        for (size_t i = 0; i < block.getNumSamples(); ++i)
        {
            const auto in = inputGain.getNextValue();
            const auto threshold = thresholdDb.getNextValue(), soft = softness.getNextValue();
            const TransferCurves::Knee knee (threshold, soft, shape), fadingFrom (threshold, soft, previousShape);
            const auto fade = shapeFade.getNextValue();
            const auto out = outputGain.getNextValue();
            const auto detailAmount = detail.getNextValue();

            for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
            {
                auto* samples = block.getChannelPointer (ch);
                // The low band's overshoot comes off before the clip. Below the knee it has none, so nothing changes.
                lowBand[ch] += detailCoefficient * (samples[i] * in - lowBand[ch]);
                const auto x = samples[i] * in - detailAmount * (lowBand[ch] - TransferCurves::clip (lowBand[ch], knee));
                auto y = curve (x, previousInput[ch], knee);

                if (fade < 1.0f)
                    y += (curve (x, previousInput[ch], fadingFrom) - y) * (1.0f - fade);

                samples[i] = y * out;
                previousInput[ch] = x;
            }
        }
    }

private:
    float curve (float x, float previousX, TransferCurves::Knee knee) const
    {
        return antialiasing ? TransferCurves::clipAntialiased (x, previousX, knee) : TransferCurves::clip (x, knee);
    }

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inputGain;
    juce::SmoothedValue<float> thresholdDb;
    juce::SmoothedValue<float> softness;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain;
    juce::SmoothedValue<float> shapeFade { 1.0f };
    juce::SmoothedValue<float> detail;
    TransferCurves::Shape shape = TransferCurves::Shape::soft, previousShape = TransferCurves::Shape::soft;
    std::array<float, 2> previousInput {};
    std::array<float, 2> lowBand {};
    float detailCoefficient = 0.0f;
    bool antialiasing = false;
};
