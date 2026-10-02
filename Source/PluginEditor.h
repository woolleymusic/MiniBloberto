#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class MiniBlobertoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MiniBlobertoEditor (MiniBlobertoProcessor&);
    ~MiniBlobertoEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    // one animation step (the timer calls this; the snapshot harness calls it directly)
    void advance (double dt);
    mb::Cell currentCell() const { return cell; }

private:
    void timerCallback() override { advance (1.0 / 30.0); }
    juce::Rectangle<int> stage() const;

    MiniBlobertoProcessor& proc;
    mb::Brain brain;
    mb::Cell cell;
    bool held = false;
    double heldClock = 0.0;
    juce::Image sheets[4];

    struct Row
    {
        juce::Label label;
        juce::ComboBox box;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
    };
    std::array<Row, 6> rows;

    juce::LookAndFeel_V4 lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniBlobertoEditor)
};
