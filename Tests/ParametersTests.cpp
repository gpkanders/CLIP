#include "Parameters.h"

class ParametersTests : public juce::UnitTest
{
public:
    ParametersTests() : juce::UnitTest("Parameters") {}

    void runTest() override
    {
        beginTest("layout exposes every parameter ID once");
        TestProcessor processor;
        juce::AudioProcessorValueTreeState apvts (processor, nullptr, "Parameters", Parameters::createLayout());

        for (const auto& id : { Parameters::inputGain, Parameters::threshold, Parameters::softness,
                                Parameters::outputGain, Parameters::mix, Parameters::bypass, Parameters::oversampling, Parameters::shape })
            expect(apvts.getParameter(id.getParamID()) != nullptr, id.getParamID());

        expectEquals(processor.getParameters().size(), 8);

        beginTest("defaults are transparent");
        expectWithinAbsoluteError(apvts.getRawParameterValue(Parameters::inputGain.getParamID())->load(), 0.0f, 1.0e-4f);
        expectWithinAbsoluteError(apvts.getRawParameterValue(Parameters::threshold.getParamID())->load(), 0.0f, 1.0e-4f);
        expectWithinAbsoluteError(apvts.getRawParameterValue(Parameters::outputGain.getParamID())->load(), 0.0f, 1.0e-4f);
        expectWithinAbsoluteError(apvts.getRawParameterValue(Parameters::mix.getParamID())->load(), 100.0f, 1.0e-4f);
    }

private:
    struct TestProcessor : juce::AudioProcessor
    {
        const juce::String getName() const override { return "Test"; }
        void prepareToPlay(double, int) override {}
        void releaseResources() override {}
        void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        double getTailLengthSeconds() const override { return 0.0; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        bool hasEditor() const override { return false; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return {}; }
        void changeProgramName(int, const juce::String&) override {}
        void getStateInformation(juce::MemoryBlock&) override {}
        void setStateInformation(const void*, int) override {}
    };
};

static ParametersTests parametersTests;
