#pragma once

#include <juce_dsp/juce_dsp.h>

// Real-time beat tracker for audio with no host clock (OBS, a DAW with the transport stopped).
// Ported from the Bloberto companion app; here push() runs on the AUDIO thread, so nothing in
// it allocates: every buffer is sized in prepare().
//
//   onset strength: log-compressed spectral flux, 1024 FFT, hop 512
//   tempo: autocorrelation of the last 8 s of onsets, 70..180 BPM, weighted toward 120,
//          and only switched after the new tempo wins several estimates in a row
//   phase: comb filter over the last 8 beats; a soft phase-locked loop pulls the running
//          beat counter toward it, so beats never jump
//
// push() runs on the audio/capture thread. beats() / bpm() / active() are safe from any thread.
class BeatTracker
{
public:
    BeatTracker();

    void prepare (double sampleRate);
    void push (const float* mono, int numSamples);   // capture thread

    // beat counter (1.0 per beat), extrapolated to now; drives the dance like a host's PPQ
    double beats() const;
    double bpm() const { return tempo.load(); }
    // music playing and tempo locked. Also false when no audio has ARRIVED for half a second:
    // Windows loopback sends nothing at all while the computer is silent.
    bool active() const
    {
        return isActive.load() && juce::Time::getMillisecondCounterHiRes() - lastHopMs.load() < 500.0;
    }
    float peak() { return peakSinceRead.exchange (0.0f); }

    // offline use (BeatTest): advance by samples, not wall clock
    double beatsAtLastHop() const { return beatPos.load(); }

private:
    void hop();
    void estimateTempo();
    double measuredPhase (double periodFrames) const;   // beats since the last beat, 0..1

    static constexpr int fftOrder = 10, fftSize = 1 << fftOrder, hopSize = 512;
    static constexpr int envSeconds = 8;

    double sr = 48000.0, frameRate = 48000.0 / hopSize;
    juce::dsp::FFT fft { fftOrder };
    std::vector<float> window, fifo, fftBuf, prevMag;
    int fifoPos = 0;

    std::vector<float> env;          // onset strength ring, envLen frames
    std::vector<float> scratchEnv;   // estimateTempo work buffers, sized in prepare()
    std::vector<double> scratchAc;
    int envLen = 0, envPos = 0;
    long long framesSeen = 0;

    float rmsSlow = 0.0f;
    int quietFrames = 0;

    double period = 0.0;             // frames per beat, 0 = no tempo yet
    double candidate = 0.0;
    int candidateVotes = 0;
    int framesSinceEstimate = 0;

    std::atomic<double> beatPos { 0.0 }, tempo { 0.0 };
    std::atomic<double> lastHopMs { 0.0 };
    std::atomic<bool> isActive { false };
    std::atomic<float> peakSinceRead { 0.0f };
};
