#pragma once
#include <JuceHeader.h>
#include "ClipLookAndFeel.h"
#include "../Parameters.h"
#include "../dsp/PeakFifo.h"

// Scrolling input waveform with the output drawn over it, newest on the right. Both are shown at the
// clipper itself (input after input gain, output before output gain), so the threshold lines mark exactly
// where clipping starts, and the input beyond them is highlighted as the clipped part.
class WaveformDisplay : public juce::Component
{
public:
    // 8 ms per column, about 4 seconds across: long enough for bass cycles to read as a solid envelope
    static constexpr int framesPerColumn = 8;

    explicit WaveformDisplay (juce::AudioProcessorValueTreeState& apvts)
        : inputGain (*apvts.getRawParameterValue (Parameters::inputGain.getParamID())),
          threshold (*apvts.getRawParameterValue (Parameters::threshold.getParamID())),
          outputGain (*apvts.getRawParameterValue (Parameters::outputGain.getParamID())),
          shown { inputGain.load(), threshold.load(), outputGain.load() }
    {
        // Painting every pixel means a repaint each frame never repaints the editor behind it
        setOpaque (true);
    }

    // Called once per display frame. Rebuilds and repaints when there are new peaks or the gains or
    // threshold have moved. Returns whether it did.
    bool update (const PeakFifo::Frame* frames, int numFrames)
    {
        for (auto i = 0; i < numFrames; ++i)
            addFrame (frames[i]);

        const std::array newShown { inputGain.load(), threshold.load(), outputGain.load() };
        if (numFrames == 0 && newShown == shown)
            return false;

        shown = newShown;
        rebuild();
        return true;
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Palette::background);
        g.setColour (Palette::display);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

        g.setColour (Palette::clip);
        g.fillRectList (clippedColumns);
        g.setColour (Palette::accent);
        g.fillRectList (outputColumns);

        g.setColour (Palette::text.withAlpha (0.5f));
        const float dashes[] = { 4.0f, 4.0f };
        for (auto y : { thresholdBand.getY(), thresholdBand.getBottom() })
            g.drawDashedLine ({ plot.getX(), y, plot.getRight(), y }, dashes, 2);
    }

    void resized() override
    {
        plot = getLocalBounds().toFloat().reduced (8.0f);
        columns.assign ((size_t) std::max (1, (int) plot.getWidth()), silence);
        nextColumn = 0;
        rebuild();
    }

private:
    void addFrame (const PeakFifo::Frame& frame)
    {
        auto merge = [] (PeakFifo::Peak& into, PeakFifo::Peak from) {
            into.min = std::min (into.min, from.min);
            into.max = std::max (into.max, from.max);
        };

        merge (pending.input, frame.input);
        merge (pending.output, frame.output);

        if (++pendingFrames < framesPerColumn || columns.empty())
            return;

        columns[nextColumn] = pending;
        nextColumn = (nextColumn + 1) % columns.size();
        pending = {};
        pendingFrames = 0;
    }

    void rebuild()
    {
        const auto inputScale = juce::Decibels::decibelsToGain (shown[0]);
        const auto ceiling = juce::Decibels::decibelsToGain (shown[1]);
        const auto outputScale = 1.0f / juce::Decibels::decibelsToGain (shown[2]);

        thresholdBand = juce::Rectangle<float>::leftTopRightBottom (plot.getX(), toY (ceiling), plot.getRight(), toY (-ceiling));

        // One rectangle per column, oldest first. The output covers the input inside the threshold lines,
        // so only the clipped part of the input needs drawing.
        clippedColumns.clear();
        outputColumns.clear();

        for (size_t i = 0; i < columns.size(); ++i)
        {
            const auto& peaks = columns[(nextColumn + i) % columns.size()];
            const auto x = plot.getX() + (float) i;
            const auto inputMin = peaks.input.min * inputScale, inputMax = peaks.input.max * inputScale;

            outputColumns.addWithoutMerging (column (x, peaks.output.min * outputScale, peaks.output.max * outputScale));

            if (inputMax > ceiling)
                clippedColumns.addWithoutMerging (column (x, ceiling, inputMax));

            if (inputMin < -ceiling)
                clippedColumns.addWithoutMerging (column (x, inputMin, -ceiling));
        }

        repaint();
    }

    juce::Rectangle<float> column (float x, float low, float high) const
    {
        // Whole pixel edges, so the columns fill without antialiasing
        return juce::Rectangle<float>::leftTopRightBottom (x, std::round (toY (high)), x + 1.0f, std::round (toY (low)));
    }

    // Full scale reaches 90% of the way to the edge, and louder peaks run off it
    float toY (float level) const
    {
        return juce::jlimit (plot.getY(), plot.getBottom(), plot.getCentreY() - level * plot.getHeight() * 0.45f);
    }

    std::atomic<float>& inputGain;
    std::atomic<float>& threshold;
    std::atomic<float>& outputGain;
    std::array<float, 3> shown;

    static constexpr PeakFifo::Frame silence { { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    std::vector<PeakFifo::Frame> columns { 1, silence };
    size_t nextColumn = 0;
    PeakFifo::Frame pending;
    int pendingFrames = 0;

    juce::Rectangle<float> plot, thresholdBand;
    juce::RectangleList<float> clippedColumns, outputColumns;
};
