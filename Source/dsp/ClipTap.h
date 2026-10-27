#pragma once
#include "Clipper.h"

// Feeds the display. Clips a mono mix of the input again at the base rate, once as set and once without
// detail, and pipes the samples from the audio thread to the UI, so the display can show what the clip
// removes and what Detail keeps. Output gain is left out, like the curve, so this shows the clipper itself.
// Single producer, single consumer with fixed storage. When the UI falls behind, new samples are dropped.
class ClipTap
{
public:
    static constexpr int capacity = 1 << 15;

    // At the clipper: the input after input gain, the output, and the output with detail off
    struct Frame
    {
        float input = 0.0f, output = 0.0f, plain = 0.0f;
    };

    // Audio side, before processing starts. Call setParameters first to start without a ramp.
    void prepare (double newSampleRate, int maxBlockSize)
    {
        sampleRate = newSampleRate;
        for (auto* each : { &clipper, &plainClipper })
            each->prepare (newSampleRate);

        input.setSize (1, maxBlockSize);
        output.setSize (1, maxBlockSize);
        plain.setSize (1, maxBlockSize);
    }

    void setParameters (float inputGainDb, float thresholdDb, float softness, TransferCurves::Shape shape, float detail)
    {
        inputGain = juce::Decibels::decibelsToGain (inputGainDb);
        for (auto* each : { &clipper, &plainClipper })
        {
            each->setParameters (inputGainDb, thresholdDb, softness, 0.0f);
            each->setShape (shape);
        }
        clipper.setDetail (detail);
    }

    // Audio thread
    void push (juce::dsp::AudioBlock<const float> block) noexcept
    {
        for (size_t start = 0; start < block.getNumSamples(); start += (size_t) input.getNumSamples())
            pushChunk (block.getSubBlock (start, std::min (block.getNumSamples() - start, (size_t) input.getNumSamples())));
    }

    // UI thread. Returns the number of frames copied, oldest first.
    int pop (Frame* destination, int maxFrames) noexcept
    {
        auto numFrames = 0;
        fifo.read (maxFrames).forEach ([&] (int index) { destination[numFrames++] = frames[(size_t) index]; });
        return numFrames;
    }

    // UI thread. Drops frames that queued up while nothing was reading.
    void discard() noexcept { fifo.finishedRead (fifo.getNumReady()); }

    // Written before processing starts, read by the UI to place frequencies
    std::atomic<double> sampleRate { 44100.0 };

private:
    void pushChunk (juce::dsp::AudioBlock<const float> block) noexcept
    {
        const auto numSamples = (int) block.getNumSamples();
        input.clear();
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
            input.addFrom (0, 0, block.getChannelPointer (ch), numSamples, 1.0f / (float) block.getNumChannels());

        output.copyFrom (0, 0, input, 0, 0, numSamples);
        plain.copyFrom (0, 0, input, 0, 0, numSamples);
        clipper.process (juce::dsp::AudioBlock<float> (output).getSubBlock (0, (size_t) numSamples));
        plainClipper.process (juce::dsp::AudioBlock<float> (plain).getSubBlock (0, (size_t) numSamples));

        auto i = 0;
        fifo.write (numSamples).forEach ([&] (int index) {
            frames[(size_t) index] = { input.getSample (0, i) * inputGain, output.getSample (0, i), plain.getSample (0, i) };
            ++i;
        });
    }

    Clipper clipper, plainClipper;
    juce::AudioBuffer<float> input, output, plain;
    float inputGain = 1.0f;

    juce::AbstractFifo fifo { capacity };
    std::array<Frame, capacity> frames;
};
