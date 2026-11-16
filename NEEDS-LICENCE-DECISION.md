# Licence: your call before going public

The repo has no LICENSE file yet. Without one, a public repo is "all rights reserved":
people can look at the code but not legally reuse it. Pick one before you flip it to public,
add it as `LICENSE`, and delete this file.

## The thing that shapes the choice: JUCE

Mini Bloberto is built on JUCE 9. JUCE is dual licensed: AGPLv3, or the paid/free tiers of
the JUCE licence (see `LICENSE.md` in your JUCE folder and juce.com/legal/juce-9-licence).

- If you build and give out binaries under JUCE's **AGPLv3** option, the whole program you
  ship is covered by AGPLv3, so the source has to be available. A public repo already does
  that.
- If you hold a **JUCE licence** (for example the free Starter tier, if you qualify), you
  can ship binaries under whatever terms you like.

Check your JUCE account to see which applies. That decides how much the choice below matters.

## The two options

**MIT** (short and permissive)
- Anyone can reuse your code for anything, even closed source, as long as they keep your
  copyright line.
- Your own files can be MIT even if the shipped binary falls under AGPLv3 through JUCE.
  MIT code can be combined with AGPL code.
- Good if you want people to learn from him, fork him, and make their own dancers.

**GPLv3** (copyleft)
- Anyone can reuse your code, but if they ship something built from it, they must share
  their source under the same licence.
- Lines up with JUCE's AGPLv3 option, so there is one story for the whole thing.
  (AGPLv3 itself is also an option if you want to match JUCE exactly.)
- Good if you don't want someone to take him, close the source and sell him.

## Either way

- The sprites in `resources/` are your character art. The licence you pick covers them too,
  unless you add a line saying the art is separate (for example "code MIT, art all rights
  reserved"). Decide if you want people to reuse Bloberto himself.
- Put your name, or "Woolley", in the copyright line, and the year 2026.
