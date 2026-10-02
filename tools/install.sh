#!/bin/sh
# Copies the built AU and VST3 into the user plug-in folders, replacing any old copy
# (remove then copy, never over a bundle a DAW has loaded). Quit the DAW first.
set -e
cd "$(dirname "$0")/.."
ART=build/MiniBloberto_artefacts/Release
AU=~/Library/Audio/Plug-Ins/Components
VST3=~/Library/Audio/Plug-Ins/VST3
mkdir -p "$AU"
rm -rf "$AU/Mini Bloberto.component"
cp -R "$ART/AU/Mini Bloberto.component" "$AU/"
codesign --force --deep -s - "$AU/Mini Bloberto.component"
SUDO=""
[ -w "$VST3" ] || SUDO="sudo"
$SUDO mkdir -p "$VST3"
$SUDO rm -rf "$VST3/Mini Bloberto.vst3"
$SUDO cp -R "$ART/VST3/Mini Bloberto.vst3" "$VST3/"
$SUDO codesign --force --deep -s - "$VST3/Mini Bloberto.vst3"
echo "Installed AU + VST3. Validate the AU with: auval -v aufx Mblb Wool"
