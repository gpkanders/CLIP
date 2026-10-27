#pragma once
#include <JuceHeader.h>
#include "ClipLookAndFeel.h"
#include "../Parameters.h"
#include "../dsp/TransferCurves.h"

// The clipper's input to output curve, drawn with the same function the DSP runs. Linear axes scaled to the
// ceiling keep the knee large enough to tell the shapes apart at any threshold, so the threshold shows in the labels.
// Input and output gain only move the signal along the curve, so they aren't part of it.
// Transparent, so whatever sits behind it shows through.
class TransferCurveDisplay : public juce::Component
{
public:
    // Axis lengths in multiples of the ceiling
    static constexpr float inputRange = 2.5f;
    static constexpr float outputRange = 1.25f;

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
        // Gridlines at multiples of half the ceiling, labelled with the level they mark
        g.setFont (ClipLookAndFeel::font (11.0f));
        for (auto multiple = 0.5f; multiple < inputRange; multiple += 0.5f)
        {
            const auto label = juce::String (shownThreshold + juce::Decibels::gainToDecibels (multiple), 1) + " dB";
            g.setColour (Palette::text.withAlpha (0.08f));
            g.drawVerticalLine (juce::roundToInt (toX (multiple)), plot.getY(), plot.getBottom());
            g.setColour (Palette::text.withAlpha (0.4f));
            g.drawText (label, juce::Rectangle (toX (multiple) + 4.0f, plot.getBottom() - 14.0f, 60.0f, 12.0f), juce::Justification::left);

            if (multiple < outputRange && multiple != 1.0f)
            {
                g.setColour (Palette::text.withAlpha (0.08f));
                g.drawHorizontalLine (juce::roundToInt (toY (multiple)), plot.getX(), plot.getRight());
                g.setColour (Palette::text.withAlpha (0.4f));
                g.drawText (label, juce::Rectangle (plot.getX() + 4.0f, toY (multiple) - 14.0f, 60.0f, 12.0f), juce::Justification::left);
            }
        }

        g.setColour (Palette::accent.withAlpha (0.12f));
        g.fillRect (knee);

        // Unity, for the curve to bend away from
        g.setColour (Palette::text.withAlpha (0.15f));
        g.drawLine ({ toX (0.0f), toY (0.0f), toX (outputRange), toY (outputRange) });

        g.setColour (Palette::text.withAlpha (0.5f));
        const float dashes[] = { 4.0f, 4.0f };
        g.drawDashedLine ({ plot.getX(), toY (1.0f), plot.getRight(), toY (1.0f) }, dashes, 2);
        g.drawText (juce::String (shownThreshold, 1) + " dB", juce::Rectangle (plot.getX() + 4.0f, toY (1.0f) - 14.0f, 60.0f, 12.0f), juce::Justification::left);

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
            const auto input = juce::jmap (x, plot.getX(), plot.getRight(), 0.0f, inputRange) * curveKnee.ceiling;
            const juce::Point point (x, toY (TransferCurves::clip (input, curveKnee) / curveKnee.ceiling));

            if (i == 0)
                curve.startNewSubPath (point);
            else
                curve.lineTo (point);
        }

        // Tanh never quite reaches the ceiling, so its knee runs off the right edge
        const auto kneeStart = toX (curveKnee.start() / curveKnee.ceiling);
        const auto kneeEnd = std::isfinite (curveKnee.end()) ? toX (curveKnee.end() / curveKnee.ceiling) : plot.getRight();
        knee = juce::Rectangle<float>::leftTopRightBottom (kneeStart, plot.getY(), std::min (kneeEnd, plot.getRight()), plot.getBottom());

        repaint();
    }

    // In multiples of the ceiling
    float toX (float level) const { return juce::jmap (level, 0.0f, inputRange, plot.getX(), plot.getRight()); }
    float toY (float level) const { return juce::jmap (level, 0.0f, outputRange, plot.getBottom(), plot.getY()); }

    std::atomic<float>& threshold;
    std::atomic<float>& softness;
    std::atomic<float>& shape;
    float shownThreshold, shownSoftness, shownShape;

    juce::Rectangle<float> plot, knee;
    juce::Path curve;
    juce::VBlankAttachment vblank { this, [this] { refresh(); } };
};
