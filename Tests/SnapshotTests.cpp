#include "PluginEditor.h"
#include "ui/DisplayGlass.h"
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

        beginTest("the faceplate and display glass are cached and ignore the mouse");
        {
            const auto faceplates = findAll<Faceplate>(*editor);
            const auto glasses = findAll<DisplayGlass>(*editor);
            expectEquals((int) faceplates.size(), 1);
            expectEquals((int) glasses.size(), 2);

            for (auto* layer : { (juce::Component*) faceplates.front(), (juce::Component*) glasses.front(), (juce::Component*) glasses.back() })
            {
                expect(layer->getCachedComponentImage() != nullptr);
                auto clicks = true, childClicks = true;
                layer->getInterceptsMouseClicks(clicks, childClicks);
                expect(! clicks && ! childClicks);
            }
        }

        beginTest("renders at 1x and 2x");
        {
            fillHistory(*editor);
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
    // Bursts of a low tone with a quiet high one riding on it, clipped as set and without detail like ClipTap does,
    // so the snapshot shows what the history looks like with audio
    static void fillHistory(juce::Component& editor)
    {
        constexpr auto sampleRate = 44100.0;
        auto* history = findAll<PeakHistory>(editor).front();
        const auto numSamples = (int) (sampleRate * PeakHistory::secondsShown);

        juce::AudioBuffer<float> output(1, numSamples), plain(1, numSamples);
        std::vector<ClipTap::Frame> frames((size_t) numSamples);
        for (auto i = 0; i < numSamples; ++i)
        {
            const auto seconds = (double) i / sampleRate;
            const auto burst = std::fmod(seconds, 0.5) < 0.12 ? 2.4f : 0.4f;
            const auto input = burst * (float) std::sin(juce::MathConstants<double>::twoPi * 100.0 * seconds)
                               + 0.15f * (float) std::sin(juce::MathConstants<double>::twoPi * 4000.0 * seconds);
            frames[(size_t) i].input = input;
            output.setSample(0, i, input);
            plain.setSample(0, i, input);
        }

        Clipper detailed, flat;
        for (auto* clipper : { &detailed, &flat })
        {
            clipper->setParameters(0.0f, 0.0f, 50.0f, 0.0f);
            clipper->setDetail(clipper == &detailed ? 100.0f : 0.0f);
            clipper->prepare(sampleRate);
        }
        detailed.process(juce::dsp::AudioBlock<float>(output));
        flat.process(juce::dsp::AudioBlock<float>(plain));

        for (auto i = 0; i < numSamples; ++i)
        {
            frames[(size_t) i].output = output.getSample(0, i);
            frames[(size_t) i].plain = plain.getSample(0, i);
        }

        history->update(frames.data(), (int) frames.size(), sampleRate);
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
