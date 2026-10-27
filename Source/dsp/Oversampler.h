#pragma once
#include <juce_dsp/juce_dsp.h>

// Holds a prebuilt oversampler per factor so switching on the audio thread never allocates.
// Linear phase FIR filters keep the latency constant across frequencies, which the dry path relies on.
class Oversampler
{
public:
    static constexpr int numFactors = 5; // Off, 2x, 4x, 8x, 16x

    void prepare (int numChannels, int maxBlockSize)
    {
        for (size_t i = 1; i < stages.size(); ++i)
        {
            // Fractional latency is padded below, at the oversampled rate where the allpass stays accurate
            stages[i] = std::make_unique<juce::dsp::Oversampling<float>> ((size_t) numChannels, i, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, false);
            stages[i]->initProcessing ((size_t) maxBlockSize);
        }

        alignment.prepare ({ 44100.0, (juce::uint32) (maxBlockSize << (numFactors - 1)), (juce::uint32) numChannels });
    }

    // processingDelay is any delay, in oversampled samples, added by the processing between up and down
    // sampling. It is padded together with the filter latency to a whole number of base rate samples.
    void setFactorIndex (int index, float processingDelay = 0.0f) noexcept
    {
        factorIndex = juce::jlimit (0, numFactors - 1, index);
        latency = 0;

        if (auto& stage = stages[(size_t) factorIndex])
        {
            stage->reset();
            alignment.reset();

            // At least one oversampled sample of padding keeps the Thiran allpass well conditioned
            const auto factor = (float) getFactor();
            const auto exactLatency = stage->getLatencyInSamples() + processingDelay / factor;
            latency = (int) std::ceil (exactLatency + 1.0f / factor);
            alignment.setDelay (((float) latency - exactLatency) * factor);
        }
    }

    int getFactorIndex() const noexcept { return factorIndex; }
    int getFactor() const noexcept { return 1 << factorIndex; }
    int getLatencySamples() const noexcept { return latency; }

    int getMaxLatencySamples() const noexcept
    {
        return (int) std::ceil (stages.back()->getLatencyInSamples()) + 1;
    }

    template <typename ProcessFn>
    void process (juce::dsp::AudioBlock<float> block, ProcessFn&& processOversampled)
    {
        auto& stage = stages[(size_t) factorIndex];

        if (stage == nullptr)
        {
            processOversampled (block);
            return;
        }

        auto upsampled = stage->processSamplesUp (block);
        processOversampled (upsampled);
        alignment.process (juce::dsp::ProcessContextReplacing<float> (upsampled));
        stage->processSamplesDown (block);
    }

private:
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, numFactors> stages;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> alignment { 64 };
    int factorIndex = 0;
    int latency = 0;
};
