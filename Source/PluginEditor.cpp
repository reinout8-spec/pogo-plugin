#include "PluginEditor.h"

static const juce::Colour BG_DARK   { 0xff0f0f1a };
static const juce::Colour BG_PANEL  { 0xff1c1c30 };
static const juce::Colour ACCENT    { 0xff6c63ff };
static const juce::Colour TEXT_MAIN { 0xffffffff };
static const juce::Colour TEXT_DIM  { 0xff888899 };

PogoAudioProcessorEditor::PogoAudioProcessorEditor(PogoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(300, 340);

    // ── Pogo Knob ──────────────────────────────────────────────────────
    pogoKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    pogoKnob.setRange(-1.0, 1.0);
    pogoKnob.setValue(0.0);
    pogoKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 22);
    pogoKnob.setColour(juce::Slider::rotarySliderFillColourId, ACCENT);
    pogoKnob.setColour(juce::Slider::textBoxTextColourId,      TEXT_DIM);
    pogoKnob.setColour(juce::Slider::textBoxOutlineColourId,   juce::Colours::transparentBlack);
    addAndMakeVisible(pogoKnob);

    pogoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "pogo", pogoKnob);

    pogoLabel.setText("POGO", juce::dontSendNotification);
    pogoLabel.setJustificationType(juce::Justification::centred);
    pogoLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    pogoLabel.setColour(juce::Label::textColourId, TEXT_DIM);
    addAndMakeVisible(pogoLabel);

    // ── File Label ─────────────────────────────────────────────────────
    fileLabel.setText("Drop sample here", juce::dontSendNotification);
    fileLabel.setJustificationType(juce::Justification::centred);
    fileLabel.setFont(juce::Font(11.5f));
    fileLabel.setColour(juce::Label::textColourId, TEXT_DIM);
    addAndMakeVisible(fileLabel);

    // ── Load Button ────────────────────────────────────────────────────
    loadButton.setColour(juce::TextButton::buttonColourId,   BG_PANEL);
    loadButton.setColour(juce::TextButton::textColourOffId,  ACCENT);
    loadButton.setColour(juce::TextButton::buttonOnColourId, ACCENT);
    loadButton.onClick = [this] { openFileChooser(); };
    addAndMakeVisible(loadButton);
}

PogoAudioProcessorEditor::~PogoAudioProcessorEditor() {}

void PogoAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(BG_DARK);

    // Header
    auto header = getLocalBounds().removeFromTop(56);
    g.setColour(BG_PANEL);
    g.fillRect(header);

    g.setColour(TEXT_MAIN);
    g.setFont(juce::Font(28.0f, juce::Font::bold));
    g.drawText("POGO", header, juce::Justification::centred);

    // Accent underline
    g.setColour(ACCENT);
    g.fillRect(0, 55, getWidth(), 2);

    // Drop zone
    auto dz = juce::Rectangle<int>(20, 70, getWidth() - 40, 56);
    g.setColour(BG_PANEL);
    g.fillRoundedRectangle(dz.toFloat(), 8.0f);
    g.setColour(ACCENT.withAlpha(0.5f));
    g.drawRoundedRectangle(dz.toFloat(), 8.0f, 1.2f);
}

void PogoAudioProcessorEditor::resized()
{
    // Drop zone + file label  (y=70, h=56)
    fileLabel.setBounds(20, 70, getWidth() - 40, 56);

    // Load button
    loadButton.setBounds((getWidth() - 120) / 2, 138, 120, 28);

    // Pogo label
    pogoLabel.setBounds(0, 178, getWidth(), 20);

    // Knob
    pogoKnob.setBounds((getWidth() - 150) / 2, 196, 150, 130);
}

bool PogoAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
    {
        juce::File file(f);
        auto ext = file.getFileExtension().toLowerCase();
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
    fileLabel.setColour(juce::Label::textColourId, TEXT_MAIN);
}

void PogoAudioProcessorEditor::openFileChooser()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Load Sample", juce::File{},
        "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");

    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                         juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            if (fc.getResults().isEmpty()) return;
            auto file = fc.getResult();
            audioProcessor.loadSample(file);
            fileLabel.setText(file.getFileName(), juce::dontSendNotification);
            fileLabel.setColour(juce::Label::textColourId, TEXT_MAIN);
        });
}
