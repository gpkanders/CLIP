#pragma once
#include <JuceHeader.h>
#include "ClipLookAndFeel.h"
#include "../dsp/ClipTap.h"

// Scrolling spectrogram of the clipper's output, newest on the right and low frequencies at the bottom.
// Each pixel is tinted by what makes it up: red for what the clip removes, and yellow for what Detail
// keeps on the clipped peaks that a plain clip would have flattened.
class Spectrogram : public juce::Component
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int hopSize = 512;
    static constexpr float minHz = 30.0f;
    static constexpr float maxHz = 20000.0f;
    static constexpr float floorDb = -72.0f;

    Spectrogram()
    {
        for (auto& ring : rings)
            ring.resize (fftSize);
        for (auto& magnitude : magnitudes)
            magnitude.resize (fftSize / 2);
    }

    // Called once per display frame with the samples that arrived since the last one.
    // Adds a column for every hop and returns whether it added any.
    bool update (const ClipTap::Frame* frames, int numFrames, double sampleRate)
    {
        auto added = false;
        for (auto i = 0; i < numFrames; ++i)
        {
            const auto& frame = frames[i];
            rings[output][(size_t) ringPosition] = frame.output;
            rings[kept][(size_t) ringPosition] = frame.output - frame.plain;
            rings[removed][(size_t) ringPosition] = frame.input - frame.output;
            ringPosition = (ringPosition + 1) % fftSize;

            if (++samplesSinceColumn == hopSize)
            {
                samplesSinceColumn = 0;
                addColumn (sampleRate);
                added = true;
            }
        }

        if (added)
            repaint();
        return added;
    }

    float toY (float hz) const
    {
        return (float) getHeight() * (1.0f - std::log (hz / minHz) / std::log (maxHz / minHz));
    }

    void paint (juce::Graphics& g) override
    {
        juce::Path outline;
        outline.addRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        g.setColour (Palette::display);
        g.fillPath (outline);
        g.reduceClipRegion (outline);

        // The oldest column sits just after the newest, so draw from there to the end, then the start
        const auto width = image.getWidth(), height = image.getHeight();
        g.drawImage (image, 0, 0, width - nextColumn, height, nextColumn, 0, width - nextColumn, height);
        g.drawImage (image, width - nextColumn, 0, nextColumn, height, 0, 0, nextColumn, height);
    }

    void resized() override
    {
        image = juce::Image (juce::Image::ARGB, std::max (1, getWidth()), std::max (1, getHeight()), true);
        nextColumn = 0;
    }

private:
    enum Signal
    {
        output,
        kept,
        removed
    };

    void addColumn (double sampleRate)
    {
        for (auto signal : { output, kept, removed })
        {
            // Oldest sample first
            for (auto i = 0; i < fftSize; ++i)
                fftData[(size_t) i] = rings[signal][(size_t) ((ringPosition + i) % fftSize)];

            window.multiplyWithWindowingTable (fftData.data(), fftSize);
            fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

            // A full scale sine peaks at a quarter of the size with a Hann window
            for (auto bin = 0; bin < fftSize / 2; ++bin)
                magnitudes[signal][(size_t) bin] = fftData[(size_t) bin] / (fftSize / 4.0f);
        }

        const juce::Image::BitmapData pixels (image, juce::Image::BitmapData::writeOnly);
        const auto binsPerHz = (float) fftSize / (float) sampleRate;

        for (auto y = 0; y < image.getHeight(); ++y)
        {
            // The bins between this row's edges, at least one
            const auto top = std::min (fftSize / 2 - 1, (int) (frequencyAt ((float) y) * binsPerHz));
            const auto bottom = std::min (top, (int) (frequencyAt ((float) y + 1.0f) * binsPerHz));
            const auto loudest = [&] (Signal signal) {
                return *std::max_element (magnitudes[signal].begin() + bottom, magnitudes[signal].begin() + top + 1);
            };

            pixels.setPixelColour (nextColumn, y, colourFor (loudest (output), loudest (kept), loudest (removed)));
        }

        nextColumn = (nextColumn + 1) % image.getWidth();
    }

    float frequencyAt (float y) const
    {
        return minHz * std::pow (maxHz / minHz, 1.0f - y / (float) image.getHeight());
    }

    static juce::Colour colourFor (float outputLevel, float keptLevel, float removedLevel)
    {
        const auto level = std::max (outputLevel, removedLevel);
        const auto brightness = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (level, floorDb) - floorDb) / -floorDb);
        const auto clipped = removedLevel / std::max (outputLevel + removedLevel, 1.0e-9f);
        const auto detail = std::min (1.0f, keptLevel / std::max (outputLevel, 1.0e-9f));

        return Palette::accent.interpolatedWith (Palette::clip, clipped)
            .interpolatedWith (Palette::detail, detail)
            .withAlpha (brightness * brightness * 0.55f);
    }

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false };
    std::array<float, fftSize * 2> fftData {};
    std::array<std::vector<float>, 3> rings, magnitudes;
    int ringPosition = 0, samplesSinceColumn = 0;

    juce::Image image { juce::Image::ARGB, 1, 1, true };
    int nextColumn = 0;
};
