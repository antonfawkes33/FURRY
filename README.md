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
- `music track.ogg`
- `sfx click.wav`
- `choice Prompt|Option->label|Option->label` (multi-choice)
- `end`

## Parity-focused runtime upgrades included
- Choice callback hook in `FurryRuntimeConfig` for UI-driven selection.
- Call stack support (`call`/`return`) for reusable scene blocks.
- Runtime snapshot encode/decode utilities (`furry_snapshot_save/load`) as a base for save/load systems.

## Stylish base main menu
`furry_ui` provides a modern menu theme schema:
- gradient colors
- accent palette
- typography slots
- semantic actions (`new_game`, `continue`, `scenes`, `mods`, `settings`, `quit`)

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
