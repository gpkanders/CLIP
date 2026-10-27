#include "PluginProcessor.h"
#include "ui/TransferCurveDisplay.h"

class TransferCurveDisplayTests : public juce::UnitTest
{
public:
    TransferCurveDisplayTests() : juce::UnitTest("TransferCurveDisplay") {}

    void runTest() override
    {
        beginTest("redraws only when threshold, softness or shape change");
        ClipAudioProcessor processor;
        TransferCurveDisplay display(processor.apvts);
        display.setSize(300, 300);

        expect(! display.refresh());

        for (const auto& [id, value] : { std::pair { Parameters::threshold, -12.0f }, std::pair { Parameters::softness, 50.0f },
                                          std::pair { Parameters::shape, 3.0f } })
        {
            setParameter(processor, id, value);
            expect(display.refresh(), id.getParamID());
            expect(! display.refresh(), id.getParamID());
        }

        for (const auto& id : { Parameters::inputGain, Parameters::outputGain, Parameters::mix })
        {
            setParameter(processor, id, 6.0f);
            expect(! display.refresh(), id.getParamID());
        }
    }

private:
    static void setParameter(ClipAudioProcessor& processor, const juce::ParameterID& id, float value)
    {
        auto* parameter = processor.apvts.getParameter(id.getParamID());
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }
};

static TransferCurveDisplayTests transferCurveDisplayTests;
