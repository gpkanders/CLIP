#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

namespace TransferCurves
{
    struct Knee
    {
        Knee (float thresholdDb, float softness)
            : ceiling (juce::Decibels::decibelsToGain (thresholdDb)),
              width (ceiling * juce::jlimit (0.0f, 100.0f, softness) / 100.0f)
        {
        }

        float ceiling;
        float width;
    };

    // Linear below the knee and flat at the ceiling above it, joined by a quadratic knee centered on
    // the threshold. Slope stays between 0 and 1, so the curve never boosts or exceeds the ceiling.
    // Softness is the knee width as a percentage of the ceiling; 0 is a hard clip.
    inline float clip (float x, Knee knee)
    {
        const auto kneeStart = knee.ceiling - knee.width / 2.0f;
        const auto magnitude = std::abs (x);

        if (magnitude <= kneeStart)
            return x;

        if (magnitude >= knee.ceiling + knee.width / 2.0f)
            return std::copysign (knee.ceiling, x);

        const auto d = magnitude - kneeStart;
        return std::copysign (std::min (magnitude - d * d / (2.0f * knee.width), knee.ceiling), x);
    }

    inline float clip (float x, float thresholdDb, float softness)
    {
        return clip (x, Knee (thresholdDb, softness));
    }

    // Integral of clip from 0 to x, in double because ADAA subtracts two nearby values
    inline double antiderivative (double x, Knee knee)
    {
        const auto ceiling = (double) knee.ceiling;
        const auto width = (double) knee.width;
        const auto kneeStart = ceiling - width / 2.0;
        const auto kneeEnd = ceiling + width / 2.0;
        const auto magnitude = std::abs (x);

        if (magnitude <= kneeStart)
            return magnitude * magnitude / 2.0;

        if (magnitude >= kneeEnd)
            return kneeEnd * kneeEnd / 2.0 - width * width / 6.0 + ceiling * (magnitude - kneeEnd);

        const auto d = magnitude - kneeStart;
        return magnitude * magnitude / 2.0 - d * d * d / (6.0 * width);
    }

    // First order antiderivative antialiasing: the average of clip over [previousX, x].
    // Falls back to the midpoint when the two samples are too close for the division to be accurate.
    inline float clipAntialiased (float x, float previousX, Knee knee)
    {
        const auto dx = (double) x - (double) previousX;

        if (std::abs (dx) < 1.0e-5)
            return clip ((x + previousX) / 2.0f, knee);

        return (float) ((antiderivative (x, knee) - antiderivative (previousX, knee)) / dx);
    }
}
