#include "dsp/Clipper.h"

class ClipperTests : public juce::UnitTest
{
public:
    ClipperTests() : juce::UnitTest("Clipper") {}

    void runTest() override
    {
        beginTest("output never exceeds the ceiling");
        forEachSetting([this](float thresholdDb, float softness) {
            const auto ceiling = juce::Decibels::decibelsToGain(thresholdDb);
            for (auto x = -4.0f; x <= 4.0f; x += 0.001f)
                expect(std::abs(TransferCurves::clip(x, thresholdDb, softness)) <= ceiling);
        });

        beginTest("unity gain below the knee");
        forEachSetting([this](float thresholdDb, float softness) {
            const auto kneeStart = juce::Decibels::decibelsToGain(thresholdDb) * (1.0f - softness / 200.0f);
            for (auto x = -kneeStart; x <= kneeStart; x += kneeStart / 100.0f + 1.0e-6f)
                expectEquals(TransferCurves::clip(x, thresholdDb, softness), x);
        });

        beginTest("curve is continuous and monotonic");
        forEachSetting([this](float thresholdDb, float softness) {
            auto previousX = -2.0f;
            auto previousY = TransferCurves::clip(previousX, thresholdDb, softness);
            for (auto i = 1; i <= 40000; ++i)
            {
                const auto x = -2.0f + (float) i * 1.0e-4f;
                const auto y = TransferCurves::clip(x, thresholdDb, softness);
                expect(y >= previousY);
                expect(y - previousY <= (x - previousX) * 1.001f);
                previousX = x;
                previousY = y;
            }
        });

        beginTest("softness 0 is a hard clip");
        for (auto thresholdDb : thresholds)
        {
            const auto ceiling = juce::Decibels::decibelsToGain(thresholdDb);
            for (auto x = -4.0f; x <= 4.0f; x += 0.01f)
                expectEquals(TransferCurves::clip(x, thresholdDb, 0.0f), juce::jlimit(-ceiling, ceiling, x));
        }

        beginTest("applies input gain, curve, then output gain");
        {
            juce::AudioBuffer<float> buffer(2, 4);
            buffer.clear();
            buffer.setSample(0, 0, 0.1f);
            buffer.setSample(1, 1, 0.9f);

            Clipper clipper;
            clipper.setParameters(6.0f, -6.0f, 0.0f, -6.0f);
            clipper.process(buffer);

            const auto gain = juce::Decibels::decibelsToGain(6.0f);
            expectWithinAbsoluteError(buffer.getSample(0, 0), 0.1f, 1.0e-6f);
            expectWithinAbsoluteError(buffer.getSample(1, 1), 1.0f / gain / gain, 1.0e-6f);
        }

        beginTest("input gain step ramps over the smoothing time");
        {
            constexpr auto sampleRate = 48000.0;
            const auto rampSamples = (int) (sampleRate * Clipper::smoothingSeconds);
            juce::AudioBuffer<float> buffer(1, rampSamples * 2);
            juce::FloatVectorOperations::fill(buffer.getWritePointer(0), 0.1f, buffer.getNumSamples());

            Clipper clipper;
            clipper.setParameters(0.0f, 0.0f, 0.0f, 0.0f);
            clipper.prepare(sampleRate);
            clipper.setParameters(12.0f, 0.0f, 0.0f, 0.0f);
            clipper.process(buffer);

            const auto target = 0.1f * juce::Decibels::decibelsToGain(12.0f);
            expect(buffer.getSample(0, 0) < 0.11f, "first sample jumped");
            expect(buffer.getSample(0, rampSamples / 2) > 0.11f && buffer.getSample(0, rampSamples / 2) < target * 0.9f);
            for (auto i = 1; i < rampSamples; ++i)
                expect(buffer.getSample(0, i) > buffer.getSample(0, i - 1));
            expectWithinAbsoluteError(buffer.getSample(0, rampSamples), target, 1.0e-5f);
            expectWithinAbsoluteError(buffer.getSample(0, buffer.getNumSamples() - 1), target, 1.0e-5f);
        }

        beginTest("process does not allocate");
        {
            juce::AudioBuffer<float> buffer(2, 512);
            for (auto ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (auto i = 0; i < buffer.getNumSamples(); ++i)
                    buffer.setSample(ch, i, std::sin(0.05f * (float) i) * 2.0f);

            Clipper clipper;
            clipper.setParameters(3.0f, -12.0f, 50.0f, -3.0f);

            const juce::UnitTestAllocationChecker checker(*this);
            clipper.process(buffer);
        }
    }

private:
    static constexpr float thresholds[] { -36.0f, -12.0f, -6.0f, 0.0f };
    static constexpr float softnesses[] { 0.0f, 10.0f, 50.0f, 100.0f };

    template <typename Fn>
    void forEachSetting(Fn&& fn)
    {
        for (auto thresholdDb : thresholds)
            for (auto softness : softnesses)
                fn(thresholdDb, softness);
    }
};

static ClipperTests clipperTests;
