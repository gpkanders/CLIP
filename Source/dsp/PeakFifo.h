#pragma once
#include <juce_dsp/juce_dsp.h>

// Single producer, single consumer pipe of min/max peaks from the audio thread to the UI.
// Frames come at a fixed rate, so the storage is fixed too and never reallocates under a reader.
// When the UI falls behind, new frames are dropped instead of waiting.
class PeakFifo
{
public:
    static constexpr double framesPerSecond = 1000.0;
    static constexpr int capacity = 4096;

    struct Peak
    {
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::lowest();
    };

    // Peaks across all channels over one window
    struct Frame
    {
        Peak input, output;
    };

    // Audio side, before processing starts
    void prepare (double sampleRate)
    {
        samplesPerFrame = std::max (1, juce::roundToInt (sampleRate / framesPerSecond));
        pending = {};
        pendingSamples = 0;
    }

    // Audio thread. Input and output must be time aligned and the same length.
    void push (juce::dsp::AudioBlock<const float> input, juce::dsp::AudioBlock<const float> output) noexcept
    {
        jassert (input.getNumSamples() == output.getNumSamples());

        for (size_t i = 0; i < input.getNumSamples(); ++i)
        {
            accumulate (pending.input, input, i);
            accumulate (pending.output, output, i);

            if (++pendingSamples == samplesPerFrame)
            {
                fifo.write (1).forEach ([this] (int index) { frames[(size_t) index] = pending; });
                pending = {};
                pendingSamples = 0;
            }
        }
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

private:
    static void accumulate (Peak& peak, const juce::dsp::AudioBlock<const float>& block, size_t i) noexcept
    {
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            const auto sample = block.getChannelPointer (ch)[i];
            peak.min = std::min (peak.min, sample);
            peak.max = std::max (peak.max, sample);
        }
    }

    juce::AbstractFifo fifo { capacity };
    std::array<Frame, capacity> frames;
    Frame pending;
    int pendingSamples = 0;
    int samplesPerFrame = 1;
};
