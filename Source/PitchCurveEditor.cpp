#include "PitchCurveEditor.h"

static const juce::Colour BG      { 0xff001208 };   // CRT dark green
static const juce::Colour GRID    { 0xff003318 };   // CRT grid
static const juce::Colour ZEROLINE{ 0xff006633 };   // brighter zero
static const juce::Colour CURVE   { 0xffff44ff };   // neon magenta/pink
static const juce::Colour FILL    { 0x33ff44ff };   // translucent pink fill

PitchCurveEditor::PitchCurveEditor()
{
    curve.fill(0.0f);
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
}

juce::Rectangle<float> PitchCurveEditor::canvasBounds() const
{
    return getLocalBounds().toFloat().reduced(1.0f);
}

int PitchCurveEditor::xToIndex(float x) const
{
    auto b = canvasBounds();
    float t = (x - b.getX()) / b.getWidth();
    return juce::jlimit(0, CURVE_SIZE - 1, (int)(t * CURVE_SIZE));
}

float PitchCurveEditor::yToSemitones(float y) const
{
    auto b = canvasBounds();
    float rel = (y - b.getCentreY()) / (b.getHeight() * 0.5f);
    return juce::jlimit(-SEMI_MAX, SEMI_MAX, -rel * SEMI_MAX);
}

float PitchCurveEditor::semitonesToY(float semi) const
{
    auto b = canvasBounds();
    return b.getCentreY() - (semi / SEMI_MAX) * (b.getHeight() * 0.5f);
}

void PitchCurveEditor::paint(juce::Graphics& g)
{
    auto b = canvasBounds();

    // Background (CRT phosphor)
    g.setColour(BG);
    g.fillRect(b);

    // Horizontal grid lines at ±6, ±12, ±18, ±24 semitones
    for (float semi : { -24.0f, -18.0f, -12.0f, -6.0f, 6.0f, 12.0f, 18.0f, 24.0f })
    {
        float y = semitonesToY(semi);
        g.setColour(GRID);
        g.drawHorizontalLine((int)y, b.getX(), b.getRight());

        // Labels
        g.setColour(GRID.brighter(0.5f));
        g.setFont(juce::Font(9.0f));
        g.drawText(juce::String((int)semi), (int)b.getX() + 3, (int)y - 8, 25, 16,
                   juce::Justification::left);
    }

    // Vertical grid every 25%
    for (float t : { 0.25f, 0.5f, 0.75f })
    {
        float x = b.getX() + t * b.getWidth();
        g.setColour(GRID);
        g.drawVerticalLine((int)x, b.getY(), b.getBottom());
    }

    // Zero line
    float zeroY = semitonesToY(0.0f);
    g.setColour(ZEROLINE);
    g.drawHorizontalLine((int)zeroY, b.getX(), b.getRight());

    // Fill under curve
    juce::Path fillPath;
    fillPath.startNewSubPath(b.getX(), zeroY);
    for (int i = 0; i < CURVE_SIZE; ++i)
    {
        float x = b.getX() + (float)i / (CURVE_SIZE - 1) * b.getWidth();
        float y = semitonesToY(curve[i]);
        fillPath.lineTo(x, y);
    }
    fillPath.lineTo(b.getRight(), zeroY);
    fillPath.closeSubPath();
    g.setColour(FILL);
    g.fillPath(fillPath);

    // Curve line
    juce::Path curvePath;
    for (int i = 0; i < CURVE_SIZE; ++i)
    {
        float x = b.getX() + (float)i / (CURVE_SIZE - 1) * b.getWidth();
        float y = semitonesToY(curve[i]);
        if (i == 0) curvePath.startNewSubPath(x, y);
        else        curvePath.lineTo(x, y);
    }
    g.setColour(CURVE);
    g.strokePath(curvePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));

    // No inner border — outer frame drawn by PluginEditor

    // Labels
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(juce::Font(10.0f));
    g.drawText("START",  (int)b.getX() + 4,  (int)b.getBottom() - 14, 40, 12,
               juce::Justification::left);
    g.drawText("END",    (int)b.getRight()-40, (int)b.getBottom() - 14, 40, 12,
               juce::Justification::right);
    g.drawText("+24st",  (int)b.getX() + 4,  (int)b.getY() + 2,    40, 12,
               juce::Justification::left);
    g.drawText("-24st",  (int)b.getX() + 4,  (int)b.getBottom()-26, 40, 12,
               juce::Justification::left);
}

void PitchCurveEditor::mouseDown(const juce::MouseEvent& e)
{
    lastDragX = e.position.x;
    lastDragY = e.position.y;
    int idx = xToIndex(e.position.x);
    curve[idx] = yToSemitones(e.position.y);
    repaint();
    if (onCurveChanged) onCurveChanged();
}

void PitchCurveEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (lastDragX < 0.0f) { lastDragX = e.position.x; lastDragY = e.position.y; }

    int idxA = xToIndex(lastDragX);
    int idxB = xToIndex(e.position.x);
    int lo   = std::min(idxA, idxB);
    int hi   = std::max(idxA, idxB);

    // Interpolate semitone values along the drag path
    for (int i = lo; i <= hi; ++i)
    {
        float t = (hi == lo) ? 0.5f : (float)(i - lo) / (hi - lo);
        float y = lastDragY + t * (e.position.y - lastDragY);
        curve[juce::jlimit(0, CURVE_SIZE - 1, i)] = yToSemitones(y);
    }

    lastDragX = e.position.x;
    lastDragY = e.position.y;
    repaint();
    if (onCurveChanged) onCurveChanged();
}

void PitchCurveEditor::mouseUp(const juce::MouseEvent&)
{
    lastDragX = -1.0f;
}

void PitchCurveEditor::resetFlat()
{
    curve.fill(0.0f);
    repaint();
    if (onCurveChanged) onCurveChanged();
}

void PitchCurveEditor::setPreset(const std::string& name)
{
    for (int i = 0; i < CURVE_SIZE; ++i)
    {
        float t = (float)i / (CURVE_SIZE - 1);   // 0 → 1
        if (name == "drop")
            curve[i] = SEMI_MAX * (1.0f - t) * (1.0f - t) - SEMI_MAX * t * t * 0.3f;
        else if (name == "rise")
            curve[i] = -SEMI_MAX * (1.0f - t) * (1.0f - t) * 0.3f + SEMI_MAX * t * t;
        else if (name == "tape-drop")
            curve[i] = SEMI_MAX * std::exp(-4.0f * t) - SEMI_MAX * 0.018f;
        else if (name == "tape-rise")
            curve[i] = -SEMI_MAX * std::exp(-4.0f * (1.0f - t)) + SEMI_MAX * 0.018f;
        else if (name == "sweep")
            curve[i] = SEMI_MAX * std::cos(t * juce::MathConstants<float>::pi);
        else
            curve[i] = 0.0f;
    }
    repaint();
    if (onCurveChanged) onCurveChanged();
}
