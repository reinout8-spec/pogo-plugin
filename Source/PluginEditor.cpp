#include "PluginEditor.h"

// ── Palette ────────────────────────────────────────────────────────────────
static const juce::Colour BG_DARK   { 0xff0d0d12 };
static const juce::Colour BG_METAL  { 0xff1a1a28 };
static const juce::Colour BG_PANEL  { 0xff111120 };
static const juce::Colour NEON_GRN  { 0xff00ff88 };
static const juce::Colour NEON_PNK  { 0xffff44ff };
static const juce::Colour NEON_YLW  { 0xffffe000 };
static const juce::Colour NEON_ORG  { 0xffff6600 };
static const juce::Colour NEON_CYN  { 0xff00e5ff };
static const juce::Colour BTN_RED   { 0xffcc1a1a };
static const juce::Colour BTN_BLUE  { 0xff1a3acc };
static const juce::Colour CRT_BG    { 0xff001a08 };
static const juce::Colour CRT_GRID  { 0xff003318 };

// ── Constructor ────────────────────────────────────────────────────────────
PogoAudioProcessorEditor::PogoAudioProcessorEditor(PogoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(640, 480);

    // ── Curve editor ──────────────────────────────────────────────────
    curveEditor.onCurveChanged = [this]
    {
        audioProcessor.setCurve(curveEditor.curve);
    };
    addAndMakeVisible(curveEditor);

    // Sync curve from processor on load
    curveEditor.curve = audioProcessor.getCurve();

    // ── TENSION knob ──────────────────────────────────────────────────
    tensionKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    tensionKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    tensionKnob.setColour(juce::Slider::thumbColourId,          NEON_GRN);
    tensionKnob.setColour(juce::Slider::rotarySliderFillColourId, NEON_GRN.withAlpha(0.8f));
    tensionKnob.setColour(juce::Slider::rotarySliderOutlineColourId, BG_METAL);
    tensionKnob.setColour(juce::Slider::textBoxTextColourId,    NEON_GRN);
    tensionKnob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    tensionKnob.setColour(juce::Slider::textBoxBackgroundColourId, BG_PANEL);
    tensionKnob.onValueChange = [this]
    {
        audioProcessor.precomputePositions();
    };
    addAndMakeVisible(tensionKnob);

    tensionAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "tension", tensionKnob);

    tensionLabel.setText("TENSION\nSTRETCHY CHAOS", juce::dontSendNotification);
    tensionLabel.setJustificationType(juce::Justification::centred);
    tensionLabel.setFont(juce::Font(10.5f, juce::Font::bold));
    tensionLabel.setColour(juce::Label::textColourId, NEON_GRN);
    addAndMakeVisible(tensionLabel);

    // ── File label ────────────────────────────────────────────────────
    fileLabel.setText("Drop sample here or use FEED ME", juce::dontSendNotification);
    fileLabel.setJustificationType(juce::Justification::centredLeft);
    fileLabel.setFont(juce::Font(11.0f));
    fileLabel.setColour(juce::Label::textColourId, NEON_GRN.withAlpha(0.6f));
    addAndMakeVisible(fileLabel);

    // ── Buttons ───────────────────────────────────────────────────────
    styleBtn(feedMeBtn,  BTN_RED,              juce::Colours::white);
    styleBtn(chillBtn,   BTN_BLUE,             NEON_CYN);
    styleBtn(yeetBtn,    NEON_ORG,             juce::Colours::black);
    styleBtn(presetDrop, juce::Colour(0xff003300), NEON_GRN);
    styleBtn(presetRise, juce::Colour(0xff330033), NEON_PNK);
    styleBtn(presetSweep,juce::Colour(0xff333300), NEON_YLW);
    styleBtn(presetTape, juce::Colour(0xff331800), NEON_ORG);

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

PogoAudioProcessorEditor::~PogoAudioProcessorEditor() {}

void PogoAudioProcessorEditor::styleBtn(juce::TextButton& b,
                                         juce::Colour bg, juce::Colour text)
{
    b.setColour(juce::TextButton::buttonColourId,   bg);
    b.setColour(juce::TextButton::buttonOnColourId,  bg.brighter(0.3f));
    b.setColour(juce::TextButton::textColourOffId,   text);
    b.setColour(juce::TextButton::textColourOnId,    text.brighter());
}

// ── paint ──────────────────────────────────────────────────────────────────
void PogoAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Overall background
    g.fillAll(BG_DARK);

    // Outer neon border (multi-colour LED strip effect)
    auto outer = getLocalBounds().toFloat().reduced(2.0f);
    for (int i = 0; i < 3; ++i)
    {
        float alpha = 0.6f - i * 0.18f;
        g.setColour(NEON_GRN.withAlpha(alpha));
        g.drawRoundedRectangle(outer.reduced((float)i * 1.5f), 8.0f, 1.4f);
    }

    // Header bar
    auto header = getLocalBounds().removeFromTop(56).toFloat();
    g.setColour(BG_METAL);
    g.fillRect(header);

    // "POGO LOCO" neon text
    g.setFont(juce::Font(30.0f, juce::Font::bold | juce::Font::italic));
    g.setColour(NEON_PNK.withAlpha(0.25f));
    g.drawText("POGO LOCO", header.translated(2.0f, 2.0f), juce::Justification::centredLeft, false);
    g.setColour(NEON_PNK);
    g.drawText("POGO LOCO", header, juce::Justification::centredLeft, false);
    // adjust left inset
    juce::Rectangle<float> logoArea(12.0f, header.getY(), 180.0f, header.getHeight());
    g.setFont(juce::Font(30.0f, juce::Font::bold | juce::Font::italic));
    g.setColour(NEON_PNK.withAlpha(0.3f));
    g.drawText("POGO LOCO", logoArea.translated(2, 2), juce::Justification::centredLeft, false);
    g.setColour(NEON_PNK);
    g.drawText("POGO LOCO", logoArea, juce::Justification::centredLeft, false);

    // "THE BOUNCE ZONE" centre title
    juce::Rectangle<float> titleArea(160.0f, header.getY(), 300.0f, header.getHeight());
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.setColour(NEON_GRN.withAlpha(0.3f));
    g.drawText("THE BOUNCE ZONE", titleArea.translated(1, 2), juce::Justification::centred, false);
    g.setColour(NEON_GRN);
    g.drawText("THE BOUNCE ZONE", titleArea, juce::Justification::centred, false);

    // "v1.0" tag
    g.setFont(juce::Font(10.0f));
    g.setColour(NEON_GRN.withAlpha(0.5f));
    g.drawText("v1.0", logoArea.withX(logoArea.getRight() - 30).withWidth(28),
               juce::Justification::bottomLeft);

    // Header separator
    g.setColour(NEON_GRN.withAlpha(0.7f));
    g.drawLine(0, 56, (float)getWidth(), 56, 2.0f);

    // INPUT / OUTPUT labels
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(NEON_YLW.withAlpha(0.8f));
    g.drawText("INPUT",  40,  40, 60, 14, juce::Justification::centred);
    g.drawText("OUTPUT", 490, 40, 60, 14, juce::Justification::centred);

    // Canvas frame (CRT bezel)
    auto crtOuter = juce::Rectangle<float>(160.0f, 64.0f, 310.0f, 300.0f);
    g.setColour(juce::Colour(0xff0a2a18));
    g.fillRoundedRectangle(crtOuter, 10.0f);
    g.setColour(NEON_GRN.withAlpha(0.8f));
    g.drawRoundedRectangle(crtOuter, 10.0f, 2.5f);

    // Canvas labels
    g.setFont(juce::Font(10.5f, juce::Font::bold));
    g.setColour(NEON_GRN.withAlpha(0.9f));
    g.drawText("PITCH CURVE", (int)crtOuter.getX() + 8, (int)crtOuter.getY() + 6,
               120, 14, juce::Justification::left);
    g.setColour(NEON_GRN.withAlpha(0.5f));
    g.drawText("BOUNCE FACTOR", (int)crtOuter.getRight() - 120, (int)crtOuter.getBottom() - 18,
               115, 14, juce::Justification::right);

    // Left panel bg (FEED ME + CHILL)
    g.setColour(BG_METAL);
    g.fillRoundedRectangle(8.0f, 64.0f, 148.0f, 300.0f, 8.0f);
    g.setColour(NEON_GRN.withAlpha(0.15f));
    g.drawRoundedRectangle(8.0f, 64.0f, 148.0f, 300.0f, 8.0f, 1.0f);

    // Right panel bg (TENSION knob)
    g.setColour(BG_METAL);
    g.fillRoundedRectangle(478.0f, 64.0f, 154.0f, 300.0f, 8.0f);
    g.setColour(NEON_GRN.withAlpha(0.15f));
    g.drawRoundedRectangle(478.0f, 64.0f, 154.0f, 300.0f, 8.0f, 1.0f);

    // Bottom bar bg
    g.setColour(BG_METAL);
    g.fillRect(0, 370, getWidth(), getHeight() - 370);
    g.setColour(NEON_GRN.withAlpha(0.4f));
    g.drawLine(0, 370, (float)getWidth(), 370, 1.5f);
}

// ── resized ────────────────────────────────────────────────────────────────
void PogoAudioProcessorEditor::resized()
{
    // Curve canvas (inside CRT bezel, leave some padding)
    curveEditor.setBounds(168, 80, 294, 276);

    // Left column
    feedMeBtn.setBounds(18,  80,  128, 70);
    fileLabel.setBounds(12, 158,  144, 36);
    chillBtn .setBounds(18, 200,  128, 50);

    // TENSION knob (right column)
    tensionKnob .setBounds(492, 80,  130, 120);
    tensionLabel.setBounds(488, 200, 138,  40);

    // Bottom row: presets + YEET
    int py = 378;
    int pw = 108;
    presetDrop .setBounds(8,         py, pw, 44);
    presetRise .setBounds(116,       py, pw, 44);
    presetSweep.setBounds(224,       py, pw, 44);
    presetTape .setBounds(332,       py, pw, 44);
    yeetBtn    .setBounds(448,       py, 184, 44);
}

// ── file drag & drop ───────────────────────────────────────────────────────
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
    fileLabel.setColour(juce::Label::textColourId, NEON_GRN);
}

// ── file chooser ───────────────────────────────────────────────────────────
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
            fileLabel.setColour(juce::Label::textColourId, NEON_GRN);
        });
}

// ── YEET: drag rendered sample to DAW ─────────────────────────────────────
void PogoAudioProcessorEditor::doExportDrag()
{
    if (audioProcessor.inputLength == 0)
    {
        fileLabel.setText("Load a sample first!", juce::dontSendNotification);
        return;
    }

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
