# Velvox Music

A modern, high-performance local music player for **Windows**, built with
**Qt 6 / QML / C++20**.

> Experience your music library with a fluent, frosted-glass interface,
> real-time synced lyrics, and hardware-friendly rendering.

![App Screenshot](assets/screenshot-main.png)

*(Screenshot placeholder: replace `assets/screenshot-main.png` with an actual
1280×720 capture of the main window.)*

---

## Features

- **Modern frameless UI** — custom title bar, drag-to-move, edge resizing,
  smooth animations, and accessibility-friendly "reduce motion" mode.
- **Music library management** — scan local folders, read metadata, cache
  downscaled cover art.
- **Multiple browse modes** — Library, Album grid, Artist grid, Playlists, and
  global Search with fuzzy matching.
- **Playback essentials** — play/pause, previous/next, seek bar, volume,
  queue source tracking.
- **Synced lyrics** — parse external `.lrc` files and embedded lyrics, with
  per-word highlighting for enhanced LRC.
- **Mini player** — compact always-on-top window for quick control.
- **System integration** — system tray icon, Windows media keys, and taskbar
  thumbnail playback buttons.
- **Performance first** — streaming audio decoding, virtualized lists, image
  source-size limits, Canvas-based spectrum, and render-quality tiers.

![Mini Player](assets/screenshot-mini.png)

*(Mini player screenshot placeholder: replace `assets/screenshot-mini.png`.)*

---

## Tech Stack

| Layer | Technology | Notes |
|---|---|---|
| UI Framework | **Qt 6.8+** (`Quick`, `Qml`, `Sql`, `Svg`, `ShaderTools`, `Effects`, `Widgets`) | Frameless window + QML module via `qt_add_qml_module` |
| Language | **C++20** | Core backend and `QAbstractListModel` subclasses |
| Build System | **CMake 3.21+** | LTO and MSVC whole-program optimization in Release |
| Audio | **miniaudio** | Single-header decoding/playback, commercial-safe |
| Metadata | **TagLib** | Dynamically linked to satisfy LGPL |
| Spectrum FFT | **kissfft** | Compiled directly into the binary |
| Database | **QSQLITE** | Local library, playlists, favorites, and history |

---

## Download

Prebuilt Windows installers and portable ZIPs are available on the
[Releases](https://github.com/VeilCode/VelvoxMusic/releases) page.

> The installer creates Start Menu and Desktop shortcuts and registers a
> standard Windows uninstall entry.

---

## Local Build (Windows + MSVC)

### Prerequisites

- Visual Studio 2022 with the **Desktop development with C++** workload
- Qt 6.8+ for MSVC2022_64 (e.g. `C:\Qt\6.8.0\msvc2022_64`)
- CMake ≥ 3.21 in `PATH`
- vcpkg (for TagLib) or a prebuilt TagLib available to CMake

### Quick Build

```powershell
# 1. Clone
git clone https://github.com/VeilCode/VelvoxMusic.git
cd VelvoxMusic

# 2. Configure (adjust Qt path if needed)
cmake -B build -S . `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64" `
    -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"

# 3. Build
cmake --build build --config Release --parallel

# 4. Run (windeployqt + TagLib DLL copy happens automatically)
.\build\Release\VelvoxMusic.exe
```

> If Qt is on `PATH`, you may omit `-DCMAKE_PREFIX_PATH`.
> If TagLib is not installed via vcpkg, provide its install prefix with
> `-DTagLib_DIR=...` or `-Dtaglib_DIR=...`.

### Create an Installer Locally

1. Build the Release target (see above).
2. Install [Inno Setup 6](https://jrsoftware.org/isinfo.php).
3. Open `installer\VelvoxMusic.iss` in Inno Setup and click **Build**.
4. The installer is written to `dist\VelvoxMusic-<version>-win64-setup.exe`.

---

## Project Layout

```
VelvoxMusic/
├── CMakeLists.txt              # Root build config + windeployqt deploy step
├── LICENSE                     # MIT
├── THIRD_PARTY_LICENSES.md     # Third-party license & LGPL source offer
├── README.md                   # This file
├── PROJECT_CONTEXT.md          # Detailed architecture and decision log
├── vcpkg.json                  # vcpkg manifest (TagLib dependency)
├── .gitignore
├── .clang-format               # C++ code style config
├── src/
│   ├── app/main.cpp            # Entry point, QML module, tray integration
│   ├── core/                   # Audio engine, scanner, metadata, database
│   └── models/                 # QAbstractListModel subclasses for QML
├── qml/
│   ├── main.qml                # Frameless main window
│   ├── pages/                  # Library, Album, Artist, Playlists, Search, Settings
│   ├── components/             # Reusable controls
│   └── theme/                  # Singleton theme / motion tokens
├── resources/                  # Icons, fonts, shaders
├── installer/                  # Inno Setup script
│   └── VelvoxMusic.iss
├── .github/workflows/          # CI / automatic release
│   └── build.yml
└── assets/                     # README screenshots / demo media
```

---

## User Data Location

Velvox Music stores its local SQLite database and cached cover art in the
application data directory:

- Windows: `%LOCALAPPDATA%\Velvox\Velvox Music\`
- Files:
  - `library.db` — SQLite music library
  - `covers\` — downscaled embedded cover images

---

## License

Velvox Music itself is released under the [MIT License](LICENSE).

### Third-Party License Compliance

This project stands on the shoulders of several excellent open-source libraries:

| Library | License | Usage |
|---|---|---|
| [Qt 6](https://www.qt.io/licensing) | LGPL v3 / Commercial | GUI framework |
| [miniaudio](https://github.com/mackron/miniaudio) | Public domain / Unlicense / MIT-0 | Audio decoding & playback |
| [TagLib](https://github.com/taglib/taglib) | LGPL v2.1 / MPL v1.1 | Metadata parsing |
| [kissfft](https://github.com/mborgerding/kissfft) | BSD-3-Clause | FFT spectrum analysis |

Because **TagLib is LGPL**, Velvox Music links to it **dynamically**. The
installer and portable archive include the TagLib shared library
(`tag.dll` / `taglib.dll`) alongside the executable so users can replace it
with a modified version. See [`THIRD_PARTY_LICENSES.md`](THIRD_PARTY_LICENSES.md)
for full compliance details.

---

## Contributing

Contributions are welcome. Please keep code style consistent with the existing
project; see [`.clang-format`](.clang-format) and run formatting before
submitting a pull request.

```powershell
# Format all C++ sources (requires clang-format in PATH)
clang-format -i src/**/*.cpp src/**/*.h
```

---

## Acknowledgments

- [miniaudio](https://github.com/mackron/miniaudio) by David Reid
- [TagLib](https://github.com/taglib/taglib) maintainers
- [kissfft](https://github.com/mborgerding/kissfft) by Mark Borgerding
- [Qt Project](https://www.qt.io) for the fantastic Qt 6 framework
