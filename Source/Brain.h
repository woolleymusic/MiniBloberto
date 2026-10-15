#pragma once

#include <juce_core/juce_core.h>

// What Mini Bloberto is doing right now. Same rules as the M4L device's bloberto.js:
// frames lock to the host's bar grid, 8 frames per dance cycle, and in Random / Input / Cycle
// he changes move every N bars. Rows after Held are the plugin-only dances (sprite.py EXTRA).
namespace mb
{
enum Row { Bop, Sway, Cheer, Kick, Jelly, Point, Spin, Wiggle, Jump, Held,
           Pump, Headbang, Robot, Twist, Flap, Shuffle, Wave, Moonwalk, NumRows };
inline const juce::StringArray rowNames { "Bop", "Sway", "Cheer", "Kick", "Jelly", "Point", "Spin",
                                          "Wiggle", "Jump", "Held", "Pump", "Headbang", "Robot",
                                          "Twist", "Flap", "Shuffle", "Wave", "Moonwalk" };
inline const juce::StringArray colourNames { "Lavender", "Bluebell", "Periwinkle", "Lilac" };

enum Mode { Still, Dance };
enum Moves { Pick, Random, Input, Cycle };
enum Flip { FlipOff, FlipBar, FlipTwoBeats };

struct Settings
{
    int mode = Dance, moves = Random, pick = Bop;
    int speed = 1;   // 0 8n, 1 16n, 2 32n
    int bars = 1;    // 0 1 bar, 1 2, 2 4, 3 8
    int flip = FlipOff;
    bool reverse = false;
};

struct Transport
{
    bool playing = false;
    double ppq = 0.0;              // quarter notes since the song start
    int num = 4, den = 4;
};

struct Cell
{
    int row = Bop, frame = 0;
    bool flip = false;             // mirrored left-right this bar
    bool reverse = false;          // frames run backwards (trails look the other way)
    juce::int64 step = 0;          // running 16n-ish step count, seeds shake and rainbow
};

class Brain
{
public:
    // call ~30 times a second; dt in seconds, level = latest audio peak (linear)
    Cell tick (const Settings&, const Transport&, float level, double dt);

    bool isQuiet (const Settings& s) const { return s.moves == Input && lvl < 0.004; }
    double level() const { return lvl / peak; }   // 0..1 relative to the song

private:
    void choose (const Settings&);

    juce::Random rng;
    int cur = Bop;
    juce::int64 lastPhrase = -1;
    int lastMoves = -1, lastBars = -1;
    double lvl = 0.0, peak = 0.02, idleClock = 0.0;
};
} // namespace mb
