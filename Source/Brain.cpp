#include "Brain.h"

namespace mb
{
namespace
{
const int stepsPerQuarter[] { 2, 4, 8 };   // 8n 16n 32n
const int barsPerMove[] { 1, 2, 4, 8 };

// every dance, in the order Cycle walks them (Held is a pose, not a dance)
const std::vector<int> allDances { Bop, Sway, Cheer, Kick, Jelly, Point, Spin, Wiggle, Jump,
                                   Pump, Headbang, Robot, Twist, Flap, Shuffle, Wave, Moonwalk };

// loudness tiers for Input mode: calm, groove, big
const std::vector<int> pools[] {
    { Bop, Sway, Wiggle, Wave, Twist },
    { Sway, Kick, Point, Wiggle, Cheer, Bop, Shuffle, Robot, Moonwalk, Flap },
    { Jelly, Jump, Cheer, Kick, Spin, Headbang, Pump, Shuffle },
};
} // namespace

void Brain::choose (const Settings& s)
{
    auto pickFrom = [this] (const std::vector<int>& list)
    {
        std::vector<int> c;
        for (auto r : list)
            if (r != cur)
                c.push_back (r);
        cur = c[(size_t) rng.nextInt ((int) c.size())];
    };

    if (s.moves == Pick)
        cur = s.pick;
    else if (s.moves == Random)
        pickFrom (allDances);
    else if (s.moves == Cycle)
    {
        auto it = std::find (allDances.begin(), allDances.end(), cur);
        cur = (it == allDances.end() || it + 1 == allDances.end()) ? allDances.front() : *(it + 1);
    }
    else
    {
        const auto rel = lvl / peak;
        pickFrom (pools[rel < 0.45 ? 0 : (rel < 0.75 ? 1 : 2)]);
    }
}

Cell Brain::tick (const Settings& s, const Transport& t, float level, double dt)
{
    // level follower: ~0.25 s smoothing, slow-decaying peak so tiers are relative to the song
    const auto a = 1.0 - std::exp (-dt / 0.25);
    lvl += ((double) level - lvl) * a;
    peak = juce::jmax (peak * std::pow (0.97, dt), lvl, 0.02);

    if (s.moves != lastMoves || s.bars != lastBars)
    {
        lastMoves = s.moves;
        lastBars = s.bars;
        lastPhrase = -1;
    }

    const int standRow = s.moves == Pick ? s.pick : Bop;

    if (s.mode == Still || isQuiet (s))
        return { standRow, 0 };

    auto finish = [&s] (int row, juce::int64 step, double beats)
    {
        Cell c;
        c.row = row;
        c.step = step;
        c.reverse = s.reverse;
        const int f = (int) (((step % 8) + 8) % 8);
        c.frame = s.reverse ? 7 - f : f;
        if (s.flip == FlipBar)
            c.flip = ((juce::int64) std::floor (beats / 4.0)) % 2 != 0;
        else if (s.flip == FlipTwoBeats)
            c.flip = ((juce::int64) std::floor (beats / 2.0)) % 2 != 0;
        return c;
    };

    if (! t.playing)
    {
        // transport stopped: a gentle bop at 4 frames a second so he still feels alive
        idleClock += dt;
        return finish (standRow, (juce::int64) (idleClock * 4.0), idleClock * 2.0);
    }

    const double barQ = t.num * 4.0 / juce::jmax (1, t.den);
    const auto phrase = (juce::int64) std::floor (t.ppq / (barsPerMove[s.bars] * barQ));
    if (phrase != lastPhrase)
    {
        lastPhrase = phrase;
        choose (s);
    }
    if (s.moves == Pick)
        cur = s.pick;

    const auto step = (juce::int64) std::floor (t.ppq * stepsPerQuarter[s.speed]);
    return finish (cur, step, t.ppq * 4.0 / barQ);
}
} // namespace mb
