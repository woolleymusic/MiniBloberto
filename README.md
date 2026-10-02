# Mini Bloberto (JUCE)

A little Blobert who dances in your plug-in window, on the beat. Audio passes through
untouched (null-tested bit-exact). JUCE port of the "Mini Bloberto" M4L device
(`~/llm/mini-bloberto`, look spec in `look/LOOK.md`, tag `look-v2`).

Controls: Mode (Still / Dance), Moves (Pick / Random / Input), Pick (10 moves),
Speed (8n / 16n / 32n), Bars (how often Random/Input change move), Colour (4).
Frames lock to the host's bar grid via the playhead PPQ. Input follows the track's peak:
still when quiet, bigger moves when loud. Hold the mouse on him and he dangles.
Transport stopped: a gentle idle bop.

Sprites: `resources/Bloberto_<Colour>.png`, 8 frames x 10 rows of 110x128 native pixels,
drawn at 3x with nearest-neighbour. Regenerate with `~/llm/mini-bloberto/sprite.py`.

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target MiniBloberto_VST3 MiniBloberto_AU MiniBloberto_Standalone
    tools/install.sh            # quit the DAW first

Harness: `MiniBlobertoSnapshot --selftest | --png out.png [--ppq N] [--stopped] [--set id=i] | --strip out.png`.
Windows: see `WINDOWS_BUILD.md` (MSVC; JUCE 9 rejects MinGW).
