#include "PluginEditor.h"
#include "Parameters.h"
#include "ui/RotarySwitch.h"

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
            expectEquals((int) sliders.size(), 8);

            // The last two are the rotary switches, which have no readout
            for (auto* slider : std::vector(sliders.begin(), sliders.begin() + 6))
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

            const auto sliders = findAll<juce::Slider>(*editor);
            auto* shape = sliders[6];
            auto* oversampling = sliders[7];
            shape->setValue(3, juce::sendNotificationSync);
            expectEquals((int) processor.apvts.getRawParameterValue(Parameters::shape.getParamID())->load(), 3);
            oversampling->setValue(4, juce::sendNotificationSync);
            expectEquals((int) processor.apvts.getRawParameterValue(Parameters::oversampling.getParamID())->load(), 4);

            auto* button = findAll<juce::ToggleButton>(*editor).front();
            button->setToggleState(true, juce::sendNotificationSync);
            expect(processor.apvts.getRawParameterValue(Parameters::bypass.getParamID())->load() > 0.5f);

            auto* threshold = sliders[1];
            threshold->setValue(-12.0, juce::sendNotificationSync);
            expectWithinAbsoluteError(processor.apvts.getRawParameterValue(Parameters::threshold.getParamID())->load(), -12.0f, 1.0e-3f);
        }

        beginTest("rotary switches step through their choices and show them by name");
        {
            ClipAudioProcessor processor;
            const auto editor = createEditor(processor);
            auto* shape = findAll<juce::Slider>(*editor)[6];

            expectEquals(shape->getMinimum(), 0.0);
            expectEquals(shape->getMaximum(), 5.0);
            expectEquals(shape->getInterval(), 1.0);
            shape->setValue(2.4, juce::sendNotificationSync);
            expectEquals(shape->getValue(), 2.0);
            expectEquals(shape->getTextFromValue(1.0), juce::String("Soft"));
        }

        beginTest("clicking a printed choice selects it");
        {
            ClipAudioProcessor processor;
            const auto editor = createEditor(processor);
            auto* shape = findAll<juce::Slider>(*editor)[6];
            auto& rotarySwitch = *shape->getParentComponent();

            // Cubic, the last choice, sits at the end of the sweep
            const auto centre = shape->getBounds().toFloat().getCentre();
            const auto cubic = centre.getPointOnCircumference(RotarySwitch::legendRadius, RotarySwitch::sweep);
            rotarySwitch.mouseDown(juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), cubic, juce::ModifierKeys(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                                    &rotarySwitch, &rotarySwitch, juce::Time::getCurrentTime(), cubic, juce::Time::getCurrentTime(), 1, false));
            expectEquals((int) processor.apvts.getRawParameterValue(Parameters::shape.getParamID())->load(), 5);

            // The cap is the only part of the slider that takes the mouse, so the choices stay clickable
            expect(! shape->hitTest(juce::roundToInt(cubic.x) - shape->getX(), juce::roundToInt(cubic.y) - shape->getY()));
            expect(shape->hitTest(shape->getWidth() / 2, shape->getHeight() / 2));
        }

        beginTest("every control has an accessible title");
        {
            ClipAudioProcessor processor;
            const auto editor = createEditor(processor);
            for (auto* slider : findAll<juce::Slider>(*editor))
                expect(slider->getTitle().isNotEmpty());
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
