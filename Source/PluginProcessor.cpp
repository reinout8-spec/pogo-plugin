#include "PluginProcessor.h"
#include "PluginEditor.h"

PogoAudioProcessor::PogoAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("Pogo"), {})
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
// Build read-position array from the user-drawn pitch curve.
//
// Algorithm:
//   1. For each output sample i, lookup semitone value from pitchCurve
//      (with linear interpolation between curve points).
//   2. Convert semitones → speed ratio: speed = 2^(semi/12)
//   3. Normalise so that sum(speed) == sampleLength  → time preserved
//   4. Integrate: readPos[i] = cumulative sum of speed values
// ---------------------------------------------------------------------------
void PogoAudioProcessor::precomputePositions()
{
    if (sampleLength == 0) return;

    const int N = sampleLength;
    std::vector<float> speed(N);

    // Step 1 & 2: speed from pitch curve
    for (int i = 0; i < N; ++i)
    {
        float t        = (float)i / (N > 1 ? N - 1 : 1);
        float curveIdx = t * (CURVE_SIZE - 1);
        int   ci0      = juce::jlimit(0, CURVE_SIZE - 1, (int)curveIdx);
        int   ci1      = juce::jlimit(0, CURVE_SIZE - 1, ci0 + 1);
        float frac     = curveIdx - ci0;
        float semis    = pitchCurve[ci0] + frac * (pitchCurve[ci1] - pitchCurve[ci0]);
        speed[i]       = std::pow(2.0f, semis / 12.0f);
    }

    // Step 3: normalise
    double avg = 0.0;
    for (auto s : speed) avg += s;
    avg /= N;
    if (avg <= 0.0) avg = 1.0;
    for (auto& s : speed) s = (float)((double)s / avg);

    // Step 4: cumulative read positions
    readPositions.resize(N);
    double pos = 0.0;
    for (int i = 0; i < N; ++i)
    {
        readPositions[i] = juce::jlimit(0.0, (double)N - 1.001, pos);
        pos += speed[i];
    }
}

void PogoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiBuffer)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (sampleLength == 0 || readPositions.empty()) return;

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
        if (outputSample >= sampleLength) { isPlaying = false; break; }

        const double readPos = readPositions[outputSample];
        const int    readInt = (int)readPos;
        const double frac    = readPos - (double)readInt;

        for (int ch = 0; ch < numOutChannels; ++ch)
        {
            const float* src = sampleBuffer.getReadPointer(ch % sampleChannels);
            float s0  = src[readInt];
            float s1  = (readInt + 1 < sampleLength) ? src[readInt + 1] : s0;
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
    sampleLength   = (int)reader->lengthInSamples;
    sampleBuffer.setSize(sampleChannels, sampleLength);
    reader->read(&sampleBuffer, 0, sampleLength, 0, true, sampleChannels > 1);
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
    juce::AudioBuffer<float> out(sampleChannels, sampleLength);
    out.clear();
    if (sampleLength == 0 || readPositions.empty()) return out;

    for (int i = 0; i < sampleLength; ++i)
    {
        const double readPos = readPositions[i];
        const int    readInt = (int)readPos;
        const double frac    = readPos - (double)readInt;

        for (int ch = 0; ch < sampleChannels; ++ch)
        {
            const float* src = sampleBuffer.getReadPointer(ch);
            float s0 = src[readInt];
            float s1 = (readInt + 1 < sampleLength) ? src[readInt + 1] : s0;
            out.setSample(ch, i, (float)(s0 + frac * (s1 - s0)));
        }
    }
    return out;
}

void PogoAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    // Embed pitch curve as XML attribute array
    juce::String curveStr;
    for (int i = 0; i < CURVE_SIZE; ++i)
        curveStr << pitchCurve[i] << (i < CURVE_SIZE - 1 ? "," : "");
    state.setProperty("pitchCurve", curveStr, nullptr);
    state.setProperty("sampleFile", loadedFileName, nullptr);
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
