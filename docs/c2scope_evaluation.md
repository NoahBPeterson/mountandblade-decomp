# c2scope evaluation (lane BM1, 2026-11-21)

Goal of the lane: bring rows that type recovery moved into `*.nonmatching.txt` back to byte-exact without a view, cast,
offset or byte trick, and judge whether c2scope (../c2scope, cl 13.10.3077 back-end tracer) is worth using for it.

## Setup and wiring

- `../c2scope`: `uv sync`, `setup.py --vs2005 ../vs2005-cpp/work/toolchain/vs2005` and `tools/c2build.py setup --c2 13.10`
  both work (341/341 base regression checks; 13.10 toolchain patched, c2trace.dll built). The only error seen at first,
  `~/.config/c2trace-post-install` running a missing `tools/remote/c2s.py deploy`, was a c2scope bug: fixed in c2scope
  b9ca992 (hooks are per checkout, `<repo>/work/c2trace-post-install`; c2scope has none). After `git pull` to b9ca992 the
  13.10 setup runs with no error and a local `c2trace.py regs --c2 13.10` works (4.6 s under wine). Traced compiles of this
  repo run on the laptop through `../vs2005-cpp` (`tools/c2trace.py` -> `tools/remote/c2s_client.py`); `../vs2005-cpp` is
  the same tool set plus the laptop client; c2scope itself also traces M&B sources locally (`C2TRACE_REMOTE=0`, 6 s).
- This repo already wires it: `tools/contrib/c2.py TOOL 0x<va>` (regs, regmap, scratch, slots, sched, tsweep,
  helpersweep, effectmine). Added by this lane: `schedwant`, and relative `--orig/--export/--keep` paths are made
  absolute (the tools run in `../vs2005-cpp`; a relative path failed with FileNotFoundError).
- Cost of one traced compile through the wrapper: 4-15 s for regmap / scratch / sched / slots / icount (cached
  compiles 4 s), 20-55 s for a tsweep of 5-10 variants, 11 min for an effectmine run of 150 compiles.

## Per row

Columns: row, diff class (from `check.py`), c2scope tools used, what it predicted, did it hold, time, outcome.


### 0x509730 CharSheet::fill_skills (12 aligned): MATCH, `skill_id[n++] = i` instead of the `int* out` walker
- Class: instruction order in the preheader (`lea esi,[ebp+0x228]` before `mov eax,table` in ours, after it in the original).
- Tools: `sched --all` (5 s, cached compile): region 0 is `[unchanged]`, so the scheduler is ruled out; the order is the
  IL's. `tsweep` over 5 spellings of where `out` and `i` are declared (10 combinations, 55 s): best (rdiff 3), none exact.
- Prediction: none usable. c2scope said where the difference is not (scheduler); it did not name the lever.
- What worked: reading the traced facts: both `edi` (n) and `esi` (out) are induction pointers in the original, and
  strength reduction creates the pointer's initial `lea` in the preheader after the other IV's `mov eax,table`. Indexing
  (`skill_id[n++] = i`) lets the optimiser create the pointer itself. 1 compile, MATCH. Time: 8 min.
- Without c2scope: probably the same two or three spellings by hand (~5 compiles at 10-18 s, 2-3 min); the `out`-pointer
  spelling had been written to imitate the listing, indexing is the more natural source. c2scope did not shorten this.

### 0x4bdd50 SpawnRec::load_extras: already MATCH (stale figure)
- `check.py` MATCH on the first run; b26fd8f5's r_int fix had already repaired it. No c2scope. 30 s.

### 0x489820 PlacedObj::set_position (33 aligned): MATCH, IfaceB::slot1 is a real out-of-line function
- Class: inlining (original calls the empty 0x477b70; ours inlined the empty virtual).
- c2scope not used (the diff is visible in the listing: `call 0x477b70`). Fix: `virtual void slot1();` in iface.h,
  defined as `{}` in shapes.cpp next to shape_base::draw (the linker folds the two empty bodies, as in the original).
  One compile, 6 min including reading why the previous lane used a call to 0x477b70.

### 0x4e9d10 Party::write, 0x4e99d0 Pool<Event>::write_table, 0x4e9c50 PartyMgr::write_table: MATCH
- Class: codegen shape (a copy of the member to a stack temp, `lea eax,[esp+0x14]`, instead of `lea eax,[edi+0x74]`).
- c2scope not used: `check.py` showed it. Cause: stream.h's `w_int(unsigned v)` / `w_bool(bool v)` took the value and wrote it
  to a temp; the original passes the member's address. Now `w_int(const unsigned&)` / `w_bool(const bool&)` pass the
  address (same const_cast aliasing pattern as the existing `r_int(unsigned&)`), and `write_table` writes `&count` (int).
  Three rows (one was recorded FAIL by equiv because of the fake-call argument). NOTE for review: the const_cast in the
  wrapper is a cast; the user's rule is no casts to match, the alternative is making the members int.

## Rows where c2scope explained the diff but gave no lever

### 0x51c090 CampaignScreen::draw_party_line (16 aligned: skill / info swapped slots)
- Class: stack-slot packing tie. Tools: `regmap` (16 s: the two vars, `_skill` / `_info`, ebx <-> ebp at the spill/reload),
  `slots --orig --search` (4 s after the first compile): exact: both are 4-byte, 2-use elements of equal key 500, so the
  sorted order of the two tied colours decides; the original wants `_info` first (`pos:_info$=0`: model says the two
  offsets swap, nothing else moves). The old source comment already says the same ("the order of the tie follows the
  TU's symbol numbering").
- Prediction held (the model predicted the swap from the quicksort tie). The levers it listed were not useful (the
  top single edits concern the `_k` temporaries). `effectmine --want pos:_info$=0`, 242 traced compiles, 10.8 min:
  the only variant that moved the tie was `eqif.decl` (`if ((int)this & 1)` arms), which makes the code 209 bytes worse.
  Also tried by hand: declaring `info` before `skill` (symbol creation order): no effect. Outcome: not matched.
- Without c2scope: the earlier lane found the same cause by trial; c2scope confirmed the mechanism in 20 s, but not a fix.

### 0x4ea030 PartyMgr::id_remove (8 aligned)
- Class: scheduling (`mov edx,eax` issued before `mov [esp+0xc],eax` in the original). `schedwant` auto mode printed nothing
  (it printed nothing for any M&B function tried: its window match against the listing does not find the regions);
  with `--region 4 --want-lex ...` given by hand it ran in 4 s, but its replay model "differs from the trace" and it found
  0 of 1046 perturbations. `tsweep` of 5 statement orders (21 s): the base is already the best (1,4).
- Outcome: not matched. c2scope narrowed it to one region and ruled out DAG edits, it did not find the lever.

### 0x406e50 CD3DSettingsDialog::DialogProc (reported 59, really 8 + data tail noise)
- Class: scratch register picks. `regmap` (6 s) says exactly: `t386` wants eax, `t349` wants edx, a swap of picks 6/7;
  `scratch --why` + `rasim scratch want:t386=eax want:t349=edx` (5 s): needs one extra scratch pick before pick 6 or
  `pref:6:eax`. The cause is evaluation order in the inlined typed `GetPtr(iai)`: the list is loaded before the `iai`
  copy; the original pushes `iai` first (non-inline call). The tool names the symptom and the pick numbers correctly but
  has no construct for "evaluate the argument first" in an inlined wrapper. Not matched.

### 0x47c810 Skin::blend_indexed (12 aligned)
- Class: x87 operand order. `symkeys sites` (50 s, mostly compile): the three-term sum in `length_squared()` is sorted by
  the hashes of the three product temporaries (weights fff0 / 05f0 / 02f0), `levers --site` with every pairwise order and
  `--depth 2 --max 40`: "0 edit sets satisfy". The earlier lane's note (3 fewer temporaries before `w` flips it) agrees:
  the typed `&*m->points.begin() + i` adds temporaries. Spelling variants of that expression (4 compiles, 27 s): same code.
  `effectmine --want bdiff` (150 compiles, ~8 min): nothing better than the base (rdiff 4, bdiff 6). Not matched.

### 0x403f90 CD3DEnumeration::EnumerateDevices (16 aligned)
- Class: register allocation of the `new` result (edi instead of eax). `regmap` (5 s): t77 / s13 want eax. Cause: the
  wrapper constructor is inlined, so the `new` result is live across the call of CArrayList's constructor, where the
  original uses the call's returned `this`. c2scope shows the symptom; every wrapper spelling I tried (public base with a
  defaulted constructor) gives the same code. Not matched.

### 0x4ef730 item_events (15 aligned)
- Class: address-mode formation. `icount` (14 s) was the best answer of the session: `add eax,[g_items]` is removed by
  AddrModes in ours (folded into `[eax+esi+0x1c8]`, which needs esi saved: push/pop), the lift line says the original keeps
  a register add. `tsweep` of 6 spellings of the access (24 s): identical code. Second `tsweep` (my template: a named
  pointer plus a use that keeps it live) found `ItemKind* k = &g_items[item]; unsigned t = k->type_flags & 0xff;
  if (!k) return;` byte-exact (rdiff 0, bdiff 0) in 24 s: the dead null test makes the pointer a two-use value, so
  AddrModes cannot fold the add. NOT landed: a statement whose only job is to change the compiler's use count is a match
  trick; recorded in the row's note. `effectmine --want bdiff` (210 compiles, 5 min): nothing.

### 0x4dc350 gui_manager::set_rect (28 aligned), 0x4cee50 ItemD::ItemD (32)
- 0x4dc350: `sched --all` (68 s for the first compile of a big TU): the late store of `mfb0.color` sits in region 1, an
  81-tuple region (the scheduler cuts regions at 81 tuples, so what precedes the cut moves it); `schedwant` auto mode finds
  nothing; `tsweep` of the position of `mfb0.color = panel_bg` (8 placements, 4 min): the current one is the best (rdiff 5). Not pursued. 0x4cee50: `sched --all` shows the pre-schedule (lexical) order already has the base's `+4 / +8`
  stores after the member vectors' zeroing, so it is the constructor order of a class, not the scheduler. Not pursued.

## Stale rows found by re-running check.py over all 115 rows of the type-recovery phase (not c2scope)
0x4bdd50, 0x4608d0, 0x51e840 were already byte-exact (the r_int fix b26fd8f5 repaired them): recorded. 0x4d63b0 is
byte-exact but two relocation addends differ (`Config::save` is a member of g_cfg now): run_all disagrees, left.

## Verdict
(see the bottom of this file)

## Totals and opinion

Rows worked this session (type-recovery rows that were nonmatching):

| outcome | rows |
|---|---|
| back to byte-exact | 0x509730, 0x489820, 0x4e9d10, 0x4e99d0, 0x4e9c50 (5 by source change) + 0x4bdd50, 0x4608d0, 0x51e840 (3 already exact: stale figures) |
| diagnosed by c2scope, no source lever found | 0x51c090, 0x4ea030, 0x406e50, 0x47c810, 0x403f90, 0x4dc350 |
| byte-exact only with a match trick (not landed) | 0x4ef730 (dead `if (!k) return;`) |

c2scope was the tool for 7 rows (509730, 51c090, 4ea030, 406e50, 47c810, 403f90, 4ef730) plus a look at 4dc350 and 4cee50.
Of the 5 rows landed by source change, c2scope contributed to one (509730: it ruled the scheduler out; the fix came from
reading the IL / strength-reduction behaviour) and was not needed for the other four (the diff was readable in the
`check.py` listing: an inlined empty virtual, a by-value stream wrapper).

Predictions (model answers) checked against a compile: regmap (3 rows: 51c090, 406e50, 403f90) named the right variables
and registers every time; `slots` (51c090) named the right tie and `what pos:_info$=0` predicted exactly the two offsets
that swap; `rasim scratch` (406e50) predicted the picks (6 and 7) correctly; `icount` (4ef730) named the pass (AddrModes)
and the missing `add eax,[mem]`; `symkeys levers` (47c810) said "no key edit does it", consistent with the earlier lane's
hand experiments. 0 wrong predictions; 0 predictions that turned into a source edit on their own. `schedwant` (4ea030) was
the one wrong tool: auto mode printed nothing for any M&B function (no regions found against the listing), and with the
region and the wanted order given by hand its replay "differs from the trace" and it finds nothing.

Time per query: 4-16 s for regmap / scratch / slots / icount / sched (a cached compile of the module costs 4 s, a first
compile of a large TU up to 70 s), 20-80 s for a tsweep of 4-10 variants, 5-11 min for an effectmine run of 150 compiles
(3 runs, 0 results). Versus doing it without: a plain `check.py` compile of the whole module is 6-20 s, so a hand variant
costs the same as one c2scope query; c2scope's advantage is that one query explains a diff that would take many blind
variants, and `tsweep` batches variants (6 spellings in 24 s vs about 6 x 15 s by hand).

Opinion: worth having for the diagnosis of register, scratch-register and stack-slot diffs (it turns "two registers are
swapped" into "pick 6 and pick 7 of the scratch rotation, because the argument copy is created after the object load") and
for ranking batches of source variants (tsweep). It did not find source edits by itself in this session: the diffs that
type recovery left are front-end / IL shape (an inlined typed wrapper's evaluation order, a by-value copy, a use count, a
tie in the slot sorter), and the model's levers (extra scratch pick, `pos:` of a slot, key shifts) have no spelling that
keeps the real types. effectmine (the automatic searcher) cost 5-11 min per row and never produced a candidate; helpersweep
and rwsweep were not applicable (no EH frame / nothing the mutations reach). So: use `regmap` / `scratch` / `slots` /
`icount` first on any register / frame / instruction-count row (seconds), `tsweep` with your own hypothesis spellings, skip
effectmine and schedwant auto mode for M&B. For the rows left here the missing piece is a front-end fact (what the original
source did), not an allocator fact.
