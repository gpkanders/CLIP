#pragma once
#include <JuceHeader.h>
#include "ClipLookAndFeel.h"
#include "../Parameters.h"
#include "../dsp/TransferCurves.h"

// The clipper's input to output curve in dB, drawn with the same function the DSP runs.
// Input and output gain only move the signal along the curve, so they aren't part of it.
class TransferCurveDisplay : public juce::Component
{
public:
    static constexpr float minDb = -48.0f;
    static constexpr float maxDb = 6.0f;

    explicit TransferCurveDisplay (juce::AudioProcessorValueTreeState& apvts)
        : threshold (*apvts.getRawParameterValue (Parameters::threshold.getParamID())),
          softness (*apvts.getRawParameterValue (Parameters::softness.getParamID())),
          shape (*apvts.getRawParameterValue (Parameters::shape.getParamID())),
          shownThreshold (threshold.load()),
          shownSoftness (softness.load()),
          shownShape (shape.load())
    {
    }

    // Rebuilds and repaints when the curve's parameters have moved. Returns whether they had.
    bool refresh()
    {
        const auto newThreshold = threshold.load(), newSoftness = softness.load(), newShape = shape.load();
        if (juce::exactlyEqual (newThreshold, shownThreshold) && juce::exactlyEqual (newSoftness, shownSoftness)
            && juce::exactlyEqual (newShape, shownShape))
            return false;

        shownThreshold = newThreshold;
        shownSoftness = newSoftness;
        shownShape = newShape;
        rebuild();
        return true;
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Palette::display);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

        g.setFont (ClipLookAndFeel::font (11.0f));
        for (auto db = maxDb - 6.0f; db > minDb; db -= 12.0f)
        {
            g.setColour (Palette::text.withAlpha (0.08f));
            g.drawHorizontalLine (juce::roundToInt (toY (db)), plot.getX(), plot.getRight());
            g.drawVerticalLine (juce::roundToInt (toX (db)), plot.getY(), plot.getBottom());

            g.setColour (Palette::text.withAlpha (0.4f));
            g.drawText (juce::String (juce::roundToInt (db)), juce::Rectangle (plot.getX() + 4.0f, toY (db) - 14.0f, 30.0f, 12.0f), juce::Justification::left);
        }

        g.setColour (Palette::accent.withAlpha (0.12f));
        g.fillRect (knee);

        g.setColour (Palette::text.withAlpha (0.5f));
        const float dashes[] = { 4.0f, 4.0f };
        g.drawDashedLine ({ plot.getX(), toY (shownThreshold), plot.getRight(), toY (shownThreshold) }, dashes, 2);

        g.setColour (Palette::accent);
        g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void resized() override
    {
        plot = getLocalBounds().toFloat().reduced (8.0f);
        rebuild();
    }

private:
    void rebuild()
    {
        const TransferCurves::Knee curveKnee (shownThreshold, shownSoftness, (TransferCurves::Shape) (int) shownShape);

        curve.clear();
        // One point per pixel
        for (auto i = 0; i <= (int) plot.getWidth(); ++i)
        {
            const auto x = plot.getX() + (float) i;
            const auto inputDb = juce::jmap (x, plot.getX(), plot.getRight(), minDb, maxDb);
            const auto outputDb = juce::Decibels::gainToDecibels (TransferCurves::clip (juce::Decibels::decibelsToGain (inputDb), curveKnee), minDb);
            const juce::Point point (x, toY (outputDb));

            if (i == 0)
                curve.startNewSubPath (point);
            else
                curve.lineTo (point);
        }

        // Tanh never quite reaches the ceiling, so its knee runs off the right edge
        const auto kneeStart = toX (juce::Decibels::gainToDecibels (curveKnee.start(), minDb));
        const auto kneeEnd = std::isfinite (curveKnee.end()) ? toX (juce::Decibels::gainToDecibels (curveKnee.end(), minDb)) : plot.getRight();
        knee = juce::Rectangle<float>::leftTopRightBottom (kneeStart, plot.getY(), std::min (kneeEnd, plot.getRight()), plot.getBottom());

        repaint();
    }

    float toX (float db) const { return juce::jmap (db, minDb, maxDb, plot.getX(), plot.getRight()); }
    float toY (float db) const { return juce::jmap (db, minDb, maxDb, plot.getBottom(), plot.getY()); }

    std::atomic<float>& threshold;
    std::atomic<float>& softness;
    std::atomic<float>& shape;
    float shownThreshold, shownSoftness, shownShape;

    juce::Rectangle<float> plot, knee;
    juce::Path curve;
    juce::VBlankAttachment vblank { this, [this] { refresh(); } };
};
