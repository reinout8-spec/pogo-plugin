#include "PluginEditor.h"

// ── Palette ────────────────────────────────────────────────────────────────
static const juce::Colour BG_DARK   { 0xff0a0a0f };
static const juce::Colour BG_METAL  { 0xff141420 };
static const juce::Colour BG_RIVET  { 0xff1e1e2e };
static const juce::Colour NEON_GRN  { 0xff00ff88 };
static const juce::Colour NEON_PNK  { 0xffff3aff };
static const juce::Colour NEON_YLW  { 0xffffe000 };
static const juce::Colour NEON_ORG  { 0xffff6600 };
static const juce::Colour NEON_CYN  { 0xff00e5ff };
static const juce::Colour BTN_RED   { 0xffbb1111 };
static const juce::Colour BTN_BLUE  { 0xff1122cc };

static void drawNeonText(juce::Graphics& g, const juce::String& text,
                          juce::Rectangle<float> area, juce::Colour col,
                          float fontSize, int styleFlags,
                          juce::Justification just = juce::Justification::centred)
{
    g.setFont(juce::Font(fontSize, styleFlags));
    // Glow layer
    g.setColour(col.withAlpha(0.25f));
    for (int i = 1; i <= 3; ++i)
        g.drawText(text, area.translated((float)i * 0.6f, (float)i * 0.6f), just, false);
    // Main text
    g.setColour(col);
    g.drawText(text, area, just, false);
}

// ── Constructor ────────────────────────────────────────────────────────────
PogoAudioProcessorEditor::PogoAudioProcessorEditor(PogoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      feedMeBtn  ("FEED ME"),
      chillBtn   ("CHILL"),
      yeetBtn    (">> YEET <<"),
      presetDrop ("DROP  v"),
      presetRise ("RISE  ^"),
      presetSweep("SWEEP ~"),
      presetTape ("TAPE  @")
{
    setSize(640, 490);

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
    tensionKnob.setColour(juce::Slider::thumbColourId,             NEON_GRN);
    tensionKnob.setColour(juce::Slider::rotarySliderFillColourId,  NEON_GRN.withAlpha(0.9f));
    tensionKnob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1a3322));
    tensionKnob.onValueChange = [this]
    {
        audioProcessor.precomputePositions();
        tensionValLabel.setText(juce::String(tensionKnob.getValue(), 2) + "x",
                                juce::dontSendNotification);
    };
    addAndMakeVisible(tensionKnob);

    tensionAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "tension", tensionKnob);

    tensionLabel.setText("TENSION\nSTRETCHY CHAOS", juce::dontSendNotification);
    tensionLabel.setJustificationType(juce::Justification::centred);
    tensionLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    tensionLabel.setColour(juce::Label::textColourId, NEON_GRN);
    addAndMakeVisible(tensionLabel);

    tensionValLabel.setText(juce::String(tensionKnob.getValue(), 2) + "x",
                            juce::dontSendNotification);
    tensionValLabel.setJustificationType(juce::Justification::centred);
    tensionValLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    tensionValLabel.setColour(juce::Label::textColourId, NEON_GRN);
    addAndMakeVisible(tensionValLabel);

    // ── File label ────────────────────────────────────────────────────
    fileLabel.setText("Drop sample here", juce::dontSendNotification);
    fileLabel.setJustificationType(juce::Justification::centred);
    fileLabel.setFont(juce::Font(10.0f));
    fileLabel.setColour(juce::Label::textColourId, NEON_GRN.withAlpha(0.55f));
    addAndMakeVisible(fileLabel);

    // ── Buttons ───────────────────────────────────────────────────────
    styleBtn(feedMeBtn,   BTN_RED,                       juce::Colours::white,  16);
    styleBtn(chillBtn,    BTN_BLUE,                      NEON_CYN,              14);
    styleBtn(yeetBtn,     NEON_ORG,                      juce::Colours::black,  15);
    styleBtn(presetDrop,  juce::Colour(0xff003308),      NEON_GRN,              12);
    styleBtn(presetRise,  juce::Colour(0xff300030),      NEON_PNK,              12);
    styleBtn(presetSweep, juce::Colour(0xff302800),      NEON_YLW,              12);
    styleBtn(presetTape,  juce::Colour(0xff302000),      NEON_ORG,              12);

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
                                         juce::Colour bg, juce::Colour text, int fs)
{
    b.setColour(juce::TextButton::buttonColourId,  bg);
    b.setColour(juce::TextButton::buttonOnColourId, bg.brighter(0.25f));
    b.setColour(juce::TextButton::textColourOffId,  text);
    b.setColour(juce::TextButton::textColourOnId,   text.brighter());
    b.setLookAndFeel(nullptr);
    (void)fs;
}

void PogoAudioProcessorEditor::paintPanel(juce::Graphics& g,
                                           juce::Rectangle<float> r,
                                           juce::Colour border)
{
    // Metal gradient bg
    juce::ColourGradient grad(BG_RIVET, r.getX(), r.getY(),
                               BG_METAL, r.getRight(), r.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(r, 7.0f);

    // Border glow
    g.setColour(border.withAlpha(0.5f));
    g.drawRoundedRectangle(r.reduced(0.5f), 7.0f, 2.0f);
    g.setColour(border.withAlpha(0.2f));
    g.drawRoundedRectangle(r.expanded(1.5f), 8.5f, 1.0f);
}

void PogoAudioProcessorEditor::paintRivetCorners(juce::Graphics& g,
                                                   juce::Rectangle<float> r)
{
    float rv = 5.5f, rd = 11.0f;
    float cx[] = { r.getX() + rd, r.getRight() - rd, r.getX() + rd, r.getRight() - rd };
    float cy[] = { r.getY() + rd, r.getY() + rd, r.getBottom() - rd, r.getBottom() - rd };

    for (int i = 0; i < 4; ++i)
    {
        juce::Colour dark(0xff0a0a18);
        juce::Colour light(0xff383850);
        g.setColour(dark);
        g.fillEllipse(cx[i] - rv - 1.5f, cy[i] - rv - 1.5f, (rv + 1.5f) * 2, (rv + 1.5f) * 2);
        juce::ColourGradient rg(light, cx[i] - rv * 0.5f, cy[i] - rv * 0.5f,
                                 dark,  cx[i] + rv * 0.5f, cy[i] + rv * 0.5f, true);
        g.setGradientFill(rg);
        g.fillEllipse(cx[i] - rv, cy[i] - rv, rv * 2, rv * 2);
    }
}

// ── paint ──────────────────────────────────────────────────────────────────
void PogoAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background with subtle noise feel
    juce::ColourGradient bgGrad(juce::Colour(0xff0e0e18), 0, 0,
                                 juce::Colour(0xff060608), 0, (float)getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillAll();

    // Outer neon border
    auto outerR = getLocalBounds().toFloat().reduced(3.0f);
    for (int i = 0; i < 4; ++i)
    {
        g.setColour(NEON_GRN.withAlpha(0.55f - i * 0.12f));
        g.drawRoundedRectangle(outerR.reduced((float)i * 1.8f), 9.0f, 1.5f);
    }
    paintRivetCorners(g, outerR);

    // Header bar
    juce::Rectangle<float> hdr(0, 0, (float)getWidth(), 58.0f);
    juce::ColourGradient hdrGrad(juce::Colour(0xff1c1c2c), 0, 0,
                                  juce::Colour(0xff0d0d18), 0, 58, false);
    g.setGradientFill(hdrGrad);
    g.fillRect(hdr);

    // Header border bottom
    g.setColour(NEON_GRN.withAlpha(0.7f));
    g.drawLine(4, 58, (float)getWidth() - 4, 58, 2.0f);

    // "POGO LOCO" - neon pink
    drawNeonText(g, "POGO LOCO", { 12.0f, 4.0f, 200.0f, 50.0f },
                 NEON_PNK, 30.0f, juce::Font::bold | juce::Font::italic,
                 juce::Justification::centredLeft);

    // "THE BOUNCE ZONE" - neon green centre
    drawNeonText(g, "THE BOUNCE ZONE", { 160.0f, 4.0f, 320.0f, 50.0f },
                 NEON_GRN, 22.0f, juce::Font::bold,
                 juce::Justification::centred);

    // "v1.0" small
    g.setColour(NEON_GRN.withAlpha(0.45f));
    g.setFont(juce::Font(10.0f));
    g.drawText("v1.0", 200, 8, 40, 14, juce::Justification::left);

    // INPUT / OUTPUT labels
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.setColour(NEON_YLW.withAlpha(0.75f));
    g.drawText("INPUT",  12, 42, 60, 12, juce::Justification::left);
    g.drawText("OUTPUT", 490, 42, 70, 12, juce::Justification::right);

    // Left panel
    paintPanel(g, { 6.0f, 64.0f, 154.0f, 330.0f }, NEON_GRN.withAlpha(0.4f));
    paintRivetCorners(g, { 6.0f, 64.0f, 154.0f, 330.0f });

    // Right panel
    paintPanel(g, { 480.0f, 64.0f, 154.0f, 330.0f }, NEON_GRN.withAlpha(0.4f));
    paintRivetCorners(g, { 480.0f, 64.0f, 154.0f, 330.0f });

    // CRT canvas frame
    juce::Rectangle<float> crt(163.0f, 64.0f, 314.0f, 330.0f);
    // Dark bezel
    g.setColour(juce::Colour(0xff030d06));
    g.fillRoundedRectangle(crt, 10.0f);
    // Multi-layer neon border
    for (int i = 0; i < 3; ++i)
    {
        g.setColour(NEON_GRN.withAlpha(0.8f - i * 0.2f));
        g.drawRoundedRectangle(crt.reduced((float)i * 1.2f), 10.0f - i * 0.5f, 2.0f - i * 0.4f);
    }
    // Scanline overlay
    for (float y = crt.getY(); y < crt.getBottom(); y += 4.0f)
    {
        g.setColour(juce::Colours::black.withAlpha(0.08f));
        g.fillRect(crt.getX() + 2, y, crt.getWidth() - 4, 2.0f);
    }

    // Canvas labels
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(NEON_GRN.withAlpha(0.9f));
    g.drawText("PITCH CURVE", (int)crt.getX() + 8, (int)crt.getY() + 5, 120, 14,
               juce::Justification::left);
    g.setColour(NEON_GRN.withAlpha(0.4f));
    g.drawText("BOUNCE FACTOR", (int)crt.getRight() - 115, (int)crt.getBottom() - 18, 110, 14,
               juce::Justification::right);

    // Bottom bar
    juce::Rectangle<float> bot(0, 398.0f, (float)getWidth(), (float)getHeight() - 398);
    juce::ColourGradient botGrad(juce::Colour(0xff141428), 0, 398,
                                  juce::Colour(0xff0a0a1a), 0, (float)getHeight(), false);
    g.setGradientFill(botGrad);
    g.fillRect(bot);
    g.setColour(NEON_GRN.withAlpha(0.4f));
    g.drawLine(4, 398, (float)getWidth() - 4, 398, 1.5f);
}

// ── resized ────────────────────────────────────────────────────────────────
void PogoAudioProcessorEditor::resized()
{
    // Canvas inner area
    curveEditor.setBounds(170, 82, 300, 304);

    // Left column
    feedMeBtn.setBounds(18,  78, 130, 80);
    fileLabel.setBounds(12, 162, 150, 32);
    chillBtn .setBounds(18, 200, 130, 50);

    // Right column
    tensionKnob   .setBounds(492,  78, 130, 120);
    tensionValLabel.setBounds(490, 200,  134,  36);
    tensionLabel  .setBounds(488, 240,  138,  34);

    // Bottom preset row + YEET
    presetDrop .setBounds(  8, 406, 112, 46);
    presetRise .setBounds(122, 406, 112, 46);
    presetSweep.setBounds(236, 406, 112, 46);
    presetTape .setBounds(350, 406, 112, 46);
    yeetBtn    .setBounds(466, 406, 166, 46);
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
    fileLabel.setColour(juce::Label::textColourId, NEON_GRN);
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
            fileLabel.setColour(juce::Label::textColourId, NEON_GRN);
        });
}

// ── YEET ──────────────────────────────────────────────────────────────────
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
