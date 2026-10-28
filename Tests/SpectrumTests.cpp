#include "ui/Spectrum.h"

class SpectrumTests : public juce::UnitTest
{
public:
    SpectrumTests() : juce::UnitTest("Spectrum") {}

    void runTest() override
    {
        beginTest("analyses every hop");
        {
            Spectrum spectrum;
            spectrum.setSize(400, 200);
            const std::vector<ClipTap::Frame> frames(Spectrum::hopSize);

            expect(! spectrum.update(nullptr, 0, 44100.0));
            expect(! spectrum.update(frames.data(), Spectrum::hopSize - 1, 44100.0));
            expect(spectrum.update(frames.data(), 1, 44100.0));
        }

        beginTest("shows what the clip removes and what detail keeps, and lights their lamps");
        {
            // In multiples of a tone: input, output, and output without detail
            const std::tuple<float, float, float, bool, bool, const char*> cases[] {
                { 1.0f, 1.0f, 1.0f, false, false, "untouched" },
                { 4.0f, 1.0f, 1.0f, true, false, "removed by the clip" },
                { 4.0f, 1.0f, 0.5f, true, true, "kept by detail" },
            };

            for (const auto& [input, output, plain, removed, kept, name] : cases)
            {
                Spectrum spectrum;
                spectrum.setSize(400, 200);

                std::vector<ClipTap::Frame> frames(Spectrum::fftSize * 2);
                for (size_t i = 0; i < frames.size(); ++i)
                {
                    const auto tone = 0.2f * std::sin(juce::MathConstants<float>::twoPi * 1000.0f * (float) i / 44100.0f);
                    frames[i] = { input * tone, output * tone, plain * tone };
                }
                spectrum.update(frames.data(), (int) frames.size(), 44100.0);

                expectWithinAbsoluteError(spectrum.levelDb(Spectrum::output, 1000.0f), juce::Decibels::gainToDecibels(0.2f), 1.0f, name);
                expect(spectrum.isActive(Spectrum::removed) == removed, name);
                expect(spectrum.isActive(Spectrum::kept) == kept, name);
                expect((spectrum.levelDb(Spectrum::removed, 1000.0f) > Spectrum::activeDb) == removed, name);
                expect((spectrum.levelDb(Spectrum::kept, 1000.0f) > Spectrum::activeDb) == kept, name);
            }
        }

        beginTest("lets levels fall slowly after the signal stops");
        {
            Spectrum spectrum;
            spectrum.setSize(400, 200);

            std::vector<ClipTap::Frame> frames(Spectrum::fftSize * 2);
            for (size_t i = 0; i < frames.size(); ++i)
            {
                const auto tone = std::sin(juce::MathConstants<float>::twoPi * 1000.0f * (float) i / 44100.0f);
                frames[i] = { tone, 0.5f * tone, 0.5f * tone };
            }
            spectrum.update(frames.data(), (int) frames.size(), 44100.0);

            const std::vector<ClipTap::Frame> silence(Spectrum::fftSize);
            spectrum.update(silence.data(), Spectrum::hopSize, 44100.0);
            expect(spectrum.isActive(Spectrum::removed));

            for (auto i = 0; i < 6; ++i)
                spectrum.update(silence.data(), (int) silence.size(), 44100.0);
            expect(! spectrum.isActive(Spectrum::removed));
        }
    }
};

static SpectrumTests spectrumTests;
