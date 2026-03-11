#pragma once
#include <JuceHeader.h>
#include <array>
#include <functional>

class PitchCurveEditor : public juce::Component
{
public:
    static constexpr int   CURVE_SIZE = 256;
    static constexpr float SEMI_MAX   = 24.0f;

    std::array<float, CURVE_SIZE> curve {};   // semitone values per position
    std::function<void()> onCurveChanged;

    PitchCurveEditor();
    ~PitchCurveEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override {}

    void mouseDown(const juce::MouseEvent&)  override;
    void mouseDrag(const juce::MouseEvent&)  override;
    void mouseUp  (const juce::MouseEvent&)  override;

    void resetFlat();
    void setPreset(const std::string& name); // "drop" | "rise" | "tape-drop" | "tape-rise"

private:
    float lastDragX = -1.0f;
    float lastDragY =  0.0f;

    juce::Rectangle<float> canvasBounds() const;
    int   xToIndex   (float x)    const;
    float yToSemitones(float y)   const;
    float semitonesToY(float semi) const;
    void  paintCurvePoint(float x, float y);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchCurveEditor)
};
