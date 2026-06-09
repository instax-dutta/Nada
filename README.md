# Nada

**Open source, native HiFi gapless music player.**

Nada is a high-fidelity audio player built for audiophiles who want
bit-perfect, gapless playback with DSP control — no Electron, no web
views, no compromises.

![](resources/icons/nada_icon_v3.svg)

---

## Features

- **Gapless playback** — zero-gap transitions between tracks
- **HiFi audio output** — native CoreAudio on macOS (WASAPI/ALSA on
  other platforms)
- **Wide format support** — FLAC, WAV, MP3, M4A/AAC, OGG Vorbis,
  Opus, AIFF, APE, WavPack
- **Parametric EQ** — real-time DSP with interactive frequency
  response graph
- **Crossfeed (bs2b)** — natural stereo imaging for headphone listening
- **ReplayGain** — album/track normalization with pre-amp
- **Waveform seek bar** — visual track scrubbing with async waveform
  rendering
- **Library scanning** — recursive directory import with metadata
  extraction via TagLib
- **Cover art** — embedded cover extraction with on-disk thumbnail
  caching
- **Queue management** — build and reorder a play queue on the fly
- **System tray** — background playback with tray controls
- **Dark theme** — custom Qt stylesheet with gold accent palette

---

## Build from source

### Dependencies

| Dependency | Version | Notes |
|---|---|---|
| C++ compiler | C++20 | Apple Clang 15+, GCC 13+, MSVC 2022 |
| CMake | >= 3.27 | |
| Qt 6 | >= 6.6 | Core, Widgets, Quick, Sql, OpenGL |
| TagLib | >= 2.0 | Metadata reading |
| libsoxr | any | Sample rate conversion |
| FFTW3 | any | EQ analysis (single + float) |
| spdlog | >= 1.12 | Logging |
| nlohmann-json | >= 3.11 | JSON support |

### Build (macOS)

```bash
brew install cmake qt@6 taglib libsoxr fftw spdlog nlohmann-json pkg-config
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build --target nada
open build/nada.app
```

### Build (Linux)

```bash
# Debian/Ubuntu
sudo apt install cmake g++ qt6-base-dev libtag1-dev libsoxr-dev \
  libfftw3-dev libspdlog-dev nlohmann-json3-dev libpulse-dev \
  libasound2-dev ninja-build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target nada
./build/nada
```

### Build (Windows)

```powershell
# Using vcpkg
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build --target nada
```

---

## Roadmap

### Cross-platform
- [x] macOS (CoreAudio) — active, CI-built
- [ ] **Linux** (ALSA/PipeWire) — build scaffold in CI (needs AppImage packaging)
- [ ] **Windows** (WASAPI) — build scaffold in CI (needs NSIS installer)
- [ ] Bit-perfect output modes (WASAPI exclusive, ALSA direct)

### Player features
- [ ] Playlist import/export (M3U, PLS, XSPF)
- [ ] Smart playlists (criteria-based auto playlists)
- [ ] Album grid view
- [ ] MusicBrainz metadata lookup + tag editor
- [ ] DSD playback
- [ ] Android remote control
- [ ] MPRIS integration (Linux)
- [ ] Last.fm / ListenBrainz scrobbling

---

## Project structure

```
src/
├── decoder/     # Format decoders (FLAC, WAV, MP3 via dr_libs)
├── dsp/         # DSP chain (EQ, crossfeed, ReplayGain)
├── engine/      # Playback engine and buffer management
├── library/     # Metadata, cover cache, scanner, database
├── output/      # Platform audio output backends
└── ui/          # Qt widgets (player, library, queue, EQ, settings)
vendor/          # Bundled third-party (bs2b, dr_libs)
tests/           # Catch2 unit tests
resources/       # Icons, QRC, themes
```

---

## License

MIT — see [LICENSE](LICENSE).

---

*Nada is built in the open. Contributions, issues, and ideas welcome.*
