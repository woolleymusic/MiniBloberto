#!/bin/bash
# Signs Mini Bloberto (app, AU, VST3), notarizes and staples them, then builds a signed,
# notarized .pkg installer:
#   /Applications/Mini Bloberto.app
#   /Library/Audio/Plug-Ins/Components/Mini Bloberto.component
#   /Library/Audio/Plug-Ins/VST3/Mini Bloberto.vst3
#
# One-time setup first: docs/SIGNING.md (certificates + notarytool keychain profile).
#
# Usage:
#   tools/sign-and-notarize.sh [options]
#     --artefacts DIR     folder with Standalone/ AU/ VST3/   (default build/MiniBloberto_artefacts/Release)
#     --app PATH          the .app        (overrides --artefacts for this one)
#     --au PATH           the .component
#     --vst3 PATH         the .vst3
#     --identity NAME     "Developer ID Application: Name (R7T4SR6F77)"  (default: found in the keychain)
#     --installer NAME    "Developer ID Installer: Name (R7T4SR6F77)"    (default: found in the keychain)
#     --profile NAME      notarytool keychain profile                     (default: minibloberto-notary)
#     --out DIR           where the .pkg goes                             (default: dist)
#     --no-notarize       sign and build the .pkg only (no upload to Apple, no stapling)
#
# Signs copies in a staging folder; the build output is left as it is.
set -euo pipefail

TEAM_ID="R7T4SR6F77"
cd "$(dirname "$0")/.."
ROOT="$PWD"

ARTEFACTS="build/MiniBloberto_artefacts/Release"
APP="" AU="" VST3=""
APP_ID="" INST_ID=""
PROFILE="minibloberto-notary"
OUT="dist"
NOTARIZE=1

while [ $# -gt 0 ]; do
    case "$1" in
        --artefacts) ARTEFACTS="$2"; shift 2 ;;
        --app)       APP="$2"; shift 2 ;;
        --au)        AU="$2"; shift 2 ;;
        --vst3)      VST3="$2"; shift 2 ;;
        --identity)  APP_ID="$2"; shift 2 ;;
        --installer) INST_ID="$2"; shift 2 ;;
        --profile)   PROFILE="$2"; shift 2 ;;
        --out)       OUT="$2"; shift 2 ;;
        --no-notarize) NOTARIZE=0; shift ;;
        -h|--help)   sed -n '2,24p' "$0"; exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 2 ;;
    esac
done

APP="${APP:-$ARTEFACTS/Standalone/Mini Bloberto.app}"
AU="${AU:-$ARTEFACTS/AU/Mini Bloberto.component}"
VST3="${VST3:-$ARTEFACTS/VST3/Mini Bloberto.vst3}"
for b in "$APP" "$AU" "$VST3"; do
    [ -d "$b" ] || { echo "Not found: $b (build Release first, or pass --app/--au/--vst3)" >&2; exit 1; }
done

VERSION=$(sed -n 's/^project(MiniBloberto VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
[ -n "$VERSION" ] || { echo "Could not read the version from CMakeLists.txt" >&2; exit 1; }

# Identities: passed in, or the first matching one for this team in the keychain.
find_identity() {   # $1 = "Developer ID Application" or "Developer ID Installer", $2 = policy
    security find-identity -v ${2:+-p "$2"} | grep "\"$1: .*($TEAM_ID)\"" | head -n 1 \
        | sed 's/.*"\(.*\)".*/\1/'
}
[ -n "$APP_ID" ]  || APP_ID=$(find_identity "Developer ID Application" codesigning || true)
[ -n "$INST_ID" ] || INST_ID=$(find_identity "Developer ID Installer" "" || true)
if [ -z "$APP_ID" ] || [ -z "$INST_ID" ]; then
    echo "Missing a Developer ID certificate for team $TEAM_ID:" >&2
    [ -n "$APP_ID" ]  || echo "  Developer ID Application (signs the app and plug-ins)" >&2
    [ -n "$INST_ID" ] || echo "  Developer ID Installer (signs the .pkg)" >&2
    echo "See docs/SIGNING.md, step 1." >&2
    exit 1
fi
if [ "$NOTARIZE" = 1 ] && ! xcrun notarytool history --keychain-profile "$PROFILE" >/dev/null 2>&1; then
    echo "No working notarytool profile \"$PROFILE\". See docs/SIGNING.md, step 2 (or use --no-notarize)." >&2
    exit 1
fi

echo "Version:   $VERSION"
echo "App ID:    $APP_ID"
echo "Installer: $INST_ID"

STAGE=$(mktemp -d "${TMPDIR:-/tmp}/minibloberto-sign.XXXXXX")
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/app" "$STAGE/au" "$STAGE/vst3" "$OUT"
ditto "$APP"  "$STAGE/app/Mini Bloberto.app"
ditto "$AU"   "$STAGE/au/Mini Bloberto.component"
ditto "$VST3" "$STAGE/vst3/Mini Bloberto.vst3"
S_APP="$STAGE/app/Mini Bloberto.app"
S_AU="$STAGE/au/Mini Bloberto.component"
S_VST3="$STAGE/vst3/Mini Bloberto.vst3"

# Drop any old ad-hoc signature and Finder junk first.
find "$STAGE" -name .DS_Store -delete
xattr -cr "$STAGE"

# 1. Sign. Hardened runtime + secure timestamp are what notarization asks for.
echo "== Signing"
codesign --force --timestamp --options runtime --sign "$APP_ID" "$S_AU"
codesign --force --timestamp --options runtime --sign "$APP_ID" "$S_VST3"
codesign --force --timestamp --options runtime --sign "$APP_ID" \
    --entitlements "$ROOT/tools/entitlements.plist" "$S_APP"
for b in "$S_APP" "$S_AU" "$S_VST3"; do
    codesign --verify --strict --deep -vv "$b"
done

notarize() {   # $1 = file to submit (.zip or .pkg)
    echo "== Notarizing $(basename "$1") (this can take a few minutes)"
    local log="$STAGE/notary.json"
    xcrun notarytool submit "$1" --keychain-profile "$PROFILE" --wait --output-format json | tee "$log"
    if ! grep -q '"status" *: *"Accepted"' "$log"; then
        local id
        id=$(sed -n 's/.*"id" *: *"\([^"]*\)".*/\1/p' "$log" | head -n 1)
        echo "Notarization was not accepted. Apple's log:" >&2
        [ -n "$id" ] && xcrun notarytool log "$id" --keychain-profile "$PROFILE" >&2
        exit 1
    fi
}

# 2. Notarize the bundles themselves and staple them, so they pass Gatekeeper offline too,
#    even when someone copies them by hand instead of using the installer.
if [ "$NOTARIZE" = 1 ]; then
    ( cd "$STAGE" && ditto -c -k --keepParent app/"Mini Bloberto.app" bundles-app.zip )
    ( cd "$STAGE" && mkdir -p z && cp -R "$S_AU" "$S_VST3" z/ && ditto -c -k z bundles-plugins.zip )
    notarize "$STAGE/bundles-app.zip"
    notarize "$STAGE/bundles-plugins.zip"
    for b in "$S_APP" "$S_AU" "$S_VST3"; do
        xcrun stapler staple "$b"
    done
fi

# 3. Installer. Bundles are marked not relocatable, so macOS installs them where we say
#    and never "updates" some other copy it found on the disk.
echo "== Building installer"
component_pkg() {   # $1 = stage folder, $2 = id suffix, $3 = install location
    local plist="$STAGE/$2.plist"
    pkgbuild --analyze --root "$STAGE/$1" "$plist" >/dev/null
    local i=0
    while /usr/libexec/PlistBuddy -c "Print :$i" "$plist" >/dev/null 2>&1; do
        /usr/libexec/PlistBuddy -c "Set :$i:BundleIsRelocatable false" "$plist"
        i=$((i + 1))
    done
    pkgbuild --root "$STAGE/$1" --component-plist "$plist" \
        --identifier "com.woolley.minibloberto.$2" --version "$VERSION" \
        --install-location "$3" "$STAGE/$2.pkg" >/dev/null
}
component_pkg app  app  "/Applications"
component_pkg au   au   "/Library/Audio/Plug-Ins/Components"
component_pkg vst3 vst3 "/Library/Audio/Plug-Ins/VST3"

PKG="$OUT/MiniBloberto-$VERSION-mac.pkg"
rm -f "$PKG"
productbuild --sign "$INST_ID" --timestamp \
    --package "$STAGE/app.pkg" --package "$STAGE/au.pkg" --package "$STAGE/vst3.pkg" \
    "$PKG"

# 4. Notarize and staple the installer.
if [ "$NOTARIZE" = 1 ]; then
    notarize "$PKG"
    xcrun stapler staple "$PKG"
fi

# 5. Verify.
echo "== Verifying"
pkgutil --check-signature "$PKG"
spctl -a -vv -t install "$PKG" || [ "$NOTARIZE" = 0 ]
spctl -a -vv -t exec "$S_APP" || [ "$NOTARIZE" = 0 ]
for b in "$S_AU" "$S_VST3"; do
    # spctl only assesses apps directly; for plug-ins check the signature and the ticket.
    codesign --verify --strict -vv "$b"
    [ "$NOTARIZE" = 0 ] || xcrun stapler validate "$b"
done

echo
echo "Done: $PKG"
[ "$NOTARIZE" = 1 ] || echo "Not notarized (--no-notarize): Gatekeeper will still warn on other Macs."
