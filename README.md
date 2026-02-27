# FURRY — C + Vulkan + SDL3 High-Performance VN Engine (Parity Foundation)

This repository now ships a stronger VN runtime core aimed at Ren'Py parity targets while keeping a performance-first C architecture.

## Engine direction
- **Language/runtime core:** C11
- **Rendering target:** Vulkan (integration flag present, renderer module planned)
- **Platform/input windowing target:** SDL3 (integration flag present, backend module planned)
- **Audio recommendation:** **miniaudio** (single-file, open-source, royalty-free)

## Script features implemented
- `label:`
- `say Speaker|Line`
- `goto label`
- `call label` / `return` (subroutines)
- `set key=value`
- `add key=int`
- `if_eq key|value|label`
- `bg background_id`
- `fg sprite.png|x|y|rotation|animation`
- `button id|label|target_label`
- `ui_begin layer_id` / `ui_end`
- `ui_panel id|x|y|w|h`
- `ui_text id|text`
- `ui_image id|asset`
- `ui_anim id|asset|play_mode`
- `ui_video id|asset|loop_flag`
- `ui_bind id|state_key`
- `music track.ogg`
- `sfx click.wav`
- `save slot_name` / `load slot_name`
- `choice Prompt|Option->label|Option->label` (multi-choice)
- `end`

## Parity-focused runtime upgrades included
- Choice callback hook in `FurryRuntimeConfig` for UI-driven selection.
- Host command callback hook for renderer/UI actions (`bg`, `fg`, `button`, `music`, `sfx`).
- Advanced UI host operations (`ui_begin`, `ui_panel`, `ui_text`, `ui_image`, `ui_anim`, `ui_video`, `ui_bind`) for script-defined UI composition.
- Call stack support (`call`/`return`) for reusable scene blocks.
- Runtime snapshot encode/decode utilities (`furry_snapshot_save/load`) plus `save_slot`/`load_slot` hooks for game save flows.

## Engine boundary (important)
- FURRY does **not** ship built-in game UI presets/themes/widgets as an engine feature.
- FURRY provides runtime script execution + host callback hooks so each game author builds their own UI/frontend.
- Direction choice: use a Lua authoring frontend that maps into this runtime model (Lua-first authoring, engine-managed execution).

## Diagnostics and media support
- `furry_compile_script_ex` reports line-based compile errors with clear reasons.
- `furry_media_is_supported` validates free/common media extensions for images/animation/video (`png`, `jpg`, `jpeg`, `webp`, `gif`, `apng`, `webm`, `mp4`, `m4v`, `flv`, `anim`).

## Authoring guide
- See `docs/LUA_UI_AUTHORING_GUIDE.md` for an AI/human focused specification and examples to build UI/gameplay scripts.

## Build on Windows
### Batch
```bat
scripts\build_windows.bat Release
```

### PowerShell
```powershell
./scripts/build_windows.ps1 -Configuration Release
```

### Manual
```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Local dev check
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/furry_app
```
