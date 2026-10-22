# Translation-unit map of the game code

Goal: split the game code (0x401000..0x5342a6, plus the post-CRT EH stubs / initializers / atexit thunks)
into the original `.obj` files in link order, so matched slices can be migrated into `src/<area>/<module>.cpp`.

Tools (rerunnable, ~1 s): `tools/migrate/facts.py` (per-function facts -> `work/migrate/facts.json`),
`tools/migrate/tu_map.py` (inference), `tools/migrate/objorder.py` (prints an .obj's section order; used
for the compiler experiments in `work/migrate/exp/`).

Outputs in `work/migrate/`:
- `tu_map.csv`: one row per inferred TU: `start_va`, `end_va`, `n_functions`, `code_bytes`, `module`
  (proposed name), `confidence`, `start_window` (where the start boundary could move), `regime`
  (B/T/? counts, see below), owned `.rdata`/`.data`/`.bss` ranges, post-CRT items, slice status counts
  (`exact`, `nonmatching`, `partial`, `missing` = in a slice with no row, `noslice`), `exact_share`,
  `evidence`. The last 4 rows are the import-library objects (FMOD, d3d9, d3dx9 thunks; dinput8 data).
- `tu_functions.csv`: every function with its TU, module, kind (`code`, `lib-comdat`, `eh-stub`,
  `initializer`, `atexit-thunk`), status, regime, name, and how a post-CRT item was assigned.
- `tu_boundaries.csv`: every boundary (kind, confidence, window, glue, class pair, reason) and every
  "free gap" (no same-TU evidence at all, but no boundary evidence either).
- `tu_validation.txt`: the check against the known TUs and the layout-consistency numbers.

## Result

121 game-code TUs (+4 library objects). Confidence (minimum of the two boundaries): 8 high, 2 medium,
111 low. 24 boundaries come from forward calls (reliable), 94 from class-name changes (about 50%
precision on the ground truth), 2 from emission-regime switches. 845 further gaps have no evidence either
way; each TU's `evidence` column says how many it contains ("possible inner splits"). The true number of
TUs is certainly higher than 121: the map is under-segmented, and its TUs should be read as "one or more
consecutive objects".

## What each signal is worth (measured on the known TUs)

Ground truth: the six DX9 framework TUs (src/dxframework manifests) and TinyXML 2.5
(`tinystr` 0x42d820, `tinyxml` 0x42d9a0, `tinyxmlparser` 0x42f720..0x431400; split by which file each
TinyXML function lives in, not from a map).

| signal | finding | use |
|---|---|---|
| padding / alignment | Every function is 16-aligned with `int3` fill, inside and between TUs (all COMDATs). Gaps that are not `0xCC` are jump tables. | none |
| `.rdata` order | 96.6% of items referenced from one TU are in TU order, but cl emits each function's literals/floats with the function, so the order is also monotone *inside* a TU (d3dapp: no inversion at all). Shared `/GF` strings and `__real@` constants sit with the first object that compiled them, even when that object's user was dropped by /OPT:REF (d3dsettings shows this). | glue only: out-of-order unique items (11) |
| `.xdata$x` funcinfo, `.text$x` EH stubs | Associative COMDATs of their function: exactly text order, no extra information. 136 stubs, all magic 0x19930520. | post-CRT assignment |
| `.data` / `.bss` order | Only 42% / 26% of single-TU items fall in TU order: most game globals are externs defined in another object and used by one. On the framework, file statics are clean (d3dapp 0x5d6f40.., d3dsettings 0x5d6f54, dxutil 0x5d6f58..). | weak glue (shared statics), inversions as strong glue |
| `.CRT$XCU` (0x5a8000..0x5a80c0, 46 game initializers) | The constructed globals' addresses rise monotonically with the table (object order confirmed), but owner = "TU that uses the global" gives a non-monotone sequence, so the owners are not known. Initializers and their atexit thunks are assigned to the main user TU and flagged. | ordering check only |
| forward calls | **cl 13.10 with /Og emits a TU bottom-up** (every callee defined in the TU precedes its callers; confirmed by compiling test files: `/O2`, `/O1`, `/Ox`, `/Og`, `/EHsc`, `/EHa`, `/Ob0..2`, PCH all bottom-up; only `/Od`, `/Og-`, `/Os`/`/Ot` alone give source order). Framework: 75/75 intra-TU calls go backward, 52/52 forward calls cross a TU. A forward call from such a TU proves a boundary in (caller, callee]. Compiler-generated deleting destructors (`??_G`/`??_E`) are excluded: they come before the destructor they call. | the only hard boundary evidence |
| class names (slices/symbols) | A change between two runs of different classes (each with a function >= 0x50 bytes) is right for d3denum/d3dfont/d3dsettings and gui_manager/tinystr, wrong at DXUtil/CArrayList and twice inside tinyxml.cpp (inline header methods of other classes). | low-confidence boundaries |
| register-convention callees | Only 17 functions take arguments in EAX/EDX/EBX/ESI/EDI; each such static ties its callers to its TU. | strong glue |
| few-caller helpers, near calls, vtables, shared strings | Helpers next to their only callers, calls within 6 functions, a vtable's own slots, a literal used by nearby functions. All can cross a boundary (tinystr/tinyxml is glued by two near calls; TinyXML class methods are split across files), so they only rank gaps. | weak glue |

Validation (`tu_validation.txt`):
- Framework, 6 boundaries: d3denum found exactly (forward call, high); d3dfont and d3dsettings found
  exactly (class change, low); d3dutil found with a window 0x407220..0x407a30 (forward call); dxutil
  and the end at 0x407c10 missed (no evidence: no forward calls, no statics, no substantive class
  names); one extra split at 0x407a70 inside dxutil.
- TinyXML: tinystr start found (class), tinyxmlparser found exactly (forward call, high), tinyxml
  start missed (two cross-TU near calls), parser end 0x431400 missed, two extra splits inside tinyxml.cpp.
- So: forward-call boundaries 4/4 correct; class-change ~50%; recall overall 6/10.

## Two emission regimes (open question)

Placement of a helper relative to its nearby callers (`regime` column) is bottom-up ("B", 2197
functions) in most of 0x401000..0x4cd000, but caller-first ("T", 994 functions) in 0x408000..0x40d000,
0x41f000..0x42b000 and nearly all of 0x4d4000..0x533000: single-caller helpers come right after their
caller (Scene::init -> init_scene -> Team::init -> Sub3::init). That is the order cl produces without
/Og, and also the LTCG order, but:
- `/Og-` is ruled out: slices in that region that are byte-exact with `/O2` (e.g. s00510850, s00512c80)
  are 89-3400 bytes off with `/O2 /Og-`.
- LTCG is ruled out: a test link (`/GL` + `/LTCG`, link 7.10) gives source order but puts register
  arguments on nearly every internal function and places all LTCG code before non-LTCG objects; the
  binary has only 17 register-argument functions.
- An older compiler is ruled out (2026-10-22, work/migrate/exp_compilers/): cl 12.00 (VC6 RTM and SP6+PP) emits
  source order but calls `__ftol` (the image links only `__ftol2`, called 93 times from T code) and is 39-325
  bytes off on functions 13.10 matches; cl 13.00.9466 orders callees first like 13.10 and matches none of 93
  test VAs (13.10.3052 and 13.10.3077 both match 32). cl 13.10 keeps callee-first order under every flag tried
  with /Og on, in TUs up to 9000 functions. Caller-first on 13.10 only with /Og-, /Od, or vtable calls.
  Untested: per-function `#pragma optimize`, a linker /ORDER file, a different object split.
- Pragmas, inline asm / WinLicense markers, a link /ORDER file, a per-helper object split and cl 13.10 builds
  2179 (2003 DDK), 3077, 4035 (2003 SP1 DDK) and 6030 (VS 2003 SP1) are all ruled out too (work/migrate/exp_order/).
  T code shows source-order emission (inline/template COMDATs land after their first caller: 30 vs 5; B: 1 vs
  170) AND whole-TU interprocedural register conventions for static helpers (decode_utf8 0x487b00 takes
  EDI/EDX from T callers), a combination no tested mode produces. Matching consequence: write T slices in
  binary order and keep helpers defined after their caller out of line (/Ob1 per slice or noinline):
  /Ob1 on the noinline slices gains 14 T functions, loses 4.
In T regions forward calls are normal, so the boundaries there rest on class names only (low). The
byte-exact rate is also much lower there (20-35% vs ~85% in 0x401000..0x43ffff), so whatever causes this
may matter for matching as well.

## Unresolved

- 845 free gaps; most TUs are probably 2-4 real objects.
- `.data`/`.bss` ownership, hence where each global and each dynamic initializer belongs.
- Initializer order: the 46 `.CRT$XCU` entries should map to nondecreasing TUs; the current assignment
  does not, so it is flagged per row.
- The T regime's cause.
- Link order looks alphabetical in both known groups (d3dapp < d3denumeration < d3dfont < d3dsettings <
  d3dutil < dxutil; tinystr < tinyxml < tinyxmlparser). If it holds per project folder, module names must
  sort in TU order, which could constrain naming and the unresolved gaps.

## Migrate first

Ranked by byte-exact rows (and share) among TUs with a usable boundary. "exact/nm/partial" are
function counts from the slice bookkeeping.

| # | range | module (proposed) | funcs | exact/nm/partial | exact share | conf | note |
|---|---|---|---|---|---|---|---|
| 1 | 0x42d820..0x431400 | tinystr / tinyxml / tinyxmlparser | 4+66+21 | 90/1/0 | 0.99 | known source | Use the source-file split, not the map (map: TU 37-40) |
| 2 | 0x413b60..0x415900 | text_stream | 32 | 27/4/1 | 0.84 | high | both ends forward-call |
| 3 | 0x41f250..0x41fee0 | entity | 33 | 31/2/0 | 0.94 | low/high | end forward-call |
| 4 | 0x417010..0x417bb0 | mem_stream | 30 | 29/1/0 | 0.97 | low | |
| 5 | 0x417bb0..0x419190 | resource_manager | 39 | 34/5/0 | 0.87 | low | |
| 6 | 0x40e940..0x413b60 | psys (+rnode, mesh) | 137 | 119/18/0 | 0.87 | low/high | 34 free gaps: likely several objects |
| 7 | 0x407a70..0x40ad80 | sphere / capsule / face shapes | 75 | 64/7/4 | 0.85 | medium/low | starts with dxutil's CArrayList (0x407a70..0x407c10 belongs to dxutil.cpp) |
| 8 | 0x461630..0x4625d0 | renderer | 21 | 21/0/0 | 1.00 | low | |
| 9 | 0x460160..0x4610c2 | dialog_mgr (2 TUs) | 20 | 19/1/0 | 0.95 | low | |
| 10 | 0x42a090..0x42ce50 | strat_ent / gui | 58 | 48/10/0 | 0.83 | medium/low | |

(The framework, 0x401000..0x407c10, is already migrated: src/dxframework.)

Totals over the game code: 1870 byte-exact, 1029 nonmatching, 332 partial, 50 missing, 1 not in a slice.
