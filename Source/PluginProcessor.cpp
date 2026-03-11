#include "PluginProcessor.h"
#include "PluginEditor.h"

PogoAudioProcessor::PogoAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("PogoLoco"),
      {
          std::make_unique<juce::AudioParameterFloat>(
              juce::ParameterID{"tension", 1},
              "Tension",
              juce::NormalisableRange<float>(0.25f, 4.0f, 0.01f, 0.5f),
              1.0f)
      })
{
    formatManager.registerBasicFormats();
    pitchCurve.fill(0.0f);
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

// ---------------------------------------------------------------------------
// Build read-position array from pitch curve + tension (stretch) parameter.
//
//   outputLength = inputLength * tension
//
//   For each output sample i in [0, outputLength):
//     t        = i / (outputLength - 1)                    → normalised 0..1
//     semitones = pitchCurve interpolated at t
//     speed[i] = 2^(semitones/12)
//
//   Normalise so Σ speed == outputLength  (reads the full input exactly once)
//   readPositions[i] = cumulative sum → maps output → input sample index
// ---------------------------------------------------------------------------
void PogoAudioProcessor::precomputePositions()
{
    if (inputLength == 0) return;

    float tension = *parameters.getRawParameterValue("tension");
    outputLength  = juce::jmax(1, (int)(inputLength * tension));

    const int N = outputLength;
    std::vector<float> speed(N);

    for (int i = 0; i < N; ++i)
    {
        float t        = N > 1 ? (float)i / (N - 1) : 0.0f;
        float curveIdx = t * (CURVE_SIZE - 1);
        int   ci0      = juce::jlimit(0, CURVE_SIZE - 1, (int)curveIdx);
        int   ci1      = juce::jlimit(0, CURVE_SIZE - 1, ci0 + 1);
        float frac     = curveIdx - (float)ci0;
        float semis    = pitchCurve[ci0] + frac * (pitchCurve[ci1] - pitchCurve[ci0]);
        speed[i]       = std::pow(2.0f, semis / 12.0f);
    }

    // Normalise: Σ speed  →  inputLength  (so we read exactly the full input)
    double total = 0.0;
    for (auto s : speed) total += s;
    double scale = (total > 0.0) ? (double)inputLength / total : 1.0;
    for (auto& s : speed) s = (float)((double)s * scale);

    readPositions.resize(N);
    double pos = 0.0;
    for (int i = 0; i < N; ++i)
    {
        readPositions[i] = juce::jlimit(0.0, (double)inputLength - 1.001, pos);
        pos += speed[i];
    }
}

void PogoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiBuffer)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (inputLength == 0 || readPositions.empty()) return;

    for (const auto meta : midiBuffer)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn() && msg.getVelocity() > 0)
        {
            noteVelocity = msg.getVelocity() / 127.0f;
            outputSample = 0;
            isPlaying    = true;
        }
    }

    if (!isPlaying) return;

    const int numSamples     = buffer.getNumSamples();
    const int numOutChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        if (outputSample >= outputLength) { isPlaying = false; break; }

        const double readPos = readPositions[outputSample];
        const int    ri      = (int)readPos;
        const double frac    = readPos - (double)ri;

        for (int ch = 0; ch < numOutChannels; ++ch)
        {
            const float* src = sampleBuffer.getReadPointer(ch % sampleChannels);
            float s0 = src[ri];
            float s1 = (ri + 1 < inputLength) ? src[ri + 1] : s0;
            buffer.addSample(ch, i, (float)(s0 + frac * (s1 - s0)) * noteVelocity);
        }
        ++outputSample;
    }
}

void PogoAudioProcessor::loadSample(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));
    if (!reader) return;

    isPlaying      = false;
    outputSample   = 0;
    sampleChannels = juce::jmin(2, (int)reader->numChannels);
    inputLength    = (int)reader->lengthInSamples;
    sampleBuffer.setSize(sampleChannels, inputLength);
    reader->read(&sampleBuffer, 0, inputLength, 0, true, sampleChannels > 1);
    loadedFileName = file.getFileName();
    precomputePositions();
}

void PogoAudioProcessor::setCurve(const std::array<float, CURVE_SIZE>& newCurve)
{
    pitchCurve = newCurve;
    precomputePositions();
}

juce::AudioBuffer<float> PogoAudioProcessor::renderProcessed() const
{
    juce::AudioBuffer<float> out(sampleChannels, outputLength);
    out.clear();
    if (inputLength == 0 || readPositions.empty()) return out;

    for (int i = 0; i < outputLength; ++i)
    {
        const double readPos = readPositions[i];
        const int    ri      = (int)readPos;
        const double frac    = readPos - (double)ri;

        for (int ch = 0; ch < sampleChannels; ++ch)
        {
            const float* src = sampleBuffer.getReadPointer(ch);
            float s0 = src[ri];
            float s1 = (ri + 1 < inputLength) ? src[ri + 1] : s0;
            out.setSample(ch, i, (float)(s0 + frac * (s1 - s0)));
        }
    }
    return out;
}

void PogoAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    juce::String curveStr;
    for (int i = 0; i < CURVE_SIZE; ++i)
        curveStr << pitchCurve[i] << (i < CURVE_SIZE - 1 ? "," : "");
    state.setProperty("pitchCurve",  curveStr,        nullptr);
    state.setProperty("sampleFile",  loadedFileName,  nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PogoAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (!xml || !xml->hasTagName(parameters.state.getType())) return;
    auto state = juce::ValueTree::fromXml(*xml);
    parameters.replaceState(state);

    auto curveStr = state.getProperty("pitchCurve").toString();
    if (curveStr.isNotEmpty())
    {
        auto tokens = juce::StringArray::fromTokens(curveStr, ",", "");
        for (int i = 0; i < juce::jmin((int)tokens.size(), CURVE_SIZE); ++i)
            pitchCurve[i] = tokens[i].getFloatValue();
        precomputePositions();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PogoAudioProcessor();
}
