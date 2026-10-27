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

        for (const auto& [id, value] : { std::pair { Parameters::threshold, -12.0f }, std::pair { Parameters::softness, 25.0f },
                                          std::pair { Parameters::shape, 3.0f } })
        {
            setParameter(processor, id, value);
            expect(display.refresh(), id.getParamID());
            expect(! display.refresh(), id.getParamID());
        }

        for (const auto& id : { Parameters::inputGain, Parameters::outputGain, Parameters::mix, Parameters::detail })
        {
            setParameter(processor, id, 6.0f);
            expect(! display.refresh(), id.getParamID());
        }

        beginTest("every shape draws a visibly different curve at the default knee");
        {
            std::vector<juce::Image> snapshots;
            for (auto shape = 0; shape < 6; ++shape)
            {
                ClipAudioProcessor shaped;
                setParameter(shaped, Parameters::shape, (float) shape);
                TransferCurveDisplay shown(shaped.apvts);
                shown.setSize(768, 298);
                snapshots.push_back(shown.createComponentSnapshot(shown.getLocalBounds()));
            }

            for (size_t a = 0; a < snapshots.size(); ++a)
                for (auto b = a + 1; b < snapshots.size(); ++b)
                {
                    auto differing = 0;
                    for (auto y = 0; y < snapshots[a].getHeight(); ++y)
                        for (auto x = 0; x < snapshots[a].getWidth(); ++x)
                            differing += snapshots[a].getPixelAt(x, y) != snapshots[b].getPixelAt(x, y) ? 1 : 0;

                    expect(differing > 200, "shapes " + juce::String(a) + " and " + juce::String(b) + ": " + juce::String(differing) + " pixels differ");
                }
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
