#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

namespace TransferCurves
{
    // Linear below the knee and flat at the ceiling above it, joined by a quadratic knee centered on
    // the threshold. Slope stays between 0 and 1, so the curve never boosts or exceeds the ceiling.
    // Softness is the knee width as a percentage of the ceiling; 0 is a hard clip.
    inline float clip (float x, float thresholdDb, float softness)
    {
        const auto ceiling = juce::Decibels::decibelsToGain (thresholdDb);
        const auto knee = ceiling * juce::jlimit (0.0f, 100.0f, softness) / 100.0f;
        const auto kneeStart = ceiling - knee / 2.0f;
        const auto magnitude = std::abs (x);

        if (magnitude <= kneeStart)
            return x;

        if (magnitude >= ceiling + knee / 2.0f)
            return std::copysign (ceiling, x);

        const auto d = magnitude - kneeStart;
        return std::copysign (std::min (magnitude - d * d / (2.0f * knee), ceiling), x);
    }
}
