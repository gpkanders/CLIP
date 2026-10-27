#include "PluginProcessor.h"
#include "Parameters.h"

class ClipTapTests : public juce::UnitTest
{
public:
    ClipTapTests() : juce::UnitTest("ClipTap") {}

    void runTest() override
    {
        beginTest("shows what the processor's clipper does at the base rate");
        {
            ClipAudioProcessor processor;
            setParameter(processor, Parameters::inputGain, 6.0f);
            setParameter(processor, Parameters::threshold, -6.0f);
            setParameter(processor, Parameters::detail, 100.0f);
            setParameter(processor, Parameters::oversampling, 0.0f);

            constexpr auto blockSize = 512, numBlocks = 40;
            processor.prepareToPlay(44100.0, blockSize);

            juce::AudioBuffer<float> buffer(2, blockSize), input(1, blockSize * numBlocks), output(1, blockSize * numBlocks);
            juce::MidiBuffer midi;
            for (auto block = 0; block < numBlocks; ++block)
            {
                for (auto i = 0; i < blockSize; ++i)
                {
                    const auto t = (float) (block * blockSize + i) / 44100.0f;
                    const auto x = 0.6f * std::sin(juce::MathConstants<float>::twoPi * 60.0f * t) + 0.1f * std::sin(juce::MathConstants<float>::twoPi * 5000.0f * t);
                    input.setSample(0, block * blockSize + i, x);
                    for (auto ch = 0; ch < 2; ++ch)
                        buffer.setSample(ch, i, x);
                }
                processor.processBlock(buffer, midi);
                output.copyFrom(0, block * blockSize, buffer, 0, 0, blockSize);
            }

            std::vector<ClipTap::Frame> frames(ClipTap::capacity);
            const auto numFrames = processor.tap.pop(frames.data(), (int) frames.size());
            expectEquals(numFrames, blockSize * numBlocks);

            auto outputError = 0.0f, inputError = 0.0f, detailKept = 0.0f;
            for (auto i = 0; i < numFrames; ++i)
            {
                const auto& frame = frames[(size_t) i];
                outputError = std::max(outputError, std::abs(frame.output - output.getSample(0, i)));
                inputError = std::max(inputError, std::abs(frame.input - input.getSample(0, i) * juce::Decibels::decibelsToGain(6.0f)));
                detailKept = std::max(detailKept, std::abs(frame.output - frame.plain));
            }

            expect(outputError < 1.0e-5f, "output error " + juce::String(outputError));
            expect(inputError < 1.0e-5f, "input error " + juce::String(inputError));
            expect(detailKept > 0.01f, "detail should change the output: " + juce::String(detailKept));
        }

        beginTest("a full tap drops new samples and push does not allocate");
        {
            ClipTap tap;
            tap.setParameters(0.0f, 0.0f, 0.0f, TransferCurves::Shape::hard, 0.0f);
            tap.prepare(44100.0, 64);

            juce::AudioBuffer<float> buffer(2, 64);
            {
                const juce::UnitTestAllocationChecker checker(*this);
                for (auto i = 0; i < ClipTap::capacity * 2 / 64; ++i)
                    tap.push(juce::dsp::AudioBlock<const float>(buffer));
            }

            std::vector<ClipTap::Frame> frames(ClipTap::capacity);
            expectEquals(tap.pop(frames.data(), (int) frames.size()), ClipTap::capacity - 1);
        }

        beginTest("blocks longer than the prepared size all come through, mixed to mono");
        {
            ClipTap tap;
            tap.setParameters(0.0f, 0.0f, 0.0f, TransferCurves::Shape::hard, 0.0f);
            tap.prepare(44100.0, 64);

            juce::AudioBuffer<float> buffer(2, 1000);
            for (auto i = 0; i < buffer.getNumSamples(); ++i)
            {
                buffer.setSample(0, i, (float) i / 1000.0f);
                buffer.setSample(1, i, 0.0f);
            }
            tap.push(juce::dsp::AudioBlock<const float>(buffer));

            std::vector<ClipTap::Frame> frames(2000);
            expectEquals(tap.pop(frames.data(), (int) frames.size()), 1000);
            for (auto i = 0; i < 1000; i += 99)
                expectWithinAbsoluteError(frames[(size_t) i].input, (float) i / 2000.0f, 1.0e-6f);
        }

        beginTest("discard drops queued samples");
        {
            ClipTap tap;
            tap.prepare(44100.0, 64);
            juce::AudioBuffer<float> buffer(1, 64);
            buffer.clear();
            tap.push(juce::dsp::AudioBlock<const float>(buffer));

            tap.discard();
            ClipTap::Frame frame;
            expectEquals(tap.pop(&frame, 1), 0);
        }
    }

private:
    static void setParameter(ClipAudioProcessor& processor, const juce::ParameterID& id, float value)
    {
        auto* parameter = processor.apvts.getParameter(id.getParamID());
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }
};

static ClipTapTests clipTapTests;
