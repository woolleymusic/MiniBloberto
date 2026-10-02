#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
constexpr int cellW = 110, cellH = 128, px = 3;   // native sprite cell, drawn at 3x
constexpr int panelW = 220, W = panelW + cellW * px + 24, H = cellH * px + 24;

const juce::Colour bg { 0xff24242c }, panel { 0xff1c1b23 }, ink { 0xff2a1838 },
    lavender { 0xffbf9ae2 }, lavLight { 0xffecdcfa }, dim { 0xff8d84a0 };

// one accent per colour, for the title and the active controls
const juce::Colour accents[] { juce::Colour (0xffbf9ae2), juce::Colour (0xff7fa4ec),
                               juce::Colour (0xff8f90e4), juce::Colour (0xffdda4e2) };
} // namespace

MiniBlobertoEditor::MiniBlobertoEditor (MiniBlobertoProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    sheets[0] = juce::ImageCache::getFromMemory (BinaryData::Bloberto_Lavender_png, BinaryData::Bloberto_Lavender_pngSize);
    sheets[1] = juce::ImageCache::getFromMemory (BinaryData::Bloberto_Bluebell_png, BinaryData::Bloberto_Bluebell_pngSize);
    sheets[2] = juce::ImageCache::getFromMemory (BinaryData::Bloberto_Periwinkle_png, BinaryData::Bloberto_Periwinkle_pngSize);
    sheets[3] = juce::ImageCache::getFromMemory (BinaryData::Bloberto_Lilac_png, BinaryData::Bloberto_Lilac_pngSize);

    lnf.setColourScheme ({ panel, bg, bg, dim, lavLight, lavender, ink, lavender, lavLight });
    lnf.setColour (juce::ComboBox::backgroundColourId, bg);
    lnf.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3a3448));
    lnf.setColour (juce::ComboBox::arrowColourId, lavender);
    lnf.setColour (juce::ComboBox::textColourId, lavLight);
    lnf.setColour (juce::PopupMenu::backgroundColourId, panel);
    lnf.setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff3d2f55));
    lnf.setColour (juce::PopupMenu::textColourId, lavLight);
    setLookAndFeel (&lnf);

    const char* ids[] { "mode", "moves", "pick", "speed", "bars", "colour" };
    const char* names[] { "Mode", "Moves", "Pick", "Speed", "Bars", "Colour" };
    for (size_t i = 0; i < rows.size(); ++i)
    {
        auto& r = rows[i];
        r.label.setText (names[i], juce::dontSendNotification);
        r.label.setColour (juce::Label::textColourId, dim);
        r.label.setFont (juce::FontOptions (14.0f));
        addAndMakeVisible (r.label);

        auto* param = dynamic_cast<juce::AudioParameterChoice*> (proc.params.getParameter (ids[i]));
        r.box.addItemList (param->choices, 1);
        addAndMakeVisible (r.box);
        r.attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.params, ids[i], r.box);
    }
    rows[5].box.onChange = [this] { repaint(); };

    setSize (W, H);
#if ! MB_HEADLESS
    startTimerHz (30);
#endif
}

MiniBlobertoEditor::~MiniBlobertoEditor()
{
    setLookAndFeel (nullptr);
}

juce::Rectangle<int> MiniBlobertoEditor::stage() const
{
    return { panelW + 12, 12, cellW * px, cellH * px };
}

void MiniBlobertoEditor::advance (double dt)
{
    auto next = brain.tick (proc.settings(), proc.transportNow(), proc.takePeak(), dt);
    heldClock = held ? heldClock + dt : 0.0;
    if (held)
        next = { mb::Held, (int) (heldClock * 8.0) % 8 };
    if (next.row != cell.row || next.frame != cell.frame)
    {
        cell = next;
        repaint (stage());
    }
}

void MiniBlobertoEditor::paint (juce::Graphics& g)
{
    const int c = juce::jlimit (0, 3, proc.colour());
    g.fillAll (bg);

    // control panel
    g.setColour (panel);
    g.fillRect (0, 0, panelW, getHeight());
    g.setColour (accents[c]);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("MINI BLOBERTO", 18, 16, panelW - 24, 28, juce::Justification::left);
    g.setColour (dim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("by Woolley", 18, 42, panelW - 24, 16, juce::Justification::left);
    g.drawFittedText ("Pick: the move you choose.\nRandom: a new move every few bars.\n"
                      "Input: follows the track, still when quiet.\nGrab him to pick him up.",
                      18, getHeight() - 92, panelW - 30, 76, juce::Justification::topLeft, 5);

    // stage: him, crisp, at 3x
    const auto s = stage();
    g.setColour (juce::Colour (0xff1f1f26));
    g.fillRoundedRectangle (s.toFloat().expanded (6), 10.0f);
    g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);   // nearest neighbour
    g.drawImage (sheets[c], s.getX(), s.getY(), s.getWidth(), s.getHeight(),
                 cell.frame * cellW, cell.row * cellH, cellW, cellH);
}

void MiniBlobertoEditor::resized()
{
    int y = 74;
    for (auto& r : rows)
    {
        r.label.setBounds (14, y, 60, 26);
        r.box.setBounds (76, y, panelW - 92, 26);
        y += 36;
    }
}

// Fruity Dance tradition: hold the mouse on him and he dangles
void MiniBlobertoEditor::mouseDown (const juce::MouseEvent& e)
{
    if (stage().contains (e.getPosition()))
        held = true;
}

void MiniBlobertoEditor::mouseUp (const juce::MouseEvent&)
{
    held = false;
}
