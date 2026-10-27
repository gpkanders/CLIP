#include "PluginProcessor.h"
#include "ui/WaveformDisplay.h"

class WaveformDisplayTests : public juce::UnitTest
{
public:
    WaveformDisplayTests() : juce::UnitTest("WaveformDisplay") {}

    void runTest() override
    {
        beginTest("redraws only for new peaks or a moved gain or threshold");
        ClipAudioProcessor processor;
        WaveformDisplay display(processor.apvts);
        display.setSize(470, 280);

        expect(! display.update(nullptr, 0));

        const PeakFifo::Frame frame { { -0.5f, 0.5f }, { -0.4f, 0.4f } };
        expect(display.update(&frame, 1));
        expect(! display.update(nullptr, 0));

        for (const auto& id : { Parameters::inputGain, Parameters::threshold, Parameters::outputGain })
        {
            setParameter(processor, id, -6.0f);
            expect(display.update(nullptr, 0), id.getParamID());
            expect(! display.update(nullptr, 0), id.getParamID());
        }

        for (const auto& id : { Parameters::softness, Parameters::mix })
        {
            setParameter(processor, id, 50.0f);
            expect(! display.update(nullptr, 0), id.getParamID());
        }
    }

private:
    static void setParameter(ClipAudioProcessor& processor, const juce::ParameterID& id, float value)
    {
        auto* parameter = processor.apvts.getParameter(id.getParamID());
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }
};

static WaveformDisplayTests waveformDisplayTests;
