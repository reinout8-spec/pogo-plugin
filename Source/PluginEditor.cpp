#include "PluginEditor.h"

static const juce::Colour BG_DARK  { 0xff0c0c1a };
static const juce::Colour BG_PANEL { 0xff181828 };
static const juce::Colour ACCENT   { 0xff6c63ff };
static const juce::Colour TEXT_DIM { 0xff888899 };
static const juce::Colour TEXT_ON  { 0xffffffff };

PogoAudioProcessorEditor::PogoAudioProcessorEditor(PogoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(500, 440);

    // ── Curve editor ──────────────────────────────────────────────────
    curveEditor.onCurveChanged = [this]
    {
        audioProcessor.setCurve(curveEditor.curve);
    };
    addAndMakeVisible(curveEditor);

    // ── File label ────────────────────────────────────────────────────
    fileLabel.setText("Drop sample here", juce::dontSendNotification);
    fileLabel.setJustificationType(juce::Justification::centred);
    fileLabel.setFont(juce::Font(11.5f));
    fileLabel.setColour(juce::Label::textColourId, TEXT_DIM);
    addAndMakeVisible(fileLabel);

    // ── Load button ───────────────────────────────────────────────────
    loadButton.setColour(juce::TextButton::buttonColourId,  BG_PANEL);
    loadButton.setColour(juce::TextButton::textColourOffId, ACCENT);
    loadButton.onClick = [this] { openFileChooser(); };
    addAndMakeVisible(loadButton);

    // ── Reset button ──────────────────────────────────────────────────
    resetButton.setColour(juce::TextButton::buttonColourId,  BG_PANEL);
    resetButton.setColour(juce::TextButton::textColourOffId, TEXT_DIM);
    resetButton.onClick = [this]
    {
        curveEditor.resetFlat();
        audioProcessor.setCurve(curveEditor.curve);
    };
    addAndMakeVisible(resetButton);

    // ── Drag-to-DAW button ────────────────────────────────────────────
    dragButton.setColour(juce::TextButton::buttonColourId,  ACCENT);
    dragButton.setColour(juce::TextButton::textColourOffId, TEXT_ON);
    dragButton.onClick = [this] { doExportDrag(); };
    addAndMakeVisible(dragButton);

    // ── Preset buttons ────────────────────────────────────────────────
    for (auto* btn : { &presetDrop, &presetRise, &presetSweep, &presetTape })
    {
        stylePresetButton(*btn);
        addAndMakeVisible(*btn);
    }
    presetDrop.onClick  = [this]{ curveEditor.setPreset("drop");      audioProcessor.setCurve(curveEditor.curve); };
    presetRise.onClick  = [this]{ curveEditor.setPreset("rise");      audioProcessor.setCurve(curveEditor.curve); };
    presetSweep.onClick = [this]{ curveEditor.setPreset("sweep");     audioProcessor.setCurve(curveEditor.curve); };
    presetTape.onClick  = [this]{ curveEditor.setPreset("tape-drop"); audioProcessor.setCurve(curveEditor.curve); };
}

PogoAudioProcessorEditor::~PogoAudioProcessorEditor() {}

void PogoAudioProcessorEditor::stylePresetButton(juce::TextButton& btn)
{
    btn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff22223a));
    btn.setColour(juce::TextButton::textColourOffId, ACCENT);
}

void PogoAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(BG_DARK);

    // Header bar
    auto header = getLocalBounds().removeFromTop(50);
    g.setColour(BG_PANEL);
    g.fillRect(header);
    g.setColour(TEXT_ON);
    g.setFont(juce::Font(26.0f, juce::Font::bold));
    g.drawText("POGO", header, juce::Justification::centred);
    g.setColour(ACCENT);
    g.fillRect(0, 49, getWidth(), 2);

    // Drop zone outline
    auto dz = juce::Rectangle<int>(10, 56, getWidth() - 20, 36);
    g.setColour(BG_PANEL);
    g.fillRoundedRectangle(dz.toFloat(), 6.0f);
    g.setColour(ACCENT.withAlpha(0.3f));
    g.drawRoundedRectangle(dz.toFloat(), 6.0f, 1.0f);
}

void PogoAudioProcessorEditor::resized()
{
    // Drop zone: y=56, h=36
    auto dropRow = juce::Rectangle<int>(10, 56, getWidth() - 130, 36);
    fileLabel.setBounds(dropRow);
    loadButton.setBounds(getWidth() - 116, 60, 106, 28);

    // Curve editor: y=100, fill most of height
    curveEditor.setBounds(10, 100, getWidth() - 20, getHeight() - 180);

    // Preset row below curve
    int presetY  = getHeight() - 76;
    int presetW  = (getWidth() - 20) / 4 - 4;
    presetDrop .setBounds(10,              presetY, presetW, 26);
    presetRise .setBounds(14 + presetW,    presetY, presetW, 26);
    presetSweep.setBounds(18 + presetW*2,  presetY, presetW, 26);
    presetTape .setBounds(22 + presetW*3,  presetY, presetW, 26);

    // Bottom row: Reset + Drag to DAW
    int btnY = getHeight() - 42;
    resetButton.setBounds(10,            btnY, 80,  32);
    dragButton .setBounds(100,           btnY, getWidth() - 110, 32);
}

bool PogoAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
    {
        auto ext = juce::File(f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" ||
            ext == ".mp3" || ext == ".flac" || ext == ".ogg")
            return true;
    }
    return false;
}

void PogoAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    if (files.isEmpty()) return;
    juce::File file(files[0]);
    audioProcessor.loadSample(file);
    fileLabel.setText(file.getFileName(), juce::dontSendNotification);
    fileLabel.setColour(juce::Label::textColourId, TEXT_ON);
}

void PogoAudioProcessorEditor::openFileChooser()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Load Sample", juce::File{},
        "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");

    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            if (fc.getResults().isEmpty()) return;
            auto file = fc.getResult();
            audioProcessor.loadSample(file);
            fileLabel.setText(file.getFileName(), juce::dontSendNotification);
            fileLabel.setColour(juce::Label::textColourId, TEXT_ON);
        });
}

void PogoAudioProcessorEditor::doExportDrag()
{
    if (audioProcessor.sampleLength == 0)
    {
        fileLabel.setText("Load a sample first!", juce::dontSendNotification);
        return;
    }

    // Render the processed audio
    auto rendered = audioProcessor.renderProcessed();

    // Write to temp WAV
    auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("pogo_export.wav");
    tempFile.deleteFile();

    juce::WavAudioFormat wavFmt;
    auto stream = std::make_unique<juce::FileOutputStream>(tempFile);
    if (!stream->openedOk()) return;

    // Use original sample rate (44100 fallback)
    double sr = 44100.0;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFmt.createWriterFor(stream.release(), sr,
                               (unsigned int)rendered.getNumChannels(), 24, {}, 0));
    if (!writer) return;
    writer->writeFromAudioSampleBuffer(rendered, 0, rendered.getNumSamples());
    writer.reset();  // flush & close

    // Initiate system drag
    juce::DragAndDropContainer::performExternalDragDropOfFiles(
        { tempFile.getFullPathName() }, false, this);
}
