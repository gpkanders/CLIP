#include "PluginProcessor.h"
#include "Parameters.h"

// Measures the whole plugin offline: parameters go through the APVTS and audio through processBlock
class ProcessorTests : public juce::UnitTest
{
public:
    ProcessorTests() : juce::UnitTest("Processor") {}

    void runTest() override
    {
        beginTest("default parameters null against the delayed input");
        forEachConfig([this](double sampleRate, int blockSize) {
            ClipAudioProcessor processor;
            const auto input = multitone(sampleRate, (int) sampleRate / 2);
            const auto output = render(processor, input, sampleRate, blockSize);
            const auto latency = processor.getLatencySamples();

            auto error = 0.0, signal = 0.0;
            for (auto ch = 0; ch < input.getNumChannels(); ++ch)
                for (auto i = input.getNumSamples() / 2; i < input.getNumSamples(); ++i)
                {
                    const auto reference = (double) input.getSample(ch, i - latency);
                    error += juce::square(output.getSample(ch, i) - reference);
                    signal += juce::square(reference);
                }

            expect(10.0 * std::log10(error / signal) < -40.0, describe(sampleRate, blockSize));
        });

        beginTest("peaks stay at the threshold plus output gain at every softness");
        forEachConfig([this](double sampleRate, int blockSize) {
            const auto limit = juce::Decibels::decibelsToGain(thresholdDb + outputGainDb);
            const auto input = sine(sampleRate, 997.0, 0.9f, (int) sampleRate / 4);

            for (auto factor = 0; factor < numFactors; ++factor)
                for (auto softness : { 0.0f, 25.0f, 50.0f, 100.0f })
                {
                    ClipAudioProcessor processor;
                    setParameter(processor, Parameters::inputGain, 12.0f);
                    setParameter(processor, Parameters::threshold, thresholdDb);
                    setParameter(processor, Parameters::outputGain, outputGainDb);
                    setParameter(processor, Parameters::softness, softness);
                    setParameter(processor, Parameters::oversampling, (float) factor);

                    const auto output = render(processor, input, sampleRate, blockSize);
                    const auto settled = juce::dsp::AudioBlock<const float>(output).getSubBlock((size_t) output.getNumSamples() / 2);
                    const auto samplePeakDb = juce::Decibels::gainToDecibels(peak(settled) / limit);
                    const auto setting = describe(sampleRate, blockSize) + ", " + juce::String(1 << factor) + "x, softness " + juce::String(softness);

                    // The oversampling filters ring at the clipped edges, so only the base rate path is exact
                    if (factor == 0)
                        expect(samplePeakDb <= 1.0e-4f, setting);
                    else
                        expect(samplePeakDb < maxFilterOvershootDb, setting + ": " + juce::String(samplePeakDb, 2) + " dB");

                    if (blockSize == 512 && softness == 0.0f)
                        logMessage(setting + ": sample peak " + juce::String(samplePeakDb, 2) + " dB, true peak "
                                   + juce::String(juce::Decibels::gainToDecibels(truePeak(settled) / limit), 2) + " dB over the ceiling");
                }
        });

        beginTest("aliasing drops as the oversampling factor rises");
        forEachConfig([this](double sampleRate, int blockSize) {
            // Bin centered, so every harmonic and alias lands on a bin and leakage stays within a few bins
            const auto frequency = std::round(5000.0 * fftSize / sampleRate) * sampleRate / fftSize;
            const auto input = sine(sampleRate, frequency, 0.5f, fftSize * 2);

            auto previousDb = 0.0f;
            for (auto factor = 0; factor < numFactors; ++factor)
            {
                ClipAudioProcessor processor;
                setParameter(processor, Parameters::inputGain, 12.0f);
                setParameter(processor, Parameters::threshold, thresholdDb);
                setParameter(processor, Parameters::oversampling, (float) factor);

                const auto output = render(processor, input, sampleRate, blockSize);
                const auto aliasDb = aliasingDb(output, sampleRate, frequency);
                const auto setting = describe(sampleRate, blockSize) + ", " + juce::String(1 << factor) + "x";

                if (blockSize == 512)
                    logMessage(setting + ": aliasing " + juce::String(aliasDb, 1) + " dB");

                // Below the floor the remainder is float rounding, which no longer drops with the factor
                if (factor > 0)
                    expect(aliasDb < previousDb || aliasDb < aliasingFloorDb, setting + ": " + juce::String(aliasDb, 1) + " dB");

                previousDb = aliasDb;
            }
        });

        beginTest("input and output peaks line up at every factor");
        for (auto factor = 0; factor < numFactors; ++factor)
        {
            ClipAudioProcessor processor;
            setParameter(processor, Parameters::oversampling, (float) factor);
            render(processor, multitone(44100.0, 44100 / 4), 44100.0, 512);

            // Tones with no common period, so no latency can line up with a shifted copy of the input
            std::vector<PeakFifo::Frame> frames(PeakFifo::capacity);
            const auto numFrames = processor.peakFifo.pop(frames.data(), (int) frames.size());
            expect(numFrames >= 240, juce::String(numFrames) + " frames");

            auto maxError = 0.0f;
            for (auto i = numFrames / 2; i < numFrames; ++i)
                maxError = std::max({ maxError,
                                      std::abs(frames[(size_t) i].output.min - frames[(size_t) i].input.min),
                                      std::abs(frames[(size_t) i].output.max - frames[(size_t) i].input.max) });

            expect(maxError < 0.01f, juce::String(1 << factor) + "x: " + juce::String(maxError));
        }

        beginTest("opening and closing the editor while audio runs is safe");
        {
            ClipAudioProcessor processor;
            processor.prepareToPlay(44100.0, 64);

            std::atomic<bool> running { true };
            std::thread audio([&] {
                juce::AudioBuffer<float> buffer(2, 64);
                juce::MidiBuffer midi;
                auto phase = 0.0;
                while (running)
                {
                    for (auto i = 0; i < 64; ++i, phase += 440.0 / 44100.0)
                        for (auto ch = 0; ch < 2; ++ch)
                            buffer.setSample(ch, i, 0.5f * (float) std::sin(juce::MathConstants<double>::twoPi * phase));
                    processor.processBlock(buffer, midi);
                }
            });

            std::array<PeakFifo::Frame, 64> frames;
            for (auto i = 0; i < 20; ++i)
            {
                std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
                for (auto n = 0; n < 50; ++n)
                    processor.peakFifo.pop(frames.data(), (int) frames.size());
            }

            running = false;
            audio.join();
            processor.releaseResources();
        }
    }

private:
    static constexpr int numFactors = 5;
    static constexpr int fftOrder = 14;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr float thresholdDb = -6.0f;
    static constexpr float outputGainDb = -3.0f;
    static constexpr float maxFilterOvershootDb = 0.5f;
    static constexpr float aliasingFloorDb = -100.0f;

    template <typename Fn>
    static void forEachConfig(Fn&& fn)
    {
        for (auto sampleRate : { 44100.0, 48000.0, 96000.0 })
            for (auto blockSize : { 1, 7, 512, 4096 })
                fn(sampleRate, blockSize);
    }

    static juce::String describe(double sampleRate, int blockSize)
    {
        return juce::String(sampleRate / 1000.0, 1) + " kHz, block " + juce::String(blockSize);
    }

    static void setParameter(ClipAudioProcessor& processor, const juce::ParameterID& id, float value)
    {
        auto* parameter = processor.apvts.getParameter(id.getParamID());
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }

    // Prepares with the parameters already set, so nothing ramps, then feeds the input in fixed size blocks
    static juce::AudioBuffer<float> render(ClipAudioProcessor& processor, const juce::AudioBuffer<float>& input, double sampleRate, int blockSize)
    {
        processor.prepareToPlay(sampleRate, blockSize);

        juce::AudioBuffer<float> output(input);
        juce::MidiBuffer midi;
        for (auto start = 0; start < output.getNumSamples(); start += blockSize)
        {
            juce::AudioBuffer<float> block(output.getArrayOfWritePointers(), output.getNumChannels(), start,
                                           std::min(blockSize, output.getNumSamples() - start));
            processor.processBlock(block, midi);
        }

        processor.releaseResources();
        return output;
    }

    static juce::AudioBuffer<float> sine(double sampleRate, double frequency, float amplitude, int numSamples)
    {
        juce::AudioBuffer<float> buffer(2, numSamples);
        for (auto i = 0; i < numSamples; ++i)
        {
            const auto x = amplitude * (float) std::sin(juce::MathConstants<double>::twoPi * frequency * i / sampleRate);
            buffer.setSample(0, i, x);
            buffer.setSample(1, i, -x);
        }
        return buffer;
    }

    static juce::AudioBuffer<float> multitone(double sampleRate, int numSamples)
    {
        juce::AudioBuffer<float> buffer(2, numSamples);
        buffer.clear();
        for (auto frequency : { 110.0, 1250.0, 6100.0 })
        {
            const auto tone = sine(sampleRate, frequency, 0.2f, numSamples);
            for (auto ch = 0; ch < 2; ++ch)
                buffer.addFrom(ch, 0, tone, ch, 0, numSamples);
        }
        return buffer;
    }

    static float peak(juce::dsp::AudioBlock<const float> block)
    {
        auto result = 0.0f;
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
            for (size_t i = 0; i < block.getNumSamples(); ++i)
                result = std::max(result, std::abs(block.getSample((int) ch, (int) i)));
        return result;
    }

    // 4x upsampled peak, which approximates the reconstructed peak a DAC or lossy encoder sees
    static float truePeak(juce::dsp::AudioBlock<const float> block)
    {
        juce::dsp::Oversampling<float> upsampler(block.getNumChannels(), 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        upsampler.initProcessing(block.getNumSamples());

        juce::AudioBuffer<float> copy((int) block.getNumChannels(), (int) block.getNumSamples());
        juce::dsp::AudioBlock<float>(copy).copyFrom(block);
        const auto upsampled = upsampler.processSamplesUp(juce::dsp::AudioBlock<float>(copy));

        // Skip the filter's startup transient
        return peak(upsampled.getSubBlock(upsampled.getNumSamples() / 4));
    }

    // Energy outside the harmonics of the input, relative to the total, over the last fftSize samples.
    // Only the audible band counts: the half band filters pass harmonics just above Nyquist at about -6 dB,
    // so at 48 kHz the 5th harmonic (25 kHz) folds to 23 kHz at every factor.
    static float aliasingDb(const juce::AudioBuffer<float>& output, double sampleRate, double frequency)
    {
        const auto normalisedFrequency = frequency / sampleRate;
        const auto lastBin = std::min(fftSize / 2, (int) (20000.0 * fftSize / sampleRate));
        std::vector<float> data(fftSize * 2, 0.0f);
        std::copy_n(output.getReadPointer(0, output.getNumSamples() - fftSize), fftSize, data.begin());

        juce::dsp::WindowingFunction<float>(fftSize, juce::dsp::WindowingFunction<float>::hann, false).multiplyWithWindowingTable(data.data(), fftSize);
        juce::dsp::FFT(fftOrder).performFrequencyOnlyForwardTransform(data.data(), true);

        const auto fundamentalBin = (int) std::round(normalisedFrequency * fftSize);
        auto isHarmonic = [fundamentalBin](int bin) {
            const auto nearest = (int) std::round((double) bin / fundamentalBin) * fundamentalBin;
            return nearest > 0 && std::abs(bin - nearest) <= 3;
        };

        auto aliased = 0.0, total = 0.0;
        for (auto bin = 4; bin <= lastBin; ++bin)
        {
            const auto power = juce::square((double) data[(size_t) bin]);
            total += power;
            if (! isHarmonic(bin))
                aliased += power;
        }

        return (float) (10.0 * std::log10(aliased / total));
    }
};

static ProcessorTests processorTests;
