#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
JUCE_DIR="${JUCE_DIR:-$ROOT/../JUCE}"
cmake -S "$ROOT" -B "$ROOT/build-macos" -G Xcode -DJUCE_DIR="$JUCE_DIR" -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build "$ROOT/build-macos" --config Release --target DuckPocket_VST3 DuckPocket_AAX PocketV10Test PocketShortDurationTest
for PLUGIN in "$ROOT/build-macos/DuckPocket_artefacts/Release/VST3/Duck Pocket.vst3" \
              "$ROOT/build-macos/DuckPocket_artefacts/Release/AAX/Duck Pocket.aaxplugin"; do
    codesign --force --deep --sign - "$PLUGIN"
    codesign --verify --deep --strict "$PLUGIN"
    printf '\nBuilt: %s\n' "$PLUGIN"
done
ctest --test-dir "$ROOT/build-macos" -C Release --output-on-failure
