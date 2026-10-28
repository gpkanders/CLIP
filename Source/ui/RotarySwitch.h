#pragma once
#include <JuceHeader.h>
#include "ClipLookAndFeel.h"

// Stepped rotary switch bound to a choice parameter, with its name above and the choices printed around it.
// Drag or scroll to step through them, or click a choice to jump to it. Double click resets it to the default.
class RotarySwitch : public juce::Component
{
public:
    static constexpr float sweep = juce::MathConstants<float>::pi * 2.0f / 3.0f;
    static constexpr float legendRadius = 38.0f;

    RotarySwitch (juce::AudioProcessorValueTreeState& apvts, const juce::ParameterID& id)
        : attachment (apvts, id.getParamID(), slider)
    {
        const auto& parameter = *apvts.getParameter (id.getParamID());
        const auto& choices = dynamic_cast<const juce::AudioParameterChoice&> (parameter).choices;

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (-sweep, sweep, true);
        slider.setDoubleClickReturnValue (true, parameter.convertFrom0to1 (parameter.getDefaultValue()));
        slider.setTitle (parameter.getName (32));
        slider.getProperties().set (ClipLookAndFeel::rotarySwitchProperty, true);
        slider.onValueChange = [this] { repaint(); };

        for (const auto& choice : choices)
            legends.push_back ({ choice.toUpperCase(), {} });

        label.setText (parameter.getName (32).toUpperCase(), juce::dontSendNotification);
        label.setFont (juce::FontOptions (9.5f, juce::Font::bold).withKerningFactor (0.12f));
        label.setColour (juce::Label::textColourId, Palette::silkscreen);
        label.setJustificationType (juce::Justification::centred);

        addAndMakeVisible (label);
        addAndMakeVisible (slider);
    }

    void paint (juce::Graphics& g) override
    {
        const auto selected = juce::roundToInt (slider.getValue());
        for (size_t i = 0; i < legends.size(); ++i)
        {
            const auto isSelected = (int) i == selected;
            const auto& [text, bounds] = legends[i];
            g.setFont (font (isSelected));

            if (isSelected)
            {
                g.setColour (Palette::accent.withAlpha (0.25f));
                for (const auto offset : { -0.7f, 0.7f })
                    g.drawText (text, bounds.translated (offset, 0.0f), juce::Justification::centred, false);
            }

            g.setColour (isSelected ? Palette::accent : Palette::textDim);
            g.drawText (text, bounds, juce::Justification::centred, false);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        label.setBounds (bounds.removeFromTop (14));
        bounds.removeFromTop (4);
        slider.setBounds (bounds.removeFromTop (88));

        // Evenly spaced around the slider's centre, the first at the start of the sweep
        const auto centre = slider.getBounds().toFloat().getCentre();
        for (size_t i = 0; i < legends.size(); ++i)
        {
            auto& [text, legendBounds] = legends[i];
            const auto angle = -sweep + 2.0f * sweep * (float) i / (float) (legends.size() - 1);
            const auto width = juce::GlyphArrangement::getStringWidth (font (true), text) + 4.0f;
            legendBounds = juce::Rectangle (width, 12.0f).withCentre (centre.getPointOnCircumference (legendRadius, angle));
        }
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        for (size_t i = 0; i < legends.size(); ++i)
            if (legends[i].second.expanded (2.0f).contains (event.position))
                slider.setValue ((double) i, juce::sendNotificationSync);
    }

private:
    // Only the cap takes the mouse, so the choices around it can be clicked
    struct Cap : juce::Slider
    {
        bool hitTest (int x, int y) override
        {
            return getLocalBounds().toFloat().getCentre().getDistanceFrom ({ (float) x, (float) y }) < 24.0f;
        }
    };

    static juce::Font font (bool bold) { return ClipLookAndFeel::font (8.0f, bold).withExtraKerningFactor (0.04f); }

    juce::Label label;
    Cap slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    std::vector<std::pair<juce::String, juce::Rectangle<float>>> legends;
};
