#include "PluginEditor.h"
#include "Parameters.h"

class EditorTests : public juce::UnitTest
{
public:
    EditorTests() : juce::UnitTest("Editor") {}

    void runTest() override
    {
        beginTest("opens at the default size with a fixed aspect ratio");
        {
            ClipAudioProcessor processor;
            const auto editor = createEditor(processor);
            expectEquals(editor->getWidth(), ClipAudioProcessorEditor::defaultWidth);
            expectEquals(editor->getHeight(), ClipAudioProcessorEditor::defaultHeight);
            expect(editor->isResizable());
            expectWithinAbsoluteError(editor->getConstrainer()->getFixedAspectRatio(), 1.6, 1.0e-9);
        }

        beginTest("the size is saved with the plugin state");
        {
            juce::MemoryBlock state;
            {
                ClipAudioProcessor processor;
                createEditor(processor)->setSize(1000, 625);
                processor.getStateInformation(state);
            }

            ClipAudioProcessor processor;
            processor.setStateInformation(state.getData(), (int) state.getSize());
            const auto editor = createEditor(processor);
            expectEquals(editor->getWidth(), 1000);
            expectEquals(editor->getHeight(), 625);
        }

        beginTest("an out of range saved size is clamped");
        {
            ClipAudioProcessor processor;
            processor.editorWidth = 10000;
            expectEquals(createEditor(processor)->getWidth(), ClipAudioProcessorEditor::defaultWidth * 2);
        }

        beginTest("the panel stays fully visible when a host ignores the aspect ratio");
        {
            ClipAudioProcessor processor;
            const auto editor = createEditor(processor);
            auto* panel = editor->getChildComponent(0);

            // 1251x750 is what macOS produced for a 1200x750 request on a window with a title bar
            for (auto [width, height] : { std::pair { 1251, 750 }, std::pair { 800, 600 }, std::pair { 1000, 625 } })
            {
                editor->setSize(width, height);
                const auto shown = panel->getBoundsInParent();
                expect(editor->getLocalBounds().contains(shown), shown.toString() + " in " + juce::String(width) + "x" + juce::String(height));
                expect(shown.getWidth() >= width - 2 || shown.getHeight() >= height - 2, "fills one side");
            }
        }

        beginTest("knobs show units and reset to their defaults on double click");
        {
            ClipAudioProcessor processor;
            const auto editor = createEditor(processor);
            const auto sliders = findAll<juce::Slider>(*editor);
            expectEquals((int) sliders.size(), 5);

            for (auto* slider : sliders)
            {
                const auto text = slider->getTextFromValue(slider->getValue());
                expect(text.endsWith(" dB") || text.endsWith(" %"), text);
                expect(slider->isDoubleClickReturnEnabled());
            }
        }

        beginTest("controls are bound to their parameters");
        {
            ClipAudioProcessor processor;
            const auto editor = createEditor(processor);

            auto* box = findAll<juce::ComboBox>(*editor).front();
            box->setSelectedItemIndex(4, juce::sendNotificationSync);
            expectEquals((int) processor.apvts.getRawParameterValue(Parameters::oversampling.getParamID())->load(), 4);

            auto* button = findAll<juce::ToggleButton>(*editor).front();
            button->setToggleState(true, juce::sendNotificationSync);
            expect(processor.apvts.getRawParameterValue(Parameters::bypass.getParamID())->load() > 0.5f);

            auto* threshold = findAll<juce::Slider>(*editor)[1];
            threshold->setValue(-12.0, juce::sendNotificationSync);
            expectWithinAbsoluteError(processor.apvts.getRawParameterValue(Parameters::threshold.getParamID())->load(), -12.0f, 1.0e-3f);
        }
    }

private:
    static std::unique_ptr<juce::AudioProcessorEditor> createEditor(ClipAudioProcessor& processor)
    {
        return std::unique_ptr<juce::AudioProcessorEditor>(processor.createEditor());
    }

    template <typename ComponentType>
    static std::vector<ComponentType*> findAll(juce::Component& parent)
    {
        std::vector<ComponentType*> found;
        for (auto* child : parent.getChildren())
        {
            if (auto* match = dynamic_cast<ComponentType*>(child))
                found.push_back(match);

            const auto nested = findAll<ComponentType>(*child);
            found.insert(found.end(), nested.begin(), nested.end());
        }
        return found;
    }
};

static EditorTests editorTests;
