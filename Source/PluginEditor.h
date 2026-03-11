#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PitchCurveEditor.h"

class PogoAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  public juce::FileDragAndDropTarget
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

    juce::Slider     tensionKnob;
    juce::Label      tensionLabel;
    juce::Label      tensionValLabel;
    juce::Label      fileLabel;

    juce::TextButton feedMeBtn;
    juce::TextButton chillBtn;
    juce::TextButton yeetBtn;
    juce::TextButton presetDrop;
    juce::TextButton presetRise;
    juce::TextButton presetSweep;
    juce::TextButton presetTape;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tensionAttach;

    void openFileChooser();
    void doExportDrag();
    void styleBtn(juce::TextButton&, juce::Colour bg, juce::Colour text, int fontSize = 14);
    void paintRivetCorners(juce::Graphics& g, juce::Rectangle<float> r);
    void paintPanel(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour border);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PogoAudioProcessorEditor)
};
