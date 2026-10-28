#pragma once
#include <JuceHeader.h>
#include "ClipLookAndFeel.h"
#include "../Parameters.h"
#include "../dsp/ClipTap.h"

// The clipper's input and output peaks scrolling right to left in dBFS, under a line at the threshold.
// Red shows where the input went past the output, which is what the clip took off. Yellow hangs down from
// the threshold by how many dB Detail gave back on the clipped peaks, on the same scale as the red above it.
// A lamp for each in the corner lights while that layer showed up recently.
class PeakHistory : public juce::Component
{
public:
    static constexpr float secondsShown = 3.0f;
    static constexpr float floorDb = -48.0f;
    static constexpr float topDb = 12.0f;
    static constexpr float gridStepDb = 12.0f;

    // A layer counts as active past this many dB, and its lamp stays lit this long after
    static constexpr float activeDb = 0.1f;
    static constexpr float lampSeconds = 0.25f;

    enum Layer
    {
        clip,
        detail
    };

    explicit PeakHistory (juce::AudioProcessorValueTreeState& apvts)
        : threshold (*apvts.getRawParameterValue (Parameters::threshold.getParamID())),
          shownThreshold (threshold.load())
    {
    }

    // Called once per display frame with the samples that arrived since the last one.
    // Adds a column every time enough samples have come in to fill one, and returns whether it did.
    bool update (const ClipTap::Frame* frames, int numFrames, double sampleRate)
    {
        if (columns.empty())
            return false;

        samplesPerColumn = std::max (1, juce::roundToInt (sampleRate * secondsShown / (double) columns.size()));

        auto added = false;
        for (auto i = 0; i < numFrames; ++i)
        {
            const auto& frame = frames[i];
            current.input = std::max (current.input, std::abs (frame.input));
            current.output = std::max (current.output, std::abs (frame.output));
            current.plain = std::max (current.plain, std::abs (frame.plain));

            if (++samplesInCurrent == samplesPerColumn)
            {
                columns[(size_t) next] = current;
                next = (next + 1) % (int) columns.size();
                current = {};
                samplesInCurrent = 0;
                added = true;
            }
        }

        const auto thresholdMoved = ! juce::exactlyEqual (threshold.load(), shownThreshold);
        shownThreshold = threshold.load();

        if (added || thresholdMoved)
            repaint();
        return added;
    }

    bool isActive (Layer layer) const
    {
        const auto lampColumns = std::min ((int) columns.size(), (int) std::ceil (lampSeconds * (float) columns.size() / secondsShown));
        for (auto age = 1; age <= lampColumns; ++age)
        {
            const auto& column = columns[(size_t) ((next - age + (int) columns.size()) % (int) columns.size())];
            if ((layer == clip ? column.clippedDb() : column.detailDb()) > activeDb)
                return true;
        }
        return false;
    }

    void paint (juce::Graphics& g) override
    {
        juce::Path outline;
        outline.addRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        g.setColour (Palette::display);
        g.fillPath (outline);
        g.reduceClipRegion (outline);

        g.setColour (Palette::text.withAlpha (0.08f));
        for (auto db = topDb - gridStepDb; db > floorDb; db -= gridStepDb)
            g.drawHorizontalLine (juce::roundToInt (toY (db)), 0.0f, (float) getWidth());

        const auto inputY = [this] (const Column& column) { return toY (toDb (column.input)); };
        const auto outputY = [this] (const Column& column) { return toY (toDb (column.output)); };

        // Between the input and the output, where the clip took the input down
        auto clipped = edge (inputY);
        for (auto x = (int) columns.size() - 1; x >= 0; --x)
            clipped.lineTo ((float) x, outputY (at ((size_t) x)));
        clipped.closeSubPath();
        g.setColour (Palette::clip.withAlpha (0.55f));
        g.fillPath (clipped);

        const auto output = edge (outputY);
        auto outputFill = output;
        outputFill.lineTo ((float) getWidth() + 2.0f, (float) getHeight() + 2.0f);
        outputFill.lineTo (-2.0f, (float) getHeight() + 2.0f);
        outputFill.closeSubPath();
        g.setColour (Palette::accent.withAlpha (0.12f));
        g.fillPath (outputFill);
        g.setColour (Palette::accent.withAlpha (0.8f));
        g.strokePath (output, juce::PathStrokeType (1.5f));

        paintDetail (g);

        const auto thresholdY = toY (shownThreshold);
        g.setColour (Palette::text.withAlpha (0.6f));
        const float dashes[] = { 4.0f, 4.0f };
        g.drawDashedLine ({ 0.0f, thresholdY, (float) getWidth(), thresholdY }, dashes, 2);

        g.setFont (ClipLookAndFeel::displayFont (11.0f));
        g.setColour (Palette::text.withAlpha (0.4f));
        for (auto db = topDb - gridStepDb; db > floorDb; db -= gridStepDb)
            g.drawText (juce::String (db, 0) + " dB", juce::Rectangle (8.0f, toY (db) - 14.0f, 60.0f, 12.0f), juce::Justification::left);

        paintLamps (g);
    }

    void resized() override
    {
        columns.assign ((size_t) std::max (1, getWidth()), {});
        next = 0;
    }

private:
    // The loudest samples in one column of pixels, at the clipper
    struct Column
    {
        float input = 0.0f, output = 0.0f, plain = 0.0f;

        float clippedDb() const { return toDb (input) - toDb (output); }
        float detailDb() const { return toDb (output) - toDb (plain); }
    };

    static float toDb (float gain) { return juce::Decibels::gainToDecibels (gain, floorDb); }

    float toY (float db) const { return juce::jmap (juce::jlimit (floorDb, topDb, db), floorDb, topDb, (float) getHeight() + 2.0f, 0.0f); }

    // Oldest on the left
    const Column& at (size_t x) const { return columns[(x + (size_t) next) % columns.size()]; }

    // Through every column, one per pixel, left to right
    template <typename Y>
    juce::Path edge (Y y) const
    {
        juce::Path path;
        path.startNewSubPath (-2.0f, y (at (0)));
        for (size_t x = 0; x < columns.size(); ++x)
            path.lineTo ((float) x, y (at (x)));
        return path;
    }

    // Down from the threshold, as many dB as Detail added to the clipped peak
    void paintDetail (juce::Graphics& g) const
    {
        const auto thresholdY = toY (shownThreshold);
        const auto pixelsPerDb = (float) getHeight() / (topDb - floorDb);
        // Filled only, since an outline would run along the threshold wherever there's no detail
        auto hanging = edge ([&] (const Column& column) { return thresholdY + column.detailDb() * pixelsPerDb; });
        hanging.lineTo ((float) getWidth() + 2.0f, thresholdY);
        hanging.lineTo (-2.0f, thresholdY);
        hanging.closeSubPath();
        g.setColour (Palette::detail.withAlpha (0.6f));
        g.fillPath (hanging);
    }

    // In the top right corner, the last one rightmost
    void paintLamps (juce::Graphics& g) const
    {
        auto row = getLocalBounds().toFloat().reduced (12.0f, 10.0f).withHeight (12.0f);
        for (const auto [layer, colour, name] : { std::tuple (detail, Palette::detail, "DETAIL"), std::tuple (clip, Palette::clip, "CLIP") })
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

    std::atomic<float>& threshold;
    float shownThreshold;

    std::vector<Column> columns;
    Column current;
    int next = 0, samplesInCurrent = 0, samplesPerColumn = 1;
};
