# FURRY Lua-Style UI + VN Script Authoring Guide (AI/Human)

This file is the engine-facing authoring spec for building UI and gameplay declaratively.

> Note: FURRY does not ship built-in game UI presets/themes. You define your own game UI and behavior in script (Lua-first authoring direction) and wire rendering/input in your host/frontend.

## Distribution model (BYO game UI)
- Engine binaries (`exe`/`dll`) are runtime infrastructure.
- Game creators work in their own game folder (`scripts`, `assets`, `saves`) and define all UI themselves.
- The engine executes script + state; your frontend decides visuals, widgets, layout, and interaction style.

## Goals
- Script defines scene flow, UI tree, bindings, animation/video usage, and save/load triggers.
- Host engine (SDL3/Vulkan/etc.) executes rendering/input/audio/storage through callbacks.
- Errors are surfaced with line numbers through `furry_compile_script_ex`.

## Core flow commands
- `label:`
- `say Speaker|Text`
- `goto label`
- `call label` / `return`
- `set key=value`
- `add key=int`
- `if_eq key|value|label`
- `choice Prompt|Text->label|Text->label`
- `save slot` / `load slot`
- `end`

## Scene/media commands
- `bg background_asset`
- `fg sprite_asset|x|y|rotation|animation`
- `music music_asset`
- `sfx sfx_asset`

## Advanced UI commands
- `ui_begin layer_id`
- `ui_panel id|x|y|w|h`
- `ui_text id|text`
- `ui_image id|asset`
- `ui_anim id|asset|play_mode`
- `ui_video id|asset|loop_flag`
- `ui_bind id|state_key`
- `button id|label|target_label`
- `ui_end`

### Coordinate/model notes
- `x`, `y`, `w`, `h` are host-interpreted numeric strings (recommended normalized 0..1).
- `play_mode` examples: `loop`, `once`, `pingpong` (host decides exact behavior).
- `loop_flag` values: `0` or `1`.

## Supported free/common media extensions
Use assets with these extensions to pass compile-time media checks:
- Images: `png`, `jpg`, `jpeg`, `webp`
- Animation: `gif`, `apng`, `anim`
- Video: `webm`, `mp4`, `m4v`, `flv`

## Error reporting contract
When compilation fails:
- `furry_compile_script_ex(...)` returns non-zero
- `FurryCompileError.line` gives the 1-based source line
- `FurryCompileError.message` gives the parse/validation reason

## Host integration contract
`FurryRuntimeConfig` should provide:
- `on_host_command`: execute render/UI/audio commands from script opcodes
- `save_slot`: persist snapshot per slot
- `load_slot`: restore snapshot per slot

If callbacks are not provided, runtime prints fallback logs for development.

## AI generation checklist
When generating scripts, always:
1. Start with a `label:` block.
2. Keep each command on its own line.
3. Ensure every `goto`, `call`, `button` target, and `choice` branch references a valid label.
4. Use only supported media extensions.
5. Wrap UI declaration with `ui_begin` / `ui_end`.
6. Keep `ui_bind` keys aligned with variables set in script (`set`/`add`).
7. End all terminal paths with `end`.

## Example template
```text
boot:
ui_begin hud
ui_panel root|0|0|1|1
ui_text title|Demo
ui_image logo|logo.webp
ui_anim pulse|pulse.anim|loop
ui_video opener|intro.mp4|1
ui_bind title|route
button start_btn|Start|route_pick
ui_end
bg city_night
fg guide_idle.png|0.5|0.9|0|idle
music theme.ogg
set route=none
choice Pick route|Tech->tech_route|Social->social_route

tech_route:
set route=tech
save autosave
goto done

social_route:
set route=social
goto done

done:
say Narrator|Ready.
end
```
