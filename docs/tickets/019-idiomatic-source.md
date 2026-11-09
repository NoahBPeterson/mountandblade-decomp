# 019: Idiomatic source: accessors, real names, no offset arithmetic

Written 2026-11-08. Do this after byte-exactness, not during it. Every change must leave the build byte-identical: run `tools/matching/run_all.py` on each
touched module and `tools/matching/insn_align.py` on each touched non-matching row, and keep only edits where nothing moves.

**Effort:** M. **Impact:** high for modding (ticket 006 and 014 start from readable code).

## Why

Lanes chasing bytes often spell lookups the way the disassembly reads instead of the way the original source must have been written. Example
(`Agent::update_ai` 0x4465e0, fixed 2026-11-08 with no byte change):

```cpp
// before
int near_ = f55c0;
Pages pg;
if (near_ >= 0 && (pg = g_mission->agents.pages.begin(), (*(pg + ((unsigned)near_ >> 4)))[near_ & 0xf].f14 == 1)) {
    Agent* o = &(*(pg + ((unsigned)f55c0 >> 4)))[f55c0 & 0xf];
// after
int near_ = last_attacker;
if (near_ >= 0 && g_mission->agents[near_].f14 == 1) {
    Agent* o = &g_mission->agents[last_attacker];   // re-read, not near_: the original reloads it here
```

`AgentTable::operator[]` (src/game/agent_obj.h) now holds the pages-of-16 lookup; the compiler keeps the page-table pointer in a register on its own, so
the hand-cached iterator was never needed.

## Work

1. **Paged-table accessors.** Raw `pages[x >> 4][x & 0xf]` indexing remains in about 100 places (2026-11-08 count): agent_logic.cpp 52 (including the
   `AG()` macro at ~5952), party_2.cpp 11, poly_mesh.cpp 8, mission_5.cpp 8, mesh_game.cpp 6, save_records.cpp 6, script_interpreter.cpp 4,
   world_data.cpp 3. Give each paged table (agents, parties, and any other `pages` vector) an inline `operator[]` and use it.
2. **Offset-named fields.** Fields named after their offset (`f55c0`, `f14`, `f2c8`, `v56b4`, ...) get real names from how their writers and readers use
   them. `f55c0` -> `last_attacker` is the model: `take_blow` sets it from the last blow, `reset` clears it.
3. **`at<T>(0x...)` / `RAWAT` / cast views** (`*(T*)((char*)this + off)`) become members of the real struct once its layout is known.
4. **Hand-cached temporaries and helper shims** that only exist to reproduce a register or slot choice: keep the ones that are proven necessary (their
   comment says "stand-in" and why), and try the plain spelling for the rest.

Do it file by file, one lane per file, with the byte check above as the only gate.
