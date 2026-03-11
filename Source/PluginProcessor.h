#pragma once
#include <JuceHeader.h>

class PogoAudioProcessor : public juce::AudioProcessor
{
public:
    PogoAudioProcessor();
    ~PogoAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void loadSample(const juce::File& file);

    juce::AudioProcessorValueTreeState parameters;
    juce::String loadedFileName;

private:
    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> sampleBuffer;
    int sampleLength   = 0;
    int sampleChannels = 0;

    // Playback state
    bool isPlaying   = false;
    int  outputSample = 0;
    float noteVelocity = 1.0f;

    // Pre-computed read positions for current note
    std::vector<double> readPositions;
    void precomputePositions(float pogoAmount);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PogoAudioProcessor)
};
