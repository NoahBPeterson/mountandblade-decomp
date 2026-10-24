# M4 migration: src/game/world_data (0x490030..0x499830)

Source slices: tu83 world_2, tu84 game_data_2, tu85 rgl_mesh, tu86 hud2, tu87 obj_tbl (112 functions, 109 distinct
symbols: 0x492930 and 0x492c96 are cold funclets inside the COMDATs of 0x4927d0 / 0x492b30). Flags for every row:
`/O2 /MT /Gy /TP`.

## Result

| module | range | functions | byte-exact before (slices) | byte-exact after | nonmatching | partial |
|---|---|---:|---:|---:|---:|---:|
| world_data | 0x490030..0x4958f0 | 90 | 23 | 46 | 42 | 0 |
| mesh_hud | 0x4958f0..0x499050 | 15 | 4 | 4 | 11 | 0 |
| obj_tbl | 0x499050..0x499830 | 7 | 3 | 3 | 4 | 0 |
| total | | 112 | 30 | 53 | 57 | 0 |

(The two funclet VAs count as functions but have no manifest row: run_all/cmpobj compare the whole COMDAT, so they are
verified byte-for-byte inside 0x4927d0 (581 bytes) and 0x492b30 (587 bytes).) No byte-exact slice function regressed.

New byte-exact versus the slices: 0x4902a0, 0x490310, 0x4903e0, 0x490430, 0x490a30, 0x490fa0, 0x492170, 0x4927d0,
0x492b30, 0x492d80, 0x493090, 0x4936b0, 0x493830, 0x493960, 0x493a30, 0x493c40, 0x493d70, 0x493db0, 0x493df0, 0x493e90,
0x493f80, 0x494310, 0x4955e0 and others (see the manifests).

## Boundary decisions

The region is emitted bottom-up (regime "B"); no function has forward-call evidence for a TU edge here, so the five
provisional slice TUs are low-confidence class-name changes. Fewer, larger modules:

* world_data = tu83 + tu84 (0x490030..0x4958f0): GameData / PartyMgr / Party methods, the container and list panels,
  the window / sound managers, the tableau light table, std instantiations (`std::deque<int>`, `std::vector<__int64>`,
  `std::vector<SceneObject*>`) that sit between them. There is no gap or alignment break between the two slices and
  the std COMDATs (0x492d80, 0x493090, 0x493120) are shared by functions of both.
* mesh_hud = tu85 + tu86 (0x4958f0..0x499050): rgl::mesh derivatives (`MeshX`) and the HUD bar widgets built from them;
  `HudBar` / `build_legend_mesh` call `MeshX` members and share the `Color4` helpers.
* obj_tbl = tu87 (0x499050..0x499830): kept separate; it builds meshes through `MeshY : MeshX` and the tableau table
  and uses a different `rgl::mesh` model (spawners) from mesh_hud.

## Canonical headers

Included (not redefined): game_data.h, party.h, troop.h, game_recs.h, msg_log.h, str_table.h, vec.h, mission.h,
agent_obj.h, rec1c8.h, rec_dtors.h, i64_vec.h, e68.h, rgl/mesh.h, rgl_math.h, mission_ui.h, ui/mission.h,
config/widget_mgr.h. No canonical header was edited; the module adds "view" classes (`PartyW : Party`,
`PartyMgrW`, `GameW : GameData`, `MissionW`, `AgentW`, `ContainerW`, `MeshX : gui_mesh`, `HudBar : MissionUi`,
`WidgetMgrW : widget_mgr`) with raw-offset accessors (`WD_FLD(T, off)`). Offsets are proven by the cited function
addresses in the comments (e.g. Party +0x30/+0x34 position 0x491e50, follower list 0x4915d0, GameData party table
0x4910a0/0x4918d0). Other agents edited party.h / game_data.h / troop.h / rgl/mesh.h concurrently; compile errors
from their in-progress edits were retried.

Locally defined (no canonical home): `ContainerBase` ctor/dtor views (0x4931a0 / 0x490520), `Rec98` pool, `E68W`,
`SoundW`, `WindowMgrW`, tableau light/scene records (`tableau.h`), `SceneObject` / `SceneBase` views, HUD bar helpers.
`widget_mgr_w.h` extends the canonical `widget_mgr` with add_button 0x494110, add_slider, add_mesh_widget*.

## Tricks removed / kept

Removed: the hand-written STL emulations of the slices (vectors, deques, `_Growmap`, `_Insert_n`) are replaced by real
explicit instantiations (`template void std::deque<int>::_Growmap(size_t);` etc.), which reproduce the original
bytes exactly. The slices' local re-declarations of Party / GameData / Troop prefixes are gone.

Kept: raw-offset view accessors; opaque `RecS/RecT/RecQ/RecPoly` destructor records; one `noinline` static
`decode_utf8_487b00`; the `IntVecW::_Insert_n` wrapper for the ICF-folded `vector<int>` code; float zero stores in
`EmitterCtl::add` (floats compile to immediate stores, ints share a register: 0x4955e0 now exact); `new rgl::mesh`
cast to `MeshX*` (a derived `new` would add a vptr store).

## Verification

* `run_all.py --manifest` on each manifest: 46/46, 4/4, 3/3 byte-exact. `compile_all.py --filter src/game/world_data`: 3/3.
* Resolution audit (53 rows): 52 agree, 1 unverified (0x4936a0, a 5-byte tail-jump thunk), 0 disagree. Three local
  std copies (0x493830, 0x4949d0 `vector<int>::_Insert_n` -> 0x43e7b0; 0x4955e0 `push_back` -> 0x440b30) are mapped in
  `symbols/slices/world_data.txt` (+ phys_set / s00440a30 for 0x440b30).
* Equivalence sweep of the 57 nonmatching rows: 13 PASS, 29 WEAK (harness cannot reach the same edges on both sides,
  no mismatch), 13 UNSUPPORTED (pure-virtual imports, register ABI, unresolved harness references, uninitialised
  harness buffers), 2 FAIL: 0x4947a0 and 0x4948c0, which are bracketed by a WinLicense "WL " tag that cl cannot emit
  (permanent nonmatching, as is 0x492750). Real bugs found and fixed during the sweep: 0x4931a0 / 0x4931d0 called the
  base panel ctor 0x48ae20 with `this` as a stack argument (now a thiscall member, PASS); 0x495f00 passed the packed
  colour by value to 0x40e4e0 instead of by pointer (resolution audit disagree, fixed).

## Remaining nonmatching (all complete source)

Mostly register allocation / stack-slot ordering / x87 scheduling: recruit, handout, dissolve, can_transfer,
gate_rating (0x4910a0..0x491b20), the HUD bar updates (0x496100..0x4985f0), count_party, mark_kind, pick_near*;
ctors 0x4931a0 (vtable store ordering), 0x4934d0, 0x493ab0. Reasons per row are in the `.nonmatching.txt` files.
