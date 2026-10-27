#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

namespace TransferCurves
{
    // Saved in sessions by index through the shape parameter: only ever add new shapes at the end
    enum class Shape
    {
        hard,
        soft,
        linear,
        tanh,
        sine,
        cubic
    };

    namespace detail
    {
        // How far past the knee start, in units of headroom, each shape reaches the ceiling
        inline double reach (Shape shape)
        {
            switch (shape)
            {
                case Shape::soft:
                case Shape::linear: return 2.0;
                case Shape::tanh: return std::numeric_limits<double>::infinity();
                case Shape::sine: return juce::MathConstants<double>::halfPi;
                case Shape::cubic: return 1.5;
                case Shape::hard: break;
            }
            return 0.0;
        }

        // Each shape's rise into the ceiling, for 0 <= u <= reach. Starts at slope 1 and ends at 1.
        inline double rise (Shape shape, double u)
        {
            switch (shape)
            {
                case Shape::soft: return u - u * u / 4.0;
                case Shape::linear: return u / 2.0;
                case Shape::tanh: return std::tanh (u);
                case Shape::sine: return std::sin (u);
                case Shape::cubic: return u - 4.0 / 27.0 * u * u * u;
                case Shape::hard: break;
            }
            return 0.0;
        }

        // Integral of rise from 0 to u
        inline double riseIntegral (Shape shape, double u)
        {
            switch (shape)
            {
                case Shape::soft: return u * u / 2.0 - u * u * u / 12.0;
                case Shape::linear: return u * u / 4.0;
                case Shape::tanh: return u + std::log1p (std::exp (-2.0 * u)) - std::log (2.0); // log (cosh (u)) without overflow
                case Shape::sine: return 1.0 - std::cos (u);
                case Shape::cubic: return u * u / 2.0 - u * u * u * u / 27.0;
                case Shape::hard: break;
            }
            return 0.0;
        }
    }

    // Linear below the knee and flat at the ceiling above it. In between, each shape eases into the ceiling
    // along its own curve scaled to the headroom: y = start + headroom * rise ((|x| - start) / headroom).
    // Every rise has a slope between 0 and 1, so no shape boosts or exceeds the ceiling.
    // Softness is the headroom as a percentage of the ceiling, the same for every shape except hard, which
    // ignores it. 0 is a hard clip, and 100 bends the curve all the way from silence.
    struct Knee
    {
        Knee (float thresholdDb, float softness, Shape newShape)
            : shape (newShape),
              ceiling (juce::Decibels::decibelsToGain (thresholdDb)),
              headroom (shape == Shape::hard ? 0.0f : ceiling * juce::jlimit (0.0f, 100.0f, softness) / 100.0f)
        {
        }

        float start() const { return ceiling - headroom; }

        // Where the curve reaches the ceiling. Tanh only approaches it, so its end is infinite.
        float end() const { return headroom > 0.0f ? start() + headroom * (float) detail::reach (shape) : ceiling; }

        Shape shape;
        float ceiling;
        float headroom;
    };

    inline float clip (float x, Knee knee)
    {
        const auto start = knee.start();
        const auto magnitude = std::abs (x);

        if (magnitude <= start)
            return x;

        if (magnitude >= knee.end())
            return std::copysign (knee.ceiling, x);

        const auto u = (double) (magnitude - start) / knee.headroom;
        return std::copysign (std::min ((float) (start + knee.headroom * detail::rise (knee.shape, u)), knee.ceiling), x);
    }

    inline float clip (float x, float thresholdDb, float softness, Shape shape)
    {
        return clip (x, Knee (thresholdDb, softness, shape));
    }

    // Integral of clip from 0 to x, in double because ADAA subtracts two nearby values
    inline double antiderivative (double x, Knee knee)
    {
        const auto ceiling = (double) knee.ceiling;
        const auto headroom = (double) knee.headroom;
        const auto start = ceiling - headroom;
        const auto magnitude = std::abs (x);

        if (magnitude <= start)
            return magnitude * magnitude / 2.0;

        // The knee up to where it flattens (or up to x, if x is still inside it), then the ceiling beyond that
        const auto u = headroom > 0.0 ? std::min ((magnitude - start) / headroom, detail::reach (knee.shape)) : 0.0;
        const auto throughKnee = start * start / 2.0 + start * headroom * u + headroom * headroom * detail::riseIntegral (knee.shape, u);
        return throughKnee + ceiling * (magnitude - start - headroom * u);
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
