#include "dsp/ClipChain.h"

class ClipChainTests : public juce::UnitTest
{
public:
    ClipChainTests() : juce::UnitTest("ClipChain") {}

    void runTest() override
    {
        beginTest("50% mix below the threshold nulls against the delayed input at every factor");
        {
            ClipChain chain;
            chain.setParameters(0.0f, 0.0f, 0.0f, 0.0f, 50.0f, false);
            chain.prepare(sampleRate, 2, blockSize, 0);

            for (auto i : { 0, 1, 2, 3, 4, 2, 0 })
            {
                chain.setOversampling(i);
                const auto errorDb = delayedInputErrorDb(chain);
                logMessage(juce::String(1 << i) + "x: error " + juce::String(errorDb, 1) + " dB");
                expect(errorDb < -40.0f, juce::String(1 << i) + "x");
            }
        }

        beginTest("bypass outputs the delayed input at every factor");
        for (auto i = 0; i < Oversampler::numFactors; ++i)
        {
            ClipChain chain;
            chain.setParameters(24.0f, -24.0f, 0.0f, 0.0f, 100.0f, true);
            chain.prepare(sampleRate, 2, blockSize, i);
            expect(delayedInputErrorDb(chain) < -50.0f, juce::String(1 << i) + "x");
        }

        beginTest("toggling bypass crossfades instead of jumping");
        {
            ClipChain processed, toggled;
            for (auto* chain : { &processed, &toggled })
            {
                chain->setParameters(12.0f, -12.0f, 0.0f, 0.0f, 100.0f, false);
                chain->prepare(sampleRate, 1, blockSize, 2);
            }

            juce::AudioBuffer<float> a(1, blockSize), b(1, blockSize);
            auto phase = 0.0;
            for (auto n = 0; n < 20; ++n)
            {
                for (auto i = 0; i < blockSize; ++i, phase += 440.0 / sampleRate)
                {
                    const auto x = 0.8f * (float) std::sin(juce::MathConstants<double>::twoPi * phase);
                    a.setSample(0, i, x);
                    b.setSample(0, i, x);
                }

                if (n == 10)
                    toggled.setParameters(12.0f, -12.0f, 0.0f, 0.0f, 100.0f, true);

                processed.process(a);
                toggled.process(b);

                if (n == 10)
                    expectWithinAbsoluteError(b.getSample(0, 0), a.getSample(0, 0), 0.01f);
            }
        }

        beginTest("processing, mixing and switching do not allocate");
        {
            ClipChain chain;
            chain.prepare(sampleRate, 2, blockSize, 2);
            juce::AudioBuffer<float> buffer(2, blockSize);
            buffer.clear();

            const juce::UnitTestAllocationChecker checker(*this);
            for (auto i : { 2, 4, 0, 1, 3 })
            {
                chain.setOversampling(i);
                chain.setParameters(6.0f, -6.0f, 20.0f, 0.0f, (float) i * 20.0f, i == 1);
                chain.process(buffer);
            }
        }
    }

private:
    static constexpr double sampleRate = 44100.0;
    static constexpr int blockSize = 512;

    // Runs a multitone through the chain and returns the residual against the input delayed by the
    // reported latency, relative to the input, once filters and parameter ramps have settled
    static float delayedInputErrorDb(ClipChain& chain)
    {
        const auto numSamples = blockSize * 64;
        juce::AudioBuffer<float> input(2, numSamples);
        for (auto i = 0; i < numSamples; ++i)
        {
            const auto t = (double) i / sampleRate;
            const auto x = 0.2 * std::sin(juce::MathConstants<double>::twoPi * 110.0 * t)
                         + 0.2 * std::sin(juce::MathConstants<double>::twoPi * 1250.0 * t)
                         + 0.2 * std::sin(juce::MathConstants<double>::twoPi * 6100.0 * t);
            input.setSample(0, i, (float) x);
            input.setSample(1, i, (float) -x);
        }

        juce::AudioBuffer<float> output(input);
        for (auto start = 0; start < numSamples; start += blockSize)
            chain.process(juce::dsp::AudioBlock<float>(output).getSubBlock((size_t) start, blockSize));

        const auto latency = chain.getLatencySamples();
        auto error = 0.0, signal = 0.0;
        for (auto ch = 0; ch < 2; ++ch)
            for (auto i = numSamples / 2; i < numSamples; ++i)
            {
                const auto reference = (double) input.getSample(ch, i - latency);
                error += juce::square(output.getSample(ch, i) - reference);
                signal += juce::square(reference);
            }

        return (float) (10.0 * std::log10(error / signal));
    }
};

static ClipChainTests clipChainTests;
