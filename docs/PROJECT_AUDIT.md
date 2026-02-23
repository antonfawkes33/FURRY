# Audit Update: Parity-Oriented Improvements

## Upgrades now implemented
1. **Scripted subroutines** with `call`/`return`
2. **Multi-option branching** via `choice Prompt|Text->label|Text->label`
3. **UI-integrated choice hook** through runtime callback config
4. **Save/load groundwork** via runtime snapshot serialization API
5. **Maintained modern menu theme baseline** for SDL3+Vulkan UI integration

## Why this matters for parity
These changes close key gaps for VN scenario authoring:
- reusable script composition (`call`/`return`)
- meaningful branch trees with multiple options
- decoupled runtime logic from frontend input
- path toward deterministic save/load

## Remaining high-priority work
1. Binary bytecode compiler + static analysis
2. SDL3 event loop + input/IME text workflow
3. Vulkan renderer (text, sprite, transition layers)
4. miniaudio backend module (bus mixing, fades, ducking)
5. Full save snapshot state (variables, stack, current assets, playback positions)
6. Narrative tooling (debugger, rollback inspector, branch coverage)

## Recommendation
Keep the VM testable/headless while implementing renderer/platform/audio as modules. This enables fast iteration and keeps game logic deterministic.
