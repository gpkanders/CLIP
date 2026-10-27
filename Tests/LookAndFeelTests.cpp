#include "PluginEditor.h"

class LookAndFeelTests : public juce::UnitTest
{
public:
    LookAndFeelTests() : juce::UnitTest("LookAndFeel") {}

    void runTest() override
    {
        beginTest("the embedded typefaces load");
        expectEquals(ClipLookAndFeel::font(15.0f).getTypefaceName(), juce::String("Barlow"));
        expectEquals(ClipLookAndFeel::font(15.0f, true).getTypefaceStyle(), juce::String("SemiBold"));
        expectEquals(ClipLookAndFeel::font(15.0f).getTypefaceStyle(), juce::String("Regular"));

        beginTest("the editor and all its labels use the embedded font");
        ClipAudioProcessor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        expect(dynamic_cast<ClipLookAndFeel*>(&editor->getLookAndFeel()) != nullptr);

        auto numLabels = 0;
        forEachDescendant(*editor, [&](juce::Component& component) {
            if (auto* label = dynamic_cast<juce::Label*>(&component))
            {
                ++numLabels;
                expectEquals(label->getLookAndFeel().getLabelFont(*label).getTypefaceName(), juce::String("Barlow"), label->getText());
            }
        });

        // A name and a readout per knob, the oversampling label and the combo box's text
        expectEquals(numLabels, 5 * 2 + 2);
    }

private:
    template <typename Fn>
    static void forEachDescendant(juce::Component& parent, Fn&& fn)
    {
        for (auto* child : parent.getChildren())
        {
            fn(*child);
            forEachDescendant(*child, fn);
        }
    }
};

static LookAndFeelTests lookAndFeelTests;
