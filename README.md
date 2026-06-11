# UltraPaste for AviUtl2

AviUtl2 `.aux2` general plugin that imports REAPER clipboard data into the timeline.

> **Status**: Early alpha. REAPER clipboard only. Expect bugs.

## Features

- Copy Items in REAPER via `Ctrl+C` → switch to AviUtl2 → right-click timeline → **"[UltraPaste] 导入 REAPER 剪贴板"**
- Auto-layers per track; greedy first-fit allocation for overlapping items within a track
- Syncs REAPER Item properties: play position (`再生位置`), play rate (`再生速度`), loop (`ループ再生`)

## Install

1. Download `UltraPaste.aux2` from [Releases](https://github.com/TNOTawa/UltraPaste-Aviutl2/releases)
2. Move it to AviUtl2's `exedit2/plugin/` folder
3. Restart AviUtl2

## Usage

1. In REAPER, select Items (multi-track OK), press `Ctrl+C`
2. Switch to AviUtl2, right-click an empty area on the timeline → **[UltraPaste] 导入 REAPER 剪贴板**
3. Objects are placed starting from the current cursor layer, one layer per REAPER track

## Build

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
cmake --build build
# Output: build/UltraPaste.aux2
```

Requires: MinGW-w64 (g++ 15+), CMake 3.20+

## Limitations

- REAPER clipboard only (`REAPERMedia` format); no `.rpp` file import
- Does not import: envelopes, takes, stretch markers, MIDI items
- No settings UI, no persistent configuration
- Objects use `create_object_from_media_file` + basic param syncing (position / speed / loop)

## License

GNU Lesser General Public License v3.0 — see `LICENSE`
