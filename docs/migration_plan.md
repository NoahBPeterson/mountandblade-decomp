# Migration plan: match/slices -> src/

Goal: every function with compilable source lives in `src/<area>/<module>.cpp`, one file per original
translation unit (TU), in link order, with one header per class. `match/slices/` is staging only; it is
retired region by region once the modules pass. This is the shape the relink (PLAN stage 5) needs.

## Where we are (2026-10-22)

| | functions |
|---|---|
| Migrated (`src/dxframework`, `src/tinyxml`, `src/game`, `src/rgl` in progress) | ~360 |
| Compilable, still only in slices | ~2,100 |
| Inferred TUs left (`work/migrate/tu_map.csv`) | 96 |
| ...with byte-exact share >= 80% / 50-80% / < 50% | 425 / 859 / 1,324 functions |

Lessons from the first four migrations (dxframework 82, tinyxml 91, game 77, rgl):
- Migration *raises* the byte-exact count (+13, +1, +1): one consistent header and the real library source
  remove slice-local hacks that blocked matches.
- TU boundaries from the map must be checked per module: forward calls are hard evidence, but a jump over a
  WinLicense marker looks like one (fixed per module so far; the map tool must learn it).
- The resolution audit binds `src:` rows by source name only; same-size template instances and
  compiler-generated functions need `symbols/slices/<module>.txt` lines. Fix the tool instead.
- One agent per 2-3 TUs (~70-100 functions) took 25-35 minutes with no regressions.

## Rules (unchanged from the pilots)
1. No regressions: every function byte-exact in its slice is byte-exact in the module; run_all on each
   `<module>.manifest.txt`.
2. Resolution audit: every byte-exact row `agree`.
3. Nonmatching rows pass equiv (or UNSUPPORTED with a stated reason).
4. One header per class under the area's directory; layouts cite the function addresses that prove each
   offset. Never two definitions of a class across src/.
5. Slices stay untouched until their region's modules are committed; then they are deleted in a
   separate commit (git history keeps them).

## Directory layout
- `src/rgl/`: the engine (streams, resources, renderer, meshes, physics glue, effects, entities).
- `src/game/`: M&B game logic and UI (troops, parties, missions, screens, the script interpreter).
- `src/tinyxml/`, `src/dxframework/`: third-party code as shipped.
- `src/include/`: headers shared between areas (math, tstring, containers). The `mat4.h`/`tstring.h` now in
  `src/game/` move here once a second area needs them.
Module names come from content (classes, strings); rename freely when better evidence appears.

## Phase 0: tooling (me, ~half a day, before wave M2)
1. `resaudit_all` / `find_symbol`: bind `src:` rows through the module manifest's mangled names.
2. `tu_map.py` / `facts.py`: skip WinLicense marker jumps (`EB 10 57 4C ...`) when collecting forward calls.
3. A `tools/migrate/verify_module.py <module>`: run_all + audit + equiv on the nonmatching rows + status,
   one command for agents and for me.
4. Retire the slices already migrated (dxframework region, tinyxml, entity/dialog/renderer, rgl).
5. Move shared headers to `src/include/` and fix includes.

## Phase 1: wave M2, the clean regions (~425 functions, A band)
TUs with >= 80% byte-exact share, mostly 0x408000..0x48b000. Group neighbouring TUs so each agent owns a
contiguous run and its classes:
- M2a 0x407c10..0x40e940: sphere/capsule/face/manifold/composite (physics shapes, ~120 functions)
- M2b 0x41ef30..0x421df0 + 0x427d80..0x42a090: world, mesh/strategic/skeleton entities, fx defs
- M2c 0x42a090..0x42d820: strat_ent, gui manager
- M2d 0x433bf0..0x437830 + 0x441360..0x441ff0: curves, agent, physics set
- M2e 0x462ac0..0x4636d0 + 0x488560..0x48b930: inventory, effects, mission UI
Five Claude agents in parallel (disjoint directories / class sets), ~35 minutes each, I verify and commit
each module. Expected: ~425 migrated, a few more byte-exact.

## Phase 2: wave M3, the B band (~860 functions)
Same method; more nonmatching rows means more equiv work per module. Split into ~10 agent runs of 80-100
functions, by region, after M2's headers exist (they cover most shared classes). Run 4-5 at a time.

## Phase 3: the C band and the "T regime" region (~1,320 functions, mostly 0x4d4000..0x533000)
Blocked on two things:
- the DeepSeek redo wave (r137-r178) finishing the 333 partial functions;
- the compiler question (an older cl for that region?), being tested now. If a different compiler built
  it, those modules get their own flags/toolchain in the manifest, and matching there should improve
  before migration.
Migrate after both, same method.

## Phase 4: the script interpreter (0x49fa10, ~70 KB, one function)
Own track: one `src/game/script_interpreter.cpp` written case-range by case-range against
`header_operations.py` names; not suitable for a slice wave.

## Phase 5: link-order skeleton
Once most TUs exist: a link-order list of all modules + the CRT, globals defined in their owning TUs
(`.data`/`.bss` ownership from the initializer table), and a first trial relink with link 7.10.

## Tracking
`tools/progress.py --plot` (treemap: game / engine / 3rd party -> file -> function, area = instructions,
colour = verdict) and `docs/STATUS.md` "Source tree" table after each commit.
