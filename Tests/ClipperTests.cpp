#include "dsp/Clipper.h"

class ClipperTests : public juce::UnitTest
{
public:
    ClipperTests() : juce::UnitTest("Clipper") {}

    void runTest() override
    {
        beginTest("output never exceeds the ceiling");
        forEachSetting([this](Shape shape, float thresholdDb, float softness) {
            const auto ceiling = juce::Decibels::decibelsToGain(thresholdDb);
            for (auto x = -4.0f; x <= 4.0f; x += 0.001f)
                expect(std::abs(TransferCurves::clip(x, thresholdDb, softness, shape)) <= ceiling);
        });

        beginTest("unity gain below the knee");
        forEachSetting([this](Shape shape, float thresholdDb, float softness) {
            const auto kneeStart = juce::Decibels::decibelsToGain(thresholdDb) * (1.0f - softness / 100.0f);
            for (auto x = -kneeStart; x <= kneeStart; x += kneeStart / 100.0f + 1.0e-6f)
                expectEquals(TransferCurves::clip(x, thresholdDb, softness, shape), x);
        });

        beginTest("curve is continuous and monotonic");
        forEachSetting([this](Shape shape, float thresholdDb, float softness) {
            auto previousX = -2.0f;
            auto previousY = TransferCurves::clip(previousX, thresholdDb, softness, shape);
            for (auto i = 1; i <= 40000; ++i)
            {
                const auto x = -2.0f + (float) i * 1.0e-4f;
                const auto y = TransferCurves::clip(x, thresholdDb, softness, shape);
                expect(y >= previousY);
                expect(y - previousY <= (x - previousX) * 1.001f);
                previousX = x;
                previousY = y;
            }
        });

        beginTest("softness 0 is a hard clip, and hard ignores softness");
        forEachSetting([this](Shape shape, float thresholdDb, float softness) {
            if (shape != Shape::hard && softness > 0.0f)
                return;

            const auto ceiling = juce::Decibels::decibelsToGain(thresholdDb);
            for (auto x = -4.0f; x <= 4.0f; x += 0.01f)
                expectEquals(TransferCurves::clip(x, thresholdDb, softness, shape), juce::jlimit(-ceiling, ceiling, x));
        });

        beginTest("shapes follow their curves one headroom past the knee start");
        {
            // Threshold 0 dB and softness 50% put the knee start at 0.5 with 0.5 of headroom, so x = 1 is u = 1
            const std::pair<Shape, double> expected[] { { Shape::hard, 1.0 },
                                                        { Shape::soft, 0.5 + 0.5 * 0.75 },
                                                        { Shape::linear, 0.5 + 0.5 * 0.5 },
                                                        { Shape::tanh, 0.5 + 0.5 * std::tanh(1.0) },
                                                        { Shape::sine, 0.5 + 0.5 * std::sin(1.0) },
                                                        { Shape::cubic, 0.5 + 0.5 * 23.0 / 27.0 } };
            for (const auto& [shape, y] : expected)
                expectWithinAbsoluteError((double) TransferCurves::clip(1.0f, 0.0f, 50.0f, shape), y, 1.0e-6, juce::String((int) shape));
        }

        beginTest("each shape meets the ceiling where its knee ends");
        forEachSetting([this](Shape shape, float thresholdDb, float softness) {
            const TransferCurves::Knee knee(thresholdDb, softness, shape);
            if (std::isfinite(knee.end()))
                expectWithinAbsoluteError(TransferCurves::clip(knee.end() * 0.99999f, knee), knee.ceiling, knee.ceiling * 1.0e-4f);
            else
                expectWithinAbsoluteError(TransferCurves::clip(knee.start() + knee.headroom * 20.0f, knee), knee.ceiling, knee.ceiling * 1.0e-6f);
        });

        beginTest("antiderivative integrates the curve");
        forEachSetting([this](Shape shape, float thresholdDb, float softness) {
            const TransferCurves::Knee knee(thresholdDb, softness, shape);
            const auto h = 1.0e-4;
            for (auto x = -2.0; x <= 2.0; x += 0.0101)
            {
                const auto slope = (TransferCurves::antiderivative(x + h, knee) - TransferCurves::antiderivative(x - h, knee)) / (2.0 * h);
                expectWithinAbsoluteError(slope, (double) TransferCurves::clip((float) x, knee), 1.0e-4);
            }
        });

        beginTest("antialiased clip stays bounded and handles equal samples");
        forEachSetting([this](Shape shape, float thresholdDb, float softness) {
            const TransferCurves::Knee knee(thresholdDb, softness, shape);
            for (auto x = -4.0f; x <= 4.0f; x += 0.01f)
            {
                expectEquals(TransferCurves::clipAntialiased(x, x, knee), TransferCurves::clip(x, knee));
                const auto y = TransferCurves::clipAntialiased(x, x - 1.0e-6f, knee);
                expect(std::isfinite(y) && std::abs(y) <= knee.ceiling);
                expect(std::abs(TransferCurves::clipAntialiased(x, -x * 0.5f, knee)) <= knee.ceiling * 1.0001f);
            }
        });

        beginTest("switching shape crossfades instead of jumping");
        {
            constexpr auto sampleRate = 48000.0;
            const auto rampSamples = (int) (sampleRate * Clipper::smoothingSeconds);
            Clipper switched, hard, cubic;
            for (auto* clipper : { &switched, &hard, &cubic })
            {
                clipper->setParameters(0.0f, -6.0f, 100.0f, 0.0f);
                clipper->setShape(clipper == &cubic ? Shape::cubic : Shape::hard);
                clipper->prepare(sampleRate);
            }

            switched.setShape(Shape::cubic);
            juce::AudioBuffer<float> a(1, rampSamples * 2), b(1, rampSamples * 2), c(1, rampSamples * 2);
            for (auto i = 0; i < a.getNumSamples(); ++i)
                for (auto* buffer : { &a, &b, &c })
                    buffer->setSample(0, i, 0.45f * std::cos(0.01f * (float) i));

            switched.process(a);
            hard.process(b);
            cubic.process(c);

            // Starts on a peak inside the knee, where hard (0.45) and cubic (0.431) differ
            expect(std::abs(b.getSample(0, 0) - c.getSample(0, 0)) > 0.01f);
            expectWithinAbsoluteError(a.getSample(0, 0), b.getSample(0, 0), 1.0e-3f);
            for (auto i = rampSamples; i < a.getNumSamples(); ++i)
                expectWithinAbsoluteError(a.getSample(0, i), c.getSample(0, i), 1.0e-6f);
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

        beginTest("detail leaves signals below the knee untouched");
        for (auto antialias : { false, true })
        {
            juce::AudioBuffer<float> buffer(1, 4800);
            for (auto i = 0; i < buffer.getNumSamples(); ++i)
                buffer.setSample(0, i, 0.2f * std::sin(0.03f * (float) i));
            juce::AudioBuffer<float> reference(buffer);

            Clipper plain, detailed;
            for (auto* clipper : { &plain, &detailed })
            {
                clipper->setParameters(0.0f, -6.0f, 50.0f, 0.0f);
                clipper->setDetail(clipper == &detailed ? 100.0f : 0.0f);
                clipper->setAntialiasing(antialias);
                clipper->prepare(48000.0);
            }

            plain.process(reference);
            detailed.process(buffer);
            for (auto i = 0; i < buffer.getNumSamples(); ++i)
                expectWithinAbsoluteError(buffer.getSample(0, i), reference.getSample(0, i), 1.0e-6f);
        }

        beginTest("detail keeps ripple on clipped peaks without passing the ceiling");
        {
            constexpr auto sampleRate = 48000.0;
            const auto ceiling = juce::Decibels::decibelsToGain(-12.0f);
            const auto ripple = [] (int i) { return 0.05f * std::sin(0.9f * (float) i); };

            juce::AudioBuffer<float> flattened(1, 48000), detailed(1, 48000);
            for (auto i = 0; i < flattened.getNumSamples(); ++i)
            {
                const auto x = 0.5f * std::sin(0.002f * (float) i) + ripple(i);
                flattened.setSample(0, i, x);
                detailed.setSample(0, i, x);
            }

            Clipper hard, recovering;
            for (auto* clipper : { &hard, &recovering })
            {
                clipper->setParameters(0.0f, -12.0f, 0.0f, 0.0f);
                clipper->setShape(Shape::hard);
                clipper->setDetail(clipper == &recovering ? 100.0f : 0.0f);
                clipper->prepare(sampleRate);
            }

            hard.process(flattened);
            recovering.process(detailed);

            // On the positive tops, where the slow sine alone is well past the ceiling
            auto correlation = 0.0, rippleEnergy = 0.0, flatEnergy = 0.0;
            for (auto i = 0; i < detailed.getNumSamples(); ++i)
            {
                const auto y = detailed.getSample(0, i);
                expect(std::abs(y) <= ceiling * 1.0001f);

                if (std::sin(0.002f * (float) i) < 0.7f)
                    continue;

                correlation += (y - ceiling) * ripple(i);
                rippleEnergy += juce::square(ripple(i));
                flatEnergy += juce::square(flattened.getSample(0, i) - ceiling);
            }

            expect(flatEnergy < 1.0e-9, "hard clip should flatten the tops");
            expect(correlation / rippleEnergy > 0.1, "ripple should come back on the tops: " + juce::String(correlation / rippleEnergy));
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
            clipper.setDetail(50.0f);

            const juce::UnitTestAllocationChecker checker(*this);
            clipper.process(buffer);
        }
    }

private:
    using Shape = TransferCurves::Shape;
    static constexpr Shape shapes[] { Shape::hard, Shape::soft, Shape::linear, Shape::tanh, Shape::sine, Shape::cubic };
    static constexpr float thresholds[] { -36.0f, -12.0f, -6.0f, 0.0f };
    static constexpr float softnesses[] { 0.0f, 10.0f, 50.0f, 100.0f };

    template <typename Fn>
    void forEachSetting(Fn&& fn)
    {
        for (auto shape : shapes)
            for (auto thresholdDb : thresholds)
                for (auto softness : softnesses)
                    fn(shape, thresholdDb, softness);
    }
};

static ClipperTests clipperTests;
