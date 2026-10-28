#pragma once
#include <JuceHeader.h>
#include "ClipLookAndFeel.h"
#include "../dsp/ClipTap.h"

// Spectrum of the clipper's input and output, low frequencies on the left, filled like EQ2's analyser.
// Two layers on top show what the clipper is doing: red for what the clip removes, which only appears
// while it's clipping, and yellow for what Detail keeps on the clipped peaks that a plain clip would
// have flattened. A lamp for each in the corner lights while that layer has anything in it.
class Spectrum : public juce::Component
{
public:
    static constexpr int fftOrder = 13;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int numBins = fftSize / 2;
    static constexpr int hopSize = fftSize / 8;
    static constexpr float minHz = 20.0f;
    static constexpr float maxHz = 20000.0f;
    static constexpr float floorDb = -84.0f;
    static constexpr float topDb = 6.0f;

    // Tilts the display up this much per octave around 1 kHz, so a typical mix reads roughly flat
    static constexpr float slopeDb = 4.5f;

    // A layer counts as active above this, in dB at the clipper
    static constexpr float activeDb = -60.0f;

    enum Layer
    {
        input,
        output,
        removed,
        kept
    };

    Spectrum()
    {
        for (auto& ring : rings)
            ring.resize (fftSize);
        for (auto& level : levels)
            level.resize (numBins);
    }

    // Called once per display frame with the samples that arrived since the last one.
    // Analyses every hop and returns whether it did.
    bool update (const ClipTap::Frame* frames, int numFrames, double newSampleRate)
    {
        sampleRate = newSampleRate;
        auto analysed = false;
        for (auto i = 0; i < numFrames; ++i)
        {
            const auto& frame = frames[i];
            rings[input][(size_t) ringPosition] = frame.input;
            rings[output][(size_t) ringPosition] = frame.output;
            rings[removed][(size_t) ringPosition] = frame.input - frame.output;
            rings[kept][(size_t) ringPosition] = frame.output - frame.plain;
            ringPosition = (ringPosition + 1) % fftSize;

            if (++samplesSinceHop == hopSize)
            {
                samplesSinceHop = 0;
                analyse();
                analysed = true;
            }
        }

        if (analysed)
            repaint();
        return analysed;
    }

    // Smoothed, untilted, in dB where a full scale sine reads 0
    float levelDb (Layer layer, float hz) const { return juce::Decibels::gainToDecibels (level (layer, hz, hz), floorDb); }

    bool isActive (Layer layer) const { return active[(size_t) layer]; }

    void paint (juce::Graphics& g) override
    {
        juce::Path outline;
        outline.addRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        g.setColour (Palette::display);
        g.fillPath (outline);
        g.reduceClipRegion (outline);

        g.setColour (Palette::text.withAlpha (0.18f));
        g.strokePath (pathFor (input), juce::PathStrokeType (1.0f));

        for (const auto [layer, colour, fill, stroke] : { std::tuple (output, Palette::accent, 0.1f, 0.35f),
                                                          std::tuple (removed, Palette::clip, 0.14f, 0.8f),
                                                          std::tuple (kept, Palette::detail, 0.18f, 0.8f) })
        {
            const auto path = pathFor (layer);
            g.setColour (colour.withAlpha (fill));
            g.fillPath (path);
            g.setColour (colour.withAlpha (stroke));
            g.strokePath (path, juce::PathStrokeType (1.5f));
        }

        // Lamps in the top right corner, the last one rightmost
        g.setFont (ClipLookAndFeel::displayFont (11.0f));
        auto row = getLocalBounds().toFloat().reduced (12.0f, 10.0f).withHeight (12.0f);
        for (const auto [layer, colour, name] : { std::tuple (kept, Palette::detail, "DETAIL"), std::tuple (removed, Palette::clip, "CLIP") })
        {
            const auto lit = isActive (layer);
            const auto label = row.removeFromRight (44.0f);
            row.removeFromRight (10.0f);
            const auto lamp = juce::Rectangle (8.0f, 8.0f).withCentre ({ label.getX() + 4.0f, label.getCentreY() });

            if (lit)
            {
                g.setColour (colour.withAlpha (0.3f));
                g.fillEllipse (lamp.expanded (3.0f));
            }
            g.setColour (lit ? colour : colour.withAlpha (0.2f));
            g.fillEllipse (lamp);
            g.setColour (lit ? Palette::text : Palette::textDim.withAlpha (0.5f));
            g.drawText (name, label.withTrimmedLeft (13.0f), juce::Justification::left);
        }
    }

private:
    void analyse()
    {
        for (auto layer : { input, output, removed, kept })
        {
            // Oldest sample first
            for (auto i = 0; i < fftSize; ++i)
                fftData[(size_t) i] = rings[layer][(size_t) ((ringPosition + i) % fftSize)];

            window.multiplyWithWindowingTable (fftData.data(), fftSize);
            fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

            // A full scale sine peaks at a quarter of the size with a Hann window. Levels rise fast and fall
            // slowly, so a short burst of clipping stays visible for a moment.
            auto& levelsOfLayer = levels[layer];
            for (auto bin = 0; bin < numBins; ++bin)
            {
                const auto magnitude = fftData[(size_t) bin] / (fftSize / 4.0f);
                auto& smoothed = levelsOfLayer[(size_t) bin];
                smoothed += (magnitude - smoothed) * (magnitude > smoothed ? 0.6f : 0.15f);
            }

            active[(size_t) layer] = *std::max_element (levelsOfLayer.begin(), levelsOfLayer.end()) > juce::Decibels::decibelsToGain (activeDb);
        }
    }

    // The peak between two frequencies, interpolating between bins where a pixel spans less than one
    float level (Layer layer, float fromHz, float toHz) const
    {
        const auto& bins = levels[layer];
        const auto binsPerHz = (float) fftSize / (float) sampleRate;
        const auto position = juce::jlimit (0.0f, (float) numBins - 2.0f, fromHz * binsPerHz);
        const auto lower = (int) position;

        auto peak = juce::jmap (position - (float) lower, bins[(size_t) lower], bins[(size_t) lower + 1]);
        for (auto bin = lower + 1; bin <= std::min (numBins - 1, (int) (toHz * binsPerHz)); ++bin)
            peak = std::max (peak, bins[(size_t) bin]);
        return peak;
    }

    // One point per pixel, closed just outside the bottom and sides so the stroke doesn't show along them
    juce::Path pathFor (Layer layer) const
    {
        const auto width = (float) getWidth(), bottom = (float) getHeight() + 2.0f;
        const auto frequencyAt = [width] (float x) { return minHz * std::pow (maxHz / minHz, x / width); };

        juce::Path path;
        path.startNewSubPath (-2.0f, bottom);
        for (auto x = 0.0f; x <= width; x += 1.0f)
        {
            // Nothing stays on the floor, rather than being tilted up off it
            const auto hz = frequencyAt (x);
            const auto untilted = juce::Decibels::gainToDecibels (level (layer, hz, frequencyAt (x + 1.0f)), floorDb);
            const auto db = untilted <= floorDb ? floorDb : untilted + slopeDb * std::log2 (hz / 1000.0f);
            path.lineTo (x, juce::jmap (juce::jlimit (floorDb, topDb, db), floorDb, topDb, bottom, 0.0f));
        }
        path.lineTo (width + 2.0f, bottom);
        path.closeSubPath();
        return path;
    }

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false };
    std::array<float, fftSize * 2> fftData {};
    std::array<std::vector<float>, 4> rings, levels;
    std::array<bool, 4> active {};
    int ringPosition = 0, samplesSinceHop = 0;
    double sampleRate = 44100.0;
};
