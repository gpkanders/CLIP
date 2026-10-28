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

    // How much of the peak marker's level is left each display frame
    static constexpr float peakDecay = 0.92f;

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

    // The loudest input at the clipper since the last display frame. Marks it on the curve, falling back slowly.
    void showPeak (float level)
    {
        const auto decayed = std::max (level, peak * peakDecay);
        if (juce::exactlyEqual (decayed, peak))
            return;

        peak = decayed < 1.0e-4f ? 0.0f : decayed;
        repaint();
    }

    bool isClipping() const { return peak > kneeFor().start(); }

    void paint (juce::Graphics& g) override
    {
        // Gridlines at multiples of half the ceiling, labelled with the level they mark
        g.setFont (ClipLookAndFeel::displayFont (11.0f));
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

        // A glow under the curve, widest and faintest first
        for (const auto [width, alpha] : { std::pair (9.0f, 0.06f), std::pair (6.0f, 0.1f), std::pair (4.0f, 0.16f) })
        {
            g.setColour (Palette::accent.withAlpha (alpha));
            g.strokePath (curve, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setColour (Palette::accent);
        g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (peak > 0.0f)
            paintPeak (g);
    }

    void resized() override
    {
        plot = getLocalBounds().toFloat().reduced (8.0f);
        rebuild();
    }

private:
    // A dot where the peak sits on the curve. While it's past the knee, the stretch of curve from the knee up to it,
    // where the clip is bending the signal, lights red.
    void paintPeak (juce::Graphics& g) const
    {
        const auto curveKnee = kneeFor();
        const auto peakX = std::min (toX (peak / curveKnee.ceiling), plot.getRight());
        const auto clipping = isClipping();

        if (clipping)
        {
            juce::Path clipped;
            clipped.startNewSubPath (pointAt (toX (curveKnee.start() / curveKnee.ceiling), curveKnee));
            for (auto x = toX (curveKnee.start() / curveKnee.ceiling) + 1.0f; x < peakX; x += 1.0f)
                clipped.lineTo (pointAt (x, curveKnee));
            clipped.lineTo (pointAt (peakX, curveKnee));

            for (const auto [width, alpha] : { std::pair (9.0f, 0.12f), std::pair (5.0f, 0.25f), std::pair (2.5f, 1.0f) })
            {
                g.setColour (Palette::clip.withAlpha (alpha));
                g.strokePath (clipped, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }

        const auto colour = clipping ? Palette::clip : Palette::accent;
        const auto dot = juce::Rectangle (8.0f, 8.0f).withCentre (pointAt (peakX, curveKnee));
        g.setColour (colour.withAlpha (0.3f));
        g.fillEllipse (dot.expanded (4.0f));
        g.setColour (colour);
        g.fillEllipse (dot);
    }

    TransferCurves::Knee kneeFor() const { return { shownThreshold, shownSoftness, (TransferCurves::Shape) (int) shownShape }; }

    // On the curve, at a horizontal position in the plot
    juce::Point<float> pointAt (float x, TransferCurves::Knee curveKnee) const
    {
        const auto input = juce::jmap (x, plot.getX(), plot.getRight(), 0.0f, inputRange) * curveKnee.ceiling;
        return { x, toY (TransferCurves::clip (input, curveKnee) / curveKnee.ceiling) };
    }

    void rebuild()
    {
        const auto curveKnee = kneeFor();

        // One point per pixel
        std::vector<juce::Point<float>> points;
        for (auto i = 0; i <= (int) plot.getWidth(); ++i)
            points.push_back (pointAt (plot.getX() + (float) i, curveKnee));

        // Points inside a flat run add nothing, and the glow's wide strokes leave specks at the joins between them
        curve.clear();
        curve.startNewSubPath (points.front());
        for (size_t i = 1; i < points.size(); ++i)
            if (i == points.size() - 1 || ! juce::exactlyEqual (points[i - 1].y, points[i].y) || ! juce::exactlyEqual (points[i + 1].y, points[i].y))
                curve.lineTo (points[i]);

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
    float peak = 0.0f;

    juce::Rectangle<float> plot, knee;
    juce::Path curve;
    juce::VBlankAttachment vblank { this, [this] { refresh(); } };
};
