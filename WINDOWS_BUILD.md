# Build Mini Bloberto for Windows (instructions for Claude on the Windows PC)

Mini Bloberto is a JUCE 9 audio effect plugin (VST3 + Standalone) developed on a Mac: a
pixel-art blob who dances in the plugin window on the host's beat, or (no host clock, e.g. OBS)
on the beats a tracker hears in the input. It opens on a settings screen; LET HIM DANCE shrinks
the window to just him, clicking him brings the settings back. Audio passes through untouched.
This is v2 (18 dances, tricks, beat tracker); replace any earlier C:\dev\MiniBloberto completely. Build a Windows x64 release, verify it without a DAW, and package it for Charlie.

Work in `C:\dev`. Report exact error text if a step fails. Don't edit the plugin source to get
past a compiler error without saying what you changed and why; Charlie's Mac is the main copy.

## 1. Tools (skip what is already installed; earlier Woolley kits installed them)

```powershell
winget install --id Kitware.CMake -e
winget install --id Git.Git -e
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

Use an "x64 Native Tools Command Prompt for VS 2022" (or run `vcvars64.bat`). JUCE 9 does not
support MinGW; use MSVC.

## 2. Sources

JUCE MUST be exactly 9.0.2. If `C:\dev\JUCE` exists from an earlier kit, check it is 9.0.2 and reuse it:

```powershell
git clone --branch 9.0.2 --depth 1 https://github.com/juce-framework/JUCE.git C:\dev\JUCE
```

Delete any old `C:\dev\MiniBloberto`, then unzip `MiniBloberto-src.zip` from this kit into it
(the zip has no top folder, so extract INTO that folder: you should get
`C:\dev\MiniBloberto\CMakeLists.txt`). Delete any old `build` folder too.

Building from GitHub instead of the kit? Clone the repo into that folder:

```powershell
git clone https://github.com/woolleymusic/MiniBloberto.git C:\dev\MiniBloberto
```

The test WAV in step 4 is only in the kit. Any song with a clear beat works for the `--wav` check.

## 3. Build

```powershell
cd C:\dev\MiniBloberto
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_SOURCE_DIR=C:/dev/JUCE
cmake --build build --config Release --target MiniBloberto_VST3 MiniBloberto_Standalone MiniBlobertoSnapshot
```

Outputs (the product name has a space):
- `build\MiniBloberto_artefacts\Release\VST3\Mini Bloberto.vst3` (a folder bundle)
- `build\MiniBloberto_artefacts\Release\Standalone\Mini Bloberto.exe`
- `build\MiniBlobertoSnapshot_artefacts\Release\MiniBlobertoSnapshot.exe`

Conversion warnings are fine. Errors are not: report them verbatim.

## 4. Verify without a DAW

```powershell
$s = "build\MiniBlobertoSnapshot_artefacts\Release\MiniBlobertoSnapshot.exe"
& $s --selftest
& $s --png C:\dev\mb-setup.png --ppq 1.25 --set moves=0 --set pick=16 --set trails=1 --set reflect=1
& $s --dancer --png C:\dev\mb-dancer.png --ppq 2 --set backdrop=1 --set moves=0 --set pick=11
& $s --strip C:\dev\mb-strip.png --set moves=0 --set pick=17 --set reflect=1
& $s --wav <kit>\test-128bpm.wav --seconds 24
```

Expected (from the Mac build):
- `pass-through null test: max difference 0 (PASS)`
- `playing at ppq 1.25 -> Wave frame 5`
- `playing at ppq 2.00 -> Headbang frame 0`
- the strip prints `Moonwalk frame 0` .. `Moonwalk frame 7` for ppq 0.00 .. 1.75
- the WAV run prints four lines, `bpm=` between 127 and 130, `active=1`, about 42 frames per
  5 s after the first line (the moves listed are random, they will differ)
- the PNGs match `mac-reference\`: `setup-wave-ppq1.25.png` (settings panel left with 9
  dropdowns, TRICKS checkboxes, a LET HIM DANCE button; blob with arms up, faint trails and a
  floor reflection on the right), `dancer-green-headbang.png` (only the blob on flat green,
  330x384), `strip-moonwalk.png`. Fonts may differ slightly on Windows; describe any other
  difference, especially if the blob looks blurry.

Then launch `Mini Bloberto.exe`: it opens on the settings screen. Click LET HIM DANCE: the
window shrinks to just him. Click him once: settings come back. Hold the mouse on him: he
dangles. (The Standalone mutes its input by default; Options > Audio settings to unmute it if
you want to see him follow the mic.)

## 5. Package for Charlie

Make `C:\dev\MiniBloberto-win64.zip` containing:
- `Mini Bloberto.vst3` (the whole folder)
- `Mini Bloberto.exe`
- `README-TESTER.txt` (copy from this kit)

Tell Charlie where the zip is, what step 4 showed, and anything you had to change.
