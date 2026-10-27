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
            stages[i] = std::make_unique<juce::dsp::Oversampling<float>> ((size_t) numChannels, i, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
            stages[i]->initProcessing ((size_t) maxBlockSize);
        }
    }

    void setFactorIndex (int index) noexcept
    {
        factorIndex = juce::jlimit (0, numFactors - 1, index);

        if (auto& stage = stages[(size_t) factorIndex])
            stage->reset();
    }

    int getFactorIndex() const noexcept { return factorIndex; }
    int getFactor() const noexcept { return 1 << factorIndex; }

    int getLatencySamples() const noexcept
    {
        const auto& stage = stages[(size_t) factorIndex];
        return stage != nullptr ? (int) std::lround (stage->getLatencyInSamples()) : 0;
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
        stage->processSamplesDown (block);
    }

private:
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, numFactors> stages;
    int factorIndex = 0;
};
