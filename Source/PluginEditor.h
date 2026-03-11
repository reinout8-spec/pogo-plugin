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

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    PogoAudioProcessor& audioProcessor;

    PitchCurveEditor curveEditor;

    juce::Label      fileLabel;
    juce::TextButton loadButton   { "Load Sample" };
    juce::TextButton resetButton  { "Flat" };
    juce::TextButton dragButton   { "⬇ Drag to DAW" };

    // Preset buttons
    juce::TextButton presetDrop   { "Drop"   };
    juce::TextButton presetRise   { "Rise"   };
    juce::TextButton presetSweep  { "Sweep"  };
    juce::TextButton presetTape   { "Tape ↓" };

    void openFileChooser();
    void doExportDrag();
    void stylePresetButton(juce::TextButton&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PogoAudioProcessorEditor)
};
