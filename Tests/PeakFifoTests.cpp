#include "dsp/PeakFifo.h"

class PeakFifoTests : public juce::UnitTest
{
public:
    PeakFifoTests() : juce::UnitTest("PeakFifo") {}

    void runTest() override
    {
        beginTest("frames hold the min and max of each window across channels");
        {
            PeakFifo fifo;
            fifo.prepare(PeakFifo::framesPerSecond * 4);

            // Window 0 spans [-0.3, 0.4] on the input, window 1 is all 0.25
            juce::AudioBuffer<float> input(2, 8), output(2, 8);
            const float left[] = { 0.1f, -0.3f, 0.2f, 0.0f, 0.25f, 0.25f, 0.25f, 0.25f };
            const float right[] = { 0.4f, 0.0f, -0.1f, 0.0f, 0.25f, 0.25f, 0.25f, 0.25f };
            input.copyFrom(0, 0, left, 8);
            input.copyFrom(1, 0, right, 8);
            output.copyFrom(0, 0, input, 0, 0, 8);
            output.copyFrom(1, 0, input, 1, 0, 8);
            output.applyGain(0.5f);

            // Split across blocks so the partial window carries over
            for (auto [start, length] : { std::pair { 0, 3 }, std::pair { 3, 5 } })
                fifo.push(juce::dsp::AudioBlock<const float>(input).getSubBlock((size_t) start, (size_t) length),
                          juce::dsp::AudioBlock<const float>(output).getSubBlock((size_t) start, (size_t) length));

            std::array<PeakFifo::Frame, 4> frames;
            expectEquals(fifo.pop(frames.data(), (int) frames.size()), 2);
            expectEquals(frames[0].input.min, -0.3f);
            expectEquals(frames[0].input.max, 0.4f);
            expectEquals(frames[0].output.min, -0.15f);
            expectEquals(frames[0].output.max, 0.2f);
            expectEquals(frames[1].input.min, 0.25f);
            expectEquals(frames[1].input.max, 0.25f);
            expectEquals(fifo.pop(frames.data(), (int) frames.size()), 0);
        }

        beginTest("a full FIFO drops new frames and push does not allocate");
        {
            PeakFifo fifo;
            fifo.prepare(PeakFifo::framesPerSecond);

            {
                const juce::UnitTestAllocationChecker checker(*this);
                for (auto i = 0; i < PeakFifo::capacity * 2; ++i)
                    pushFrame(fifo, (float) i);
            }

            std::vector<PeakFifo::Frame> frames(PeakFifo::capacity * 2);
            const auto numFrames = fifo.pop(frames.data(), (int) frames.size());
            expectEquals(numFrames, PeakFifo::capacity - 1);
            expectEquals(frames[0].input.max, 0.0f);
            expectEquals(frames[(size_t) numFrames - 1].input.max, (float) numFrames - 1);
        }

        beginTest("discard drops queued frames");
        {
            PeakFifo fifo;
            fifo.prepare(PeakFifo::framesPerSecond);
            for (auto i = 0; i < 10; ++i)
                pushFrame(fifo, (float) i);

            fifo.discard();
            PeakFifo::Frame frame;
            expectEquals(fifo.pop(&frame, 1), 0);
        }

        beginTest("frames arrive in order while the producer runs on another thread");
        {
            PeakFifo fifo;
            fifo.prepare(PeakFifo::framesPerSecond);

            constexpr auto numPushed = 200000;
            std::atomic<bool> done { false };
            std::thread producer([&] {
                for (auto i = 0; i < numPushed; ++i)
                    pushFrame(fifo, (float) i);
                done = true;
            });

            std::array<PeakFifo::Frame, 256> frames;
            auto previous = -1.0f;
            auto received = 0, outOfOrder = 0;
            for (auto finished = false; ! finished;)
            {
                finished = done;
                const auto numFrames = fifo.pop(frames.data(), (int) frames.size());
                for (auto i = 0; i < numFrames; ++i)
                {
                    if (frames[(size_t) i].input.max <= previous || ! juce::exactlyEqual(frames[(size_t) i].output.min, frames[(size_t) i].input.max))
                        ++outOfOrder;
                    previous = frames[(size_t) i].input.max;
                }
                received += numFrames;
            }

            producer.join();
            logMessage("received " + juce::String(received) + " of " + juce::String(numPushed) + " frames");
            expectEquals(outOfOrder, 0);
            expect(received > 0);
        }
    }

private:
    // One sample per frame at framesPerSecond, so each push completes a frame holding value
    static void pushFrame(PeakFifo& fifo, float value)
    {
        float sample[] = { value };
        float* channels[] = { sample };
        const juce::dsp::AudioBlock<const float> block(channels, 1, 1);
        fifo.push(block, block);
    }
};

static PeakFifoTests peakFifoTests;
