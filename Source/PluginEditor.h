#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PogoAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  public juce::FileDragAndDropTarget
{
public:
    explicit PogoAudioProcessorEditor(PogoAudioProcessor&);
    ~PogoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // File drag & drop
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    PogoAudioProcessor& audioProcessor;

    juce::Slider    pogoKnob;
    juce::Label     pogoLabel;
    juce::Label     fileLabel;
    juce::TextButton loadButton { "Load Sample" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pogoAttachment;

    void openFileChooser();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PogoAudioProcessorEditor)
};
