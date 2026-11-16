# Signing and notarizing Mini Bloberto (Mac)

Without this, people who download him get a "cannot be opened" or "damaged" warning.
With it, the installer just works. You do steps 1 and 2 once. Step 3 is every release.

Team ID: `R7T4SR6F77`

## 1. Make the two certificates (once)

You need two Developer ID certificates. Only the Account Holder of the team can make them.

**In Xcode (easiest):**

1. Xcode > Settings > Accounts. Sign in with your Apple ID if it isn't there.
2. Pick the team, then Manage Certificates.
3. Click + and choose **Developer ID Application**.
4. Click + again and choose **Developer ID Installer**.

**Or on the website:** developer.apple.com > Certificates, IDs & Profiles > Certificates > +.
Make a Developer ID Application certificate, then a Developer ID Installer one. Each asks for a
certificate signing request: Keychain Access > Certificate Assistant > Request a Certificate
From a Certificate Authority, saved to disk. Download each .cer and double-click it.

Check they are there:

```sh
security find-identity -v -p codesigning   # shows "Developer ID Application: <name> (R7T4SR6F77)"
security find-identity -v | grep Installer  # shows "Developer ID Installer: <name> (R7T4SR6F77)"
```

Back them up: Keychain Access > My Certificates, select both, right-click > Export, and keep
the .p12 file and its password somewhere safe (not in this repo). If you lose the private key
you have to make new certificates.

## 2. Save notarization login in the keychain (once)

1. Go to account.apple.com > Sign-In and Security > App-Specific Passwords and make one
   called "notarytool".
2. Run this and paste the app-specific password when it asks:

```sh
xcrun notarytool store-credentials minibloberto-notary \
    --apple-id "<your Apple ID email>" --team-id R7T4SR6F77
```

The password stays in your keychain. The script only uses the profile name
`minibloberto-notary`. Never put the password in a file in this repo.

## 3. Every release

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target MiniBloberto_VST3 MiniBloberto_AU MiniBloberto_Standalone
tools/sign-and-notarize.sh
```

It signs copies of the app, the AU and the VST3 (hardened runtime, secure timestamp),
notarizes and staples them, wraps them in an installer, signs that, notarizes and staples
it, then checks everything with `spctl` and `pkgutil --check-signature`. The result is
`dist/MiniBloberto-<version>-mac.pkg`. Upload that to the GitHub release.

The installer puts:

- `Mini Bloberto.app` in `/Applications`
- `Mini Bloberto.component` in `/Library/Audio/Plug-Ins/Components`
- `Mini Bloberto.vst3` in `/Library/Audio/Plug-Ins/VST3`

Useful options: `--identity` and `--installer` to pick a certificate by name,
`--profile` for another keychain profile, `--artefacts` for another build folder,
`--no-notarize` to test signing without sending anything to Apple.

## If notarization fails

The script prints Apple's log. The usual causes:

- A certificate is "Apple Development" instead of "Developer ID". Use Developer ID.
- The Mac clock is wrong, so the timestamp fails.
- The app-specific password was revoked. Make a new one and rerun step 2.

## Notes

- The Standalone app asks for microphone access the first time, because he listens to the
  input to find the beat. `tools/entitlements.plist` and the Info.plist text in
  `CMakeLists.txt` make that work under the hardened runtime.
- Windows is not covered here. The Windows build is unsigned for now, so Windows shows
  "Windows protected your PC" the first time (More info > Run anyway).
