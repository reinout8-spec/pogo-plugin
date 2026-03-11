#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

class PogoAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int CURVE_SIZE = 256;

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
    void setCurve(const std::array<float, CURVE_SIZE>& newCurve);
    const std::array<float, CURVE_SIZE>& getCurve() const { return pitchCurve; }

    juce::AudioBuffer<float> renderProcessed() const;
    void precomputePositions();  // public: called by tension knob

    juce::AudioProcessorValueTreeState parameters;
    juce::String loadedFileName;

    int inputLength    = 0;   // raw sample frames
    int outputLength   = 0;   // after stretch
    int sampleChannels = 0;

private:
    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float>  sampleBuffer;
    std::array<float, CURVE_SIZE> pitchCurve {};

    bool  isPlaying    = false;
    int   outputSample = 0;
    float noteVelocity = 1.0f;

    std::vector<double> readPositions;  // size = outputLength

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PogoAudioProcessor)
};
