#include "Brain.h"

namespace mb
{
namespace
{
const int stepsPerQuarter[] { 2, 4, 8 };   // 8n 16n 32n
const int barsPerMove[] { 1, 2, 4, 8 };

// loudness tiers for Input mode: calm, groove, big
const std::vector<int> pools[] {
    { Bop, Sway, Wiggle },
    { Sway, Kick, Point, Wiggle, Cheer, Bop },
    { Jelly, Jump, Cheer, Kick, Point, Spin },
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
        pickFrom ({ Bop, Sway, Cheer, Kick, Jelly, Point, Spin, Wiggle, Jump });
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

    if (! t.playing)
    {
        // transport stopped: a gentle bop at 4 frames a second so he still feels alive
        idleClock += dt;
        return { standRow, (int) (idleClock * 4.0) % 8 };
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
    return { cur, (int) (((step % 8) + 8) % 8) };
}
} // namespace mb
