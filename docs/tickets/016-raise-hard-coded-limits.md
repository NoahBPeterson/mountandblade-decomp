# 016 Modding: find and raise the hard-coded limits (fixed records, static pools)

Area: modding / engine | Effort: L | Impact: high | Depends on: 006 (named-field structs), 003 (pools as source)

## Summary

The original keeps its world in one object of fixed layout (`g_game`, troops at +0x13a00, ...), fixed-size records, and 144 MB of static pools. Modders hit these ceilings. With named-field 64-bit structs (006) most can become dynamic.
This ticket produces the audit and the conversions in priority order.

## Evidence: limits already visible in src/ (not exhaustive)

| limit | where | note |
|---|---|---|
| `Troop` record = 0x508 bytes, inventory `ItemRef inv[0x60]` (96 slots), equipment array at +0x4a4, notes at +0x420 | src/include/troop.h:89-167; `SI_ASSERT(troop_size, 0x508)` | per-troop data embedded; changing a field count changes every offset |
| `PartyV` = 0x5ac8 bytes with stacks at +0xa8, notes at +0x5a28; `PartyTemplate` 0xb0, `QuestRec` 0x138, `FactionRec` 0x300, `SceneRec` 0x494, `ItemKindX` 0x238 | src/game/script/script_interpreter.h:428-495 | the interpreter pins them with `SI_ASSERT` |
| `GameX`: parties embedded (`PartyMgr` = 0x139d8 - 0xf8 bytes), troops pointer at +0x13a00, cloud at +0x13b3c, message log +0x13b50 ... | script_interpreter.h:661-665 | one giant object; pools sized at compile time |
| script registers: `reg[128]` (64-bit), `str[128]`, 128 position registers (0x40 bytes), `down[256]` keys | dialog_manager.h:47-49, script_interpreter.h:289,229 | registers are per `ScriptState` (`g_script`) |
| dialog: `DlgLine list[0x400]`, `sel[0x400]` | dialog_manager.h:75-76 | 1,024 lines |
| operation operands: 16 (`params[16]`, `types[16]`) | NOTES.md | text-format bound |
| UI: `widgets[2][0x60]`, `flags[2][0x60]`, `values[2][0x60]` (inventory pages), `deform_rows[0x30]` (face-gen) | party_window.h:103, inventory_panel.h:56-58, facegen_*.h | 96 slots/page, 48 face keys |
| scene files: `ItemRef pairs114[0x40]` | terrain_b/scene_file.h:51 | 64 pairs |
| static image: `.data` VirtualSize 0x893db94 = 143.9 MB | PE header of mount&blade.exe | pools: 126.9 MB "borrowed" .bss (data_history.json) |
| dev/editor: nav, grids: `world_grid` cells, `CellGrid` | rgl/dynamic | sized by scene bounds |
| other sizes not yet audited | agent array (`Agent` = 0x5928 bytes), prop pool, `ptr_vec` etc. | step 1 |

Pools shape: `data_history.json` lists 787 "borrowed" + 790 "reached through tables" items: tables ("the pages of a pool") are runs of pointers to fixed pages = static object pools.

## Detailed plan

1. **Audit tool** (`tools/port/limits_audit.py`): scan headers for (a) arrays with literal counts, (b) `SI_ASSERT`/size asserts, (c) loops bounded by literals compared to `count`, (d) the `.bss` item list from `data_coverage.py` (largest first). Output a ranked CSV with usage counts so effort goes where mods ask.
2. **Convert pools to containers** as part of 003: replace each static pool with `std::vector<T>` (or a chunked `PagedPool<T>` if pointer stability is required - game code holds raw pointers into pools) sized from module data at load (counts are known after `parse_module_info`/`load_module_files`).
3. **Records**: after 006, make `Troop`, `PartyV`, `ItemKind`, `Agent` ordinary structs; replace embedded fixed arrays that are really limits (inventory 96, equipment 8+, notes) with sized vectors or raised constants. Keep serialisation (typed streams) length-prefixed so saves stay compatible: add a format-version bump where counts change.
4. **Registers**: `reg`, `str`, `pos` as vectors sized `max(128, module requested)`; `module.ini`/`module_info.txt` directive `registers = N`.
5. **UI pages**: 96-slot inventory pages -> dynamic; `deform_rows[0x30]` -> vector (face-gen keys are a Warband-era mod pain point).
6. **Document** each limit and its new default in `docs/limits.md`; expose in `rgl_config.txt`/module ini where it is a cost/benefit choice.

## Code sketch

```cpp
// before: GameX: Troop* troops; embedded counts are implicit in offsets (+0x13a00)
// after:
struct Game { std::vector<Troop> troops; std::vector<PartyRec> parties; ... };
// pooled objects with stable addresses
template<class T> struct PagedPool { std::vector<std::unique_ptr<T[]>> pages; T& at(size_t i); size_t size; };
```

## Risks

- Pointer stability: raw pointers into vectors die on reallocation (use `PagedPool`/deque or reserve by module counts).
- Offset-based code in not-yet-converted TUs (the 649 raw accesses) will silently read wrong data after a field changes: finish 006 for a struct before changing it; keep `static_assert` offsets under the 32-bit tag as tripwires.
- Saves: an old save with 96-slot inventory must load into the new layout (typed stream versioning), and new saves should not load in old engine (version gate).
- Behavioural expectations: some limits are gameplay (companion limit, party size 0x5ac8 stacks): do not raise without a config.

## Acceptance

Audit CSV committed; a test module with 300 troops-in-party stacks, 200 inventory slots, 512 registers and 5,000 scene props loads and plays; static image under 5 MB.

## Effort

L (8-12 wk, incremental per struct). Estimate only.
