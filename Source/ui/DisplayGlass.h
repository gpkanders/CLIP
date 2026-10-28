#pragma once
#include <JuceHeader.h>

// The glass over the display: shade along the edges where it meets the bezel, and a glare across the top left.
// It never changes, so it's cached, and it lets the mouse through to the display.
class DisplayGlass : public juce::Component
{
public:
    static constexpr float cornerRadius = 6.0f;

    DisplayGlass()
    {
        setBufferedToImage (true);
        setInterceptsMouseClicks (false, false);
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Shade fading in from the edges
        constexpr auto steps = 11;
        for (auto i = 0; i < steps; ++i)
        {
            const auto fade = 1.0f - (float) i / steps;
            g.setColour (juce::Colours::black.withAlpha (0.16f * fade * fade));
            g.drawRoundedRectangle (bounds.reduced (1.0f + (float) i * 2.0f), std::max (0.0f, cornerRadius - (float) i), 2.0f);
        }

        g.setColour (juce::Colours::black.withAlpha (0.9f));
        g.fillRect (bounds.reduced (cornerRadius, 0.0f).withHeight (1.0f));

        // A band of glare lit along a direction slightly right of straight down, with a hard lower edge a third of the way along it
        const auto direction = juce::Point (std::sin (juce::degreesToRadians (165.0f)), -std::cos (juce::degreesToRadians (165.0f)));
        const auto length = bounds.getWidth() * std::abs (direction.x) + bounds.getHeight() * std::abs (direction.y);
        const auto at = [&] (float proportion) { return bounds.getCentre() + direction * (length * (proportion - 0.5f)); };

        // Where the edge at a third crosses the left and top sides
        const auto edge = at (0.33f);
        const auto leftY = edge.y + (edge.x - bounds.getX()) * direction.x / direction.y;
        const auto topX = edge.x + (edge.y - bounds.getY()) * direction.y / direction.x;

        juce::Path glare;
        glare.addTriangle (bounds.getTopLeft(), { std::min (topX, bounds.getRight()), bounds.getY() }, { bounds.getX(), std::min (leftY, bounds.getBottom()) });

        juce::Path screen;
        screen.addRoundedRectangle (bounds, cornerRadius);
        g.reduceClipRegion (screen);
        g.setGradientFill ({ juce::Colours::white.withAlpha (0.07f), at (0.0f), juce::Colours::white.withAlpha (0.02f), at (0.32f), false });
        g.fillPath (glare);
    }
};
