# Mini Bloberto (JUCE)

A little Blobert who dances in your plug-in window, on the beat. Audio passes through
untouched (null-tested bit-exact). JUCE port of the "Mini Bloberto" M4L device
(`~/llm/mini-bloberto`, look spec in `look/LOOK.md`, tag `look-v2`).

Opens on a settings screen with a live preview. LET HIM DANCE shrinks the window to just him
(Fruity Dance style); click him to get the settings back, hold him to pick him up. The view is
saved with the session.

Controls: Mode (Still / Dance), Moves (Pick / Random / Input / Cycle), Pick (17 dances + Held),
Speed (8n / 16n / 32n), Bars (how often Random/Input/Cycle change move), Colour (4), Flip
(mirror each bar / every 2 beats), Size (2x 3x 4x), Backdrop (Dark, or Green / Magenta / Blue
for chroma key). Tricks, after FAOSDance's extensions but kept on the pixel grid: Reverse,
Shake (whole pixels, harder when loud), Trails (last two frames as ghosts), Reflection,
Rainbow (palette hue step per beat, face keeps its colours).
Frames lock to the host's bar grid via the playhead PPQ. Input follows the track's peak:
still when quiet, bigger moves when loud. Hold the mouse on him and he dangles.
No playing transport (OBS, or the DAW stopped): a beat tracker on the input is his clock
and he dances on the beats he hears. Silence: a gentle idle bop.

Sprites: `resources/Bloberto_<Colour>.png`, 8 frames x 18 rows of 110x128 native pixels,
drawn at 3x with nearest-neighbour. Regenerate with `python3 ~/llm/mini-bloberto/sprite.py --plugin resources`.

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target MiniBloberto_VST3 MiniBloberto_AU MiniBloberto_Standalone
    tools/install.sh            # quit the DAW first

Harness: `MiniBlobertoSnapshot --selftest | --png out.png [--ppq N] [--stopped] [--set id=i] | --strip out.png`.
Windows: see `WINDOWS_BUILD.md` (MSVC; JUCE 9 rejects MinGW).
