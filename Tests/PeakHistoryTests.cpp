#include "PluginProcessor.h"
#include "ui/PeakHistory.h"

class PeakHistoryTests : public juce::UnitTest
{
public:
    PeakHistoryTests() : juce::UnitTest("PeakHistory") {}

    void runTest() override
    {
        constexpr auto sampleRate = 44100.0;
        constexpr auto width = 441;

        // Three seconds across 441 pixels
        constexpr auto samplesPerColumn = 300;

        beginTest("adds a column once enough samples arrive to fill one");
        {
            ClipAudioProcessor processor;
            PeakHistory history(processor.apvts);
            history.setSize(width, 200);
            const std::vector<ClipTap::Frame> frames(samplesPerColumn);

            expect(! history.update(nullptr, 0, sampleRate));
            expect(! history.update(frames.data(), samplesPerColumn - 1, sampleRate));
            expect(history.update(frames.data(), 1, sampleRate));
        }

        beginTest("lights the clip lamp when the output falls short of the input, and the detail lamp when detail adds to it");
        {
            // Peaks: input, output, and output without detail
            const std::tuple<float, float, float, bool, bool, const char*> cases[] {
                { 0.5f, 0.5f, 0.5f, false, false, "untouched" },
                { 2.0f, 1.0f, 1.0f, true, false, "clipped" },
                { 2.0f, 1.3f, 1.0f, true, true, "clipped with detail" },
            };

            for (const auto& [input, output, plain, clipped, detailed, name] : cases)
            {
                ClipAudioProcessor processor;
                PeakHistory history(processor.apvts);
                history.setSize(width, 200);
                history.update(tone(input, output, plain, samplesPerColumn * 10).data(), samplesPerColumn * 10, sampleRate);

                expect(history.isActive(PeakHistory::clip) == clipped, name);
                expect(history.isActive(PeakHistory::detail) == detailed, name);
            }
        }

        beginTest("lamps go out a moment after the clipping stops");
        {
            ClipAudioProcessor processor;
            PeakHistory history(processor.apvts);
            history.setSize(width, 200);
            history.update(tone(2.0f, 1.3f, 1.0f, samplesPerColumn * 10).data(), samplesPerColumn * 10, sampleRate);

            const auto quiet = tone(0.5f, 0.5f, 0.5f, (int) sampleRate);
            history.update(quiet.data(), samplesPerColumn, sampleRate);
            expect(history.isActive(PeakHistory::clip));

            history.update(quiet.data(), (int) (sampleRate * PeakHistory::lampSeconds) + samplesPerColumn, sampleRate);
            expect(! history.isActive(PeakHistory::clip));
            expect(! history.isActive(PeakHistory::detail));
        }
    }

private:
    // A 1 kHz tone at each of the peaks given
    static std::vector<ClipTap::Frame> tone(float input, float output, float plain, int numFrames)
    {
        std::vector<ClipTap::Frame> frames((size_t) numFrames);
        for (size_t i = 0; i < frames.size(); ++i)
        {
            const auto sine = std::sin(juce::MathConstants<float>::twoPi * 1000.0f * (float) i / 44100.0f);
            frames[i] = { input * sine, output * sine, plain * sine };
        }
        return frames;
    }
};

static PeakHistoryTests peakHistoryTests;
