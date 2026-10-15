#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
constexpr int cellW = 110, cellH = 128, ground = 116;   // native sprite cell; feet sit on row 116
constexpr int reflH = 36;                               // native rows of floor reflection
constexpr int previewPx = 3;                            // setup preview is always 3x
constexpr int panelW = 280, W = panelW + cellW * previewPx + 36, H = 568;

const juce::Colour bg { 0xff24242c }, panel { 0xff1c1b23 }, ink { 0xff2a1838 },
    lavender { 0xffbf9ae2 }, lavLight { 0xffecdcfa }, dim { 0xff8d84a0 };

// one accent per colour, for the title and the active controls
const juce::Colour accents[] { juce::Colour (0xffbf9ae2), juce::Colour (0xff7fa4ec),
                               juce::Colour (0xff8f90e4), juce::Colour (0xffdda4e2) };

// what he stands on: the dark theme, or flat chroma-key colours for OBS / video
const juce::Colour backdrops[] { juce::Colour (0xff1f1f26), juce::Colour (0xff00b140),
                                 juce::Colour (0xffff00ff), juce::Colour (0xff0047bb) };

// the face keeps its colours when Rainbow shifts the body (sprite.py FACE_DARK MOUTH TONGUE BLUSH)
bool isFace (juce::Colour c)
{
    const auto rgb = c.getARGB() & 0xffffff;
    return rgb == 0x3a1834 || rgb == 0x6e223c || rgb == 0xec6880 || rgb == 0xf79cc4 || rgb == 0xffffff;
}

const int sizes[] { 2, 3, 4 };
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
    lnf.setColour (juce::ToggleButton::textColourId, lavLight);
    lnf.setColour (juce::ToggleButton::tickColourId, lavender);
    lnf.setColour (juce::ToggleButton::tickDisabledColourId, dim);
    lnf.setColour (juce::TextButton::buttonColourId, lavender);
    lnf.setColour (juce::TextButton::textColourOffId, ink);
    setLookAndFeel (&lnf);

    const char* ids[] { "mode", "moves", "pick", "speed", "bars", "colour", "flip", "size", "backdrop" };
    const char* names[] { "Mode", "Moves", "Pick", "Speed", "Bars", "Colour", "Flip", "Size", "Backdrop" };
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
        r.box.onChange = [this] { repaint(); };
    }

    const char* trickIds[] { "reverse", "shake", "trails", "reflect", "rainbow" };
    const char* trickNames[] { "Reverse", "Shake", "Trails", "Reflection", "Rainbow" };
    for (size_t i = 0; i < tricks.size(); ++i)
    {
        auto& t = tricks[i];
        t.button.setButtonText (trickNames[i]);
        addAndMakeVisible (t.button);
        t.attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.params, trickIds[i], t.button);
        t.button.onClick = [this] { repaint(); };
    }

    go.onClick = [this] { showDancer (true); };
    addAndMakeVisible (go);

    showDancer (proc.dancerView());
#if ! MB_HEADLESS
    startTimerHz (30);
#endif
}

MiniBlobertoEditor::~MiniBlobertoEditor()
{
    setLookAndFeel (nullptr);
}

int MiniBlobertoEditor::nativeHeight() const
{
    return cellH + (proc.flag ("reflect") ? reflH : 0);
}

juce::Colour MiniBlobertoEditor::backdrop() const
{
    return backdrops[juce::jlimit (0, 3, proc.choice ("backdrop"))];
}

juce::Rectangle<int> MiniBlobertoEditor::stage() const
{
    if (dancer)
        return getLocalBounds();
    const int h = nativeHeight() * previewPx;
    return { panelW + 18, juce::jmax (18, (H - h) / 2), cellW * previewPx, h };
}

void MiniBlobertoEditor::showDancer (bool on)
{
    dancer = on;
    proc.setDancerView (on);
    for (auto& r : rows)
    {
        r.label.setVisible (! on);
        r.box.setVisible (! on);
    }
    for (auto& t : tricks)
        t.button.setVisible (! on);
    go.setVisible (! on);

    if (on)
    {
        const int px = sizes[juce::jlimit (0, 2, proc.choice ("size"))];
        setSize (cellW * px, nativeHeight() * px);
    }
    else
        setSize (W, H);
    repaint();
}

void MiniBlobertoEditor::advance (double dt)
{
    auto next = brain.tick (proc.settings(), proc.transportNow(), proc.takePeak(), dt);

    if (pressed)
        pressClock += dt;
    if (pressed && ! held && pressClock > 0.22)   // a press that lasts is a pick-up, not a click
        held = true;

    heldClock = held ? heldClock + dt : 0.0;
    if (held)
        next = { mb::Held, (int) (heldClock * 8.0) % 8, false, false, (juce::int64) (heldClock * 8.0) };

    // Size or Reflection changed while it's just him: follow it
    if (dancer)
    {
        const int px = sizes[juce::jlimit (0, 2, proc.choice ("size"))];
        if (getWidth() != cellW * px || getHeight() != nativeHeight() * px)
            setSize (cellW * px, nativeHeight() * px);
    }

    if (next.row != cell.row || next.frame != cell.frame || next.flip != cell.flip || next.step != cell.step)
    {
        cell = next;
        repaint (stage());
    }
}

juce::Image MiniBlobertoEditor::cellImage (int row, int frame) const
{
    const auto& sheet = sheets[juce::jlimit (0, 3, proc.colour())];
    auto img = sheet.getClippedImage ({ frame * cellW, row * cellH, cellW, cellH }).createCopy();

    if (proc.flag ("rainbow") && proc.settings().mode == mb::Dance)
    {
        // whole-palette hue step on every beat (FAOSDance's colour overlay, but per pixel so
        // the outline and shading survive and the face keeps its colours)
        const float hue = (float) ((cell.step / 4) % 8) / 8.0f;
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::readWrite);
        for (int y = 0; y < bd.height; ++y)
            for (int x = 0; x < bd.width; ++x)
            {
                auto c = bd.getPixelColour (x, y);
                if (c.getAlpha() == 255 && ! isFace (c))
                    bd.setPixelColour (x, y, c.withRotatedHue (hue));
            }
    }
    return img;
}

juce::Image MiniBlobertoEditor::composeNative() const
{
    const int nh = nativeHeight();
    juce::Image out (juce::Image::ARGB, cellW, nh, true);
    {
        juce::Graphics g (out);

        // Shake: whole native pixels, a new nudge each step, harder when the track is loud
        int sx = 0, sy = 0;
        if (proc.flag ("shake") && ! held && proc.settings().mode == mb::Dance)
        {
            juce::Random r (cell.step * 7919 + 17);
            const int amt = brain.level() > 0.7 ? 2 : 1;
            sx = r.nextInt ({ -amt, amt + 1 });
            sy = r.nextInt ({ -amt, amt + 1 });
        }

        auto place = juce::AffineTransform::translation ((float) sx, (float) sy);
        if (cell.flip)
            place = juce::AffineTransform::scale (-1.0f, 1.0f).translated ((float) (cellW + sx), (float) sy);

        // Trails: the two frames he just left, faint, behind him
        if (proc.flag ("trails") && ! held && proc.settings().mode == mb::Dance)
        {
            const int dir = cell.reverse ? 1 : -1;
            const float alpha[] { 0.14f, 0.28f };
            for (int k = 2; k >= 1; --k)
            {
                g.setOpacity (alpha[2 - k]);
                g.drawImageTransformed (cellImage (cell.row, (cell.frame + dir * k + 16) % 8), place);
            }
            g.setOpacity (1.0f);
        }
        g.drawImageTransformed (cellImage (cell.row, cell.frame), place);
    }

    // Reflection: the solid pixels mirrored about the ground line, fading in 3-row bands
    if (nh > cellH)
    {
        juce::Image::BitmapData bd (out, juce::Image::BitmapData::readWrite);
        for (int i = 1; i <= reflH; ++i)
        {
            const int dy = ground + i, sy = ground + 1 - i;
            if (sy < 0 || dy >= nh)
                continue;
            const float fade = 0.32f * (1.0f - (float) ((i - 1) / 3 * 3) / (float) reflH);
            for (int x = 0; x < cellW; ++x)
            {
                auto src = bd.getPixelColour (x, sy);
                if (src.getAlpha() < 255)
                    continue;
                auto dst = bd.getPixelColour (x, dy);
                bd.setPixelColour (x, dy, dst.interpolatedWith (src.withAlpha (1.0f), fade).withAlpha (juce::jmax (dst.getFloatAlpha(), fade)));
            }
        }
    }
    return out;
}

void MiniBlobertoEditor::paint (juce::Graphics& g)
{
    const int c = juce::jlimit (0, 3, proc.colour());
    const auto s = stage();
    const auto him = composeNative();

    if (dancer)
    {
        g.fillAll (backdrop());
        g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);   // nearest neighbour
        g.drawImage (him, s.getX(), s.getY(), s.getWidth(), s.getHeight(), 0, 0, him.getWidth(), him.getHeight());
        return;
    }

    g.fillAll (bg);

    // control panel
    g.setColour (panel);
    g.fillRect (0, 0, panelW, getHeight());
    g.setColour (accents[c]);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("MINI BLOBERTO", 18, 14, panelW - 24, 28, juce::Justification::left);
    g.setColour (dim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("by Woolley", 18, 40, panelW - 24, 16, juce::Justification::left);

    g.setColour (accents[c]);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("TRICKS", 18, 370, panelW - 24, 16, juce::Justification::left);

    g.setColour (dim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawFittedText ("Then it's just him. Click him to come back here.\nHold him to pick him up.",
                      18, 528, panelW - 30, 32, juce::Justification::topLeft, 2);

    // preview: him, crisp, at 3x, on the chosen backdrop
    g.setColour (backdrop());
    g.fillRoundedRectangle (s.toFloat().expanded (6), 10.0f);
    g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
    g.drawImage (him, s.getX(), s.getY(), s.getWidth(), s.getHeight(), 0, 0, him.getWidth(), him.getHeight());
}

void MiniBlobertoEditor::resized()
{
    int y = 66;
    for (auto& r : rows)
    {
        r.label.setBounds (14, y, 72, 26);
        r.box.setBounds (88, y, panelW - 104, 26);
        y += 33;
    }
    for (size_t i = 0; i < tricks.size(); ++i)
        tricks[i].button.setBounds (12 + (int) (i % 2) * 128, 390 + (int) (i / 2) * 28, 124, 26);
    go.setBounds (14, 480, panelW - 30, 40);
}

// Fruity Dance tradition: hold the mouse on him and he dangles.
// In the dancer view a quick click (no drag) brings the settings back instead.
void MiniBlobertoEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! stage().contains (e.getPosition()))
        return;
    if (dancer)
    {
        pressed = true;
        dragged = false;
        pressClock = 0.0;
    }
    else
        held = true;
}

void MiniBlobertoEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (pressed && e.getDistanceFromDragStart() > 4)
    {
        dragged = true;
        held = true;
    }
}

void MiniBlobertoEditor::mouseUp (const juce::MouseEvent&)
{
    const bool click = dancer && pressed && ! held && ! dragged;
    pressed = dragged = held = false;
    if (click)
        showDancer (false);
}
