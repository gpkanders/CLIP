#include "ui/Spectrogram.h"

class SpectrogramTests : public juce::UnitTest
{
public:
    SpectrogramTests() : juce::UnitTest("Spectrogram") {}

    void runTest() override
    {
        beginTest("adds a column for every hop");
        {
            Spectrogram spectrogram;
            spectrogram.setSize(400, 200);
            const std::vector<ClipTap::Frame> frames(Spectrogram::hopSize);

            expect(! spectrogram.update(nullptr, 0, 44100.0));
            expect(! spectrogram.update(frames.data(), Spectrogram::hopSize - 1, 44100.0));
            expect(spectrogram.update(frames.data(), 1, 44100.0));
        }

        beginTest("tints by what the clip removes and what detail keeps");
        {
            // In multiples of a tone: input, output, and output without detail
            const std::tuple<float, float, float, juce::Colour, const char*> cases[] {
                { 1.0f, 1.0f, 1.0f, Palette::accent, "untouched" },
                { 1.0f, 1.0f, 0.0f, Palette::detail, "kept by detail" },
                { 4.0f, 1.0f, 1.0f, Palette::clip, "removed by the clip" },
            };

            for (const auto& [input, output, plain, expected, name] : cases)
            {
                Spectrogram spectrogram;
                spectrogram.setSize(400, 200);

                std::vector<ClipTap::Frame> frames(Spectrogram::fftSize * 2);
                for (size_t i = 0; i < frames.size(); ++i)
                {
                    const auto tone = 0.2f * std::sin(juce::MathConstants<float>::twoPi * 1000.0f * (float) i / 44100.0f);
                    frames[i] = { input * tone, output * tone, plain * tone };
                }
                spectrogram.update(frames.data(), (int) frames.size(), 44100.0);

                // Newest column, on the tone's row
                const auto colour = spectrogram.createComponentSnapshot(spectrogram.getLocalBounds())
                                        .getPixelAt(spectrogram.getWidth() - 1, juce::roundToInt(spectrogram.toY(1000.0f)));
                expect(nearest(colour) == expected, juce::String(name) + ": " + colour.toDisplayString(false));
            }
        }
    }

private:
    // By hue, since blending onto the background dims the colour but keeps its hue
    static juce::Colour nearest(juce::Colour colour)
    {
        const auto distance = [colour](juce::Colour other) {
            const auto difference = std::abs(colour.getHue() - other.getHue());
            return std::min(difference, 1.0f - difference);
        };

        auto best = Palette::accent;
        for (auto candidate : { Palette::detail, Palette::clip })
            if (distance(candidate) < distance(best))
                best = candidate;
        return best;
    }
};

static SpectrogramTests spectrogramTests;
