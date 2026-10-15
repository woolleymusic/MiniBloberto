#include "BeatTracker.h"

BeatTracker::BeatTracker() { prepare (48000.0); }

void BeatTracker::prepare (double sampleRate)
{
    sr = sampleRate;
    frameRate = sr / hopSize;
    window.resize (fftSize);
    for (int i = 0; i < fftSize; ++i)
        window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) fftSize);
    fifo.assign (fftSize, 0.0f);
    fftBuf.assign (fftSize * 2, 0.0f);
    prevMag.assign (fftSize / 2, 0.0f);
    fifoPos = 0;
    envLen = (int) std::ceil (frameRate * envSeconds);
    env.assign ((size_t) envLen, 0.0f);
    scratchEnv.assign ((size_t) envLen, 0.0f);
    scratchAc.assign ((size_t) std::ceil (frameRate * 60.0 / 70.0) + 2, 0.0);
    envPos = 0;
    framesSeen = 0;
    period = candidate = 0.0;
    candidateVotes = 0;
    beatPos = 0.0;
    tempo = 0.0;
    isActive = false;
}

void BeatTracker::push (const float* x, int n)
{
    float pk = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        pk = juce::jmax (pk, std::abs (x[i]));
        // sliding window: keep the last fftSize samples, analyse every hopSize
        fifo[(size_t) fifoPos++] = x[i];
        if (fifoPos == fftSize)
        {
            hop();
            std::copy (fifo.begin() + hopSize, fifo.end(), fifo.begin());
            fifoPos = fftSize - hopSize;
        }
    }
    auto prev = peakSinceRead.load();
    while (pk > prev && ! peakSinceRead.compare_exchange_weak (prev, pk)) {}
}

void BeatTracker::hop()
{
    // ---- onset strength
    float energy = 0.0f;
    for (int i = 0; i < fftSize; ++i)
    {
        fftBuf[(size_t) i] = fifo[(size_t) i] * window[(size_t) i];
        energy += fifo[(size_t) i] * fifo[(size_t) i];
    }
    std::fill (fftBuf.begin() + fftSize, fftBuf.end(), 0.0f);
    fft.performFrequencyOnlyForwardTransform (fftBuf.data());

    float flux = 0.0f;
    const int maxBin = (int) (8000.0 / sr * fftSize);   // kicks, snares, hats live below 8 kHz
    for (int b = 1; b < maxBin; ++b)
    {
        const float m = std::log1p (100.0f * fftBuf[(size_t) b]);
        const float d = m - prevMag[(size_t) b];
        if (d > 0.0f)
            flux += b < maxBin / 16 ? 2.0f * d : d;          // weight the low end (kick)
        prevMag[(size_t) b] = m;
    }
    env[(size_t) envPos] = flux;
    envPos = (envPos + 1) % envLen;
    ++framesSeen;

    // ---- silence gate (~-50 dBFS RMS held for half a second)
    const float rms = std::sqrt (energy / fftSize);
    rmsSlow += (rms - rmsSlow) * 0.1f;
    quietFrames = rmsSlow < 0.003f ? quietFrames + 1 : 0;

    // ---- tempo, twice a second
    if (++framesSinceEstimate >= (int) (frameRate / 2) && framesSeen > (long long) (frameRate * 3))
    {
        framesSinceEstimate = 0;
        estimateTempo();
    }

    // ---- advance the beat counter, then nudge its phase toward what the comb filter hears
    if (period > 0.0)
    {
        double pos = beatPos.load() + 1.0 / period;
        const double heard = measuredPhase (period);       // beats since the last heard beat
        double mine = pos - std::floor (pos);
        double err = heard - mine;
        if (err > 0.5) err -= 1.0;
        if (err < -0.5) err += 1.0;
        pos += err * 0.04;                                   // gentle: ~25 frames to settle
        beatPos = pos;
    }

    isActive = period > 0.0 && quietFrames < (int) (frameRate / 2);
    lastHopMs = juce::Time::getMillisecondCounterHiRes();
}

void BeatTracker::estimateTempo()
{
    // mean-removed copy of the onset envelope, oldest first
    auto& e = scratchEnv;
    double mean = 0.0;
    for (int i = 0; i < envLen; ++i)
    {
        e[(size_t) i] = env[(size_t) ((envPos + i) % envLen)];
        mean += e[(size_t) i];
    }
    mean /= envLen;
    for (auto& v : e)
        v = (float) juce::jmax (0.0, v - mean);

    const int minLag = (int) std::floor (frameRate * 60.0 / 180.0);
    const int maxLag = (int) std::ceil (frameRate * 60.0 / 70.0);
    double best = 0.0;
    int bestLag = 0;
    auto& ac = scratchAc;
    std::fill (ac.begin(), ac.end(), 0.0);
    for (int lag = minLag; lag <= maxLag + 1; ++lag)
    {
        double s = 0.0;
        for (int i = lag; i < envLen; ++i)
            s += (double) e[(size_t) i] * e[(size_t) (i - lag)];
        ac[(size_t) lag] = s;
    }
    // a beat that also lines up at 2x the lag is stronger evidence; prefer ~120 BPM
    auto scoreAt = [&] (int lag)
    {
        if (lag < minLag || lag > maxLag)
            return 0.0;
        const double bpmHere = 60.0 * frameRate / lag;
        const double w = std::exp (-0.5 * std::pow (std::log2 (bpmHere / 120.0) / 0.9, 2.0));
        double twice = 0.0;
        if (2 * lag < (int) ac.size())
            twice = ac[(size_t) (2 * lag)];
        else
            for (int i = 2 * lag; i < envLen; ++i)
                twice += (double) e[(size_t) i] * e[(size_t) (i - 2 * lag)];
        return (ac[(size_t) lag] + 0.5 * twice) * w;
    };
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        const double score = scoreAt (lag);
        if (score > best)
        {
            best = score;
            bestLag = lag;
        }
    }
    if (bestLag == 0 || best <= 0.0)
        return;

    // parabolic refinement between neighbouring lags
    double lag = bestLag;
    {
        const double a = ac[(size_t) bestLag - 1], b = ac[(size_t) bestLag], c = ac[(size_t) bestLag + 1];
        const double den = a - 2 * b + c;
        if (std::abs (den) > 1e-12)
            lag += juce::jlimit (-0.5, 0.5, 0.5 * (a - c) / den);
    }

    if (period == 0.0)
    {
        period = lag;
    }
    else if (std::abs (lag - period) / period < 0.04)
    {
        period += (lag - period) * 0.3;                      // same tempo: refine
        candidateVotes = 0;
    }
    else
    {
        // a different tempo has to beat the current one CLEARLY (30%) for 4 s in a row before
        // we switch. Swung or triplet music scores close at 3:4 ratios; without this the
        // dance flips between them every few seconds.
        const bool clearlyBetter = best > 1.3 * scoreAt ((int) std::round (period));
        if (clearlyBetter && candidate > 0.0 && std::abs (lag - candidate) / candidate < 0.04)
            ++candidateVotes;
        else if (clearlyBetter)
        {
            candidate = lag;
            candidateVotes = 1;
        }
        else
            candidateVotes = 0;
        if (candidateVotes >= 8)
        {
            period = candidate;
            candidateVotes = 0;
        }
    }
    tempo = 60.0 * frameRate / period;
}

double BeatTracker::measuredPhase (double p) const
{
    // comb: for each candidate offset, sum the onsets at offset, offset+p, offset+2p ... back
    // through the last 8 beats (newer beats count more). Best offset = frames since last beat.
    const int P = (int) std::round (p);
    double best = -1.0;
    int bestOff = 0;
    for (int off = 0; off < P; ++off)
    {
        double s = 0.0, w = 1.0;
        for (int k = 0; k < 8; ++k)
        {
            const int back = (int) std::round (off + k * p);
            if (back >= envLen - 1)
                break;
            const int idx = ((envPos - 1 - back) % envLen + envLen) % envLen;
            s += w * env[(size_t) idx];
            w *= 0.85;
        }
        if (s > best)
        {
            best = s;
            bestOff = off;
        }
    }
    return (double) bestOff / p;
}

double BeatTracker::beats() const
{
    double pos = beatPos.load();
    const double t = tempo.load();
    if (t > 0.0)
    {
        const auto ms = juce::jlimit (0.0, 250.0, juce::Time::getMillisecondCounterHiRes() - lastHopMs.load());
        pos += ms / 60000.0 * t;
    }
    return pos;
}
