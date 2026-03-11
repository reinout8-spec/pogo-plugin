#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PitchCurveEditor.h"

class PogoAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  public juce::FileDragAndDropTarget,
                                  private juce::Timer
{
public:
    explicit PogoAudioProcessorEditor(PogoAudioProcessor&);
    ~PogoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;

private:
    PogoAudioProcessor& audioProcessor;

    PitchCurveEditor curveEditor;

    // Controls
    juce::Slider     tensionKnob;
    juce::Label      tensionLabel;
    juce::Label      fileLabel;
    juce::TextButton feedMeBtn  { "FEED ME"   };
    juce::TextButton chillBtn   { "CHILL \xE2\x98\xA0" };  // ☠
    juce::TextButton yeetBtn    { "\xF0\x9F\x93\xA4 YEET" }; // 📤
    juce::TextButton presetDrop { "DROP \xF0\x9F\x94\xA8" };   // 🔨
    juce::TextButton presetRise { "RISE \xF0\x9F\x9A\x80" };   // 🚀
    juce::TextButton presetSweep{ "SWEEP \xF0\x9F\x8C\x88" };  // 🌈
    juce::TextButton presetTape { "TAPE \xF0\x9F\x93\xBC" };   // 📼

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tensionAttach;

    void openFileChooser();
    void doExportDrag();
    void timerCallback() override {}

    // Paint helpers
    void paintHeader(juce::Graphics&);
    void paintCanvasFrame(juce::Graphics&);
    void paintKnobSection(juce::Graphics&);
    void styleBtn(juce::TextButton&, juce::Colour bg, juce::Colour text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PogoAudioProcessorEditor)
};
