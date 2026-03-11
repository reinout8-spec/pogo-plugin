#include "PluginEditor.h"
#include <BinaryData.h>

// ── Palette (overlay kleuren — moeten passen op de skin) ──────────────────
static const juce::Colour NEON_GRN  { 0xff00ff88 };
static const juce::Colour NEON_PNK  { 0xffff3aff };
static const juce::Colour NEON_YLW  { 0xffffe000 };
static const juce::Colour NEON_ORG  { 0xffff6600 };
static const juce::Colour NEON_CYN  { 0xff00e5ff };
static const juce::Colour TRANSP    { 0x00000000 };

// ── LookAndFeel: transparante knoppen die opgaan in de skin ───────────────
class SkinButtonLF : public juce::LookAndFeel_V4
{
public:
    juce::Colour textCol;
    explicit SkinButtonLF(juce::Colour c) : textCol(c) {}

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                               const juce::Colour&, bool over, bool down) override
    {
        auto b = btn.getLocalBounds().toFloat().reduced(1.0f);
        float alpha = down ? 0.55f : (over ? 0.35f : 0.18f);
        g.setColour(textCol.withAlpha(alpha));
        g.fillRoundedRectangle(b, 5.0f);
        g.setColour(textCol.withAlpha(down ? 0.9f : 0.55f));
        g.drawRoundedRectangle(b, 5.0f, 1.5f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                        bool, bool) override
    {
        g.setColour(textCol);
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(btn.getButtonText(), btn.getLocalBounds(),
                   juce::Justification::centred, false);
    }
};

// ── Constructor ────────────────────────────────────────────────────────────
PogoAudioProcessorEditor::PogoAudioProcessorEditor(PogoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      feedMeBtn  ("Load Audio!"),
      chillBtn   ("Reset Curve"),
      yeetBtn    ("YEET"),
      presetDrop ("DROP"),
      presetRise ("RISE"),
      presetSweep("SWEEP"),
      presetTape ("TAPE")
{
    setSize(640, 490);

    // Load background skin
    skinImage = juce::ImageCache::getFromMemory(BinaryData::skin_png,
                                                 BinaryData::skin_pngSize);

    // ── Curve editor ──────────────────────────────────────────────────
    curveEditor.onCurveChanged = [this]
    {
        audioProcessor.setCurve(curveEditor.curve);
    };
    curveEditor.curve = audioProcessor.getCurve();
    addAndMakeVisible(curveEditor);

    // ── TENSION knob ──────────────────────────────────────────────────
    tensionKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    tensionKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    tensionKnob.setColour(juce::Slider::thumbColourId,              NEON_PNK);
    tensionKnob.setColour(juce::Slider::rotarySliderFillColourId,   NEON_GRN.withAlpha(0.9f));
    tensionKnob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff003322));
    tensionKnob.setColour(juce::Slider::backgroundColourId,          TRANSP);
    tensionKnob.onValueChange = [this]
    {
        audioProcessor.precomputePositions();
        tensionValLabel.setText(juce::String(tensionKnob.getValue(), 2) + "x",
                                juce::dontSendNotification);
    };
    addAndMakeVisible(tensionKnob);

    tensionAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "tension", tensionKnob);

    tensionValLabel.setText(juce::String(tensionKnob.getValue(), 2) + "x",
                            juce::dontSendNotification);
    tensionValLabel.setJustificationType(juce::Justification::centred);
    tensionValLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    tensionValLabel.setColour(juce::Label::textColourId, NEON_GRN);
    tensionValLabel.setColour(juce::Label::backgroundColourId, TRANSP);
    addAndMakeVisible(tensionValLabel);

    // ── File label ────────────────────────────────────────────────────
    fileLabel.setText("", juce::dontSendNotification);
    fileLabel.setJustificationType(juce::Justification::centred);
    fileLabel.setFont(juce::Font(10.0f));
    fileLabel.setColour(juce::Label::textColourId, NEON_GRN);
    fileLabel.setColour(juce::Label::backgroundColourId, TRANSP);
    addAndMakeVisible(fileLabel);

    // ── LookAndFeels ──────────────────────────────────────────────────
    lfFeedMe   = std::make_unique<SkinButtonLF>(NEON_GRN);
    lfChill    = std::make_unique<SkinButtonLF>(NEON_CYN);
    lfYeet     = std::make_unique<SkinButtonLF>(NEON_ORG);
    lfPreset   = std::make_unique<SkinButtonLF>(NEON_YLW);

    feedMeBtn .setLookAndFeel(lfFeedMe .get());
    chillBtn  .setLookAndFeel(lfChill  .get());
    yeetBtn   .setLookAndFeel(lfYeet   .get());
    presetDrop .setLookAndFeel(lfPreset.get());
    presetRise .setLookAndFeel(lfPreset.get());
    presetSweep.setLookAndFeel(lfPreset.get());
    presetTape .setLookAndFeel(lfPreset.get());

    // ── Callbacks ─────────────────────────────────────────────────────
    feedMeBtn.onClick  = [this] { openFileChooser(); };
    chillBtn.onClick   = [this] {
        curveEditor.resetFlat();
        audioProcessor.setCurve(curveEditor.curve);
    };
    yeetBtn.onClick    = [this] { doExportDrag(); };

    presetDrop.onClick  = [this]{ curveEditor.setPreset("drop");      audioProcessor.setCurve(curveEditor.curve); };
    presetRise.onClick  = [this]{ curveEditor.setPreset("rise");      audioProcessor.setCurve(curveEditor.curve); };
    presetSweep.onClick = [this]{ curveEditor.setPreset("sweep");     audioProcessor.setCurve(curveEditor.curve); };
    presetTape.onClick  = [this]{ curveEditor.setPreset("tape-drop"); audioProcessor.setCurve(curveEditor.curve); };

    for (auto* b : { &feedMeBtn, &chillBtn, &yeetBtn,
                     &presetDrop, &presetRise, &presetSweep, &presetTape })
        addAndMakeVisible(b);
}

PogoAudioProcessorEditor::~PogoAudioProcessorEditor()
{
    feedMeBtn .setLookAndFeel(nullptr);
    chillBtn  .setLookAndFeel(nullptr);
    yeetBtn   .setLookAndFeel(nullptr);
    presetDrop .setLookAndFeel(nullptr);
    presetRise .setLookAndFeel(nullptr);
    presetSweep.setLookAndFeel(nullptr);
    presetTape .setLookAndFeel(nullptr);
}

// ── paint ──────────────────────────────────────────────────────────────────
void PogoAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Draw full background skin
    if (skinImage.isValid())
        g.drawImage(skinImage, 0, 0, getWidth(), getHeight(),
                    0, 0, skinImage.getWidth(), skinImage.getHeight());
    else
        g.fillAll(juce::Colour(0xff0a0a12));

    // Filename overlay (small, above canvas)
    if (fileLabel.getText().isNotEmpty())
    {
        g.setColour(NEON_GRN.withAlpha(0.85f));
        g.setFont(juce::Font(9.5f));
        g.drawText(fileLabel.getText(),
                   135, 108, 370, 14, juce::Justification::centred);
    }
}

// ── resized — posities afgestemd op de skin ───────────────────────────────
void PogoAudioProcessorEditor::resized()
{
    // CRT canvas (PITCH CURVE area in skin)
    curveEditor.setBounds(137, 122, 368, 228);

    // TENSION knob (right side arc area)
    tensionKnob   .setBounds(538,  78,  94, 120);
    tensionValLabel.setBounds(535, 195,  98,  28);

    // Buttons left side
    feedMeBtn.setBounds(22, 255, 108, 28);
    chillBtn .setBounds(22, 425, 108, 24);

    // YEET (bottom right — catapult area)
    yeetBtn.setBounds(498, 388, 120, 44);

    // Preset row
    int py = 372, ph = 28;
    int pw = 80;
    presetDrop .setBounds(140, py, pw, ph);
    presetRise .setBounds(228, py, pw, ph);
    presetSweep.setBounds(316, py, pw, ph);
    presetTape .setBounds(404, py, pw, ph);
}

// ── file drop ──────────────────────────────────────────────────────────────
bool PogoAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
    {
        auto ext = juce::File(f).getFileExtension().toLowerCase();
        if (ext == ".wav"  || ext == ".aif"  || ext == ".aiff" ||
            ext == ".mp3"  || ext == ".flac" || ext == ".ogg")
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
}

// ── file chooser ───────────────────────────────────────────────────────────
void PogoAudioProcessorEditor::openFileChooser()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Load Sample", juce::File{}, "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");

    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            if (fc.getResults().isEmpty()) return;
            auto file = fc.getResult();
            audioProcessor.loadSample(file);
            fileLabel.setText(file.getFileName(), juce::dontSendNotification);
        });
}

// ── YEET ──────────────────────────────────────────────────────────────────
void PogoAudioProcessorEditor::doExportDrag()
{
    if (audioProcessor.inputLength == 0) return;

    auto rendered = audioProcessor.renderProcessed();
    auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("pogoloco_export.wav");
    tempFile.deleteFile();

    juce::WavAudioFormat wavFmt;
    auto stream = std::make_unique<juce::FileOutputStream>(tempFile);
    if (!stream->openedOk()) return;

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFmt.createWriterFor(stream.release(), 44100.0,
                               (unsigned int)rendered.getNumChannels(), 24, {}, 0));
    if (!writer) return;
    writer->writeFromAudioSampleBuffer(rendered, 0, rendered.getNumSamples());
    writer.reset();

    juce::DragAndDropContainer::performExternalDragDropOfFiles(
        { tempFile.getFullPathName() }, false, this);
}
