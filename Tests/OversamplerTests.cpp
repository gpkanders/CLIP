#include "dsp/Clipper.h"
#include "dsp/Oversampler.h"

class OversamplerTests : public juce::UnitTest
{
public:
    OversamplerTests() : juce::UnitTest("Oversampler") {}

    void runTest() override
    {
        beginTest("latency is zero when off and grows with the factor");
        {
            Oversampler oversampler;
            oversampler.prepare(2, blockSize);

            auto previous = -1;
            for (auto i = 0; i < Oversampler::numFactors; ++i)
            {
                oversampler.setFactorIndex(i);
                expectEquals(oversampler.getFactor(), 1 << i);
                expect(oversampler.getLatencySamples() > previous);
                previous = oversampler.getLatencySamples();
            }

            oversampler.setFactorIndex(0);
            expectEquals(oversampler.getLatencySamples(), 0);
        }

        beginTest("aliasing drops as the factor rises");
        {
            auto previous = 0.0f;
            for (auto i = 0; i < Oversampler::numFactors; ++i)
            {
                const auto aliasingDb = measureAliasingDb(i, false);
                logMessage("factor " + juce::String(1 << i) + "x: aliasing " + juce::String(aliasingDb, 1) + " dB");
                if (i > 0)
                    expect(aliasingDb < previous - 3.0f);
                previous = aliasingDb;
            }
        }

        beginTest("antialiasing lowers aliasing at every factor");
        for (auto i = 0; i < 4; ++i)
            expect(measureAliasingDb(i, true) < measureAliasingDb(i, false) - 5.0f, juce::String(1 << i) + "x");

        beginTest("switching factors does not allocate");
        {
            Oversampler oversampler;
            oversampler.prepare(2, blockSize);
            Clipper clipper;
            juce::AudioBuffer<float> buffer(2, blockSize);
            buffer.clear();

            const juce::UnitTestAllocationChecker checker(*this);
            for (auto i : { 2, 4, 0, 1, 3 })
            {
                oversampler.setFactorIndex(i);
                clipper.setAntialiasing(i > 0);
                clipper.prepare(sampleRate * oversampler.getFactor());
                oversampler.process(buffer, [&](juce::dsp::AudioBlock<float> block) { clipper.process(block); });
            }
        }
    }

private:
    static constexpr double sampleRate = 44100.0;
    static constexpr int blockSize = 512;
    static constexpr int fftOrder = 14;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int sineBin = 1301; // odd, so aliases never land on a harmonic bin

    // Hard clips a bin centred sine and returns the power outside the harmonic bins relative to the total
    static float measureAliasingDb(int factorIndex, bool antialiasing)
    {
        Oversampler oversampler;
        oversampler.prepare(1, blockSize);
        oversampler.setFactorIndex(factorIndex);

        Clipper clipper;
        clipper.setAntialiasing(antialiasing);
        clipper.setParameters(12.0f, -6.0f, 0.0f, 0.0f);
        clipper.prepare(sampleRate * oversampler.getFactor());

        // One frame to settle the filters, then analyse the next one, which is periodic
        juce::AudioBuffer<float> buffer(1, fftSize * 2);
        for (auto i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample(0, i, std::sin(juce::MathConstants<float>::twoPi * (float) sineBin * (float) (i % fftSize) / (float) fftSize));

        for (auto start = 0; start < buffer.getNumSamples(); start += blockSize)
            oversampler.process(juce::dsp::AudioBlock<float>(buffer).getSubBlock((size_t) start, blockSize),
                                [&](juce::dsp::AudioBlock<float> block) { clipper.process(block); });

        std::vector<float> spectrum((size_t) fftSize * 2, 0.0f);
        std::copy_n(buffer.getReadPointer(0, fftSize), fftSize, spectrum.begin());
        juce::dsp::FFT(fftOrder).performFrequencyOnlyForwardTransform(spectrum.data(), true);

        auto harmonic = 0.0, alias = 0.0;
        for (auto bin = 1; bin < fftSize / 2; ++bin)
        {
            const auto power = (double) spectrum[(size_t) bin] * spectrum[(size_t) bin];
            (bin % sineBin == 0 ? harmonic : alias) += power;
        }

        return (float) (10.0 * std::log10(alias / (harmonic + alias)));
    }
};

static OversamplerTests oversamplerTests;
