#include "PluginEditor.h"
#include "ui/Faceplate.h"

// Renders the editor to PNGs next to the test binary (snapshots/editor@1x.png and @2x.png) for reviewing
// the look by eye. They aren't compared against stored images, since text renders differently between machines.
class SnapshotTests : public juce::UnitTest
{
public:
    SnapshotTests() : juce::UnitTest("Snapshot") {}

    void runTest() override
    {
        ClipAudioProcessor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());

        beginTest("the faceplate is cached and ignores the mouse");
        {
            const auto faceplates = findAll<Faceplate>(*editor);
            expectEquals((int) faceplates.size(), 1);
            expect(faceplates.front()->getCachedComponentImage() != nullptr);
            auto clicks = true, childClicks = true;
            faceplates.front()->getInterceptsMouseClicks(clicks, childClicks);
            expect(! clicks && ! childClicks);
        }

        beginTest("renders at 1x and 2x");
        {
            fillSpectrogram(*editor);
            const auto folder = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("snapshots");
            folder.createDirectory();

            for (auto scale : { 1, 2 })
            {
                const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, (float) scale);
                expectEquals(image.getWidth(), ClipAudioProcessorEditor::defaultWidth * scale);

                const auto file = folder.getChildFile("editor@" + juce::String(scale) + "x.png");
                file.deleteFile();
                juce::FileOutputStream stream(file);
                expect(juce::PNGImageFormat().writeImageToStream(image, stream), file.getFullPathName());
            }
        }
    }

private:
    // A clipped tone sweeping up, so the snapshot shows what the spectrogram looks like with audio
    static void fillSpectrogram(juce::Component& editor)
    {
        constexpr auto sampleRate = 44100.0;
        auto* spectrogram = findAll<Spectrogram>(editor).front();
        std::vector<ClipTap::Frame> frames((size_t) (spectrogram->getWidth() * Spectrogram::hopSize));

        auto phase = 0.0;
        for (size_t i = 0; i < frames.size(); ++i)
        {
            phase += juce::MathConstants<double>::twoPi * (60.0 + 4000.0 * (double) i / (double) frames.size()) / sampleRate;
            const auto input = 1.6f * (float) std::sin(phase);
            const auto output = (float) std::tanh(input);
            frames[i] = { input, output, output };
        }

        spectrogram->update(frames.data(), (int) frames.size(), sampleRate);
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

static SnapshotTests snapshotTests;
