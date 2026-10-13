#!/usr/bin/env bash
set -euo pipefail

repo="$(cd "$(dirname "$0")/.." && pwd)"
build="$repo/build-macos"
dist="$repo/dist-macos"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This script must run on macOS."
  exit 1
fi

command -v xcodebuild >/dev/null || { echo "Xcode is required."; exit 1; }
command -v cmake >/dev/null || { echo "CMake 3.22 or newer is required."; exit 1; }
command -v python3 >/dev/null || { echo "Python 3 is required."; exit 1; }

rm -rf "$build" "$dist"

cmake -S "$repo" -B "$build" -G Xcode \
  -DBUILD_PLUGIN=ON \
  -DBUILD_NATIVE_MIDI_PROBE=OFF \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0

cmake --build "$build" --config Release --parallel 3 \
  --target BassPedal_VST3 BassPedalAU_AU vst3_processor_tests

ctest --test-dir "$build" -C Release \
  -R '^(product_version|vst3_processor|vst3_scan)$' \
  --output-on-failure

for bundle in \
  "$build/BassPedal_artefacts/Release/VST3/Piano Auto Sustain.vst3" \
  "$build/BassPedalAU_artefacts/Release/AU/Piano Auto Sustain MIDI FX.component"
do
  binary_name="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$bundle/Contents/Info.plist")"
  binary="$bundle/Contents/MacOS/$binary_name"
  lipo "$binary" -verify_arch arm64 x86_64
  codesign --force --deep --sign - "$bundle"
  codesign --verify --deep --strict "$bundle"
done

mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
ditto \
  "$build/BassPedalAU_artefacts/Release/AU/Piano Auto Sustain MIDI FX.component" \
  "$HOME/Library/Audio/Plug-Ins/Components/Piano Auto Sustain MIDI FX.component"
auval -v aumi Pam1 Amup

python3 "$repo/tests/PackagingTests.py"
python3 "$repo/scripts/package_portable.py" \
  --build "$build" \
  --output "$dist" \
  --platform macOS-Universal \
  --commit local-mac-build

echo
echo "Build complete. Send the ZIP and SHA256 file in:"
echo "$dist"
