# src/game migration: entity, dialog_manager, renderer

Three regions of the TU map (docs/tu_map.md, "Migrate first" #3, #8, #9) are merged from their
`match/slices/` staging slices into one translation unit each, following the method of
src/dxframework/MIGRATION.md. The slices under `match/slices/` are unchanged and stay the reference.

All three modules build with the default flags `/O2 /MT /Gy /TP` (one flag set per module; no per-row
flags). Real headers only (`<vector>`, `<list>`, `<windows.h>`, `<d3d9.h>`, `<d3dx9.h>`) plus the shared
`match/include/rgl.h` (vector4) and the headers in this directory.

Verification: `chk.py` on every module VA, `run_all.py --manifest` on each `<module>.manifest.txt`, the
resolution audit (`resaudit_all.py`, "src:" rows), and the equivalence harness on the nonmatching rows and
on every function whose source changed shape (results below).

## Summary

| module | range | functions (index rows) | byte-exact before (slices) | byte-exact after | improved | regressed |
|---|---|---|---|---|---|---|
| entity.cpp | 0x41f1d0..0x41fee0 | 34 (32 + 2 catch funclets) | 31 | 32 | 1 | 0 |
| dialog_manager.cpp | 0x460160..0x461200 | 22 (21 + 1 interior VA) | 20 | 20 | 0 | 0 |
| renderer.cpp | 0x461630..0x4625d0 | 21 | 21 | 21 | 0 | 0 |
| **total** | | **77** | **72** | **73** | **1** | **0** |

Nonmatching rows (`<module>.nonmatching.txt`), none of them a function with its own symbol:
- entity 0x41f6e6, 0x41f880: the `catch(...)` funclets of the two `_Insert_n` instantiations (0x41f680,
  0x41f720). cl emits them inside the parent's COMDAT with no symbol, so run_all cannot address them;
  `cmpobj.py` over the parent's whole COMDAT (len 156 and 581) is MATCH, 0 differing bytes.
- dialog_manager 0x4610b0 / 0x4610c2: `DialogMgr::start`. The original brackets the body with two 18-byte
  markers `jmp +0x10; "WL  ", id, 0, "WL  "` (id 0x0c at the start, 0x0d before `ret 0x14`). These are the
  **WinLicense / Themida (Oreans) SDK** code markers, which the SDK writes as `__asm _emit` sequences (the
  rules forbid `_emit`). The body between them is byte-identical; 0x4610c2 is the interior VA right after the
  first marker. Same marker pattern as 0x51dbd0, 0x4711e0, 0x4f63c0 (other slices).

No `partial.txt` rows, so none was created.

Improved (slice status -> module status):
- 0x41f1d0 `FxElem::~FxElem` (was `owner_t::~owner_t`, nonmatching 13 bytes, "scheduling") -> MATCH. It
  was not in the map's range (see Boundaries). The fix is freeing the owned array in the destructor
  body (`if (has_arr) operator delete(arr);`) instead of through a `guard` member's destructor: cl then
  schedules the vptr store exactly as the original (checked: it matches with either `std::vector` or the
  slice's `vec<>` for the two list members; the module uses `std::vector`, as FxElem in s00421110 does).

Regressions: none. Every function byte-exact in its slice is byte-exact in its module.

## Boundaries (what changed against the brief)

The forward-call rule (cl 13.10 emits a TU callee-first, so a call to a higher address leaves the TU)
was re-checked with `work/migrate/facts.json` over each range and its neighbours.

- **entity: start moved from 0x41f250 to 0x41f1d0.** The map's start (0x41f250, "class world|owner_t",
  low) is a class-name boundary placed after 0x41f1d0 only because 0x41f1d0 had no symbol name in the map
  (it was a nonmatching row). 0x41f1d0 is the destructor of the same class as 0x41f250 / 0x41f2a0 (vtable
  0x54b454, the 0x1dc-byte effect element `FxElem` of s00421110 / s00428b10 / s004291c0), so the class
  change is world -> FxElem at 0x41f1d0. No forward call constrains it either way (0x41f1d0 calls only
  `operator delete`); evidence is class-name level (low), stated as such.
- **entity: end 0x41fee0 confirmed** (forward call 0x41fe90 -> 0x41fee0, `entity::set_local_scaled` ->
  `smat::scale_rows`, high). Consequence: `scale_rows` is in the next object, so the slice's
  `-DOPAQUE_SCALE_ROWS` build of 0x41fe90 is unnecessary: the module simply declares it.
- **dialog_manager: end moved from 0x4610c2 to 0x461200.** The map's high-confidence "forward call
  0x4610b0 -> 0x4610c2" is not a call: it is the `jmp +0x10` over the WinLicense start marker inside
  `DialogMgr::start` (0x4610b0..0x461132 is one function). The TU therefore runs at least through
  `DialogMgr::advance_all` (0x461140), which calls `refresh_text` (0x460c10) and `collect` (0x460de0)
  backwards and is a DialogMgr member. 0x461200 (`Agent::init_max_hp`, single caller 0x44c210) and the rest
  of the map's "wind" TU have no evidence tying them to the dialog code and stay out.
- **dialog_manager: one TU, not two.** The brief's second TU (map split at 0x460ab0, "class
  DialogRec|DialogMgr", low) has no forward-call evidence, and the class-name signal fails here: DialogMgr
  methods sit on both sides (0x460620..0x4606c0 before, 0x460ab0.. after). Every intra-range call goes
  backwards (expand -> expand_token, DialogRec::read -> Block::read, DialogMgr::read -> DialogRec::read /
  `??_EDialogRec`, refresh_text / collect -> expand, advance -> select / collect, ...), and the literals of
  expand_token (0x591298..0x5912e0) and DialogRec::read (0x5912e4, 0x5912fc) are contiguous. A split at
  0x4607d0 or 0x4609d0 cannot be excluded (both are "free gaps"); without evidence the region is kept as one
  module.
- **renderer: 0x461630..0x4625d0 kept.** Both ends are class-name boundaries (low) with no forward call in
  or out of the range; there are 14 free gaps inside, so it may be several objects (the frustum helpers,
  the buffer pair, the cell grid and the envelope setters are candidates). One literal inversion
  (set_specular's "vSpecularColor" 0x5914a0 above mesh_part::set_state's "fMaterialPower" 0x591490, while
  set_state calls set_specular) is not explained by a boundary (a boundary would need the opposite order).

Engine vs game: entity.cpp and renderer.cpp are engine (rgl) code by content (scene entity / frame
objects, the D3D9 render-state cache and effect uniforms); dialog_manager.cpp is game code (Module System
dialogs, string registers, parties, troops). All three are under src/game for now, as instructed.

## Shared headers (one per class used by more than one module)

- `tstring.h`: the engine string (16 bytes: read cursor, buffer, capacity, length; members 0x42bac0..0x42c900).
  Used by entity (`entity::type_name` returns one: 0x42bbe0) and dialog_manager. `rgl.h` declares the same
  class as `rgl::string` with unverified field names; this is the verified layout (expand_token / expand read
  `buf`/`len` directly).
- `mat4.h`: the 64-byte frame (`rgl::mat4`: s, f, u, o; same layout as `rgl::matrix`) with the out-of-line
  operations: `operator*` 0x40e870, `transform_point` 0x408460, `transform_point_to_local` 0x407f70,
  `relative` 0x4329d0, `identity()` 0x40cd00 (by value: hidden return pointer). Replaces the slices' `mat4`,
  `pos`, `matrix64`, `frame4`/`rot3` casts, `smat` base, and the two declarations of 0x40cd00
  (`pos identity_pos()` and `pos* identity_pos(pos*)`): both renderer callers match with the by-value form.
  The 3x3 `rot3` and the constructor `mat4(rot3, origin)` reproduce set_view's 12-dword + 4-dword copies.

Per-module headers: `entity.h` (rgl::entity, FxElem, rgl::scene, rgl::mesh fields, pool, vec, world's entity
list), `dialog_manager.h` (stream_base, GameData with PartyMgr embedded at +0xf8, TroopRec, ScriptState,
Block/Elem, DialogRec, DialogMgr), `renderer.h` (renderer, material, mesh_part, mesh_inst, frustum,
dev_buffer/buffer_pair, cell_rows, envelope). Each field carries the address of a function that reads it.

## Canonical names (slices -> module)

| slices | module |
|---|---|
| `owner_t` (s0041ee20) | `FxElem` (vtable 0x54b454; the name s00421110 / s00428b10 / s004291c0 and their symbols use) |
| `sub_t` / `kid_t` (s0041ee20) | `rgl::mesh` (psys list element: kids at +0xb0, data at +0xc0, id at +0x1a8) |
| `scene_node::destroy()` | `rgl::scene::~scene()` (a destructor: see below) |
| `world_b` | `world` (only its entity vector at +0x7c) |
| `resmgr_t` | `rgl::resmgr` (0x5d6fa0) |
| `std::list<item_t*>` erase/_Insert/_Insert_n | `std::list<int>` (the instantiation list<int>::resize uses) |
| `mat4`, `pos`, `frame4`, `rot3`, `V4` mat4, `identity_pos` (2 forms), `identity` | `rgl::mat4`, `rgl::rot3`, `rgl::identity()` |
| `rgl::string` return of type_name | `tstring` |
| renderer `f558`, `f5ec`, int stores to `spec[]` | `cur_ps`, `material_power`, float `spec[]` |
| envelope `time` (+0xec, s00461bd0) / `time` (+0xf4, s00462540) | `fade_time` / `time` |
| GameData `f139d4` + `g_parties` cast | `GameData::parties` (PartyMgr at +0xf8), `parties.deflt` (+0x139d4) |

## Removed slice-local tricks

- `-DOPAQUE_SCALE_ROWS` (s0041f720): unnecessary, `smat::scale_rows` is in the next TU.
- Per-row `/EHa` (s0041f720's STL rows): every row matches with `/O2 /MT /Gy /TP`; with `/EHa` or `/EHsc`
  the entity constructor gets an EH frame, with `/EHsc` vector::_Insert_n changes too.
- The separate compile `s0041ee20_b.cpp` for `erase_entity`: merged (the std::remove declaration stays, below).
- `owner_t`'s hand-written `vec<>` members and `guard` member: real `std::vector`s; fixes ~FxElem (above).
- `scene_node::destroy()` with three explicit `clear()` calls: it is the destructor; the three `vec`
  members are destroyed in reverse declaration order, which is exactly the original's tail.
- `DialogMgr::select_inl` (`__forceinline` copy of `select`): cl inlines the plain `select()` at the same
  two sites (advance, advance_all) and keeps the call at the third.
- `advance()`'s byte test `((unsigned char*)&flags)[2] & 4` and its redundant inner `if (n > 0)`:
  written like advance_all (`while (n > 0 && (flags & 0x40000)) {...} if (n > 0) select(sel);`).
- `destroy_dialogs()` dummy + out-of-line `~DialogRec() {}`: the implicit destructor; `??_EDialogRec` is
  generated by the `delete[]` in DialogMgr::read.
- `*(int*)&spec[i] = 0xc36a3bf6` and the other int-typed sentinel stores in reset_state: float literals
  (-234.23422241210938f, -12312.2138671875f, ...); cl stores the same immediates.
- `post:` label (unused) in expand_token.

Kept, with reason:
- `std::remove` declared but not defined in entity.cpp: erase_entity calls the instance in the previous
  object (0x41f080) as an opaque function; with `<algorithm>`'s body visible cl keeps `end()` in a register
  across the call. Why the original TU did not see the body is not known.
- `vec<T>` (hand-written VC7.1 vector layout whose `clear()` frees): the game's own container (also used by
  s0040d7d0 for the collision shapes); the out-of-line `vec<mface>::clear` (0x41f4b0) is kept as an explicit
  instantiation.
- `*(volatile int*)&b.count` in `buffer_pair::restore_all` (0x4623c0): the original re-reads `count` after
  the loop test; three natural spellings (inline release helper, indexed form, type==0 branch) give 65-76
  differing bytes.
- `(tstring&)` cast of expand_token's `const tstring&` argument (the function edits the token in place).
- Explicit template instantiations as row labels; template rows are marked `// 0x<va> <name>` without `@`,
  because a `// @` marker binds the next *definition* for chk/equiv/resaudit, and explicit instantiation
  lines are skipped.

## Verification results

`run_all.py --manifest`: entity 32/32, dialog_manager 20/20, renderer 21/21 byte-exact.

Resolution audit (`resaudit_all.py --slices src:src/game/<m>.manifest.txt`, log
`work/migrate/scratch_game_resaudit.jsonl`): 73 rows, 71 agree. Two rows the "src:" sweep cannot bind to the
right symbol; both audit **agree** when bound through their slice manifest (equiv.py `<slice> <va> --src
<module>` binds by the slice's mangled name, which the module keeps):
- 0x41fa00 `std::vector<Obj*>::resize`: a module row is bound by name, and `vector::resize` has two
  same-size instances here (mesh* 0x41f970, Obj* 0x41fa00); the sweep picks the mesh* one for both, whose
  `_Insert_n` resolves to 0x410610, so it reports "disagree" (original 0x41f720). This is a binding artefact:
  `run_all` checks the Obj* symbol named in the manifest, and the slice-bound audit agrees.
- 0x4609d0 `??_EDialogRec`: a compiler-generated symbol has no source name to bind ("not-audited").
Local copies (as in the slices): 0x41f640 list<int>::_Buynode / _Incsize (original 0x41eeb0 / 0x41a1e0),
0x41f720 vector helpers, 0x41fb00 vector<E100>::_Insert_n (0x410dc0).

Equivalence harness (`--flags "/O2 /MT /Gy /TP"`, log `work/migrate/scratch_game_equiv.log`):
No mismatch in any run.
- PASS: 0x41f1d0 ~FxElem (newly byte-exact), 0x41f4e0 ~scene (was `destroy()`), 0x4609d0 ??_EDialogRec
  (bound through s004608d0), 0x460d80 select, 0x461630 set_view, 0x462170 view_matrix, 0x4623c0 restore_all.
- WEAK, no mismatch: 0x41fa00 vector<Obj*>::resize (bound through s0041f720; one edge, the length-error
  throw path, unreachable for the harness: RaiseException discards); 0x460ff0 advance and 0x461140
  advance_all (5 of 6 edges uncovered on both sides: the loops depend on what collect() returns, which the
  harness's random dialog tables almost never make non-empty; same edges missed on both builds).
- UNSUPPORTED (harness): 0x41f6e6, 0x41f880, 0x4610c2 (no symbol of their own: funclets / interior VA);
  0x4610b0 (orig-abi-broken: the VA is the jmp over the WinLicense marker).

## Problems / flags

- The "src:" resolution sweep binds rows by source name only (`symbols/slices/<module>.txt` would give it the
  mangled names, but symbols/ is outside this task's write scope). Two rows need the slice-bound run (above).
  A `symbols/slices/entity.txt` with `0041fa00 ?resize@?$vector@PAUObj@rgl@@...` and
  `symbols/slices/dialog_manager.txt` with `004609d0 ??_EDialogRec@@QAEPAXI@Z` would make the sweep clean.
- tu_map: the 0x4610b0 -> 0x4610c2 "forward call" is a jmp over a WinLicense marker; facts.py treats the
  `jmp` as a tail call. Other marker sites (0x4711e0, 0x4f63c0, 0x51dbd0) may produce the same false
  boundary.
- Element type names of the STL instantiations (`E100`, `rgl::Obj`, `rgl::mesh`) are kept as the
  neighbouring slices registered them, so the resolver maps their ICF-folded callees (0x410dc0, 0x410610).
