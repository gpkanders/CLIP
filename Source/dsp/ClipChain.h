#pragma once
#include "Clipper.h"
#include "Oversampler.h"

// Oversampled clipper with a dry path delayed to match it. Bypass fades the wet signal out through
// the same mixer, so bypassing never shifts timing and the mixer's ramp avoids clicks.
class ClipChain
{
public:
    // Parameters set before this are applied immediately instead of ramping
    void prepare (double newSampleRate, int numChannels, int maxBlockSize, int factorIndex)
    {
        sampleRate = newSampleRate;
        oversampler.prepare (numChannels, maxBlockSize);

        mixer = std::make_unique<juce::dsp::DryWetMixer<float>> (oversampler.getMaxLatencySamples());
        mixer->setWetMixProportion (wetProportion);
        mixer->prepare ({ sampleRate, (juce::uint32) maxBlockSize, (juce::uint32) numChannels });

        applyOversampling (factorIndex);
    }

    void setOversampling (int factorIndex) noexcept
    {
        if (factorIndex != oversampler.getFactorIndex())
            applyOversampling (factorIndex);
    }

    void setParameters (float inputGainDb, float thresholdDb, float softness, float outputGainDb, float mix, bool bypassed)
    {
        clipper.setParameters (inputGainDb, thresholdDb, softness, outputGainDb);
        wetProportion = bypassed ? 0.0f : mix / 100.0f;

        if (mixer != nullptr)
            mixer->setWetMixProportion (wetProportion);
    }

    void setShape (TransferCurves::Shape shape) { clipper.setShape (shape); }
    void setDetail (float percent) { clipper.setDetail (percent); }

    void process (juce::dsp::AudioBlock<float> block)
    {
        mixer->pushDrySamples (block);
        oversampler.process (block, [this] (juce::dsp::AudioBlock<float> upsampled) { clipper.process (upsampled); });
        mixer->mixWetSamples (block);
    }

    int getLatencySamples() const noexcept { return oversampler.getLatencySamples(); }
    int getMaxLatencySamples() const noexcept { return oversampler.getMaxLatencySamples(); }

private:
    void applyOversampling (int factorIndex) noexcept
    {
        // ADAA rolls off the top octave at the base rate, so only use it when oversampling.
        // It delays by half an oversampled sample, which the oversampler pads along with its own latency.
        const auto antialiasing = factorIndex > 0;
        oversampler.setFactorIndex (factorIndex, antialiasing ? 0.5f : 0.0f);
        clipper.setAntialiasing (antialiasing);
        clipper.prepare (sampleRate * oversampler.getFactor());
        mixer->setWetLatency ((float) getLatencySamples());
    }

    Clipper clipper;
    Oversampler oversampler;
    std::unique_ptr<juce::dsp::DryWetMixer<float>> mixer;
    double sampleRate = 44100.0;
    float wetProportion = 1.0f;
};
