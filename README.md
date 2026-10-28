# CLIP

A hard/soft clipper audio plugin built with JUCE 8. Formats: AU, VST3, Standalone. macOS 14+, universal (arm64 + x86_64).

## Build

Requires CMake 3.22+ (`brew install cmake`) and Xcode command line tools. JUCE is fetched by CPM into `~/.cache/CPM` on first configure.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
open build/CLIP_artefacts/Debug/Standalone/CLIP.app
```

For faster local builds, add `-DCMAKE_OSX_ARCHITECTURES=arm64` to the configure step.

Xcode project:

```sh
cmake -S . -B build-xcode -G Xcode
open build-xcode/CLIP.xcodeproj
```

## Tests

```sh
./build/CLIPTests_artefacts/Debug/CLIPTests
```

CI runs the unit tests and pluginval (strictness 10) on Windows for every PR and push to main. The macOS job (unit tests, pluginval and auval) is manual only: Actions tab > Build > Run workflow.

## Release

Bump `VERSION` in `CMakeLists.txt`, then push a matching tag. CI attaches the Windows zip (VST3, Standalone and install notes) to a GitHub Release.

```sh
git tag v0.1.0 && git push origin v0.1.0
```
