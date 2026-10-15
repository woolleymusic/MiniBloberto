#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Brain.h"
#include "BeatTracker.h"

// Audio passes straight through. The processor only watches: the host transport
// (so he dances on the grid), the input peak (so Input mode can follow the track), and a beat
// tracker on the input, which is his clock when there is no playing transport (OBS).
class MiniBlobertoProcessor : public juce::AudioProcessor
{
public:
    MiniBlobertoProcessor();

    void prepareToPlay (double sampleRate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Mini Bloberto"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    mb::Settings settings() const;
    int colour() const { return (int) params.getRawParameterValue ("colour")->load(); }
    int choice (const char* id) const { return (int) params.getRawParameterValue (id)->load(); }
    bool flag (const char* id) const { return params.getRawParameterValue (id)->load() > 0.5f; }

    // Fruity Dance style: after "Let him dance" the window is just him. Saved with the
    // session, so a reopened project (or OBS scene) comes back as just him.
    bool dancerView() const;
    void setDancerView (bool);

    // transport as of now, extrapolated from the last block on the message thread
    mb::Transport transportNow() const;
    float takePeak() { return peakSinceRead.exchange (0.0f); }

    juce::AudioProcessorValueTreeState params;

    // headless harness / Standalone without a host clock
    void setFakeTransport (bool playing, double ppq, double bpm);
    const BeatTracker& tracker() const { return beats; }

private:
    std::atomic<bool> playing { false };
    std::atomic<double> blockPpq { 0.0 }, bpm { 120.0 };
    std::atomic<double> blockTimeMs { 0.0 };
    std::atomic<int> num { 4 }, den { 4 };
    std::atomic<float> peakSinceRead { 0.0f };
    BeatTracker beats;
    std::vector<float> mono;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniBlobertoProcessor)
};
