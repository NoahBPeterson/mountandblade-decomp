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
  defined as `{}` in collision_shapes.cpp next to shape_base::draw (the linker folds the two empty bodies, as in the original).
  One compile, 6 min including reading why the previous lane used a call to 0x477b70.

### 0x4e9d10 Party::write, 0x4608d0, 0x4bdd50 (stream rows): byte-exact with the right field types
- Class: codegen shape (a copy of the member to a stack temp, `lea eax,[esp+0x14]`, instead of `lea eax,[edi+0x74]`).
- c2scope not used: `check.py` showed it. stream.h had typed by-value / reference wrappers (`w_int(unsigned v)`, `r_int(unsigned&)`
  via `(int*)&v`, a temporary, or a const_cast); all four are deleted. The call sites call the slot the original calls
  (`insn_align` showed a 1-byte vtable displacement difference when a site used r_uint where the original has r_int) and the
  field has the type that slot takes: Party::personality, DialogRec::flags, SpawnRec::flags2 are int, Party::flag a char.
  Pool::serial and SkillRec::attr stay unsigned (the original compares / shifts them unsigned, share_experience 0x494b60 and
  fill_skills 0x509730 break when they are int) and use r_uint / w_uint: the three rows that touch serial (0x4e99d0, 0x4e9c50,
  0x51e840) are 1 byte off (slot 7 / 16 against the original's 8 / 19, one ICF'd function); equiv FAIL there is the harness
  (its fake stream tells the slots apart). So the original source evidently read an unsigned field through the int slot.

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

## Second round (after the coordinator's request: register / scheduling rows in earnest)

Survey of the 115 type-recovery rows with `insn_align.py --show`: most remaining diffs are not allocator diffs (a different
field read, a reloaded copy of a vector member, a different parameter, a different global symbol with an addend, a class's
constructor order). Rows that are allocator / scheduler / frame diffs and were tried:

| row | question | answer | held? | outcome |
|---|---|---|---|---|
| 0x529d80 sync_scene_props (12) | `regmap`: why eax/edi swap? | t146 wants eax: "RewriteAsReal/RegSimpleAssign without a RegAssign call (not modelled by rasim)" | true but the model has no edit for it | none |
| same | `icount` (14 s): which instruction? | MISSING `lea edi,[eax+ecx]` (line 1844 `r = &props[i]`) removed by Riscify_undo: "a split memory operand put back into its use, as a new tuple, unless the scheduler separated them"; ours folds the add into `[edi+ecx+0x10]` and adds later | yes, exactly the diff | not a source edit; tsweep of 6 spellings of `r` (reference, index, begin()+i, ...): rdiff 5 for the base, 24-29 for the others; effectmine --want bdiff (210 traced compiles, 40 min on a shared laptop; first run void, see tool state): best variants rdiff 5 bdiff 6 = the base, all code-neutral; 0 improve |
| 0x524420 Troop::read_file (8) | `regmap` | t814 (`base += ftoi_floor(t)`) wants ebp: `add ebp,eax` not `add eax,ebp`: "the IL's operand order" | yes | `symkeys sites` shows line 31 `OPADD(ftoi_floor temp [SU0 n2 h027c], base [SU0 n1 h0120])`: the count (2 vs 1) decides, `levers --site '31:_base>_t@ftoi_floor#1'` with 2880 single edits: 0 sets: correct and explains why no key edit helps; tsweep of 5 spellings (`+=`, `a = a + b`, `b + a`, via a local): identical code. The other diff of this row (the second table read is `[ebx+4]` of one symbol in ours, a separate label in the original) is data identity, not the allocator |
| 0x406e50 DialogProc | `realize.py --from scratch` (the automatic scratch-pick realiser) | "18 of 18 known picks on the original's register ... 0 answers read" | WRONG: `regmap` and `rasim scratch` on the same function say picks 6 and 7 are swapped; realize reads the wrong snapshot (or counts only picks it knows) and searches nothing | tool gap: no answer |
| 0x51c090 draw_party_line | `realize.py --from slots` (60 compiles) | its candidates are `if ((int)x & 1) ... else ...` arms (construct `eqif`), which move the `_info` / `_skill` offsets but cost +100 bytes; closest 5/14 offsets right | model right, constructs unusable (they are the README's "not what the original had" trick) | none |
| 0x4ea030 id_remove (8) | `rwsweep` (all families, 475 compiled variants: wrap 49, alias 43, swap 2, split 2, reread 2, touch 355, cflow 9, cmp 4, paren 5, const 4; 42 min on a shared laptop) | "0 code-neutral variants improve the gap or the score; 0 variants that change the code improve the score" (base rdiff 1 bdiff 4) | the search space is right (a moved store is exactly what touch / alias / swap perturb) and it is exhaustive for those families | not matched; first attempt (22 min) was void: I edited stream.h while it ran and its variants stopped compiling (rwsweep and effectmine read the live tree's headers: never edit during a run) |
| 0x4cee50 / 0x4dc350 | `sched --all` | the difference is in the pre-schedule order / an 81-tuple region cut | | none |

Tool state for 13.10 (what is broken or thin, exactly):
- `schedwant` auto mode prints nothing on any M&B function tried (4ea030, 406e50, 4dc350: the regions are found, "0 differ"),
  although `check.py` shows a moved instruction in 4ea030; with `--region K --want-lex ...` it runs but its replay "differs from
  the trace" (13.10 scheduler rules not matching) so its 0-of-1046 is not trustworthy.
- `realize.py --from scratch` reports 18/18 picks right on 0x406e50 where `regmap` / `rasim scratch` report two wrong picks.
- `tools/contrib/c2.py` passes `--c2 13.10.3052` to tsweep and `13.10` (3077) to the rest; the docs say both builds give identical
  listings, no difference was seen.
- `rwsweep` / `effectmine` read the working tree's headers while they run: an edit during a run makes the later variants fail to compile (it happened to two runs here; both were re-run on a stable tree).
- `effectmine` costs 5 to 38 minutes per row (the laptop was shared) and found nothing in 4 runs (item_events 5 min, blend_indexed 38 min,
  draw_party_line 11 min, sync_scene_props 40 min); its best variants were always code-changing (`eqif`) or neutral.
- c2scope's own `tools/c2trace.py` (local wine) works for 13.10 on M&B sources (6 s); remote traced compiles go through
  `../vs2005-cpp`'s client.

## Stale rows found by re-running check.py over all 115 rows of the type-recovery phase (not c2scope)
0x4bdd50, 0x4608d0, 0x51e840 were already byte-exact (the r_int fix b26fd8f5 repaired them): recorded. 0x4d63b0 is
byte-exact but two relocation addends differ (`Config::save` is a member of g_cfg now): run_all disagrees, left.

## Verdict
(see the bottom of this file)

## Totals and opinion

Rows worked this session (type-recovery rows that were nonmatching):

| outcome | rows |
|---|---|
| back to byte-exact | 0x509730, 0x489820 (batch 1) + 0x48c570 (`static_cast<Rec8*>(memmove(..)) + n`), 0x4ca600 / 0x494260 (`unsigned char` cursor flag, gui_manager::show_cursor unsigned char): 5 by source change + 0x4e9d10 / 0x4bdd50 / 0x4608d0 with the typed-field fix (the three rows through the unsigned Pool::serial, 0x4e99d0 / 0x4e9c50 / 0x51e840, stay nonmatching at 1 byte) |
| diagnosed by c2scope, no source lever found | 0x51c090, 0x4ea030, 0x406e50, 0x47c810, 0x403f90, 0x4dc350 |
| byte-exact only with a match trick (not landed) | 0x4ef730 (dead `if (!k) return;`) |

c2scope was the tool for 7 rows (509730, 51c090, 4ea030, 406e50, 47c810, 403f90, 4ef730) plus a look at 4dc350 and 4cee50.
Of the 8 rows landed by source change, c2scope contributed to one (509730: it ruled the scheduler out; the fix came from
reading the IL / strength-reduction behaviour) and was not needed for the other seven (the diff was readable in the
`check.py` listing: an inlined empty virtual, a by-value stream wrapper, a dropped memmove result, a signed vs unsigned
flag byte).

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

## Secondary task: the "UNSUPPORTED (timeout)" rows (not c2scope)
32 rows re-run on the laptop, 3 shards each (`tools/remote/eqremote.py` through a small driver over sweep_nonmatching's rows,
`--full-edges` so the shards merge; the laptop declined every row at first, "at its job cap", other lanes were using it, so the
driver ran one row at a time until it was accepted). Results: work/equiv_results/2026-11-22/bm1, `verdicts.py --add`.
Verdicts: 22 WEAK, 8 UNSUPPORTED (0 valid inputs: heap exhaustion / faults on soup objects; 0x50a1c0 has 5 of 5 valid), 1 PASS
(0x4c2170), 1 FAIL (0x49b1c0: a shard reports "heap size", a harness limit of the library `_Insert_n` instance). 31 notes in
the `.nonmatching.txt` files carry the new verdict.

## restructure rows (2026-11-22): the nine rows the agent_logic split knocked out of byte-exact

Setup: batch B (54e24615) cut agent_logic.cpp into 11 files; nine rows stopped matching (chk.py aligned: pick_front 85, pick_duel 164,
fire_check 12, pick_threat 10, pick_reachable_goal 32, Font::draw_glyph 17, refresh_carried 6, gui_manager::add 523, update_attack 282).
The pre-split text matches all nine (chk.py on the old source: 2/2 for the first two, the rest were manifest rows of 7d9c7934).
`tools/contrib/c2.py icount 0x<va>` was run on each row in its part (8 in parallel, 2 to 14 minutes each: the laptop compiles one
big TU per row; update_attack's 1401-instruction analysis took 14 min and 1.4 GB).

| row | question | c2scope's answer | held? |
|---|---|---|---|
| pick_front 0x43b9e0 | `icount`: why 123 instructions vs 121? | 2 extra: `mov ebx,[ebx]` (the `n = g_mission->agents.count` load, class "structure") and `mov edi,[g_mission]` OPENTERLOAD: Globregs made g_mission a register candidate across the loop (prio 48, 4 refs) and the original keeps it in memory; lever offered: "fewer uses or a shorter live range make it a non-candidate" | The diagnosis held (after the fix the object has the original's 121 instructions); the offered lever was wrong: no source edit of the function's own uses is needed, the cause is outside the function (a callee body that the part cannot see). c2scope cannot say that: it traces one TU and reports what differs in it, not what the TU lacks |
| pick_front | `regs --why ?g_mission`: why a candidate? | pop 25, prio 48 = sum of block terms (refs +12, +28, +24, +12; live-through blocks -2..-6), assigned edi | held as arithmetic; no lever |
| pick_duel 0x43bb80 | `icount`: 151 vs 150 | `_this` and `g_mission` are Globregs candidates (OPENTERLOAD, `mov ebp,[esp+0x20]`, `lea ecx,[ebp+4]`) where the original reads memory and recomputes `add ecx,4`; one `mov edx,[g_mission]` reload the original keeps is removed by GlobalOptimize/LocalCopyProp | held: same cause as pick_front (the same fix returned both) |
| gui_manager::add 0x452c00 | `icount`: 224 vs 227 | the original CALLS `0x428e80` (`vector4::vector4(float,float,float)`); ours inlined it (missing "removed after Inline"); further misses are register coalescing (`mov ecx,edi`, `lea edi,[edx+esi+0x1728]` kept by Globregs in the original) | held in the sense that the rejoined object has the call again (893 bytes, 0 diff); not tested alone |
| fire_check, pick_threat, pick_reachable_goal, draw_glyph, refresh_carried, update_attack | `icount` | the same instruction count (232/232, 118/118, 90/90, 67/67, 222/222, 1401/1401) and "lift: no difference the rules can see"; only "moved" instructions (a load or a store at another position) | held: these 6 differ in order only, which is what a different memory-dependence / side-effect picture of the callees does to the scheduler; icount cannot name the callee |

Bisect that actually found the cause (no c2scope; 4 compiles of the old file truncated at 2233, 3363, 6432, 7603 lines, then 3405, 4732, 4765,
5613, 5991): pick_front matches with the file cut at line 3405 and not at 3363, i.e. the text between them is what it needs; that text is the
agent_ai part's prelude, whose "More bodies the original's object saw" block holds `inline __declspec(noinline) vector4::normalize()` (plus rot,
cross, fmax_ref). Pasting just those 19 lines into the targeting part also made pick_front and pick_duel exact (chk.py 2/2), which proved the
mechanism; the other seven rows have 40-odd more such bodies (the old file's "visible body" blocks) and no single one was looked for.

Outcome: all nine byte-exact again by rejoining the parts (source_tree.md section 10), chk.py 9/9, clean source (the pre-split text, no extra
header). Value of c2scope here: it confirmed in 14 minutes of laptop time that two of the nine are Globregs/inlining differences and six are
order-only, and it ruled out a per-row source cause (no spelling lever); it did not find the cross-part cause and its lever text for the first
two rows pointed the wrong way. `c2.py` cannot be used on a row that is already byte-exact (task.py has no source line for it: "knows no
source for 0x..."), so the "after" confirmation for pick_duel / gui_manager::add is chk.py's 0-diff, not an icount.

### re-split attempt with a shared header (2026-11-22, the coordinator's reading: normalize and the rgl::mesh members are header inlines)

Hypothesis (coordinator): the visible `rgl::vector4::normalize` body and the other foreign-class members in agent_logic.cpp are header inline
functions whose out-of-line COMDAT the linker kept from this object, so they are not evidence for one .cpp; give them a header and re-split.
Test: `tools/migrate/resplit_agent_logic.py` cuts the single file at the planned 11 seams and moves into `agent_logic_shared.h` every
`inline __declspec(noinline)` visible body (about 50, wherever in the file) plus every helper two parts use (position-bound names, e.g.
the a_vadd overloads, are copied into later parts only). All 11 parts compile; `rows_after_split.py` then settled the manifests.

| row | single file | split + shared header | held? |
|---|---|---|---|
| pick_front 0x43b9e0, pick_duel 0x43bb80, fire_check 0x441020 | exact | exact | the hypothesis holds for these three: their dependency was the visible foreign bodies (normalize ...) |
| pick_threat 0x443430 (10) | exact | 10 | no: `icount` 118/118 instructions, "lift: no difference", one instruction moved |
| pick_reachable_goal 0x445720 (32) | exact | 32 | no: 90/90, four instructions moved |
| Font::draw_glyph 0x44f5d0 (17) | exact | 17 | no: 67/67, three moved |
| refresh_carried 0x452650 (6) | exact | 6 | no: 222/222, two moved |
| gui_manager::add 0x452c00 (523) | exact | 523 | no: 224 vs 227: the original calls vector4(float,float,float) 0x428e80, the part inlines it; the other misses are register coalescing |
| update_attack 0x45c220 (282) | exact | 282 | no: `icount` 1401/1401, "lift: no difference", no moved instruction listed (the 282 aligned bytes are scheduling / register order inside 5.6 KB) |

c2scope names the symptom (order-only differences = the scheduler's memory-dependence picture of the callees; an inlined ctor), not the
cause. Cause found by a hypothesis test: pick_threat calls Agent::line_of_sight 0x440e30 and Agent::fire_check 0x441020, REAL functions of the
attack_checks part (rows of their own, no header inline). A scratch copy of the effects_sound part with those two bodies appended as
`inline __declspec(noinline)` copies is byte-exact (chk.py MATCH). So cl uses the bodies of same-object callees (side effects, registers
they clobber), which is the "visible callee" knowledge of docs/codegen.md 6.3. The remaining rows call functions of the other parts the same
way: pick_reachable_goal calls 0x43cb30 / 0x43dd70 (targeting part) and 0x443870 / 0x443aa0 / 0x444370 (effects_sound part); draw_glyph and
refresh_carried call 0x44e970 (mesh members), 0x441ff0, 0x443aa0 and 0x4520b0. The calls chain every part to its neighbours, so no seam is
left that no row crosses: the object is one translation unit in practice. Outcome: the single file stays (source_tree.md section 10).
