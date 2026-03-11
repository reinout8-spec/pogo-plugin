#include "PluginProcessor.h"
#include "PluginEditor.h"

PogoAudioProcessor::PogoAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("Pogo"),
          {
              std::make_unique<juce::AudioParameterFloat>(
                  "pogo", "Pogo",
                  juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f),
                  0.0f)
          })
{
    formatManager.registerBasicFormats();
}

PogoAudioProcessor::~PogoAudioProcessor() {}

juce::AudioProcessorEditor* PogoAudioProcessor::createEditor()
{
    return new PogoAudioProcessorEditor(*this);
}

void PogoAudioProcessor::prepareToPlay(double, int)
{
    isPlaying    = false;
    outputSample = 0;
}

void PogoAudioProcessor::releaseResources() {}

// -----------------------------------------------------------------------
// Pre-compute read positions using:
//   readPos[i] = i + (A * N / pi) * sin(pi * i / N)
//
// Where:
//   A > 0 : starts fast (high pitch) → ends slow (low pitch)
//   A < 0 : starts slow (low pitch)  → ends fast (high pitch)
//   Normal pitch is always crossed at i = N/2
//   Total samples read always equals N (time preserved)
// -----------------------------------------------------------------------
void PogoAudioProcessor::precomputePositions(float A)
{
    if (sampleLength == 0) return;

    readPositions.resize(sampleLength);
    const double N          = (double)sampleLength;
    const double piOverN    = juce::MathConstants<double>::pi / N;
    const double scaleFactor = (double)A * N / juce::MathConstants<double>::pi;

    for (int i = 0; i < sampleLength; ++i)
    {
        double pos = (double)i + scaleFactor * std::sin(piOverN * (double)i);
        readPositions[i] = juce::jlimit(0.0, N - 1.001, pos);
    }
}

void PogoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiBuffer)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (sampleLength == 0) return;

    // Process MIDI messages
    for (const auto meta : midiBuffer)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn() && msg.getVelocity() > 0)
        {
            float pogoAmount = *parameters.getRawParameterValue("pogo");
            precomputePositions(pogoAmount);
            noteVelocity = msg.getVelocity() / 127.0f;
            outputSample = 0;
            isPlaying    = true;
        }
        // Note off does NOT stop playback – sample plays to the end
    }

    if (!isPlaying) return;

    const int numSamples     = buffer.getNumSamples();
    const int numOutChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        if (outputSample >= sampleLength)
        {
            isPlaying = false;
            break;
        }

        const double readPos = readPositions[outputSample];
        const int    readInt = (int)readPos;
        const double frac    = readPos - (double)readInt;

        for (int ch = 0; ch < numOutChannels; ++ch)
        {
            const int   srcCh   = ch % sampleChannels;
            const float* src    = sampleBuffer.getReadPointer(srcCh);

            // Linear interpolation between adjacent samples
            const float s0     = src[readInt];
            const float s1     = (readInt + 1 < sampleLength) ? src[readInt + 1] : s0;
            const float sample = (float)(s0 + frac * (s1 - s0)) * noteVelocity;

            buffer.addSample(ch, i, sample);
        }

        ++outputSample;
    }
}

void PogoAudioProcessor::loadSample(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));
    if (reader == nullptr) return;

    isPlaying      = false;
    outputSample   = 0;
    sampleChannels = juce::jmin(2, (int)reader->numChannels);
    sampleLength   = (int)reader->lengthInSamples;

    sampleBuffer.setSize(sampleChannels, sampleLength);
    reader->read(&sampleBuffer, 0, sampleLength, 0, true, sampleChannels > 1);

    loadedFileName = file.getFileName();
}

void PogoAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PogoAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PogoAudioProcessor();
}
