#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// Two views, like Fruity Dance:
//   setup  - the settings menu with a live preview and a "Let him dance" button
//   dancer - the window shrinks to just him. Click him: back to setup. Hold him: he dangles.
class MiniBlobertoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MiniBlobertoEditor (MiniBlobertoProcessor&);
    ~MiniBlobertoEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    // one animation step (the timer calls this; the snapshot harness calls it directly)
    void advance (double dt);
    mb::Cell currentCell() const { return cell; }

    // him at native pixels (110 wide), every trick applied; scale by whole numbers only
    juce::Image composeNative() const;
    void showDancer (bool);
    bool isDancer() const { return dancer; }

private:
    void timerCallback() override { advance (1.0 / 30.0); }
    juce::Rectangle<int> stage() const;
    int nativeHeight() const;
    juce::Image cellImage (int row, int frame) const;
    juce::Colour backdrop() const;

    MiniBlobertoProcessor& proc;
    mb::Brain brain;
    mb::Cell cell;
    bool dancer = false;
    bool held = false, pressed = false, dragged = false;
    double heldClock = 0.0, pressClock = 0.0;
    juce::Image sheets[4];

    struct Row
    {
        juce::Label label;
        juce::ComboBox box;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
    };
    std::array<Row, 9> rows;

    struct Trick
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attach;
    };
    std::array<Trick, 5> tricks;

    juce::TextButton go { "LET HIM DANCE" };
    juce::LookAndFeel_V4 lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniBlobertoEditor)
};
