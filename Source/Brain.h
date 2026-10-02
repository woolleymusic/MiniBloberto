#pragma once

#include <juce_core/juce_core.h>

// What Mini Bloberto is doing right now. Same rules as the M4L device's bloberto.js:
// frames lock to the host's bar grid, 8 frames per dance cycle, and in Random / Input
// he changes move every N bars.
namespace mb
{
enum Row { Bop, Sway, Cheer, Kick, Jelly, Point, Spin, Wiggle, Jump, Held, NumRows };
inline const juce::StringArray rowNames { "Bop", "Sway", "Cheer", "Kick", "Jelly",
                                          "Point", "Spin", "Wiggle", "Jump", "Held" };
inline const juce::StringArray colourNames { "Lavender", "Bluebell", "Periwinkle", "Lilac" };

enum Mode { Still, Dance };
enum Moves { Pick, Random, Input };

struct Settings
{
    int mode = Dance, moves = Random, pick = Bop;
    int speed = 1;   // 0 8n, 1 16n, 2 32n
    int bars = 1;    // 0 1 bar, 1 2, 2 4, 3 8
};

struct Transport
{
    bool playing = false;
    double ppq = 0.0;              // quarter notes since the song start
    int num = 4, den = 4;
};

struct Cell { int row = Bop, frame = 0; };

class Brain
{
public:
    // call ~30 times a second; dt in seconds, level = latest audio peak (linear)
    Cell tick (const Settings&, const Transport&, float level, double dt);

    bool isQuiet (const Settings& s) const { return s.moves == Input && lvl < 0.004; }

private:
    void choose (const Settings&);

    juce::Random rng;
    int cur = Bop;
    juce::int64 lastPhrase = -1;
    int lastMoves = -1, lastBars = -1;
    double lvl = 0.0, peak = 0.02, idleClock = 0.0;
};
} // namespace mb
