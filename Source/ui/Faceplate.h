#pragma once
#include <JuceHeader.h>
#include "ClipLookAndFeel.h"

// The panel behind the controls: the anodised surface, screws, logo, the display's bezel and the section headers.
// Nothing on it moves, so it's drawn once into a cached image and repainting a control never redraws it.
class Faceplate : public juce::Component
{
public:
    struct Section
    {
        juce::String name;
        juce::Rectangle<int> bounds;
    };

    Faceplate()
    {
        setBufferedToImage (true);
        setInterceptsMouseClicks (false, false);
    }

    void setLayout (juce::Rectangle<int> newBezel, std::vector<Section> newSections)
    {
        bezel = newBezel;
        sections = std::move (newSections);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        paintSurface (g);
        paintScrews (g);
        paintLogo (g);
        paintBezel (g);
        paintSections (g);
    }

private:
    void paintSurface (juce::Graphics& g) const
    {
        const auto bounds = getLocalBounds().toFloat();
        auto gradient = juce::ColourGradient::vertical (Palette::panelTop, bounds.getY(), Palette::panelBottom, bounds.getBottom());
        gradient.addColour (0.45, Palette::panelMiddle);
        g.setGradientFill (gradient);
        g.fillAll();

        // Faint vertical grain
        g.setColour (juce::Colours::white.withAlpha (0.012f));
        for (auto x = 0; x < getWidth(); x += 3)
            g.fillRect (x, 0, 1, getHeight());

        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRect (0, 0, getWidth(), 1);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.fillRect (0, getHeight() - 1, getWidth(), 1);
    }

    void paintScrews (juce::Graphics& g) const
    {
        const auto inset = 16.0f, radius = 6.0f;
        const std::pair<juce::Point<float>, float> screws[] {
            { { inset, inset }, 30.0f },
            { { (float) getWidth() - inset, inset }, -50.0f },
            { { inset, (float) getHeight() - inset }, 75.0f },
            { { (float) getWidth() - inset, (float) getHeight() - inset }, 10.0f },
        };

        for (const auto& [centre, slotAngle] : screws)
        {
            g.setColour (juce::Colours::black.withAlpha (0.8f));
            g.fillEllipse (juce::Rectangle (radius * 2.0f, radius * 2.0f).withCentre (centre.translated (0.0f, 1.0f)));

            g.setGradientFill ({ Palette::screwLight, centre.translated (-1.2f, -1.8f), Palette::screwDark, centre.translated (radius, radius), true });
            g.fillEllipse (juce::Rectangle (radius * 2.0f, radius * 2.0f).withCentre (centre));

            juce::Path slot;
            slot.addRoundedRectangle (-4.0f, -1.0f, 8.0f, 2.0f, 1.0f);
            g.setColour (Palette::engraving);
            g.fillPath (slot, juce::AffineTransform::rotation (juce::degreesToRadians (slotAngle)).translated (centre));
        }
    }

    void paintLogo (juce::Graphics& g) const
    {
        const auto logo = ClipLookAndFeel::font (22.0f, true).withExtraKerningFactor (0.26f);
        const auto row = juce::Rectangle (32.0f, 9.0f, 300.0f, 30.0f);
        g.setColour (Palette::text);
        g.setFont (logo);
        g.drawText ("CLIP", row, juce::Justification::centredLeft);

        const auto logoWidth = juce::GlyphArrangement::getStringWidth (logo, "CLIP");
        g.setColour (Palette::textDim);
        g.setFont (ClipLookAndFeel::font (9.5f).withExtraKerningFactor (0.22f));
        g.drawText ("PEAK CLIPPER", row.withTrimmedLeft (logoWidth + 12.0f).translated (0.0f, 3.0f), juce::Justification::centredLeft);
    }

    // A ring around the display, lit along its bottom edge
    void paintBezel (juce::Graphics& g) const
    {
        const auto bounds = bezel.toFloat();
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (bounds.translated (0.0f, 1.0f), 8.0f);
        g.setColour (Palette::bezel);
        g.fillRoundedRectangle (bounds, 8.0f);
    }

    // A name between two rules above each section, and an engraved line between sections
    void paintSections (juce::Graphics& g) const
    {
        const auto font = ClipLookAndFeel::font (9.0f, true).withExtraKerningFactor (0.22f);
        g.setFont (font);

        for (const auto& section : sections)
        {
            const auto header = section.bounds.toFloat().removeFromTop (14.0f);
            const auto nameWidth = juce::GlyphArrangement::getStringWidth (font, section.name) + 2.0f;
            const auto name = header.withSizeKeepingCentre (nameWidth, header.getHeight());

            // The rules give way to a name too wide to leave room for them
            const auto ruleWidth = name.getX() - header.getX() - 18.0f;
            g.setColour (Palette::rule);
            if (ruleWidth > 4.0f)
            {
                g.fillRect (header.getX() + 10.0f, header.getCentreY(), ruleWidth, 1.0f);
                g.fillRect (name.getRight() + 8.0f, header.getCentreY(), ruleWidth, 1.0f);
            }

            g.setColour (Palette::textDim);
            g.drawText (section.name, name, juce::Justification::centred);

            if (&section != &sections.front())
            {
                g.setColour (Palette::engraving);
                g.fillRect (section.bounds.getX(), section.bounds.getY(), 1, section.bounds.getHeight());
                g.setColour (juce::Colours::white.withAlpha (0.05f));
                g.fillRect (section.bounds.getX() + 1, section.bounds.getY(), 1, section.bounds.getHeight());
            }
        }
    }

    juce::Rectangle<int> bezel;
    std::vector<Section> sections;
};
