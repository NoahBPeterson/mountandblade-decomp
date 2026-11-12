# Code generation in `mount&blade.exe`

How the Microsoft C++ compiler turns ordinary source into the shapes seen in
`mount&blade.exe` 1.011, and which source choices move those shapes. Each entry
runs mechanism -> source choice -> native shape -> where it was demonstrated.
Procedure (slices, manifests, verdicts, bookkeeping) is in [`matching.md`](matching.md);
this file is only about what the compiler does.

All game code is one compiler: Visual C++ .NET 2003, **cl 13.10** (VC7.1). We
compile with cl 13.10.3052 from the Visual C++ Toolkit 2003 through
`tools/matching/cl.sh`, with `/O2 /MT /Gy /TP` (`/EHsc` only where the original
has EH frames), x87 floating point only, no `/GS`, no `/GR`, no link-time code
generation. The original may be a later 13.10 build than 3052; no function has
needed that so far. Nothing here depends on telling compilers apart.

Evidence labels:

- **M&B:** demonstrated on `mount&blade.exe`; the address or slice is given.
- **Observed in Spore's cl 13.10 code (Havok/RenderWare):** the mechanism was
  shown on another program built by the same compiler. The addresses belong to
  that program and will not be found in `mount&blade.exe`; they are kept only as
  evidence that the mechanism exists.
- Entries with no label state a compiler behaviour that has been seen repeatedly
  in this compiler family; they are not tied to one address.

A rule here explains a mechanism. It does not promise that the same spelling
works at another site.

## How to use this guide

1. Read it once before you start iterating on a function.
2. Open it again the first time a diff appears. Find the section for the kind of
   difference you see:

   | You see | Section |
   |---|---|
   | wrong loop shape, `dec`/`sub`, pointer vs index | 3 |
   | a value loaded twice, spilled, hoisted, or kept in a register | 4 |
   | `fld`/`fstp` vs integer moves in a copy, wrong frame from a value class, wrong vtable layout | 5 |
   | the same instructions with swapped registers | 6 |
   | right instructions, wrong `[esp+N]` / `[ebp-N]`, wrong frame size | 7 |
   | right instructions, blocks in another order, `jcc` polarity | 8 |
   | x87 operand order, store-and-reload, rounding, compare masks | 9 |
   | `setcc`/`movzx`/`and` choice, an inline loop where a call was expected, intrinsics | 10 |
   | wrong `ret N`, `ecx`/`eax` at entry, stray `add esp`, return value missing | 11 |
   | a call that is inlined (or not) differently from the original | 12 |
   | `push -1; push handler; fs:[0]` frames, constructors, destructors, `new`/`delete` | 13 |
   | literals, statics, folded functions, header layout | 14 |
   | a diff that no known source shape moves | 15 |
3. Check the entry against native evidence (`tools/matching/card.py`,
   `cmpdis.py`, `insn_align.py`) before you adopt it, and retest after.
4. Read the diff in this order: function extent and calling convention (`ret
   imm`, `ecx`, `eax` residue), then frame size, then instruction deltas. A frame
   difference is one root cause that shifts every stack operand; a wrong
   convention is invisible to a byte compare (section 16).
5. Native evidence outranks a minimal-source prior. A no-op pair of parentheses,
   a split increment or an overwritten default store is real if it moves the
   output toward the native bytes.

For the MSVC 6 compiler (an earlier version of the same backend, `c2.dll`),
banteg's *MSVC 6 code generation* guide for Crimsonland
(`crimson.banteg.xyz/re/static/msvc-codegen/`) describes the same family of
mechanisms. This guide does not depend on it.

---

## 1. The binary and its toolchain

### 1.1 What the build looks like

| Setting | Evidence |
|---|---|
| `/O2` | the baseline for every manifest row |
| `/MT` (static CRT) | M&B: the CRT is linked from `libcmt.lib` at `0x535440..0x547100` and is never decompiled; `symbols/lib_names.txt` names 415 of it |
| `/Gy` (function-level COMDAT) | functions are separate COMDATs, which `/OPT:ICF` can fold (section 14.1) |
| `/TP` (C++) | mangled class methods |
| `/EHsc` only where the original has EH frames | M&B: `push -1; push <handler>; mov eax, fs:[0]` prologues and `__ehhandler$` stubs mark the functions that need it (section 13.1) |
| x87 only | no `/arch:SSE`; VC7.1 has no `/fp:` switch (it warns and ignores it) |
| no `/GS`, no `/GR` | no cookie prologues, no RTTI |

### 1.2 What the binary hides

- **No `.reloc`, no PDB, no RTTI.** Class names and layouts are recovered, not
  read. Relocated operands are masked in byte comparisons, so callee identity,
  global identity and vtable targets are not checked by a byte diff (section 16).
- **M&B:** much of `0x401000-0x407c00` is the DirectX 9 SDK sample framework
  (`CD3DApplication`, `CD3DEnumeration`, `CD3DFont`, the settings dialog).
  Writing the SDK's literal source form (its comparison order, `bool`
  temporaries, one `case` per value instead of grouped labels, its init order
  `Width, Height, Format, RefreshRate` for a `D3DDISPLAYMODE`) matched where
  natural spellings did not. Recall the library's source before inventing a
  shape.
- Static CRT code is linked from the real library, so a function there is not a
  codegen puzzle.
- **WinLicense markers are source, and we can emit them.** An 18-byte
  `EB 10 "WL  " <id dword> 00000000 "WL  "` in a function is a WinLicense/Themida
  SDK region marker (ids 12/13 start/end, also 4/5, 6): the jmp skips it, no
  runtime effect. Write `WL_MARKER(id)` from `src/include/wl_marker.h` at that
  point in the source (the one place `_emit` is allowed; asm_audit knows it).
  Functions with markers do reach byte-exact (game.cpp 0x51de70, troop_rec.cpp,
  exchange_screens.cpp). The `__asm` is also a scheduling and register-allocation
  barrier, exactly as in the original, so a missing marker shifts registers and
  slots around it too: add the markers before chasing those differences. Older
  notes that say a WinLicense row "can never be byte-exact" predate the header.
  Find them by scanning the unpacked image for `EB 10 57 4C 20 20` *and* for the long variant (`EB 15 "WL  " id 0
  "WL  "` plus five zero bytes, 23 bytes: seen once, id 7 at 0x4d0bfb), which `WL_MARKER_LONG(id)` emits. A row whose
  source already has `WL_MARKER`s can still be missing one of a pair: compare the count per function against the scan.

### 1.3 Smell tests

- `fld st(i); fmul [m]` (a just-computed value stays on the stack and is
  multiplied by memory) is cl 13.10 output **for a `float` scalar**. The reverse,
  `fld [m]; fmul st(i)`, is **also cl 13.10 output, when the scalar is a `double`**
  (section 9.2, "THE DOUBLE-SCALAR LEVER"). It is not a sign of another compiler:
  try `double` before anything else.
- `sqrt` compiles to a `_CIsqrt` call in this compiler. If the original has an
  inline `fsqrt`, no source lever is confirmed (section 9.3).
- A function that reads `eax`, `edx` or `ecx` at entry with no C signature to
  explain it is usually a `static` function whose callers share its translation
  unit (section 6.3).

---

## 2. Backend behaviours to assume

The code generator is one whole-function pipeline. These behaviours recur
everywhere; later sections give the source choices that exercise them.

| Behaviour | What it means for source |
|---|---|
| Indexed loop becomes a pointer induction variable | write the plain indexed `for`; do not hand-roll the pointer walk (section 3) |
| An unused index becomes a countdown `dec`/`jne` | a loop whose index is not otherwise used may be reversed |
| Loop inversion and peeling | the test is at the bottom; the first test may be peeled |
| The induction-variable anchor is chosen by merge order and use counts | adding a use can change the anchor |
| Whole-function register allocation keyed by internal ids | one added temporary can shift a distant pick (section 6.2) |
| Identical tails are cross-jumped | two arms that end alike share one tail (section 8) |
| The then-arm falls through; a late block mover relocates ranges | arm order follows the condition's polarity |
| May-alias classes; dead stores survive | stores through pointers are not removed or reordered across other memory |
| Arguments are evaluated right to left | `f(a(), b())` calls `b` first |
| Struct-by-value returns use a hidden destination pointer | section 11.3 |
| A `void` function can still leave a value in `eax` | section 11.3 |
| `ret N` carries the argument cleanup | section 11.4 |
| A tail call becomes `jmp` | the callee's convention must match (section 11.2) |
| `static` functions in a TU may get custom register conventions | section 6.3 |
| The linker folds identical function bodies | section 14.1 |
| String literals are pooled | section 14.5 |

---

## 3. Loops and induction variables

### 3.1 Write the plain indexed loop

Writing a pointer walk where the original has an indexed loop is a mistake: the
backend creates the induction variable itself. The latch is `jl`/`jle` or `jb`
according to the signedness of the compared source value, not according to
index versus pointer. Loop spelling also decides register assignment and `dec`
versus `sub`: `while (n--) h = f(*s++)` matched where `for (; n; --n)` did not
(observed in Spore's cl 13.10 code, Havok/RenderWare).

### 3.2 Guarded do-while for a pointer walk

M&B: an `if (n > 0) { p = ...; do { ... } while (...); }` shape reproduces an
`add esi, K` inside the guard; a plain `for` hoists it (`s00408110`). A
rotated loop (`do { body; } while (cond);`) reproduces a loop whose test is at
the bottom where a `while`/`for` produced a different block order; keep the
induction update where the assembly shows it.

### 3.3 A local copy of the trip count enables unrolling

cl 13.10 unrolls a loop only when the trip count is a **local copy** of the
member, not the member itself:

```cpp
const int nB = m_dimB;            // unrolled 4x with this
for (int i = 0; i < nB; i++) ...  // not with m_dimB directly
```

Observed in Spore's cl 13.10 code (Havok, `setTransformedPos`).

### 3.4 Straight-line repeats must be written out

When the original emits a repeated release/handle block or a pointer-vector
drain fully unrolled, writing it as a loop makes the compiler lower it to one
shared loop and the body roughly halves. Expand it by hand (macros are fine).
The unrolling is the source shape, not an optimizer pass you can trigger.

### 3.5 Same-valued induction variables merge sooner

cl 13.10 merges two induction variables that the source kept distinct (an outer
counter and a running `(i+j)` sum in a 13x13x13 table filler) and no spelling of
the variable types reproduced the original's spills. When two variables the
source kept apart collapse, that is the backend, not your source. Observed in
Spore's cl 13.10 code (Havok).

### 3.6 Pointer-parameter loops

Declare a pointer parameter as `u32**` and dereference `*base` inside the loop to
keep a pointer-compare loop with a per-iteration reload. With `u32*` the
compiler computes a trip count and uses `mul`.

---

## 4. Statement order, ownership and spelling

- **An inline constructor's stores follow source order.** M&B `resmgr::load_stream`
  0x41db20 and `get_render_target` 0x41b7d0 both expand `texture::texture()`. The
  original's `or ecx,-1` comes before the two `0x80` stores only when the source
  writes `loaded = 0; f1c = -1;` before `width = height = 0x80`. Reordering the
  ctor body matched both functions at once.
- **Count immediates before blaming the allocator.** When "cl hoists different
  constants into registers" seems to explain a diff, count how many times each
  immediate is stored in both listings. A count that differs is a wrong *value*
  in the source, not allocation. M&B `render_batch::init_decls` 0x4bc830: the
  note said "identical values", but the skinning vertex declarations used
  `D3DDECLTYPE_UBYTE4` (5) where the original has `FLOAT4` (3). That was a real
  behaviour bug.
- **A `mov ecx,...` right before a call to a function that ignores `this` means
  the source called the member.** Calling the `::` free function drops the ecx
  load (M&B `Agent::can_hit` 0x4406a0 → `id_matches_dir` 0x4326d0).

- **Repeat a pure expression instead of naming it.** CSE picks the owner
  register; a named local imposes a lifetime that can hoist, spill, or rotate
  registers. M&B: writing `Agent* a = (Agent*)(g_game->chars + idx * 0x508);` once
  per branch cost 249 diff bytes (706 -> 457) against writing the accessor inline
  at every use (`g_game->troop(idx)->items[i]`): the local forces `add base,
  idx*0x508` plus a separate `lea +0xb4`, while the inlined accessor folds into
  one `lea [chars + idx*0x508 + 0xb4]`, and that alone moved the register
  allocation to the original's (`s005134c0`).
- **The opposite case: copy a member to a local when the original did.** A member
  loaded into a callee-saved register before a call comes from a local copied at
  that point. Binding a member to a local reference before a branch
  (`string_type& t = mMatchText;`) makes the compiler cache its address in a
  register before the `jne`; that removed about 370 diff bytes from one
  function (observed in Spore). A declared `float one = ...;` placed between two
  stores stops the compiler reloading an aliasing global twice.
- **Re-read fields and globals at each use.** Without link-time code
  generation, a call is assumed to clobber everything, so native reloads after
  calls are source re-reads.
- **Test-then-reload** (`cmp [r],0; je; mov ecx,[r]`) means the member chain is
  written twice without a local; a **kept `lea`** means a `T& ref = v[i];` local.
  Calling a trivial inline accessor instead of the raw member can fix a
  stubborn register swap.
- **First assignment matters; declaration order usually does not.** Permuting
  declarations is normally byte-identical at `/O2`.
- **Write a local's initializer where it is first used.** A `void* h = 0;`
  declared early and passed as `&h` to a later call can force a `push ebp` frame
  and shift the prologue; moving the declaration to the call site matched
  (31 -> 0 bytes, observed in Spore).
- **Chained assignment stores the rightmost target first** and, for floats, keeps
  the value on the x87 stack (`fld st(0); fstp`). `a = b = v` is not two
  statements; float chains are order-sensitive.
- **Aggregate identity changes alias ordering.** Modelling a separately declared
  global as one struct makes the compiler treat sub-object accesses differently.
- **Register promotion of a global needs a file-local static.** M&B: a counter
  held in a register (load hoisted before the stores, one store at the end)
  appears only when the counter is a `static` in the file (`dwAppPausedCount` in
  `Pause`, inlined into the constructor, `s00401010`). An `extern` global at the
  same address gave 277 bytes instead of 263.
- **Compute a call's store target before the call.** `float* d = ...; *d =
  fn(...);` gives `mov edi,[esi+0xc]; add edi,0x1e4; call edx; fstp [edi]`.
- **A nested call's constants are pushed first.** M&B: in
  `SendMessage(GetDlgItem(...), ...)` the constant arguments of the outer call
  are pushed before the inner call; a temporary `HWND` assigned first reproduces
  the original order (wave 1).
- **A temporary per operator blows the frame.** M&B: a transform written with a
  local `vector4 operator+` (one temporary per operator) grew the frame from
  0x11c to 0x26c and the diff from 993 to 2,088 bytes; the same maths written
  scalar-per-component, letting the compiler CSE the shared `w * k` terms, came
  within 10 bytes (`s004e65e0`).
- **A local reference or pointer keeps loads and stores in source order.** Fields read through `RectF& rc = at<RectF>(off)` or
  `float_pair* cv = em->curve` were not moved across neighbouring stores through `this`/`em`, where the same fields spelled
  `at<float>(off)` / `em->curve[i]` were hoisted or interleaved. When the original keeps a run of loads below stores that
  can't alias, try going through a named reference (M&B `start_setup` 0x484590: +15 and +7 identical).
- **Two union members for one word keep two read-modify-writes apart.** cl merged two field puts into the same global word
  (one load, one store across the `_ftol` between them) while the original keeps load/and/or/store per put. Writing the
  second put through the union's other member (`g_terrain.flags` instead of `bits[4]`, as `Terrain::set_field_a` 0x464a60
  does) reproduced the separate RMWs and freed `ebx` for the value the original keeps there (M&B `start_setup`).
- **`mov [this+N],eax` followed by `mov ecx,[this+N]` means the member is used after the store, not a local.** M&B `PartyWindow::init_screen` 0x500ad0: writing `container = new NodeA(0); container->move_a(..)`, `hinge[k] = new MissionUi; label_refresh(hinge[k], ..)`, `name[i]`, `gold[i]`, `total_cost`, `equip_bg` and `view[i]->..` (instead of `T* w = new ..; member = w; w->..`) took the aligned figure from 1955 to 1711; the two widget arrays of the armour loop went the other way (1763 -> 1837), so test each site.
- **UI construction code (M&B `ConvScreen::open` 0x4f66e0, `CampaignScreen::init_screen` 0x4ec4d0; bmD 2026-11-09, aligned 3974 -> 702 and 746 -> 198).**
  - *tstring order is a scoping signal.* A tstring ctor that runs BEFORE the call's float arguments are pushed is a named block-scoped
    local (`{ tstring t("ui_camp"); f = new_medium_button(f34, tab.lookup(t), 0.1f, ...); }`); an argument temporary
    (`lookup(tstring("..."))`) constructs after the floats. Narrow the block around just the call, or the name will not share the
    frame's base slot with the later flag/vector temps (a name in the same block as a later `int flag` moved the tstring up 0x10).
  - *`(float*)&vector2(x, y)` temporaries share one slot; named `float pos[2]` locals do not* (frame 0xb0 vs the original 0xe0
    once everything else matched).
  - *A same-type cast is an lvalue to cl 13.10.* `push_child(kids, (MissionUi*)f34)` bound the reference to `this->f34` itself and
    kept `&f34` in a register for the whole function; the original copies the pointer into a temp (`{ MissionUi* w = f34;
    push_child(kids, w); }`, the block matters: a function-scope local with its address taken stays live to the end).
  - *Inline config lookup:* `float r = 0.0f;` goes BEFORE the `find` call (the original stores it before the call), a max folded
    into the same helper (`return r > cur ? r : cur`) puts the compare before the key's destructor, and `__forceinline` is what
    makes the `find` + `imul 0x2c` + `atof` block appear at all (a `static` helper became a register-convention call).
  - *Inline `new T` + ctor merges the null check into the result register* (`return p ? new (p, raw_place()) T : 0;`), where
    `if (p) new (p) T; return p;` leaves a copy (`mov esi, eax` after the call).
  - *A widget whose inline ctor zeroes three words* (ContainerBase(inl_tag)) is what makes cl hoist 0 into ebx for the whole
    function; leaving those stores out changed every `cmp esi, ebx` and `push ebx` in the function.
  - *`g_gui.dirty = 1; g_gui.objs.push_back(w)` with `w` a block-scoped copy* puts the dirty store after the temp's creation
    (facegen_init / party_screen spelling).
- **Combat/agent code (M&B `Agent::hit_damage` 0x453710, `Agent::on_wield_changed` 0x444370, `gui_manager::add` 0x452c00; bmB 2026-11-09).**
  - *A field held in a callee-saved register across opaque calls is a local copy.* `ItemRef sp = slot_pair(hand)` escapes to the
    opaque `sp.stat_21x()`, so every `sp.item` reloads; the original keeps the index in `edi` from its first use to the end, i.e.
    `int item = sp.item;` (+140 identical, and it fixed ebp/ebx for two other webs).
  - *An inline caller's callee shows as a whole block.* Three `rate store / prio&0x4000000 -> set_rec_phase / wield[0] vis`
    sequences were `Agent::set_order(1, id, prio, 1.0f)` 0x4379c0 expanded in place; writing it as a forceinline helper took
    on_wield_changed from 349 to 466/524 identical. Look for a known function's body before hand-writing the pieces.
  - *Initialise, don't assign, when the original returns straight into the object.* `A = cross(q, up)` goes through a temporary
    and a 4-dword copy; `vector4 s = cross(q, up)` gets the hidden pointer to `s` itself. Conversely a temporary + dword copy
    into an existing vector (`d = A - B`, `B = rot(...)`) is an assignment.
  - *`int zero = 0` where the original passes `0.0f`.* An int local holding 0 joins every other int 0 (`age = 0`, `push 0`) and
    cl hoists `xor ebp,ebp` for them all; `float zero = 0.0f` keeps immediates and freed ebp for the original's allocation.
  - *A struct copy after a call, written from the array element, not a cached sub-object pointer.* In `Mission::start_setup`
    0x484590 the original builds `B = A` from the slot pointer after `orthonormalize` (`lea edi,[esi+0x40]`);
    `rgl::matrix* a = &sl->A; ... sl->B = *a;` kept `a+0x10` in edi across the call. `SLOT(s).B = SLOT(s).A` gave +19.
  - *Named key, then the colour:* `{ tstring key("ui_..."); g_gui.add(*g_strtable.lookup(key), colour); }` pushes the colour after
    the key ctor and reloads `lea` for the lookup, as the original does; `lookup(tstring("..."))` pushes the colour first.
- **Put the `// @ 0x...` marker directly above the function it names.** A marker above a `static __forceinline` helper made chk,
  insn_align (and so the harness) compare that helper (on_wield_changed was "1601/2116 diff" against `aset_fx`).
- **A guard on the address of a field of a fresh allocation is real.**
  `lea ecx,[eax+8]; test ecx,ecx; je` comes from `if (&pNode->mValueA)`;
  removing it shrinks the function.
- **An element reloaded at every use is a reference, not a cached pointer (M&B `Terrain::light_particle` 0x471b30).** The original walks `p->meshes[k]` with a pointer in `ebx` and does `mov eax,[ebx]` before every `m->...` access, so there is no register-held `m`. `mesh* m = p->meshes[k];` caches it in `esi` (aligned 1296); `mesh*& m = p->meshes[k];` reloads it like the original (aligned 1025, size 1288 -> 1266). The same function: two scale temporaries that the original keeps in different slots must be two locals (`d`, `d2`), not one reused (-259 aligned). `Terrain::scatter` 0x4650f0: reading `g_ptypes[sel]` at each of its two uses instead of through a `ptype* gs` local gave +94 identical, and a `float rr = (float)(rand() % N) * k` expression written inline (instead of an `int r` temp) lets cl convert and scale before the unrelated loads, as the original does (-52 aligned). `if (x <= 1) a; else {b}` reproduces an original that falls through into the `a` arm and `jg`s to `b`, where `a = 0; if (x > 1) {b}` puts `b` inline.

- **A pointer to an element of a global's array is not a local when the original reloads the global at each use (M&B
  `pick_slot_item` 0x46b190, Gemini fp_gem wave 2026-11-10).** `Troop* t = g_game->troops + troop;` hoisted a `g_game`
  read onto a path where the original never touches it (equivalence FAIL: an unmapped read) and moved registers;
  writing `g_game->troops[troop]` at each of the three sites gave 240 -> 132 aligned and FAIL -> WEAK with 0
  mismatches. Then **`(x & 0x80000000) == 0` instead of `(int)x >= 0`** for the sign-bit test gave 132 -> 126.
- **Pass a cast expression straight to a `const&` parameter instead of a named local (M&B Operation::execute 0x49fa10,
  `val_min` / `val_max`, si_gem 2026-11-10).** `int a = params[0], b = params[1]; min_int(a, b)` names two
  address-taken locals; `min_int((int)params[0], (int)params[1])` binds each `const int&` to an unnamed temporary, as
  the original does: +1 case each (526 -> 527 -> 528), and the stack-renamed count rose 534 -> 536.
- **Reloads at the end of a loop body may belong in the `for` step (tentative: one case).** Opcode 1225 of
  Operation::execute: moving `main = g->main_party; pm = &g->parties;` from the body's tail into the step clause,
  `for (...; i++, main = g->main_party, pm = &g->parties)`, gave +1 case (529 -> 530).

---

## 5. Classes, value types and layout

### 5.1 Copy flavour decides the instructions

| Source | Code |
|---|---|
| class with a user copy ctor, passed by value | `fld`/`fstp` per float |
| POD struct copy | integer registers |
| POD returned from an inline helper then assigned | stack temporaries |

A user `operator=` also gives `fld`/`fstp`. So `fld`/`fstp` in a copy tells you
the original class had a user copy ctor or `operator=`. M&B: whole-struct copies
(`T m = *p`) reproduce the original's four-dword spills that field-by-field reads
do not (wave 1, `s00408110`). A float field the original copies with `mov
edx,[ecx+x]; mov [eax+y],edx` must be copied through `uint32_t*`; through
`float*` the compiler emits `fld`/`fstp`. A parameter stored with `mov` rather
than `fstp` is an integer. Copying a field pair as one
`*(long long*)&dst = *(const long long*)&src` changed a register swap to the
original's allocation (observed in Spore).

### 5.2 A user copy ctor builds a by-value argument in place

`f(String(p), Info())` builds a by-value class argument in place (no temporary,
no `rep movs` copy, no caller destructor) **only if the class has a user-declared
copy ctor**. With the implicit copy ctor the compiler builds a temporary and
copies.

- **Two floats computed, then the argument pushed, then both stored (M&B `setup_party` 0x504ef0, `setup_skills` 0x5059c0).**
  The shape `fld y; fsub; fld x; fsub; push &tmp; fstp x; fstp y` is a local filled field by field and passed as a copy
  made by a user copy ctor: `V2 d; d.y = ..; d.x = ..; w->set_pos(&V2(d))`. The copy forwards both values through the x87
  stack, and the computation order is the order of the stores into `d`. A ctor from two floats, a float array or
  `operator-` all compute and store each value in store order. If the vtable load also follows the stores, the object
  pointer was re-read after them: take it by reference to the member (`MissionUi*& w`), +21 in 0x504ef0.

### 5.3 Multiple inheritance and subobjects

- A refcount release through a **secondary base at +0x10** needs the stub to
  derive from a polymorphic primary base (a dummy `virtual` plus `char
  pad[0xc]`) plus a polymorphic refcount base. A non-polymorphic `char pad[0x10]`
  makes the compiler hoist the only vptr-bearing base to offset 0.
- An embedded polymorphic subobject at +4 is a **member with its own vptr**, not a
  pointer.
- A method whose `this` is a secondary base (`lea esi,[ebp-4]` / `add esi,-N`
  before the owner calls) should be an ordinary member of the **full** class via
  real multiple inheritance; the compiler then reproduces the subobject `this`
  and the shifted offsets. Find the vtable by scanning `.rdata` for the function
  address. Modelling a base with a 0x18-byte primary base and the interface at
  +0x18 produced the container null check from the upcast (observed in Spore).
- **A base class with an inline ctor** puts the derived class's vptr store last.
  M&B: a virtual destructor declared and never defined reproduces "vptr stored
  last" (wave 1).

### 5.4 Initialized POD locals are folded away

A plain initialized local read only for stores is constant-propagated and its
slot disappears. Native frames that materialise zeros need an object the
compiler cannot fold: a constructor-expression temporary, or a union overlay.

### 5.5 Vtable layout

- M&B: cl 13.10 gives overloads of one virtual name consecutive vtable slots in
  **reverse** declaration order (probe: `ov(int)`, `ov(const V2&)`, `ov()`
  landed at +0x3c, +0x38, +0x34). A "families numbered by parameter type"
  theory in `terrain_page_update.h` was this effect. Give every virtual its own
  name and take offsets from the call sites, not from counting placeholders.
- M&B: a struct with a hand-written vtable base (`struct dev_item : dev_rec`)
  puts its own virtual slots **after** the base's, so "slot 0x04" methods
  compiled to +0x4c.
- vtable plus 16-byte-aligned members: the compiler gives the **vfptr its own
  16-byte block**, so declare the header explicitly as `uint32_t pad[4]` and then
  the members. A `uint64` member pads after the vfptr unless `#pragma pack(4)`
  (which then shifts every field by 4); an empty member struct occupies 1 byte.
  Cheap check: diff the set of non-stack `[reg+N]` offsets, original versus
  ours.

### 5.6 Layout mistakes show up as offsets, not as bytes

- M&B: a stray padding field (one missing or extra int before a `std::vector`
  member, or an `int f78` overlapping the last array element's spare word) shifts
  every later offset. Check `offsetof` against the last access in the
  disassembly, not only the first.
- M&B: VC7.1 `std::vector`/`std::deque` members start with an allocator word (the
  first pointer is at +4). That is why `size()` shows as `first ? (last-first)/N
  : 0` and reproduces the original's reloads (`reaudit-b07`).
- **Spell such a member as the real `std::vector<T*>`, not a hand-rolled view.**
  A `{int lead; T** first; T** last; T** end;}` struct has the same 16-byte
  layout: the allocator word, then first/last/end pointers into a heap array
  whose length is only known at run time, so no `T* x[N]` can stand in for it.
  But it is not what the original wrote, and the equivalence harness's typed
  inputs only build realistic elements for the real type. M&B
  `rgl::entity::lods` and `meshes` (+0x108/+0x118) are
  `std::vector<lod_mesh2*>` and `std::vector<mesh*>` in
  `src/rgl/scene/scene_entity.h:92`. A module that copies such a class locally
  (`facegen_misc.h`: `ent_vec<T>`, `ui_vec`, and placeholder virtuals `s00()` to
  `s48()` to reach slot +0x4c) usually does so because its slice `rgl.h`
  redefines `rgl::string`/`rgl::vector4` and clashes with the shared header.
  Fix the include, not the class: move the module onto the shared headers and
  include the real declaration (check with chk that the codegen is unchanged).
- M&B: splitting a `char padN[..]` array of a shared class into several members
  changed the register allocation of unrelated functions that merely include the
  header (`party_screen` 0x5059c0: two loads swapped; `terrain_b` 0x46e590: 18
  bytes), even when the new member was a plain `char[16]`. A header-only layout
  merge is not always code-neutral; member numbering feeds the id-keyed
  allocator (section 6.2).
- **`operator new`'s size is a free size check.** `push N; call operator new`
  in the original gives `sizeof` exactly. Assert it at compile time
  (`typedef char chk[sizeof(T) == N ? 1 : -1];`). M&B `GameApp::GameApp` 0x4d3ec0:
  `Big` was 0x44 bytes short of `new(0x6868)` (a missing `char done[0x40]`, plus the
  tail padding that `__int64` members round up to 8). That moved every later heap
  block, and the FAIL showed up as an unrelated pointer in `.data`. In the same
  function `new Big` never ran `Big::init` 0x4d3880, because a plain member
  function is not a constructor. The original calls it on the non-null block
  (0x4d409a..0x4d40b4).
- **Check every field the function reads, not only the name.** M&B
  `music_player::play` 0x4be0e0 read the record's flags at +8 (0x4be10d), while
  `music_res` declared them at +0. The name at +0x1c was right, so the function
  looked plausible and only failed once an import model let inputs reach it.
- **Struct pack** around SDK structures must match the SDK's own packing, or every
  frame displacement shifts.
- **Class-key.** `class` versus `struct` changes no code, only mangled names
  (`PAV<name>@` for a class, `PAU<name>@` for a struct; member access is mangled
  too, `Q` public and `A` private). Forward declarations must use the same
  keyword as the definition, because the compiler mangles with the keyword of
  the first declaration it sees (warning C4099). Swapping the keyword also swaps
  the default access of members and bases, so write the old default out
  (`struct X : B {` becomes `class X : public B { public:`).

### 5.7 Overriding a record's vtable slots (the head_rec family)

The head_rec family's records (`head_rec.h`, `item_rec.h`'s ItemRec/ItemRec2,
`mission_items.cpp`'s MissionBlocker/ItemA/ItemB/ItemC/ItemD) are never
constructed by the binary - each table is referenced only by its own
constructor's vptr store - so a row's evidence is the table's slot targets, not
executed behaviour (docs/Papercuts.md). When the image's derived table
overrides base slots that our source leaves empty, the constructor keeps the
local table's address (`own-copy vtable 30001004 vs image 00596190`) and the
row fails on the vtable bytes. The fix pattern (mission_items 0x4ce960
0x4ceb30 0x4cee50 0x4cef20 0x4cef80):

- Declare the overridden slots **in the derived class, under the base's slot
  names** (head_rec.h's `reset_screen/2/4`, `frame_update/5/6`, `pure_7`,
  `always_true`), one `// slot N  0x<va> <Target>::<method> (owner TU)` comment
  per declaration - the item_rec.h shape. Never append new virtual names (each
  compiles to its own extra slot and the table grows past the image's), and
  never put two declarations on one annotated line.
- Declarations, not definitions: nothing in a row TU calls the overrides, so a
  bare declaration plus annotation is enough - the vtable's slot dword binds
  straight to the image's address (`by_annotation` matches the annotation's
  identifier and last scope) and the slot then agrees by name. The base's own
  annotations (scope `head_rec`) do not leak into a derived row; a per-class
  comment disambiguates shared slot names.
- Override `pure_7` inline in every derived class (`{ return 1; }` + `// slot 7
  0x48b170`): the base's table holds `__purecall` (0x5363c8) there but every
  derived table in the image holds always1.
- Slot 0 is the deleting dtor `??_E<cls>@@UAEPAXI@Z` and stays an **external**
  in the row object (cl emits no deleting dtor when nothing constructs the
  class); special members cannot be annotated, so the slot stays unverifiable
  and is skipped. `vtable_by_content` therefore never fully binds one of these
  tables - rows pass through per-slot agreement. Do not define a dtor to "fix"
  this: the generated body would not match the family's shared dtor and hands
  the relink a body to place.
- One class per vptr store: put the overrides on the class whose constructor
  stores the table (ItemD, not its base ItemD_vt) when a sibling row (the
  dtor 0x4ceec0) must keep the base's table.
- A row pass is row-level proof only: a relink rerun validates declare-only
  overrides end to end. If the relink reports unresolved externals for the slot
  symbols, fall back to one-line forwarding thunks defined in the target's
  owning TU (`void ItemRec::screen_build() { ((MainMenuScreen*)this)->create(); }`,
  cls_map.cpp:3023; also hud.cpp:390, save_game.cpp:225). Targets owned by
  rgl-layer TUs (`reset_gui`, `MapDialog::reset_state` in gui_edit.cpp,
  `static_entity::awake` in entity_types.cpp) cannot include a game header, so
  their thunks stay with the record (`void ItemRec::reset_screen() {
  reset_gui(); }`, item_rec.cpp:55).

---

## 6. Register allocation and compiler internals

### 6.1 Whole-function allocation

Allocation is whole-function and keyed by internal ids, not by textual position.
A far-away residual after a local edit is normal. Where the original has a
register swap that no source change moves (which callee-saved register holds
`this` or a loop pointer), see section 15. Which callee-saved register a value
gets is solved for small functions (reference-count priority, esi, edi, ebx,
ebp): see "Callee-saved register order" in section 7, including its limit in
very large functions.

**The allocator can be traced.** `../vs2005-cpp` has a tracer for c2.dll that runs on
this compiler (`--c2 13.10`; its `docs/c2/tracer.md` and `port-1310.md`): `regmap`
(which of our variables has the wrong register, read off the original), `scratch`
(every eax/ecx/edx rotation pick), `regs --why` (a value's priority, block by block),
`rasim need` (the single changes that give a wanted register) and `tsweep` (source
variants ranked by a trace number). What it has shown on this binary:

- A value's priority is the sum over blocks of refcost · P · w, minus P · w for every
  block it is live through unreferenced. P is the number of distinct register
  candidates the block references, w is 2^(loop depth). Loop bodies do count more,
  as on cl 14.
- **A shared constant is a candidate and counts in P.** In
  `if (n > 0x80) mb_error(..., n, 0x80);` the 0x80 of the compare and of the push is
  one candidate, so the block has P = 4 and not 3. In `Game::load_game_data` 0x5236f0
  that puts `&nfactions` at 47 against the loop stride's 44 and swaps ebx and ebp.
  With `>= 0x81` (no shared constant) every variable has the original's register.
  Naming the constant (a local, `static const`, `sizeof`, an inline helper) does not
  separate the two: 108 variants, all 47.
- **When a constant is a candidate** (`../vs2005-cpp/docs/c2/immcand.md`, measured on
  this compiler): it has two or more qualifying uses anywhere in the function, or one
  inside a loop. Qualifying: immediate stores, `push K`, compares and masks with K,
  addresses of globals and strings; not multiplies, shifts or `lea` displacements.
  Identity is the 32-bit value alone (`int 7` and `char 7` are one candidate), and on
  13.10 a float store counts too (`1.0f` is 0x3f800000, `0.0f` shares with 0). A
  constant used in a loop and after it is loaded in the loop's guard block, which is
  why a guard block can reference many constants. No effect: named constants, 0
  against NULL, declaration position, a second identical string literal, statement
  order after the loop. The one code-neutral lever: storing a narrow value through
  the other signedness (`*(unsigned char*)&x = 255` for a `char` -1) stops it sharing
  a candidate.
- **`if (p) { delete[] p; p = 0; }` is not `delete[] p; p = 0;`** even when the final
  code is the same. The original has the reset store inside the null test (the `je`
  jumps past it), which is the guarded form (a SAFE_DELETE-style macro). Written
  unguarded, cl still ends with the same instructions, but at allocation time the
  store sits in the block after the join, where it adds `&p` and the zero to that
  block's candidates (P 6 instead of 5, and other blocks shift with it).
  `Game::load_game_data` 0x5236f0: the guarded form in its four sections drops
  `&nfactions` from 47 to 43, just below the loop stride's 44, and the function
  matches. With the address at the right priority the natural
  spelling (the member `nfactions` at every use, no reference local) also gives the
  count reload in the loop that the reference spelling lost.
- **Exact ties go to the later definition**, and several near-misses are exact ties
  (`CharSheet::init_screen` 34 = 34, `MapPanel::update` 120 = 120). One more
  referenced value in a block breaks a tie; that is why moving a declaration can fix
  a register and misplace a store at the same time.
- A run of scratch picks one step off means the original has one temporary more (or
  fewer) before that point. Find it in the original's code (a reload, an extra `lea`).

### 6.2 Commutative and SIB order keyed by ids

The compiler sorts commutative operands, SIB components and scheduler ties by
internal symbol and IL ids. Any source change that adds or removes named locals,
references, inline formals, operator records or earlier global references can
flip operand order in distant code. Directly commuting one operation's operands
in source is normally neutral.

**The flip can come down to one accessor's form, not a symbol count (M&B `Operation::execute`).** Whether `troop()` was a macro or the header's `__forceinline` accessor moved 43 of 590 cases at once, all `[imul+ptr+k]` forms, and lost none. Dead locals, copied locals and pass-through inline calls didn't flip it. `party()` or `mission()` as macros flip it back (-44, -19), while `faction`, `quest` and `game` stay right as macros. So when a large function's identical count swings by dozens on an unrelated edit, compare a count that ignores slots and registers to tell the flip from real progress, then try each accessor both ways. Some flips still went unexplained: one read order and one unrelated local each moved about 30 cases.

**A CSE temporary's id decides SIB order, not the comparison's spelling (M&B `frame_update` 0x50fe20).** The original has `[edi+eax]` where ours has `[eax+edi]`, on `food*0x238` used twice. Ten spellings of the comparison and index left it unchanged. Removing the second use of `food` (so there is no CSE) gives product-first, and so does a struct copy of the item reference, though that adds a dead load. So when one SIB order resists every respelling, look at what is being shared between the uses rather than at the expression itself.

**Solved for 0x50fe20 (2026-11-11, c2 tracer): the other operand's key is the global's, and it comes in blocks of 32.** The base/index order is set by the optimizer's sort of commutative operands (the add still reads `t <- temp, g_items` at FgCoalesceLabels; the address is formed at dop2_Codegen), which puts the larger 16-bit hash first: a temporary hashes as 64*key, a named variable or global as 32*key. Globals take keys from c2's global pool, 32..63 first and a later block when the 33rd global symbol is met. Each direct field read of a global struct (`g_keyboard.down[0x1d]`) is its own symbol; in our source `g_items` was the first global past the boundary, got a high key and became the base. Reading the four cheat keys through `Slot& kb = g_keyboard;` removes those symbols, `g_items` falls back into the first block, and the function matches. Check: a dead `{ ItemKind* zz = g_items; }` before the boundary flips the order, after it does not; about 60 extra dead locals flip it from the temporary's side. So when a SIB or commutative order between a global and a CSE temporary resists every respelling, count the distinct global symbols referenced before it.

**An inline helper's formal order can decide a lane's operand order (M&B `MissionBlocker::update_camera` 0x4f8c80).** In `head.x + v98.x` the original loads `head.x` first, while the y and z lanes load `v98` first. Writing the sums `a+b` or `b+a`, a `float` temporary, pointer formals, a `const&` local for `v98` or a reference to `head` all left the x lane unchanged. Swapping the helper's formals to `hv_add_mix(const vector4& b, vector4& a)`, called as `hv_add_mix(ag->v98, head)` with the same body, fixed it and nothing else moved (22 -> 16 bytes). So when source order is ignored, try the order of the inline helper's formals.

### 6.3 Register calling conventions for translation-unit-local statics

**Without link-time code generation, a `static` function in the same TU gets a
custom register convention.** Parameters arrive in `eax`/`edx`/`ecx`; an
argument arriving in `eax` or `edi` means a static helper in the same TU, and the
caller may keep `ecx` live across the call because the compiler knows the
callee preserves it. To reproduce it:

- Define the callee `static __declspec(noinline)` and give it a same-TU caller,
  because it is emitted only when a caller exists. M&B: `0x4035d0`
  (`GetColorChannelBits(D3DFORMAT)`, argument in `eax`) matched 112/112 bytes
  this way, and `0x404140` the same.
- A `call A; call B` pair with no `mov ecx,this` between them is the same
  mechanism (the compiler knows static `A` preserves `ecx`). It is reproducible
  only with `A`'s full body in the same TU (M&B `0x402be0` stays +2 bytes
  without it).
- Conversely, a same-TU callee lets the compiler assume its register use. When the
  original does **not** have that knowledge, call an undefined declaration, or a
  stub type with no body. M&B (`s00408110`): an inlined same-TU callee gave the
  caller interprocedural register knowledge, so values stayed in caller-saved
  registers; a call to an out-of-TU function must be declared with no body.
- A function with `ecx = this` and `ret 4` that uses no members is still a
  thiscall member.

### 6.4 Scratch-register rotation in call-argument setup

When the instructions match and only the scratch registers that set up a call's
arguments are rotated (ours `ecx/edx/eax` where the original has `eax/ecx/edx`),
first check that the callee is declared the way the original calls it (see 6.3).
If it is, the cause is usually not the call itself. The first scratch register
the allocator picks there depends on how many temporaries earlier code created,
so look at the expressions before the call:

- An earlier inline helper's form changes the temporary count. M&B
  `Mesh::pick_vert`/`pick_edge`/`pick_poly` (0x52da20/0x52dc00/0x52df40, 8, 8
  and 6 bytes, unmoved by ~60 spellings of the out-params, parameter order and
  loop form) all matched when the list's `size()` used an early return instead
  of `?:`:

  ```cpp
  int size() const { return first ? (int)(last - first) : 0; }      // rotated
  int size() const { if (!first) return 0; return (int)(last - first); }   // exact
  ```

- Taking an argument into a named local ahead of the code before the call
  changes which register its `lea` gets. M&B `Rec5ac8::write` 0x4e9d10 (2
  bytes, `lea ecx` vs the original's `lea eax,[edi+0x5a08]` for `write_soa`'s
  argument) matched when the pointer was taken from its member before the two
  inlined `write_vec4` calls. The same change also replaced the
  `(char*)this + 0x5a08` casts with the members, and the two were not tested
  separately:

  ```cpp
  char* soa = f5a08;
  write_vec4(&f8bc, out);
  write_vec4(&f59f8, out);
  write_soa(out, soa);
  ```

### 6.5 Rotation is a chain, and it can start at a file-static store

Found on M&B `Operation::execute` 0x49fa10 (69 KB, 590 cases), where the original
has `ecx/edx/eax` and ours had `eax/ecx/edx` across the whole function. Dozens of
spellings at the rotated sites changed nothing, because the cause wasn't at those
sites. Trace rotation back to the **first** temporary in the function:

1. **The first pick set everything after it.** The original starts with
   `mov ecx,[edi]` for `g_cur_opcode = id`; ours used `eax`. In ours `eax` was already
   taken by the zero register (`xor eax,eax`), because `g_script_error = 0` comes first
   in the source.
2. **The original's scheduler sank that store below the `id` loads.** It can only do
   that if the store can't alias `*this`, which means the global is a **file static**.
   Every reference to 0x5e2590..0x5e25a4 in the image lies inside 0x49cc60..0x4b10d7,
   so the image agrees. With `static` and the store first, the prologue came out
   instruction for instruction like the original.
3. **Each scratch pick depends on the temporaries before it, in IL order, across
   cases.** One extra or missing temporary rotates later code until something
   resynchronises. So rotation isn't a local property of the code that shows it, and
   a local fix can lose more than it gains: one fix lost 49 cases and gained 20.

How to work on it:

- Work top-down, and fix the earliest rotated code first.
- Judge an edit by the count with registers **and** stack slots masked, as well as by
  the identical count.
- Find where the rotation changes by compiling subsets of the function.
- When a store seems to move across pointer loads in the original, ask whether the
  global could be `static`: list which addresses reference it. A narrow range of
  referencing functions is evidence of one original translation unit.

Status: the prologue is exact, the rotation is right from there on, and 295 of 590
cases were identical at that point (217 before). The row isn't byte-exact yet. So this is one
function's evidence. Try it on other rotated rows before treating it as general.

**Callee visibility sets the global register map too.** In the same function, calls
to visible callees that reach nothing opaque don't kill a local whose address escaped
elsewhere. Making `lookup`, `get`, `is_live`, `first_flagged` and similar visible
produced the original's `edi=this, ebp=locals, esi=params[0] low, ebx=params[1] low`
with no case edits, and moved about 200 cases to identical. Keep a pure callee opaque
if the original calls it in order, since a visible one can be reordered or hoisted.

**An address taken once, even if the pointer is never used, decides whether an array's dwords stay in registers across calls.** After the two files merged,
nothing took `params`' address any more and its dwords stayed in registers. One unused `__int64* args = params;` restored the original's map (0x49fa10).
The merged file also confirmed the file-static finding above: `g_script_error` is now a real file static, and 313 of 590 cases are identical.

### 6.6 Page-load folds are switched on per function

`vector::operator[]` normally gives a materialised `lea r,[b+i*4]; mov r,[r]`. cl
folds it for the whole function once something in that function gets loop-optimised.
From small probes for M&B `Mission::start_setup` 0x484590:

- **Triggers:** a partial or full unroll, a trip-count-1 fill loop (including the
  `_Ufill` in `std::vector::push_back`), and store promotion of a memory variable in
  a loop (`x = x + *p`).
- **Not triggers:** `for c<1`, `__int64` accumulation, uncounted loops with branches,
  and a size threshold (1600 extra statements changed nothing).

Still unknown: the original contains these same constructs and doesn't fold.

---

**Triggers isolated on M&B `start_setup` 0x484590, by truncating the function (2026-11-06).** Each of these folds a page load in the function, even in a minimal one, and removing all of them stops the folding: a `Party* const& page` reference helper (plain `pages[i>>4]+(i&15)` doesn't); a `for` over a range with cached bounds (a guarded do-while doesn't; `continue` inside is fine); a `for` over a small fixed count such as `curve[2..8]` (a do-while stays a real loop; `while`, a `for` from 0 and an inline helper fold again); `push_back`'s `_Ufill`, a trip-count-1 loop; and `new rgl::psys`, likely from its inline ctor's vector `_Buy(0)`. A local copy of a loop's value can bring folds back. So "the original contains these same constructs" is wrong for the reference helper and the fixed-count loop. The two STL triggers aren't explained: the original has the same store shapes and doesn't fold, and that row's stand-ins for them shouldn't be permanent.

## 7. Stack layout

- **`/O2` homes** come from a conflict graph over lifetimes, ordered by increasing
  size then decreasing use count (parameters are scanned separately). Narrow
  scoping, or per-arm short-lived values, lets later values reuse slots and
  shrinks the frame. Local-slot order did **not** depend on declaration order in
  24 shuffled variants of one function (observed in Spore's cl 13.10 code).
- **A local's size and scope decide its group.** Smaller objects are placed first, so an
  8-byte object takes a slot before 16-byte ones. Block-scoping address-taken floats
  frees a slot, and one object updated in place, rather than a chain of named values,
  reproduces store-then-reload in one slot (0x484590).
- **What decides slot sharing and order (M&B `cls_map`, 7 rows matched with this; 1, 2 and 6 tested directly, the rest inferred).**
  1. Escape is interprocedural. An address-taken local keeps a private slot if the callee's parameter escapes or the callee is opaque, even
     when the callee is defined later in the TU. Taking the address of the parameter in the callee counts, and leaves the callee's bytes
     unchanged: `int*& n = count` moved `count` out of a shared slot (0x4dd6e0). `inline` on the callee doesn't hide its body.
  2. The same analysis decides whether memory counts as clobbered. A hidden-return vector is unclobbered across later calls only if the
     callee's body is visible and doesn't leak the pointer. Reloads of a local vector's base are a sign the original saw that body (0x4e05c0).
  3. Conflicts follow scope, not just liveness. An object conflicts with everything inside its enclosing block: `{ {f2} tstring name; }`
     conflicts where sibling blocks `{f2} {name}` don't.
  4. A function-scope object that escapes stays live to the end of the function. A later local can only share with it if it was block-scoped.
  5. Slots order by the total post-optimisation uses of the merged group, descending from esp, not each local's own uses, so which group a
     late local joins can flip the order. Ties: see the solved rule below.
  6. Declaration order is irrelevant. In a probe of 14 equal-sized cases, more uses gave a lower address. Folded references add no use.
  7. A dead parameter's home is overlaid by a block-scoped copy, and the first free home wins.
  8. Partly overlapping slots mean separate locals: a 20-byte `z[5]` was really `uv[2]` plus `pos[3]`.
  Method: map every slot from the original with push depth removed, write one local per observed slot, then fix the scopes.
- **The slot order, solved for locals that all conflict (probes of 150 random functions per run on `agent_hud`'s behalf):**
  1. **Base order:** size ascending, then uses descending, with ties going to the local whose *last* reference comes earliest in evaluation order. Call
     arguments evaluate right to left. This held on 150 of 150 equal-size functions and 28 of 28 small mixed-size frames. First reference and declaration
     order scored only 5 to 16 of 150.
  2. **Large frames:** take the base order as the starting array, then run a K&R quicksort: swap the middle element to the left, partition with strict
     `key(a[i]) > key(pivot)`, recurse. The descending key is `1000*uses/size`, where uses counts every reference in the listing. This matched 122 of 122
     frames on one seed and 111 of 111 frames of 144 bytes or more on another. Seeding from any other order, or Hoare partitioning, scored 20 to 33 of 122,
     which is likely why earlier models failed on mixed keys.
  3. **The switch** is in the 128 to 143 total-bytes bucket; the 128 bucket fit the large model on all 3 cases and the small model on 2. Floored and exact
     ratios gave identical results, so the probes don't tell them apart.
  4. **Not tested:** locals with disjoint lifetimes. Inferred from 0x4fb350 only: they merge into a group first, with uses summed and the largest member's size.
  In real functions, slot swaps that looked like ties usually turned out to be structure: a scope or an escape fixed them in `ItemA`, `update_status`,
  `camera_to` and `Rec58::build`.
- **Callee-saved register order, solved for small functions (the same probe method, `work/match/bm6o_*`, on `Operation::execute`'s behalf):**
  1. **Rule:** webs that live across a call are colored in priority order, and each takes the first free register of **esi, edi, ebx, ebp**.
     Priority is the web's reference count, defs included, highest first; ties go to the web defined first. Declaration order doesn't count.
     This held on 150 of 150 random straight-line functions with 4 values, 150 of 150 with declaration order shuffled apart from assignment
     order, and 150 of 150 with some uses as pointer bases. First use, last use, value id, live-range length, uses per length and declaration
     order scored 14 to 87 of 150.
  2. **What counts:** `x += y` is 2 references. `x = q(x)` starts a new web. A named variable that holds a call result also counts its copy and
     the two-address result it is coalesced with: `int b = q(1); f(b - r(2));` counts 4, while the bare temporary of `f(q(1) - r(2))` counts
     at most 2. So naming a value raises it in the order, and dropping a name lowers it.
  3. **Weights:** a use under one `if` counts about 1/2. It counts about 1/3 under two levels, under `a && b`, or in a second `if` after a join, and still
     about 1/3 at three levels. An `else` arm counts 1/2. Loop bodies don't count more. Wrapping the probe in a 600-case switch changes none of this.
     The exchange grids (`bm6o_grid.py`) put each boundary at m = 2n or m = 3n, a little above the exact fractions.
  4. **In a switch,** a long-lived web (`this`, a pointer parameter, a promoted array dword) is dead in the cases that don't use it, so each case's
     values take the free registers in priority order.
  5. **Limit:** in the 70 KB interpreter, cases whose text gives the original's registers when compiled alone still come out differently inside
     the whole function, and keeping different subsets of its cases moves the result non-monotonically. There the order also depends on
     function-wide state. A spelling inside the case can't fix that; check the case alone with a probe before spending spellings on it.
- **A spilled variable keeps one slot for all its webs (M&B `Game::build_map_trees` 0x5213d0).** One function-wide `unsigned i` reused by six loops put the counters of the two loops that spill it in one slot. The original has them in different slots, so it had a counter per loop. `for (unsigned i = 0; ...)` in each loop gave +111 identical. Two loops whose counters share one original slot, with nothing else in it, can be one variable: an `int ni` shared by the cells loop and the per-cell node loop gave the original's group. When a slot holds two loops' counters, or they're split across slots, check how many counter variables the source has.
- **Dead references don't count toward slot order (same function, bm9bt).** `{ int t = mv; t = mv; t = mv; }` left every slot where it was, so a use count can't be raised without changing code. In this 0x2ac270-byte frame, slots whose listing ref counts are equal on both sides still came out in swapped pairs, and a simulation of the rule above, run on our own listing's groups and counts, missed our own layout by 37 inversions even after fitting the counts. When the instructions already match, look for a store or reload that differs (one more `c.w = 0` store, one more reload of a CSE temporary), not for a count.
- **Naming a global's re-read raises its register priority (same function).** `for (...; i < g.level->ground->size(); ...)` and `rgl::world* lvl = g.level; for (...; i < lvl->...; ...) { ...; lvl = g.level; }` give the same loads, but the named one ranks higher and took edi like the original. Assigning `lvl` earlier, before two more uses, ranked it too high (esi).
- **Refinement, inferred from `start_setup` 0x484590 only:** use counts are taken before cl duplicates tails, so a statement cl duplicates counts once. Each local then joins the newest compatible group. Scalars that aren't address-taken conflict by liveness; address-taken ones conflict by scope.
- **A temporary's destructor after the store** means the temporary lived in the same full expression or block as the assignment. Moving a helper inline took
  `Rec58::build` from 400 to 521 identical, because callee-saved pushes sink only when the code above allows it, so a helper's shape can move every stack offset.
- **Order by size.** In one function the locals sorted by size ascending, then by
  declaration; shrinking an array (`char q1[0x105]`) was the only way to move it
  below another.
- **Large frames** are ordered by `floor(1000*uses/size)` with an unstable
  middle-pivot quicksort; one extra use can move a group.
- **Dead incoming-argument slots hold locals** once the argument is dead. The
  compiler reuses a dead parameter's slot for a later local; reusing the
  parameter in source (`int& count = (int&)key;`) reproduces it.
- **An empty-class temporary** (for example an empty allocator argument) is passed
  as the address of an existing parameter slot. Passing `&param` reproduces the
  reload after the call.
- **An output dword local must be `uint32_t`**, not a 12-byte struct, or the
  frame grows.
- **`float pts[N]` locals set the exact `__chkstk` frame size.** M&B: N*4 + 0x58 in
  one wave-1 function (`s00408110`).
- **16-byte alignment.** A local `hkVector4` / `HK_ALIGN16` inplace array adds
  `and esp,-16`; `struct HK_ALIGN16 hkInplaceArrayD` reproduces the prologue.
  Named scalar floats do **not** reproduce an aligned `hkVector4` frame; use an
  `hkVector4` local. Observed in Spore's cl 13.10 code (Havok).
- **Uninitialised lanes the original copies** (a `vector4`'s never-written `w`, a
  `mat4` built from a 12-float identity): the equivalence harness compares stack
  garbage at absolute offsets, so reproduce the frame with a POD `struct Frame`
  holding the locals at the original's offsets. Objects whose lifetimes the
  compiler would overlay into one slot go into one local struct. (M&B.)
- **Very large functions stop sharing slots — but the trigger is *which* locals,
  not how many.** M&B: the 70 KB script interpreter (`0x49fa10`, 591 real cases)
  packs any 300 of its cases but not all of them: frame `0x2630` against the
  original's `0x1ec0`, and every stack offset differs. Synthetic one-knob probes
  (600-case switches, `work/match/scratch_si_gen4.py`/`gen5.py`/`gen6.py`,
  results in `scratch_si_pack*.asm`) disproved the earlier "case-count cutoff"
  guess and pinned the real predicate:
  - **Packs fine even at 600 cases:** block locals that are only read (ints,
    floats, doubles, `__int64` — any type, 1 or 3 per case, shared or unique
    names), and block locals that are address-taken but never read after the
    address-escaping call (`clamp(&v, &lo, &hi)` with no later use of `v`).
  - **Never packs — one private slot per scope instance:** a block local that is
    address-taken *and read after the escape* (one per case collapsed a 600-case
    switch from 48 B to `0x12e8`; three collapsed it to `0x1c48`), or co-live
    with other such locals of mixed type. Arithmetic on the collapsed frames
    matches exactly: `0x1c48 = 40 + 600*12` (3 private dwords per case).
  - Case count, address-taking per se, and name uniqueness are all innocent.
  - **The lever — hoist to function-scope shared scratch.** Exactly one case body
    runs per call, so every block local across all cases can legally share slots.
    Rewriting block-local value scalars to function-scope scratches (the
    `lv_` trick: `int si_0, si_1, ...; float sf_0, ...;`) collapsed every probe
    back to 40–56 B (`scratch_si_pack6_h*`), and mirrors the original's slot map,
    where `0x10/0x18/0x20/0x24/0x28` are shared validator-arg/clamp-arg/vector4-arg
    slots reused by all cases. Routing about 220 `set_lvalue(locals, <temporary>)`
    sites through one function-scope `__int64` had already cut the frame from
    `0x2d18` to `0x2630` and removed 6 KB of code — same mechanism.
  - **Hoist only the address-taken locals.** Hoisting every block-local scalar
    (161 decls) also turned register/spill values such as `party(p)` ids into
    function-scope homes with private slots (`si_0`: 177 uses below params).
    Hoisting only the 150 locals the pre-hoist listing shows under `lea`
    restored the original's low map: shared slot 0x10, `result` 0x17, `lv_`
    0x18, params 0x28 (original 0x2c).
  - **The real packing predicate: escape to an opaque callee.** An object
    (named local *or* `$T` temporary) whose address goes to a callee whose
    body cl cannot see is assumed to escape and keeps a private slot for the
    whole function; passed to a callee defined in the same object, it packs.
    Probe (`work/match/probe/gen_esc5.py`): 60 cases of
    `use(g((int)params[k]))` give 60 temp slots with `g` external, 2 with `g`
    defined (`__declspec(noinline)`) in the TU. In execute(), every shared
    `$T` fed a same-TU validator; every private one an external helper.
    Giving `clamp_index`, `vector4::set`, `SlotArray::get/set`, ... visible
    bodies moved the frame `0x24cc -> 0x22d0` and params to the original's 0x2c.
    The original's helpers were header inlines left out of line.
    Not every callee was visible, though: moving `Elem::set_lvalue`/`get_operand`
    (0x49d500/0x49d650) into the interpreter's object packs the lvalue
    temporaries, but cl then drops `locals` from `ebp` (the original pushes `ebp`
    at every site) and cross-jumps the SET_LV tails into 12 hubs / 39 calls (the
    original: one 57-jump hub, 147 calls; opaque callee: 134). So execute() did
    not see those two bodies; check `ebp` and the hub count before adding one.
    In a function this size a named block local escaping to an opaque callee
    stays private too (`do { __int64 lv = (e); set_lvalue(locals, lv); }`:
    326 named locals in 290 slots).
  - **The inliner has a per-function budget too.** In the whole interpreter cl
    stopped inlining the `inline` accessors (`party`, `troop`, `agent`, ...:
    147/97/45 out-of-line calls); the original inlines them all (its callee
    counts: `PartyMgr::get` 193, `AgentTable::get` 73). `__forceinline` restores
    the counts. Compare callee histograms of both sides before reading layout.
  - Listing offsets: `fb = frame + 16 - N` for `_x$ = -N` (fb = esp after the
    four callee-saved pushes), the same base as capstone displacements.
  - Measure with `work/match/scratch_si_bisect.py <a> <b>` (per-case-range
    compile; reads frame from the `sub esp`/`mov eax, N; call __chkstk` prologue
    and counts named/temporary slot symbols in the `/FAs` listing).

---

## 8. Block layout and control flow

The compiler places the then-arm as fallthrough, cross-jumps identical tails
(about a 20-byte threshold), relocates ranges with a late block mover that needs
an unconditional skip, and lowers `switch` to a range check plus jump table or to
a `sub`/`dec` compare chain.

- **Switch case order** sets block layout while the jump table stays numeric.
  The `sub`/`dec` compare tree depends on case order; brute-force orderings in one
  compile. A `switch` whose case bodies are outlined after the `ret` reproduces
  that layout; sort the source cases into the original's address order, not
  ascending value order.
- **One `case` per value** gives a direct jump table where grouped labels did not
  (M&B: DirectX 9 SDK sample code, wave 1). A switch over sparse `HRESULT`s
  reproduces the original's decision tree and table only if every `case` appears
  in the original's body order, and a label shared by two cases (`0x8200000b` /
  `0x80070002`) must be listed together.
- **`switch` versus `if` chain.** A `switch` emits out-of-line case blocks where
  three `if (id == K) { store; return; }` tests were inlined at each test.
- **Arm order** follows `if (x == 0) {A} else {B}` versus the inverted form, which
  decides which block is outlined. A named `bool` compound predicate can set
  native arm order where structured control flow cannot.
- **Two arms with one body versus `||` (M&B `Game::build_map_trees` 0x5213d0, round bm9bt).** `else if (mt == 0x16 || mt == 0x17) { if (d > 0.01f) c.w = 0; else c.w = flag ? 0 : 1; }` let cl hoist one `c.w = 0` above the compare and move the block last. Writing `else if (mt == 0x16) {body} else if (mt == 0x17) {body}` with the same body gives the original: cl still merges the arms into the `cmp 0x16; je; cmp 0x17; jne` test, but the stores stay apart (+31). The extra store also changed a use count, and that fixed a stack-slot swap elsewhere. So when the original tests `a || b` but the body's layout is wrong, try two arms with the same body.
- **Early returns versus `||`.** Two separate early returns, not `a || b`,
  collapsed one function from 93 to 20 diff bytes; `if (A && B) {...} else
  {...}` matched another (observed in Spore).
- **Rare case out of line.** Invert the guard to put a rare case's body after the
  `ret`: `if (m != 2) { ...; return; } call1;`. `if (table) return call(...);
  return false;` keeps the false block out of line, while `if (!table) return
  false;` inverts the `je`/`jne`.
- **Same-call arms are tail-merged.** M&B: an `if (a) {X} else {Y}` whose arms end
  in a call with the same argument registers is merged by the compiler, so write
  the call in both arms (`ClipCursor(&rc)` / `ClipCursor(0)`) rather than
  computing the argument first (`s00401010`).
- **Re-read the field when the original repeats a compare (M&B `place_slot_mesh` 0x510fa0, matched).** With
  `int kind = it->type_flags & 0xff;` cached in a local, cl threads `kind == 4` past the inner test, drops the
  second `cmp eax,4` and adds a `jmp`. Writing `(it->type_flags & 0xff) == 4` at each test keeps the second compare,
  as the original does. A second stray `jmp` further down was only loop-alignment padding shifted by the first, and
  it went away with the same fix. So check whether a lone diff in a long function is just alignment knock-on before chasing it.
- **Equivalent nestings can reduce to one CFG (M&B `InventoryPanel::transfer_slot` 0x512c80).** A nested form, a
  single `if (c && !(a && b)) {body} else if (locked) {refused}`, and the inverted guard with an early `return` all
  compiled to the same 281/410. When restructuring doesn't move the count at all, the difference is in one
  test's polarity (here `item != -1`: ours `jne` to the body, the original `je` to the refusal), not in arm order.
- **`do { if (a) break; ... } while (0)`** is the idiom for a guarded block with
  several forward exits to one join; nested ifs and early returns often compile
  identically because cross-jumping rebuilds shared exits.
- **`goto` ladders** are occasionally the only exact form where an if-chain and a
  switch both fail.
- **Condition polarity is a lever** in clamp and compare idioms. `return (r >=
  amount) ? 1 : 0;` gives `xor ecx,ecx; setge cl; mov al,cl`, where `>` gives
  `setg`; `if (x <= lo) x = lo;` and `if (x < lo) x = lo;` select opposite branch
  polarity. Read the original's `setcc`/`jcc` polarity off the assembly and mirror
  it in the source relational, and keep the result in a named `bool` (returning
  the comparison directly can fold the `setcc` into a `test`/`sbb` normalize,
  section 11.5).
- **Unsigned count tests.** M&B: `> 0` on an unsigned count gives `jbe`, `!= 0`
  gives `je`.
- **An else arm laid out inside the then arm's `new` diamond means two ifs (M&B `setup_party` 0x504ef0).** The original put
  `else label_refresh(party2)` between the `jmp` and the `xor eax,eax` of a `new` inside a nested `if`. Inverting the
  condition or adding a goto left it where it was. Writing `if (s == 0) A; else B; if (s == 0 && c) { new ... }` gave the
  original's block order, and cl folds the second test of `s` away (+8 identical, positional diff 2122 -> 283).

---

## 9. Floating point and x87

M&B is x87-only. This is the section with the most compiler-specific shapes.

### 9.1 Precision decides what a difference means

M&B creates its D3D9 device without `D3DCREATE_FPU_PRESERVE` (the stock DirectX
SDK sample framework does the same), so the render thread runs the x87 at
**24-bit** precision. Then every x87 result is already rounded to a 24-bit
mantissa: keeping a value in a register or storing it to a `float` gives the
**same value**; only the exponent range differs (a register can hold magnitudes
that a `float` overflows). Measured on `camera_ray::screen_to_world` (`0x4bd2f0`)
over 2,000 realistic camera setups plus corners:

- 84% of output floats were bit-identical between the original and a
  non-byte-exact rebuild; the rest differed by 1-2 ulp, with large ulp counts
  only under cancellation (a result near 0 from terms around 5,000).
- Both builds had the **same** error against an exact double reference (max 14.8,
  mean 0.46 ulp of the largest term). At 53-bit precision the rebuild was slightly
  more accurate.
- The remaining differences came from **summation/association order**
  (`((z*u + py*f) + px*s) + o` versus `((px*s + z*u) + py*f) + o`), not from which
  values were spilled.
- The harness failures that started the investigation came from absurd random
  inputs whose intermediates overflow `float` range in one build but stay finite
  in the other's 80-bit register.

Derive float sum **grouping from the assembly, not from a decompile**.

### 9.2 Operand shape

> **THE DOUBLE-SCALAR LEVER: try this first on any x87 operand-order or spill difference.**
> When a float expression is scaled by a value, **declaring that value (or casting one operand) `double`**
> changes cl 13.10's x87 shape:
> - **Operand orientation flips to memory-first:** `fld [m]; fmul st(n)` instead of `fld st(n); fmul [m]`.
> - **The value stays unrounded on the x87 stack** instead of being rounded through a float slot.
> - **Bound to a `const float&` parameter, a `double` gets a float temporary per use** (`fst dword [esp]`,
>   reload), the "rounded copy beside the register value" pattern.
>
> On 2026-11-10 it made five functions byte-exact in one afternoon, one after another:
> - `rot2::rotate_2x2` 0x4f3b40: `double s = sin(angle)` (details below).
> - shapes.cpp 0x409e90: the scale factor `t` declared `double`.
> - shapes.cpp 0x409980: `double t` plus a `vector4 dt(d.x*t, d.y*t, d.z*t)` temporary.
> - shapes.cpp 0x408a50: `v[i].y * (double)s[1]`.
> - fx_keys.cpp 0x423390: `double t = 1.0f - rec(i)->f18`.
>
> It also took fx_keys 0x423440 from 82 to 80. Earlier lanes had tried many spellings of these rows, and every
> `float` spelling failed. `tools/matching/vsearch.py` tries it mechanically: its x87-type rewrite
> (`float v = E;` <-> `double v = E;`) and cast-drop. Run it with `--beam 8`.
>
> **Is it safe?** In values, it is exact where the original keeps the value unrounded, which the instruction
> shape shows. The game's render thread runs the x87 at 24-bit precision (9.1), so float and double give the same
> results there. Still confirm each row with `tools/difftest/sweep_nonmatching.py`.

- `fld st(i); fmul [m]` (keeps a just-computed value on the stack and multiplies
  by memory) is the cl 13.10 shape **for a `float` scalar**; with a `double` scalar the same compiler emits
  `fld [m]; fmul st(n)` (the lever above). Observed in Spore's cl 13.10 code (Havok x87).
- `fstp [local]` followed by a reload is also cl 13.10 output. Where the original
  does that, the source must keep the value in a **memory-resident struct field**
  (an `hkVector4` local), because a `const float` scalar stays unrounded on the
  x87 stack. A `volatile` read of the stored field is the cheap lever. Observed
  in Spore's cl 13.10 code (Havok).
- **The NaN-test mask differs per site.** `if (a < b)` can emit `test ah,5; jp` or
  `test ah,1; je`; the original's opposite mask is sometimes `!(a >= b)`-shaped
  and sometimes not, and the NaN behaviour decides. Read the mask off the
  assembly; do not infer it from the C relational alone. Concretely `test ah,5;
  jp` is `(a<b)` and `test ah,0x41; je` is `(a>b)`; `if (!(x >= -r))` avoids a
  canonicalised operand swap when the original did not swap.
- A `double` compared against a `float` constant: `d < 0.5f` with `d` a double
  emits `fld qword`; `(float)d < 0.5f` gives `fld dword` + `fcompi`.
- A float literal read as `fmul dword [mem]` against a double needs an
  `extern const float` **variable**; a literal `0.001f` folds to a **double**
  constant (`fmul qword`).
- The **literal's type**, not its value, picks the opcode: `1.0 - x` (double)
  gives `fld1; fsubrp`, `1.0f - x` gives `fld1; fsub`.
- `(float)(uint32_t)x` lowers to `fild` plus a conditional `fadd 2^32`; the
  recurring decompiler `if (x < 0) f += 4.29e9` lines are unsigned casts, not
  hand-written source.
- **Which operand is the unrounded x87 value in a compare**: `dist > radius`
  versus `radius < dist` picks it; that alone fixed a denormal mismatch
  (M&B `polygon_sphere_contact`, `0x40a000`).
- Other source choices that change where x87 values are rounded: parenthesisation
  and cast placement, constant folding of additive chains, operand orientation
  following source order, `double` locals, a local snapshot of a float global
  (the `fld st(0)` duplicate), and integer views through `memcpy` or a one-float
  wrapper.
- **Parameter form.** M&B: when every spelling of an x87 sum compiled identically,
  the original probably used a different operand form: `void f(const vector4& t){
  vector4* o=&cur.o; o->x += t.x; ... }` fixed an asymmetric `fld` order that
  `float*` spellings could not (`s00409500`).
- **Vector helpers set operand order downstream (M&B `terrain_b`, round bm6tb).**
  - **Inline vs out-of-line normalize (`render_batch::render_scene` 0x46e590):** going through the header's inline `vector4::normalize` body, whose `length()` sums z, y, x (wrapped as `vec4n`, like `shapes.cpp`'s `vec4i`), changed which x87 operand loads first. Both cross products after it then matched.
  - **Re-derived references reproduce reloads (`PolyMesh::compute_face_normals` 0x4705e0):** `const vector4& p0 = verts.begin()[faces.begin()[i].vid[0]].pos;`, then p1 the same way, gives the original's second `vid[0]` load.
  - **A loop-scoped vector temporary (`compute_edge_creases` 0x470750):** `vector4 d = b.pos - a.pos;` inside the loop fixed the normalisation's fld/fmul order.
  - **A `const vector4&` helper bound to a temporary keeps the last lane on the stack (M&B `Game::build_map_trees` 0x5213d0, round bm9bt).** The original multiplies `c` by a lit vector with `fld c.z; fmul st(1); fstp c.z; fstp st(0)`: lit.z isn't consumed. A named `vector4 lit = ...; c.x *= lit.x; ...` consumes it (`fmul [c.z]`). `map_mul(c, lightL + lightM * s)`, with `map_mul(vector4& a, const vector4& b) { a.x *= b.x; ... }`, gives the original's shape (+4). The same kind of helper, `map_add(sum, q + d * 0.25f)`, gave a chain sum's `fld r.x; faddp st(4)`. It doesn't work everywhere: a `const float&` scale bound to a temporary or to a named float changed nothing, and in a loop where the original consumes the value, the helper's reference is wrong.
  - **A member helper (through `this`) sets the x87 operand order where caller-side spellings are canonicalised (M&B `Game::build_map_trees` 0x5213d0, round bmH).** In the caller, `d.y = d.y * inv` and `inv * d.y` both give `fld d.y; fmul inv`. The same body as an inline member, `((map_vn&)d).normalize_inl()` with `x *= inv; y *= inv; z *= inv;`, gives the original's `fld inv; fmul d.y` (+2). A 2D rotation `((map_uv&)uv).rot(ca, sa)` that returns rx and writes y, and an offset add `((map_uv&)uv).add(rx, t)`, gave +4 and +2 more. It isn't general: the y sum inside `rot` still evaluates `ca*y` first however it's spelled, and the same member add or scale made other sites worse (-3 to -6) or left them unchanged. So try the member form per site and keep it only where it helps.
  - **Inline Vec2 operators, a `double` local bound to `const float&`, and a repeated call make the original's
    spills (M&B `rot2::rotate_2x2` 0x4f3b40, byte-exact 2026-11-10).** The original saved the sine with
    `fst [esp]` right after `fsin`, reloaded it, saved a second copy into the angle's own stack slot, and spilled
    only the y lane of each product, while the cosine never left the x87 stack. The source that matches:
    ```cpp
    struct Vec2 { float x, y; Vec2(float a, float b) : x(a), y(b) {}
        Vec2 operator*(const float& f) const { return Vec2(x * f, y * f); }   // + and - the same way
    };
    struct rot2 { Vec2 r[2]; void rotate_2x2(float angle); };
    void rot2::rotate_2x2(float angle) {
        double s = sin(angle);
        Vec2 r0 = r[0] * cos(angle) + r[1] * s;
        Vec2 r1 = r[1] * cos(angle) - r[0] * s;
        r[0] = r0; r[1] = r1;
    }
    ```
    In the tree the row type is `rot2_row` in `src/include/rot2.h`, not `vec.h`'s `Vec2`. The operators were first
    added to the shared `Vec2`, and that swapped two stack slots in `MapPanel::draw_party_line` 0x51c090, a
    function that never uses them: new inline members in a header that 112 sources include shift cl's symbol ids
    (6.2). Put a new inline helper in the narrowest header that needs it.
    Each piece maps to an instruction pattern:
    - **A `double` bound to `const float&`** makes a float temporary at each use: `fst dword [esp]` / `fld [esp]`,
      and a second temporary later (here, the dead angle slot). That is the "rounded copy kept beside the unrounded
      register value" pattern; a `float` local or `volatile` cannot produce it.
    - **A call written out at each use** (`cos(angle)` twice, no local) is computed once and kept on the x87
      stack (`fld st(0)` duplicate), never stored. Naming it (`double c` or `float c`) spilled it instead.
    - **An inline operator returning `Vec2(x*f, y*f)` through a constructor** keeps x on the stack and spills y:
      only the second lane goes to memory. A body that fills a named local and returns it was worse (228-252).
    - **The result landing in two named row temporaries, then assigned**, gives the struct-copy-through-eax/edx
      ending; a matrix temporary or direct row assignment was worse.
    - **Operand order of the first row** decides which value is converted first: `r[0]*cos + r[1]*s` matched;
      `r[1]*s + r[0]*cos` was 30 aligned (cl evaluates the right operand of the operator first).

    **How it was found, and what to do next time.** One-at-a-time edits and a greedy variant search stalled at
    116-129 aligned: every single change from the matching shape was worse, so the match was only reachable as a
    *combination*. An outside analysis proposed the Vec2-operator structure; compiled verbatim it scored 175 and
    was nearly dismissed. A cartesian grid over its listed dimensions, as standalone TUs scored by chk.py (operator
    member/free, `float`/`const float&`, `x*f`/`f*x`, result landing, trig as `float`/`double`/`(float)` cast/
    `sinf` and order), found 30 in 240 compiles; a second grid over the trig values (`double` local / `float`
    local / written at each use, for sine and cosine separately) found three byte-exact forms in 204 compiles.
    Each grid took a few minutes on the laptop compile server. So: compile a proposed shape **exactly as given** first,
    then grid its dimensions, before writing it off; `tools/matching/vsearch.py --beam` searches combinations the
    same way. The grids are in `work/match/rotgrid/`.
  - **Gemini 4 Carbon fp_gem wave, 2026-11-10: rows that matched only within a float tolerance (levers from the
    transcripts, `work/opencode/levers_fp_gem.md`).**
    - *A combination, not a single edit (`emitter::update` 0x412150, 24 -> 5 aligned; 66 equivalence inputs
      bit-identical).* Typed iterator fields (`it->vel`, `it->pos`) for a raw `0x48`-offset `float*` walk, in-place
      helpers (`scale_v(vector4&, const float&)`, `add_to`), a `float& vz` reference for the z lane only, and
      `length_squared()` for the distance gave 13; `(double)noise_amount * *pdt` and `drag * (double)*pdt` then gave 5.
      Each piece alone measured worse (33-164). A `(double)` product stored straight to a float rounds once from an
      exact product, so it is value-neutral there.
    - *Per-lane association can differ inside one inlined transform (FxDef::skin 0x4291c0, 489 -> 451, -FP gone).*
      The x lane is `pos.x*M.s.x + (f.x*pos.y + u.x*pos.z)`, the y and z lanes `f.y*pos.y + (s.y*pos.x + u.y*pos.z)`:
      copy each lane's grouping from the instructions; don't assume the three lanes share one.
    - *A lane-asymmetric helper body (`compute_normals` 0x451c50, 800 -> 18, FAIL -> WEAK).* In the member helper
      `add_inl`, `x += o.x; y += o.y; z = z + o.z;` (z spelled differently) gave 58 -> 18; re-deriving
      `v70[v50[v90[i].a].index].a` in each edge expression instead of a named `p0` gave a further 40 bytes.
    - *Unnamed temporaries keep a dot product's lanes on the stack (MapPanel 0x51a2d0, 178 -> MATCH).*
      `V::dot(pt - *a, n) / V::dot(*b - *a, n)` with no `d`/`e` locals gave 18; the rest was the result built in
      place in `out`, which the original returns in eax: a by-value `vector4` return (11.3). The committed spelling
      is an explicit `out->vector4::vector4(...)`; the clean form is the by-value signature.
    - *Operand order of a quaternion sum, and unnamed temporaries (fx_anim 0x422130, 1437 -> 838, FAIL -> PASS).*
      `quaternion().set(...) + quaternion().set(...)` instead of named `qa`/`qb`, and `q = t2 + t1`. The frame stays
      0x10 short because `FxAnim::chain` (0x423b60) is an out-parameter in `fx_anim.h` but returns a `quaternion` by
      value in the original: fix the header before the next attempt.
    - *A `double` average scale and one object for three rotated vectors (light_mesh 0x47b7b0, 2855 -> 323).*
      `double k = (col.x + col.y + col.z) * 0.33f; c.x *= k; ...` gave 1238 -> 826; keeping the three rotated
      vectors in one struct object gave 323. Equivalence coverage there is only 1%, so unverified in values.
  - **Taking a re-read out can move registers elsewhere (render_scene's bone cache):** every form without the re-read of `it->m->f224` after the cache store moves `this` and `it` to other registers and stack slots, and costs about 150 instructions.

### 9.3 `sqrt`

VC7.1 has no `/fp:` switch. `sqrt` becomes a `_CIsqrt` call; an original with an
inline `fsqrt` is a difference this guide has no confirmed source lever for.

### 9.4 Constants and rounding points

- **Read constants from the binary.** M&B: the verifier checks the *value* behind
  each `__real@` / `__int64@` literal operand against the original. `fcom [eps]`
  matched on the pilot with `eps = 1e-10` where the original compares with 0.0.
  Another function's last byte revealed a far plane of `501.0f`, not 500.
- **Store-and-reload of one value** (original: `fstp [esp]` right after a divide,
  then `fmul [esp]`): `volatile float v = expr;` at the **first** computation
  only. `volatile` on every access is worse (extra loads the original lacks). M&B
  `screen_to_world`: identical instructions 43 -> 51, extra instructions 34 -> 7.
- **x87 spills to float memory** that the original performs can be reproduced
  with `volatile float` temporaries. This is fine in equivalent code and never
  claims byte-exactness. A mismatch on denormal inputs in the equivalence
  harness usually means a missing float rounding point.
- **Two halves of a 2-D value evaluated together** (`fld a; fsub b; fld c;
  fsub d; fxch; fdiv`): one aggregate initializer, `struct V2 { float x, y; } d =
  { a - b, c - e };`, then the divides.
- **Round to nearest through `floor`**: `float fl = (float)floor(v); int i =
  (int)fl; if (!(v - fl <= 0.5f)) i++;` gives the `fst / call floor / fld / call
  _ftol2` pattern; pass the limit and delta as `const float&` so `v` is not
  spilled first. `if (frac <= 0.5f) n = base; else n = base + 1;` gave the `jnp`
  form.
- **Clamps**: write `E < 0 ? 0 : (E >= 1 ? 1 : E)` as a repeated expression (a
  macro), not through a named float.
- **Vector algebra**: cross-product operand order is semantic (`f x s` versus
  `s x f` flipped signs in `psys::emit`); build products through the class's
  `operator*` temporaries when the original keeps a temporary structure.
- A function that leaves a value on the x87 stack (or in `eax`) at `ret` returns
  it, even if no caller appears to use it (M&B pilot: `vector4::normalize`
  returns the old length). An x87 return must be declared `double` to come back
  in `st0`.

### 9.4a Operation order cl rewrites (and how to find it)

cl 13.10 at `/O2` changes floating-point arithmetic in ways that change results.
In a big function, register allocation hides these among a lot of noise. So compare
only the x87 arithmetic.

- **Method.** For each unit (a function, or each case of a switch), take the
  sequence of x87 arithmetic instructions: `fadd/fsub(r)/fmul/fdiv(r)`, `fsin`,
  `fcos`, `fptan`, `fpatan`, `fsqrt`, `fchs`, `fabs`, `fscale`, `frndint`,
  `fyl2x`. Leave out loads, stores, `fxch` and register numbers. Give every
  memory operand that is a constant its value: the `__real@<hex>` name on our
  side, the bytes at the address in the image on the original's side. Then
  compare the two sequences. Any difference changes what is computed.
  `work/match/scratch_fpscan.py` (a scratch script, gitignored) does this for `Operation::execute`'s cases:
  before the fixes below, 5–9 of its 66 cases with x87 arithmetic differed;
  after them, 0. Register allocation can't move this metric, so it isn't
  goodharted by allocator noise.
- **Reassociation: `x * k / m` becomes `x / m * k`.** With a constant `k`, cl
  divides first. The original's `fmul [k]` before `fdiv st(1)` needs explicit
  parentheses: `((float)p * 0.017453292f) / m`. cl keeps the parenthesised order.
  (M&B `store_sin`/`store_cos`/`store_tan`.)
- **Parentheses around a whole float expression move a neighbouring instruction.**
  Each parenthesised float subexpression is an explicit tuple in c2's IL (OPPAREN).
  `float t = ((a * k) / v);` has one more of them after the `fdiv` than
  `float t = (a * k) / v;`, which makes the float result's chain one
  pseudo-instruction longer, and the scheduler then places an unrelated integer load
  differently. `MissionBlocker::update_camera` 0x4f8c80: with the outer pair, the
  `mov eax,[num]` of the next statement lands after the int64 hi/lo store pair as in
  the original, instead of between the two stores; that was the function's last
  difference. Parentheses around the inner pieces (`(float)X`, the constant, the
  divisor) changed nothing. Read off `c2trace.py il --at @Schedule@8`.
- **Reciprocal: several divides by one float become one `1/m` and multiplies.**
  Any `float m = …` local that divides two or more values is turned into
  `fld 1.0; fdivr` / `fdiv [m]` followed by `fmul`s, which rounds differently. It
  happens to separate statements, comma initialisers and arguments alike (probed
  5 spellings). The original's three `fdiv st(1)` come from dividing by the
  **integer global** at each use, `(float)p / g_fixed_point_multiplier`: cl
  converts it once (`fild`) and divides by it. To keep a float divisor in memory
  (original: `fild [g]; fstp [t]; fdiv [t]; fdiv [t]; fmul [t]`), write the
  conversion at every use, `(float)p / (float)g_fixed_point_multiplier`: cl
  merges the conversions into one stored temporary after the reciprocal pass
  runs. (M&B tableau light/camera cases, `store_pow`.)
- **`pow`/`atan2` with float arguments pick the float overload.** In C++, VC7.1's
  `<math.h>` has `float pow(float, float)`. Our `pow((float)a / m, …)` called it
  (`?pow@@YAMMM@Z`), while the original calls the CRT intrinsic `_CIpow`, which
  takes its arguments on the x87 stack in double precision. Cast the arguments
  to `double`. A `call` with no pushed arguments, right after two values are
  left on the x87 stack, is the sign of an intrinsic (`_CIpow`, `_CIatan2`, …).
  **Check which intrinsic it is:** M&B `emitter::update` 0x412150 called
  `pow(t, 4000.0)` where the original calls `_CIfmod` 0x536a6a (0x41223a). The
  relocation is masked in a byte diff, so only the equivalence harness saw it.
- **cl evaluates a written three-term sum in reverse.** VC7.1 emits the source
  text `n[2] + n[1] + n[0]` as `n[0] + n[1] + n[2]`. To get the original's
  descending order (0x47156f), write it ascending (`terrain_b`
  `generate_heights` 0x4713d0: a cell weight was off by 1 ulp). Read the order
  from the disassembly and check the compiled order; don't trust the source text.
  Dot products are the common case: `collide_entity` 0x427000 and
  `pick_diagonals` 0x4666b0 add the x term last (`a.y*b.y + a.z*b.z + a.x*b.x`, or
  the `z, y, x` order). A wrong order turned an exact 0 into a value > 0 and
  flipped a branch.
- **An argument slot of a "dead" neighbour gets reused.** When the original
  passes `&x` and the callee also reads the `y` stored beside it (`p[0]`, `p[1]`),
  cl may consider `y` dead after its last named use and reuse its home for
  another value. That happened even with `volatile y`, with a `float* volatile`
  pointer to y, and with a two-float temporary. The callee then reads garbage
  (`terrain_b` `blend` 0x471040, ground_height 0x470f40). Pass a real pair,
  `float p[2] = { x, y }; f(p);`. That costs two stores, but the callee gets the
  same values.

- **A clamp the original keeps on the x87 stack is a ternary** (M&B `Agent::hit_damage` 0x453710):
  `float c = t < 0.1f ? 0.1f : (t >= 1.2f ? 1.2f : t);` gives `fld [t]; fcomp; jp; fld const; jmp` with the result left in
  st(0); `if (t < 0.1f) t = 0.1f; else if ...` on an address-taken `t` stores each constant to memory. A clamp computed in its
  own scalar (`float k = h * 3.03f; if (k < 0) k = 0; ...; t = k;`) also stays in registers.
- **`v.length()` and the written-out sum are not the same order.** `sqrt(r.z*r.z + r.y*r.y + r.x*r.x)` came out x-first while
  `r.length()` (x*x+y*y+z*z) gave the original's `dz^2 + dy^2 + dx^2`; a dot product needed `x + (y + z)` to come out z, y, x.
  When three-term sums come out reversed, try the other association before reordering terms.

### 9.5 Reading float mismatches

Compare **aligned instructions**, not differing-byte counts (section 16), and
grade float-only differences with a tolerance rather than by eye. M&B: on a
mismatch `equiv.py --fp-tol` re-runs the original at 64-bit precision (control
word `0x037F`) as the reference. Three failures that had been "justified" by
reading the mismatches turned out to be real bugs under that mode (a code
address written where the original writes a float, a skipped `normalize` call,
seven values left on the x87 stack at return).

### 9.6 Reading `fcomp; fnstsw ax; test ah,M; jcc`

Byte-matching note (M&B `rigid_base::collide_shape` 0x427900): write `if (!(vn >=
0.0f))` and *repeat the expression* inside the branch instead of reusing `vn`.
cl then merges the two into the original's `fld st(0)` + `fcomp [0]` + `test
ah,1`; reusing the named float gives a plain `fcom` with no copy. Write the dot
product's last term commuted (`... + v.z*nrm.z`) to get `fld nrm.z; fmul v.z`
first.

`fcom` sets C0 (`0x01`) when st0 < operand, C3 (`0x40`) when they are equal, and
C0, C2 (`0x04`) and C3 together when either is NaN. The mask and the jump say
which C condition the **fall-through** path has. Read it from the table; don't
guess, because two FAILs on 2026-10-29 were exactly this:

| Sequence | Falls through when | Source |
|---|---|---|
| `test ah,0x41; jne L` | st0 > op (ordered) | `if (a > b) { fall-through } else { L }`; a NaN goes to `L` |
| `test ah,0x41; je L` | st0 <= op, or NaN | `if (!(a > b)) { fall-through }` |
| `test ah,5; jp L` | st0 < op (ordered) | `if (a < b) { fall-through }`; `>=` and NaN go to `L` |
| `test ah,0x44; jp L` | st0 == op | `if (a == b) { fall-through }`; not-equal and NaN go to `L` |
| `test ah,1; jne L` | st0 >= op (ordered) | `if (a >= b) { fall-through }`; `<` and NaN go to `L` |
| `test ah,1; je L` | st0 < op, or NaN | `if (!(a >= b)) { fall-through }` (dynamic_entity 0x42727e) |

- M&B `FieldPanel::update` 0x48d528 (`fld scene.h; fcomp this.h; test ah,5; jp`):
  ours wrote `==`, while the original's branch is `scene.h < this.h`.
- M&B `music_player::advance_piece` 0x4c2884 (`test ah,0x41; jne`): the original
  draws `rand()` only for an ordered sum > 0. Ours had `if (sum <= 0) best; else
  draw`, which sends a NaN sum to `rand()`. The fix is `if (sum > 0) draw; else
  best`. The two spellings are equal for ordinary numbers and differ only on NaN,
  which typed and soup inputs do reach.

---

## 10. Instruction selection and intrinsics

Source types and comparison spellings go literally into instructions.

- **Bit-test idioms are canonicalised.** `(f & 0x100) >> 8`, `!= 0`, `?1:0` and
  `/256` all become `shr; and 1` for unsigned or int flags. The original's `and
  0x100; shr 8` came only from an `int16` flags value with `(flags & 0x100) !=
  0`.
- **`return k == 0xd;`** emits `xor eax,eax` first; `if (k == 0xd) return true;
  return false;` emits `setb`.
- **Casts.** `(unsigned short)x` gives `movzx`, `& 0xffff` gives `and`. M&B: a
  division `(unsigned)((char*)end - (char*)begin) / 0x18` is an unsigned
  `mul`/`shr`; the original's `imul`/`sar`/sign-fix sequence is a *signed*
  division whose result is then compared unsigned, so cast the result
  (`(unsigned)(diff / 0x18)`), not the numerator (`s004e65e0`).
- **A mask held in a local** (`uint32_t mask = 0x80;`) gives `mov eax,0x80; test
  byte ptr [obj],al; jne; or [obj],eax`; an inline literal loads the whole dword
  and tests `al`.
- **`_alldiv`/`_allmul` must be declared `__stdcall`** (callee pops 16 bytes);
  `__cdecl` adds a stray `add esp,0x10`.
- **`#pragma intrinsic(strlen, strcmp)`** selects the `lea esi,[eax+1]; mov dl,
  [eax]; inc eax; test dl,dl` idiom and the pair-unrolled `sbb/sbb` compare; a
  hand loop gives `movzx`. `wcslen()` as an intrinsic is `lea edx,[eax+2]; mov
  cx,[eax]; add eax,2; test cx,cx; jne`; a hand loop is `cmp [eax],0; add eax,2`.
  An unrolled two-character compare ending in `sbb eax,eax; sbb eax,-1` is the
  `wcscmp` intrinsic (`#pragma intrinsic(wcscmp)` with `<wchar.h>`).
- **`strcpy` of a literal** is copied as dword loads from literal+0, +4, +8, ...;
  the relocation addend is part of the code bytes (M&B `DisplayErrorMsg`,
  `0x4014f0`, `strcpy(buf, "Not enough memory.")`).
- **Byte swaps** use `_byteswap_ushort`/`_byteswap_ulong`; a shift/or swap
  becomes `rol`, a memory swap spills.
- **`_ReadWriteBarrier()`** is a compiler fence that emits no code. The scheduler
  hoists a load above an earlier store (or sinks a store) when it sees no
  dependence; the barrier pins the order. Place it between a scheduling-critical
  store and the load or store it must precede. cl 13.10 has no `<intrin.h>`, so
  declare it `extern "C" void _ReadWriteBarrier(void);`. M&B: `0x488560`
  (`MissionUi::set_a`, src/game/ui/mission_ui.cpp) is byte-exact with `periodA = a; _ReadWriteBarrier();
  t0 = g_ticks[clock0];`. It also pinned a load after earlier stores in
  Spore's cl 13.10-family code (15 diff bytes to 0). When the compiler still CSEs
  the reload, read the field through a `volatile` access at that one site, and
  keep `volatile` local: a `volatile` access anywhere disables stack-slot
  sharing for the whole function.
- **Atomics are semantics, not shape.** Where the original has `lock xadd`, `lock
  cmpxchg`, `lock inc/dec` or `xchg mem,reg`, write the interlocked operation,
  never `++*(volatile long*)p`. A single-threaded equivalence run passes the
  plain version and the game then races.

---

## 11. Calls and ABI

### 11.1 Read the calling convention from the caller first

- `push`es then `call` with no `add esp` afterwards: the callee pops
  (`__stdcall`, or `__thiscall` if `ecx` was set). `add esp,N` after the call:
  `__cdecl`. A `ret N` in the target fixes its own argument count exactly.
- `mov/lea ecx, ...` right before a call: thiscall member.
- A function that uses `ecx` as `this` must be a real member function. A free
  function taking an explicit `self` pushes the object and gets the wrong
  cleanup. The byte diff barely notices a wrong declaration; the most common
  equivalence defect is a thiscall declared as a free `cdecl`/`stdcall`.
- **M&B: a free function declared to stand in for a `__thiscall` callee compiles
  as `__cdecl`**: the caller pushes `this` and the callee faults at its first
  `[eax+0x1a8]`. A stub *method* on the owner class is the only way to get
  `ecx = this` for a callee that looks like a free function (`s004885e0`).
- For an absolute-address callee, declare it as a member of a declaration-only
  placeholder struct (`struct TC_x { void f(int); };`): that gives `call rel32`
  with `this` in `ecx`. A function-pointer cast to the address gives `mov
  eax,addr; call eax`, five bytes longer.
- A tail call `mov ecx,esi; jmp f` needs `f` to be `__thiscall`; a `__cdecl`
  callee forces push, call, add and ret.
- A `static` member defaults to caller-pops; `static void __stdcall f(...)` gives
  `ret 0xc`.
- The CRT EH helpers (`_eh_vector_constructor_iterator` and the like) are
  `__stdcall` (`YG` in the mangled name). Better still, declare a local array of a
  class with a constructor and destructor (`RDesc8 desc[4];`) and the compiler
  emits both iterator calls and the EH frame itself.
- A 12-byte struct passed by value through a vtable slot must be a by-value
  aggregate parameter, which gives one `sub esp,0xc` block; three scalars give
  three `push`es. Conversely, a "struct" the original pushes as separate dwords
  must be declared as separate dword parameters. Count the pushes.
- Argument order is visible only in the push order. Two functions can take the
  same operands in opposite order: M&B `MsgLog` methods at `0x452c00` and
  `0x4334b0` are `add(int, tstring)` and `add(tstring*, int)`. Declare the
  overload to match the original's push order, not the plausible C++ one.

### 11.2 `__thiscall` cannot be named in a cast or on a free function

cl 13.10 rejects `__thiscall` on free functions (C3865) and as a function-pointer
attribute (C4234). Model each direct callee as a stub-class member. With one
argument, `__fastcall` loads only `ecx` and is identical for `__vec_ctor` /
`__vec_dtor`-style loops (M&B `s00401010`).

### 11.3 Hidden return pointers and `void` hiding `eax`

Struct-by-value returns use a hidden destination pointer (the first stack
argument, returned in `eax`). The body is byte-identical to an explicit
out-pointer contract, so the callee alone cannot tell them apart; the caller and
`eax` differ. M&B: a `ret` that writes through `[esp+4]` is either a by-value
struct return or an explicit out-parameter. Sequential `out.x = ...` stores are
an out-parameter; `return vector4(a, b, c)` evaluates the constructor arguments
right to left (z first).

A function declared `void` can still hand a value back in `eax`, and the byte
diff cannot see it. **Declare the return the original has.** The original returns
a value in `eax` when, for every `ret`, walking back to the last `eax` writer
stops before any `call`, and the write survives to the `ret` or is one of these
shapes:

- a hidden return pointer handed back (`mov eax,[esp+...]` with a store
  `mov [eax+...],...` in between);
- `return this` (`mov eax, ecx`);
- a call-free function whose first `eax` write is a load or `mov eax, ecx`, with
  nothing rewriting `eax` before the `ret` (a pop-front, a `lower_bound`, a copy
  returning its end pointer).

Refinements:

- A thiscall **constructor** hands `this` back in `eax` on both sides, so a `void`
  declaration there is not a defect.
- A **`void` wrapper returns its last callee's `eax`.** If the original makes a
  call and does not write `eax` after it, the caller sees that callee's `eax`;
  ours must also not write `eax` after its last call.
- A `uint8_t` return must stay `uint8_t` (`mov al,[ecx+0xc0]; shr al,3`; a
  `uint32_t` return gives `movzx eax`).
- Scalar deleting destructors return `this` in `eax`.
- A function can return a pair in `eax:edx`.

`tools/difftest/regfacts.py` implements this detection (`returns_eax`) and
`tools/difftest/equiv.py` reports `void_hides_eax`. Observed in Spore: a
pop-front leaf declared `void` returned the popped node in `eax`; its caller used
the value and the program failed at runtime although the function passed
synthetic inputs.

### 11.4 `ret N` is part of the contract

The immediate on `ret N` is the argument-cleanup amount: thiscall and stdcall
callees pop their own arguments, cdecl callers do. A `ret 4` original declared
`ret` (or the reverse) leaves the caller's `esp` off on every call. A function
extent can contain two `ret` forms (`ret 12` and `ret 20`) when neighbours were
merged by discovery; split the extent instead of treating it as one ABI.

### 11.5 Return types

- A `bool` return gives `mov al,1; ret`.
- `bool found = x != 0; return found;` gives `test; setne al`. Returning the
  comparison directly gives the 32-bit `neg; sbb; neg` normalize.

### 11.6 Virtual calls

- Virtual calls need a stub class with the slot at the right index; dump the
  vtable from the binary first.
- **Keep a virtual call a single expression.** The original's `mov eax,[this];
  mov edx,[eax+N]; mov ecx,this; call edx` is one expression's worth of IL. A
  typed call `((Iface*)self)->Method(arg)` holds the callee in `edx` and matches
  byte-for-byte; the type need not be the real domain class, because codegen
  depends only on single-expression and the slot index. Naming the pointer first
  (`fn = ...; fn(self);`) moves the callee to `eax`, and a helper that returns
  the function pointer spills it. For a call on a known `this`, the first load
  collapses. Observed in Spore's cl 13.10-family code; cl 13.10 cannot cast to
  `__thiscall`, so use the typed call on a placeholder class.
- `(*(void***)obj)[n]` emits two loads, and going through a `vftable` member
  emits three.
- A declared-symbol direct call (`extern "C" void f();`) keeps the `call rel32`
  shape; holding the address in a function pointer gives `mov reg,imm; call reg`,
  five bytes longer.

### 11.7 The rest

- Right-to-left argument evaluation; caller cleanup coalescing (a fixed-argument
  prototype versus variadic; C coalesces more aggressively than C++); varargs
  members fall back to `cdecl` with `this` pushed; `new T` emits an out-of-line
  allocation-failure block; compiler-generated scalar-deleting destructors;
  unreachable epilogues after imported `longjmp`/`exit`/`_endthread` (the
  compiler does not treat them as noreturn).
- `printf`-style members: `this`, x, y, fmt are all pushed; plain `ret`.
- Inline `__asm` in an atomic helper blocks inlining, so an original that was
  inlined did not use it.

---

## 12. Inlining and out-of-line calls

- `__declspec(noinline)` is accepted by cl 13.10 and keeps a call the compiler
  would inline. A same-TU function called before its definition is still inlined
  by `/O2`; mark the definition `noinline` when the original calls it out of line.
  M&B `InventoryPanel::transfer_slot` 0x512c80: the original calls
  `test_slot_flag` 0x5126e0 and `or_slot_flag` 0x513320 as real calls (0x512e07,
  0x512e42), but our cl inlined both at the first site. The source logic was right,
  yet the equivalence call trace went out of step (FAIL) until the declarations in
  the header got `__declspec(noinline)`. When a FAIL's first difference is "call #N:
  orig f, ours g" and `g` is the *next* call the original makes, look for an
  inlined callee before looking for a logic bug.
- `__forceinline` on a value-type copy constructor keeps it from being emitted
  out of line as `push; call ctor`.
- A call to an **empty function in the same file is dropped** even when it is
  `noinline`. Give the stub a `volatile` store to keep the call.
- A helper member defined as an empty stub `{}` in the same TU gets inlined and
  the call the original has is lost. Declare it (and any auto-refcount
  `AddRef`/`Release`) **without a body** to force the out-of-line `call`. A
  same-TU helper that receives a local's address should be given its real body
  as a `noinline` specialization so the compiler knows it does not write the
  object.
- **The compiler elides `mLayout(Tmp())` copies.** Use a distinct temporary type
  with its own constructor and destructor, and put the member in a base class so
  the derived vptr store comes after the member copy.
- **Inliner threshold (M&B, experiments only).** The back end (`c2.dll`) accepts
  `/d2<opt>`, passed as `-<opt>`. Its option table has inliner knobs whose 0
  default means "built-in value": `inlT` 74 (threshold), `inlS` 9, `inlfcsw` 30,
  `inlocsa1` 45, `inlocsa4` 45, `inluserinl` 7, `inlnobr` 10, plus `inlld`,
  `inld`, `inlniln`, `inlnild`, `inlnoln`, `inlnold`, `inlcrmax`, `inlfcsa`,
  `isize` (4). Use them to **measure**: the smallest `/d2inlT<N>` at which a call
  is inlined is that callee's cost at that site. `??_Eframe_set` (`0x41aee0`)
  matches byte for byte with `/d2inlT88` and above (the cost of `??_EBone`'s
  array path is 87), but a global `inlT88` drops `resmgr.cpp` from 62 to 54
  matches, so the original was not built with another threshold: our source
  presents `??_EBone` (or the call site) as costlier than the original's did.
  Replacing `Bone`'s `std::vector` with a minimal struct did not change the cost.
  The "INL:" trace strings print only in a PGO mode that a normal compile does
  not reach.
- **Library-style inline helpers decide shape.** M&B: the original reached `f`
  through a pointer in `matrix::orthonormalize` because it called the
  out-parameter `cross(r, a, b)` inline.

---

## 13. Exception frames, constructors, destructors

### 13.1 `/EHsc` only where the original has frames

- M&B: with `/EHsc`, a local with a destructor plus `new` emits a `push -1 /
  fs:[0]` frame the original lacks. The default for a region is no `/EHsc`; add it
  only where the original has `__ehhandler$` stubs and `fs:[0]` prologues
  (`s00404140`).
- M&B: `/EHsc` also changes `delete[] p` for a class with a destructor: the
  compiler calls the `__ehvec_dtor` helper instead of the inline loop with the
  destructor body inlined, so `??_E` vector-deleting destructors only match
  **without** `/EHsc` (the whole 25-function TU matched without it,
  `s00401010`).
- A non-trivial destructor on a local is what buys the original's `_ehhandler`
  prologue. Hand-written cleanup drops the frame. `__try/__finally` gives a full
  `ebp` frame instead; do not use it.
- A function with C linkage loses the EH cleanup of a placement `new`; keep such a
  function C++.
- `throw()` on a dealloc helper removes the extra EH state store before an
  inlined vector destructor. Declaring a constructing function `extern "C"` omits
  the constructor unwind graph that C++ linkage keeps.
- A six-argument placement `new` gets its EH state stores only when a matching
  placement `operator delete` is declared.
- A class-level `operator new` reproduces a null-checked allocation.

### 13.2 Generated functions

- Compiler-generated `??_E`/`??_G` deleting destructors have no source line to
  mark. M&B: write `void f(T* p){ delete[] p; }` (or a class with `virtual ~T()`
  and a ctor); `chk.py` reports the generated symbol, and a manifest row names
  `??_ERec34@@QAEPAXI@Z`, not the wrapper (`s00401010`).
- `delete f()` and `delete p` produce the inlined scalar-deleting-destructor
  block.
- A destructor that is declared virtual but never defined, with a ctor inline in
  the base, gives the vptr-last store order (section 5.3).
- Compiler-generated `??_U`/`??_V` (`operator new[]` / `delete[]`) appear only
  when `<new>` is included; without it, `new T[n]` and `delete[] p` call plain
  `operator new`/`delete` with identical bytes. M&B: every slice that uses array
  new/delete needs `<new>` (resaudit finding: `new` where the original has
  `new[]` at `0x535482`, `delete` versus `delete[]` at `0x53570d`).

### 13.3 Lifetime

- **A temporary lives to the end of its full expression.** When the original
  destroys a temporary *before* a later call in the same statement, split the
  statement and end the temporary's scope first. M&B `SaveWindow::init`
  0x4e4790: the original destroys the `"load_window"` tstring (0x42bc40 at
  0x4e484f) before `window->d31(0, clone)` at 0x4e485e. Ours wrote
  `d31(0, res_get(tstring("load_window"))->clone())`, so the destructor ran after
  the call. The fix is `{ tstring n("load_window"); m = res_get(n)->clone(); }
  d31(0, m);`. In the equivalence harness these show up as malloc/free or
  ctor/dtor order differences in the call trace; `gui_edit` 0x4c8ca0 had eight such
  sites.
- For a "dead" vptr store the original keeps, make just that store volatile:
  `*(void* volatile*)&mpVtbl = ...`.
- An EASTL-style bool tag passed by value is a `struct { bool b; }` local set to
  false: the compiler stores a 0 byte into a dead parameter slot and pushes the
  dword. Observed in Spore.

---

## 14. Data, sections and linking

### 14.1 The linker folds identical bodies (`/OPT:ICF`)

Identical bodies (no-op constructors, identical destructors, empty functions) fold
to one address, and the caller's prototype still decides its stack cleanup. M&B:
one VA can stand for several source functions (vtable `0x54a4b8` uses `0x408030`
for two slots). Define the one function that lives at the VA; declare the others
with the same `// 0x...` address. Emit both natural bodies and let the linker
fold.

### 14.2 Static initializers and globals

- Function-local statics share a guard with one bit per object in declaration
  order; each object with a destructor gets a COFF-local `$E<n>` thunk pushed to
  `atexit`. Introducing a new local-static scope shifts `$E` numbering.
- A global array of a class with a destructor emits a reverse array-destruction
  callback `_$E2` calling `??_M` with the stride; placement-constructing over
  static storage removes the null check and reproduces the initializer.

### 14.3 Translation-unit neighbours are mostly neutral

Symbol ids and most codegen are per function, so compiling address neighbours or
shared classes in one TU is normally byte-neutral. Exceptions: the preceding
function can decide tail cloning/merging (**the block mover sizes tails with the
previous function's frame flags**), and compiler-local names (`$S`/`$E`, `$L`/`$T`)
renumber with TU contents.

### 14.4 Atomics

See section 10.

### 14.5 String literals

The compiler pools identical literals; `/O2` emits diagnostic literals as separate
pooled COMDAT objects in address order. Use an `extern const wchar_t[]` (not a
literal) when you need the original's `strlen` loop plus `rep movsd/movsb`
instead of unrolled `movs`.

---

## 14A. Near misses: the source had more structure than ours

Found on 2026-11-05 by retrying rows 3-10 instructions from a match. Varying statements inside the flattened shape
matched 1 of 18; restoring the structure below matched 8 of 17, usually within two compiles. When a row is one address
computation, one branch or one frame slot away, look here first.

- **A small inline helper was written out.** Inlining changes constant folding and block layout. `(idx + 0xb0c) << 4`
  stopped folding once the access went through an inline `ring_add(Ring* r, ...)` indexing from the ring's own pointer
  (0x4334b0). `if (n == 0) mb_error(...); int idx = rand_below(n);` with an inline `rand_below` gave the two separate
  zero blocks (0x514360).
- **`else if (a || b) { X }` may have been two identical arms.** `PartyScreen::setup_party` 0x504ef0 matches with
  `else if (w04 == 3) { X } else if (w04 == 2) { X }` where we had `w04 == 3 || w04 == 2`, and that one change fixed
  two things. Block layout: with `||` the first compare branches straight to the shared arm, the layout's depth-first
  walk visits the arm first and it lands after the else arm; with two arms it stays in source order. Frame slots: the
  packer orders slots by `1000 * uses / size`, larger first (traced on 13.10 at StkColorCmp: an int with 5 uses, key
  1250, against a 16-byte tstring with 18, key 1125), and it counts BOTH copies of the arm (24 uses, key 1500) before
  the late flow passes merge them into the one copy the original shows. The merge also leaves a hoisted `lea` of the
  shared object as a trace. A bool local or an inline helper for the condition fixes the layout but not the slots.
- **A store to a local ordered against a load through a pointer: where was the pointer derived?** The scheduler keeps
  a store before a load only if there is a memory edge between them, and the edge exists only if the local is in the
  pointer's points-at set. A pointer whose value was defined outside the local's scope does not get the local in its
  set. `render_batch::render_scene` 0x46e590: `fog_col.w = 1.0f` comes before `mov edx,[esi+0x18]` (`sky->fog_start`)
  in the original; with the outer `sky` ours had it after. Re-deriving the pointer inside each arm
  (`sky_t* s = scene_sky(sc);`, CSE'd into the same register, no instructions added) gives the original's order at
  both sites. `volatile`, or writing the field through a pointer or reference, changes nothing.
- **A dead zero store at function entry that nothing reads is a conditional temporary's flag.** When a class temporary
  is built in one arm of `&&`, `||` or `?:`, c1xx adds a flag (`$T`) recording whether it was constructed and zeroes it
  at entry. If c2 later proves the arm unreachable it removes the arm and every test of the flag but keeps the entry
  store; the flag, never read, shares a slot with later locals. No user variable reproduces it: a zero-initialised
  local either loses the store or takes a private slot. `PartyScreen::init_screen` 0x502cc0 (`xor ebx,ebx;
  mov [esp+0x10],ebx` before `g_gui.reset()`) matches with such a statement; `Recf0::init_screen` 0x4e05c0 shows the
  same store for its `tstring("16 bits")` under `&&`. The condition must be one only c2 can fold (a member just
  stored, an inline returning a constant); one c1xx folds itself (`0 && ...`) leaves no flag. Which statement the
  original had is not recoverable from the bytes, so ours is a labelled stand-in.
- **A reused named local was really an argument temporary.** One frame slot holding different values, with stores
  delayed until after the pushes, means `f(T(...))` through a const reference, including a literal to `const int&`
  (0x4995e0, 0x4996e0).
- **Wrong scope.** Declare locals per arm when immediate stores land after a hoisted push (0x49abc0). A destructor
  that runs mid-function means a named temporary in its own block: `{ tstring t("x"); path.append(t); }` (0x4c3350).
- **A member destructor was written by hand.** A free that runs after an array member's destructors belongs in the
  member's own destructor (`VarVec`, 0x4cb9e0).
- **A store through a reference is an alias barrier.** `int& h = hword;` keeps a later load from being scheduled above
  the store (0x482170).
- **STL spelling matters.** `dq[dq.size() - 1]` and `dq.back()` compile differently (0x4ebb80).
- **`mov ecx, ...` before a call to a free function means a member call.** Fix the declaration (0x4ebb80, 0x4406a0).
- **Struct copies.** A copy through a typed pointer (`*d = *(const vector4*)c`) can match where four scalar copies
  cannot (0x48f5a0). `lea r, [r]` padding inside a loop means a real nested `for`, not `for(;;)` with gotos (0x4d6f90).
- **Base pointer plus index.** `Troop* t = &troops[sel]; t->inv[i].item = -1` instead of a precomputed field pointer
  stops cl folding the offset into one `lea` (0x5168b0).

Added by the second round (50 of 106 rows matched):

- **Small inline helpers fence store scheduling.** Flat zero or timer stores let cl sink a base constructor's stores and
  the vptr store below them. Helpers taking a struct reference fixed a family: zeroing a vector triple (0x4cee50,
  0x4ceb30, 0x4cef20, 0x4cef80, 0x4fd150, 0x4ce870, 0x4ce8d0) and starting a timer as `clock = i; start = g_ticks[i]`
  (0x4c2d10, 0x4897a0). An `int clock` followed by `__int64 stamp` is a Timer.
- **A callee can use the argument register directly.** `static __declspec(noinline) char f(const int& value)` reading
  `value` itself keeps it in ecx, with edx and eax as scratch. A local `int v = value` moves it to eax and rotates the
  scratch registers. This matched 13 validators in the script interpreter (0x49cd00..0x49d4b0), so scratch rotation is
  not always unfixable.
- **`extern float g_one` against the literal `1.0f`.** The extern may alias, so `fld` can't move above an int store; the
  literal allows it (0x4ed6f0).
- **Float floor.** `float fl = (float)floor(v)` gives `fcomp dword`; `double fl` gives `fcomp qword` (0x4f25a0).
- **Real STL calls.** `vector::operator[]` rather than `&_Myfirst[i]` (0x51e0b0), a three-argument `_Destroy_range`
  rather than an explicit tag (0x4d3e50), `worlds.pop_back()` (0x492ee0).
- **Empty tag arguments.** An argument pushed from an unwritten slot is an empty tag struct by value. Use an uninitialised
  local tag; `tag()` emits `mov byte [esp+N],0` and a frame (0x4f6360, 0x4ebdc0, 0x50d230).
- **Conditional shape picks the block layout.** `n <= 1 ? 0 : rand() % n` against `n > 1 ? rand() % n : 0` (0x48c4c0);
  an else-if chain against per-arm `return` (0x4d5290, 0x48e340); `if (v >= g_one) v = g_one; return v;` stops a tail
  merge (0x4b1fa0).
- **`shr; test r8,1` is a TU-local inline bit test.** `static inline bool bit(unsigned v, int n) { return ((v >> n) & 1) != 0; }`.
  Written inline or as a bit-field it folds to `test dl,2` (0x489e70).
- **A store before the vptr means an intermediate base** with an inline constructor (0x4931d0).
- **`mov [esp+4],N; jmp` in ours means cl saw the callee's body.** The original saw only a declaration, so this marks a TU
  boundary (0x4d5290).
- **Markers.** A helper between a `// @ 0x...` marker and its function steals the marker. Put helpers above it.

Added by the third round (59 matched, including two functions 109 and 144 instructions away):

- **A large distance is often a few structural errors.** Fix types first: real vectors with `operator[]`, `vector4` and
  `float2` locals, real arrays (a frame gap with no references marks an array only partly used, `__int64 params[16]` in
  0x4b1160), per-block temporaries (0x520c00, 0x4b1160). Re-measure before chasing instructions. These rewrites also found
  wrong field offsets and flags that no test had caught.
- **Materialised vector reference.** `lea r,[base+i*4]; mov r,[r]` or `add r,base; mov r,[r+off]` is `vector::operator[]`,
  `*(begin()+i)` or a `vector::iterator` local. Raw indexing folds the address (0x5169e0, 0x517960, 0x4c8000, 0x526130).
- **`inline` on a same-TU callee's definition.** When ours keeps a value in edx/ecx across a call to a function in the same
  file and the original saves it in esi, cl is using the callee's known register use. `inline` hides it (0x408420, 0x40a870).
- **Retained result.** `mov bl,1` plus `xor al,al; ... mov al,bl` is `char r=1; if (bad) {report; r=0;} return r;` (0x49d160).
- **Float pairs.** `mov [esp+N],0; mov r,[esp+N]` on one reused slot is a two-float temporary assigned whole (0x4644e0,
  0x4bd120). `struct V2 {x,y} d = {a-b, c-e};` keeps several x87 values live (0x4333c0).
- **Dead-parameter overlay.** A local whose slot is a parameter's home is a real local that cl overlaid on the dead
  parameter. Don't write through the parameter (0x468c80).
- **Hidden return.** A trailing `mov ecx,eax` on a "void" function, or eax being the result pointer at `ret`, means a hidden
  return pointer (0x466ed0, 0x4fd4f0).
- **Placement new against an explicit constructor call.** `new (p) T` adds `cmp reg,0; je` (0x4d3880).
- **Clamps.** `if (v<0) v=0; else if (v>cap) v=cap; return v;` matched where early returns and ternaries did not (0x520420,
  0x48bf90, 0x52cef0).
- **Missing `WL_MARKER`.** `jmp; push edi; dec esp; and [eax],ah` in the original's alignment means one is missing (0x4c7290).
- **A helper pulled in by a shared header can force a call.** `std_vec_out_of_line.h` makes every includer call 0x440b30
  where the original inlines `push_back` (0x51eda0).
- **Stores after the vptr but before a member's constructor are other members with inline constructors,** not a base. A real
  intermediate base leaves its own vptr store behind (0x4ce960). An empty constructor body can explain store order too (0x477a30).
- **`and r,M; xor r2,r2; or r,r2; jcc` is a test on a 64-bit field.** Decide per site: some sites really read a 32-bit word
  (`flags30` matched 0x513eb0, 0x491430 and 0x4957d0, and broke 0x460160, 0x4554c0 and 0x443870).
- **`mov eax,1 / xor eax,eax` where callers `test al,al` is `bool f() { return a || b; }`.** `if (...) return 1` gives `mov al,1` (0x48efa0).
- **A long `jcc` (`0f 8e`) where the original has a short one, with the same text, means the branch target is wrong.** Compare
  encoded sizes. An early `return` before a trailing `WL_MARKER` lands past the marker (0x4957d0).
- **`shr` against `sar` on a page index means an unsigned parameter to the inline accessor** (`atu`, not `at`, 0x513eb0).
- **An instruction scheduled after a guard can be a variable assigned inside the loop,** not initialised at its declaration (0x491430).
- **An explicit specialization of `_Insert_n` changes callers that expand `push_back` inline,** so those files can't include it (0x51eda0).
- **The number of separate locals decides the frame more than any statement does.** Merging each label's width and colour into one
  `{ int v[2]; ... }` took 0x4e05c0 from 2006 to 2147 identical instructions in one step. Separate per-call locals did the opposite on
  0x4dda40 and 0x4dcef0, which needed `{ int c = ...; }` per call. Try both.
- **A function-scope object gets a separate slot from earlier temporaries.** Putting it and the rest of the function in a nested block
  lets them share (0x4e05c0). A slot's size drives the frame: `float out[2]`, not vector4.
- **`and esp,-8` with no doubles means an 8-byte array local:** `int v[2]` used as a single int reproduces the prologue (0x4de6f0, 0x4e05c0).
- **Implicit temporaries order differently from explicit ones.** An inline helper with `const tstring&` parameters built from literals loads
  the arguments first, then constructs the temporaries; `lookup("key")` differs from `lookup(tstring("key"))`.
- **Switch cases must be written in the original's body order,** read from the jump table and literals (0x4e2ea0 matched once fixed).
- **Whole-value pair assignment reproduces store-then-reload through a stack slot:** `*(uv_pair*)&f = uv_pair(a, b)` (0x4dfec0).
- **Out-of-line callee bodies must be visible.** A local whose address only goes to an opaque callee keeps a private slot: making
  normalize and set_time visible took 0x4b80d0's frame from 0x90 to 0x78, and visible callees set Operation::execute's global register map.
- **Vector arithmetic through operators with temporaries reproduces the fstp/mov spill shape,** and a member reached through a reference or
  accessor gives the `edi=&pos` register shape (0x448eb0).
- **A biased IV displacement (`mov edx,0x14` with accesses at -4/0/+4) can be a small inner loop cl unrolled:** `for (j<3) *out++ = (&f.a)[j]`
  (0x46c920). An IV biased to a field outside any such loop comes from a materialised reference to that field (`const float& cy = at(x,y).py`, 0x469a70).
- **Const-reference inline helpers change allocation across a whole function:** `rand_n(const int&)` with an early return took 0x46bc70 from 238 to 324
  identical. A by-value `int` is the wrong shape.
- **`mov eax,[g]; dec eax` kept as a dword rather than narrowed:** bind the mask, `const unsigned& m = g - 1`.
- **Compare orientation is cheap to get right:** `sc > 0.0f` against `0.0f < sc` changes the x87 shape (0x468fc0 gained about 100 identical).
- **A memory accumulator in a dead parameter's slot, also passed by address, is a block-scoped address-taken local** (0x46d060).
- **Other small shapes.** `val <= mask ? val : mask` gives `jbe`; a right-nested sum `t + (a + b)` preserves fld order; by-value `fmin2(a, b)` helpers give
  `fst/fcomp st(1)`; `do { } while (++k < N)` keeps signed `%` and `/`, where `for` lets cl prove k non-negative (0x484590).
- **Static validators get esi or edi for a parameter when all three scratch registers conflict.** 0x49cda0 matched with sequential
  `if (type == N)` on the reference itself, plus `r = 0; goto done;` on errors. A local `int t = type` lets cl thread the jumps.
- **Tail-merge hubs need the same shape everywhere.** All `result = 0` tails hub to one block in the original only if other cases use
  `if (ok) result = 1; else result = 0;`. A stray `result = 0;` first arm moves the hub.
- **By-value `__forceinline` wrappers spill their arguments, and shim forwarders add inline formals that change allocation.** Calling
  the real signature or class fixed `map_get_*_position` and matched 0x52ae70.
- **`inline` on a visible callee hides its register use:** `inline __declspec(noinline)` keeps a register live across the call.
- **`insn_align`'s raw identical count can jump about 150 between equivalent layouts** (difflib noise): compare the count with slots masked.
- **`/FAs` frame symbols name every local and temp with its offset.** Grouping them by offset shows which variables to split or rescope. Reused
  floats were often separate variables: splitting by the original's slot sharing fixed whole frames (`divisor`/`inv`, `mv`/`inp`, 0x4556d0).
- **An address-taken local doesn't share its slot with variables nested in its scope; a sibling block does** (0x44e2b0).
- **Identical if/else arms leave dead loads:** `if (id == x) s = s*0.7f; else s = s*0.7f;` keeps only the two loads (0x456503).
- **A wrong field in a global can byte-match, because data addresses are masked.** `g_skill_tbl[36].cap` matched where the original reads `max_rank`,
  in several modules. Run tools/difftest/resaudit_all.py, and check any global you touch against the original.
- **Which calls a global survives shows which callee bodies were visible.** The original keeps g_mission in a register across first_used, next_used, get
  and Blow::init, and reloads it after is_enemy, note_blow and is_ready. So those first four were visible in the file but left out of line. Making them
  `inline __declspec(noinline)` and writing plain `g_mission->...` with no named locals matched 0x526130 and 0x526070. Not universal: making max_amount visible
  made 0x52b5c0 much worse.
- **Two arms each calling the same function get hoisted:** `if (x == 2) get()->f = 1; else get()->f = 2;` stops the setne if-conversion (0x5284a0).
- **A slot shared between a probe vector and later differences means one reused variable** (0x529ec0).
- **FP hoisting depends on where values are assigned, not on escape.** cl hoists `sqrt(x*x+y*y)` when `x` and `y` are copy-propagated invariants. Assigning a
  local inside the loop and normalising it in place (`vec2 u; u.x = (float)(...); u.y = ...; v2_normalize(u)`) leaves only the conversions hoisted (0x468fc0).
- **Stack stores crossing pointer loads come down to escape.** A local whose address only reaches a body cl can see doesn't escape, so its stores and loads can
  cross pointer accesses. Visible `tri_normal` and `triangle_corners` fixed 0x46ad50; visible `normalize` fixed 0x46d8c0's copies.
- **A reference to a container shifts use weights across the whole function:** `std::vector<light_t*>& ls = v->lights` gave 0x46d8c0 this=edi, v=ebp, taking it
  from 652 to 843 of 871. Register choice is decided by use counts, sometimes by one use.
- **Statement-form sums keep x87 order:** `s = x*x; s += z*z; s += y*y`, where a single expression gets reordered.
- **`fld [m]; fmul st(2)` plus a later `fstp st(0)` means the scale is a double** (0x4f8c80). A dead `lea r,[base+i*4]` after a load is a real `vector::operator[]`.
- **A float pair struct keeps the second value computed and spilled before the first `_ftol`:** `struct {float x,y;} fp = {...}` (0x4faee0).

Rows that resisted all of these, so stop early: SIB base/index order (`[ebx+ecx]` against `[ecx+ebx]`) and `fld/fmul`
operand order, one instruction scheduled a slot away with no structural difference, and a read-modify-write `or [m], r`
where the original loads, ors and stores. Rotation on its own isn't on this list any more: don't try spellings at the rotated
site, trace it back to the function's first temporary (section 6.5).
These are not absolute: a reference local flipped SIB order on 0x4c1e10, and an inline helper with reference parameters moved
`fld/fmul` order on 0x414960, so try one of those before giving up.

- **A ternary keeps a rounded int in a register (`terrain_render` 0x46baf0).** `int k = !(density-fl<=0.5f) ? k0+1 : k0` stayed in a register where `if (...) k++` homed it in memory. In the same row, `float p[2]` for a pair of products gave the original's shape: both products, then subtract, then spill.
- **Re-deriving a reference per call can flip commutative x87 loads (0x46ad50).** Load order follows when the operand's IL appears. `quad(x,y).nrm[flip]` at each call cut the wrong fld/fadd lanes from 6 to 3.
- **A constant store written first keeps its register live (0x4686a0).** `f40 = 1.0f; f0 = 0;` ahead of the -1 stores put -1 in esi; cl sinks those two stores back to the original's order.
- **Index every element read through the same base (0x46d8c0).** `v->lights[i]` for all three reads, with a local `ls` kept only for the size test, gave `mov edx,ecx` and `lea edx,[edx+esi*4]`.

- **When `insn_align` looks identical, run `chk.py` (`Agent::update` 0x45f660).** `insn_align` masks constants, so the last difference, switch case bodies in the wrong order, showed only as 4 constant mismatches in `chk.py`. Moving the 0x7b/0x7c bodies before 0x75/0x77 matched the row.
- **Arrays with enregistered first elements (`mesh::add_quad` 0x44e970).** Real `int pts[4]` and `int vts[4]`, block-scoped `q` and `u`, and visible `add_point` and `add_vertex` matched the row.
- **One short-lived local per arm (`update_attack` 0x45c220).** Separate locals in each arm gave the original's slot groups, where one shared `kind`/`var` with high use counts didn't.

- **`v.begin()[i]` keeps the materialised address (`PolyMesh::build_adjacency` 0x470920).** `v[i]` and an accessor both fold it. That one change took the row from 57 of 93 identical to a match.
- **A test placed after an if/else gets jump-threaded (0x46e590).** That explains an else arm laid out after the join.

- **Equal-output accessors aren't equal to the compiler (`note_window::build` 0x50a1c0).** A distant operand form flipped in bands as compiler-internal work grew, not with emitted size, and code-free statements such as `int Z = this->menu_id;` count (band period about 340 of them). Exact bytes under added padding mean the row is one band away and the fix is accessor weight, not structure: nine `get_w` became `get_party`, which emit the same code, and the row matched. Padding is a probe only; it never stays. Check that a form test tells the original's shape from a renamed copy of ours.
- **The same band effect, but dead padding doesn't count (`CharSheet::init_screen` 0x507400, 95 bytes off).** The register that joins the two arms after `new text_box` (original esi, ours eax) depends on how much real work survives *after* the site. Cutting the function off with `return;` before the weapon loop gives esi; cutting it after the weapon loop gives eax. Unlike note_window, dead padding doesn't move it.
  - **The anomaly is ours:** an image-wide scan of `new`-then-join sites found 12 esi/esi, 12 esi/ecx and no eax joins anywhere, so the original's source isn't winning a tie.
  - **Didn't move it:** fewer reference formals, a shared loop counter, and `vector2` out-params, all byte-identical.
  - **Not yet tried:** implicit vs explicit `tstring` temporaries at the lookup and label sites, and the `vector2(float,float)` constructor temporaries.
  - Truncation probes also move the whole callee-saved map, so read only the join from them.
- **`lea mgr,[g+0xf8]` then `[mgr+0x138d0]` means a member accessor on the sub-object, not a free helper (`MapPanel::update_target` 0x5191e0, +60 slot-masked instructions).** A free `party_at(g, id)` folds the two offsets into `[g+0x139c8]`; `g->parties.get_w(id)` keeps the `lea` as the original does. Likewise a 64-bit `and; xor; or; jcc` test means the source tested the 64-bit flags field, not its low dword.
- **Register swaps that follow how many times the source reads a global (`MapPanel::update` 0x51abd0).** Every form with one `g_game` read gave g=esi; only a second read gave the original's edi/esi, and that also moved the load into the loop head. Count the original's reloads of a global before trying accessor forms.
- **`break` against `return` is a whole-switch pattern (`terrain_dialog` 0x4fe860).** With one shared-exit difference in a large switch, test patterns across the switch, not per site: radio cases `return 1` with the rest `break` gave 0 differences, where the other three patterns gave 3456 to 4599. Temporarily renaming a callee shows which tail groups compete for the exit hub.
- **Raw index against `operator[]` (`build_second_pass` 0x4d1880).** `(&vec[0])[i].field[k]` lets cl fold first + (i*size + k*4) with its own induction variable; `vec[i].field[k]` materialises the element. One function can use both. A `T& g = vec[j]` reference CSEs field loads, so repeated reloads of one field in the original mean there was no reference local.
- **A reciprocal written out, not named (0x4d1880).** A named `float inv` used three times gives `fld m; fmul st(1)` ... `fstp st(0)`; the same expression repeated becomes a CSE temp that dies at its last use, giving the final `fmul [m]`.
- **SIB order can move from a sibling reference (`split_marked_edge` 0x4d27d0).** `[base+IV]` flipped through a reference to a different element in the same comparison; spelling at the load never moved it. A named local added anywhere can also flip distant `fld a; fadd b` orders, so recheck x87 operands afterwards.
- **A store hoisted out of a loop past an opaque call means the original saw the callee's body (`mesh_terrain`).** Other shapes from there: an unreferenced frame gap can be a larger array (`int tri[4][2]` with 3 rows used); one loop variable shared by two loops merges their slots; `x + y + z` of a local's fields can compile to `z, y, x`.
- **An address-taken local conflicts with locals nested in its block (`start_setup` 0x484590).** Where its block opens and closes moves other locals between slots and can change the frame size; spills in a block nested in its scope go to high slots.

- **A local whose address reaches a callee cl can't see keeps a private slot, through the whole call chain (`execute`, frame 0x1ee8 to 0x1ec0).** Give the chain's callees visible `inline __declspec(noinline)` bodies above the function. Check by removing each one: of 47 added, none gained when removed. Conversely, an extra visible body can hurt: `update_controls` lost 61 instructions to a visible `AgentTable::get`, and `ShadowMap::extend` needs `transform_point_to_local` opaque while `map_panel` needs it visible, so that body can't move to a shared header.
- **`tstring` temporaries: a block-scoped local, not a forwarder's temporary (`execute`).** `{ tstring msg(buf); ... }` and plain expressions gave every tstring slot; `tstring("x")` temporaries and locals like `text`, `nm` and `arg0` from forwarders were the cause, not the allocator.
- **`__forceinline` forwarders compute `this` before the arguments**, which rotates scratch registers. Call the real method directly. Cases out of the original's order rotate later cases too, so follow the jump table's body order.
- **A by-value return goes right-hand side first (`get_frame`).** Assigning `script_pos(i) = get_frame()` computes the destination before the call; a helper that takes the result first gained 34 cases. A signature that returns by pointer can't give those shapes, so check `ret 4` hidden returns against the callers. Likewise `set_pos` takes a `Vec2` by value: the two dwords are pushed after the other arguments, and no two-float spelling or forwarder can do that.
- **`mov ecx,g_game` before a call to a function that ignores `this` means a member with an unused `this`** (`sl_item_flag_all`, `is_night`). An argument in EAX to a one-caller function means a `static` in the caller's object (`trigger_item_mesh`), even when its address sits far from the caller.
- **A named local before a call keeps a parameter across it (`troop_add_item`: fixed that case and 6 others).** Branch sense also matters: `!= 0` first, `!= -1` first, or ending in `!side`.
- **Nested ternary of lookups in one expression gives cl's conditional-temporary flag (`place_slot_mesh`).** `sprintf(buf, lookup("x")->c_str(), stat, t==0 ? lookup("a")->c_str() : ...)`. With an implicit `lookup("key")`, cl emits the constructor, inlined-call arguments, the lookup, then out-of-line arguments; code between the constructor and the lookup means an inline accessor. A dropped intermediate local can fix a distant register map.
- **Immediate zero stores next to a live zero register mean float fields (`close_tooltip`).** `(float)x` conversion of a value in `[0,2]` hides unsigned vs signed. `resize(n)` is the inline overload that spills `n` first; `resize(n, T())` gives the wrong shape (`TableauTbl::read`).
- **A helper written between `// @ 0x<va>` and the function becomes the marker's symbol**, so chk and equiv test the helper. Keep helpers above the marker. And check what a row is tested as: a comment like `// 0x49bbb0 spawn the battle icon` once bound a row to the wrong function, which read as a fault and a mismatch (fixed in `marker_names`). A `FAIL` with an audit disagreement on the first callee is worth this check first.

- **A const-reference helper reading a field at offset 0 keeps the address materialised (`update_team_stats` 0x474e60).** `team_leader(teams[t])` moved the loop's induction base from +0x50 to 0, which fixed a `lea ecx,[ebx+0x50]; lea ebp,[ecx-0x34]` head. Using `operator-` with a const-reference operand moved another bias.
- **Inline vector operators leave a fingerprint (`Agent::update_ai` 0x4465e0).** Look for `fld a.x; fmul k; fld a.y; fmul k; fld a.z; fmul k; fstp [t]; fxch; fadd b.x ...`, where all three products come first and only z goes to memory. That is `a * k + b` written with the header's inline `operator*`/`operator+`. Writing the components out lets cl compute z first. When x and y are loaded as floats but z is copied as an int, the source built a 3-float `vector4(v.x, v.y, v.z)`, not a memberwise copy. Declare the result where it's computed (`vector4 d = k * t->v56b4;`). Assigning to an existing local whose address escapes goes through a temporary plus a 4-dword copy. Together with the read-twice rule below, these took the row from 1715 to 2081 identical and its frame to the original's 0xa0.
- **cl keeps the reload of a global it caches itself, but not a named local's.** `mov ebp, g_mission` after a store into `this` means the source used `g_mission` directly, so the reload is cl's own CSE. A `Mission* m` local with explicit `m = g_mission;` reloads gets a different register priority (0x4465e0 mode 5: 45 instead of 60 differing). This depends on the region: in the same function's mode-4 loop the direct spelling swapped two registers and lost 41.
- **A visible pure callee is sunk past a later call to its use.** With `clamp`'s body in the TU, `c = clamp(...); fr = frand(); ... fr * c` comes out as frand first and then clamp (0x4465e0). The original calls clamp first. Making clamp opaque restores the order, but its arguments then get private slots and the frame grows (0xa0 to 0xb0).
- **A value read twice in source blocks CSE of its index arithmetic,** even though the load itself is CSE'd (0x474e60). Confirmed in 0x4465e0: `AG(target).pos` and `AG(target).cur_pair_fi()` both read the member, and `&AG(f55c0)` re-reads it after `near_ = f55c0` was tested. That made the original's recomputed `and/imul/shr` match, and it brought the function-wide `ebx=-1`/`ebp=0` constant registers with it (+87 identical in one edit). `fstp m; fmul m` rather than `fst m; fmulp` means a block boundary between the store and the use. A function-scope loop variable merges slots across the whole function: per-loop block scopes took that frame from 0x84 to 0x80.

## 15. Not source-controllable, or without a known lever

There is no round limit on these: keep trying different ideas, and record the
function as complete when you have honestly run out of them.

- Which callee-saved register holds `this` or a loop pointer (`esi`/`edi`/`ebx`
  swaps), and `edx`/`ecx` swaps in the same instruction stream. Small functions
  now have a rule (section 7, "Callee-saved register order"); very large ones
  and reload orders don't.
  - **Reload order after a loop (M&B `Terrain::tessellate` 0x481570, 4 bytes off).**
    The original reloads edi, ebx `[0x44]`, esi `[0x14]`; ours reloads edi,
    esi, ebx. About 30 probes ruled out next-use order (the ebx use moved first
    and nothing changed), def and use order (identical in both builds), and the
    declaration order of the named locals. Struct members, walkers, loop forms,
    scope changes and forceinline helpers didn't move it either. It moved only
    together with other code: adding `qn == 0 &&` to a test, or dropping a store.
    **Solved 2026-11-11 (c2 tracer, `../vs2005-cpp` regalloc.md 3.6.9): it is source-controllable, through first
    mention.** The reloads are loads of a late split round, inserted in ascending number of the new pieces; new pieces
    take record numbers off a last-in-first-out free list and their parents are handled in ascending number, so the
    order follows the values' register-candidate numbers, and those start in order of first mention in the function
    (not declaration order, not priority, not next use). The original needs `x` numbered before `g`, `cr` and `y`:
    one `x` and one `y` for both loop nests, declared with initialisers before `g` (`int x = 0, y = 0;`), and the
    function matches. `int x; (void)x;` before `g` works too; the same mention after `g`'s line does not.
    `rasim check --predict` reproduces the record numbers and the reload groups (93 of 93, 18 of 18 here).
- Several values kept unrounded on the x87 stack across a branch while others
  live in memory (M&B `screen_to_world`: `sy`, `py`, `oy` in registers, `px`, `z`,
  `ox` in memory). Twelve source shapes and four `volatile`/pointer variants moved
  the first statements into line but not this. Accept it as `PASS-FP` when the
  tolerance mode agrees.
- ~~Slot sharing in very large functions (section 7)~~ **solved**: the predicate
  is address-taken-and-read-after-escape block locals, and the fix is the
  function-scope scratch hoist (section 7). Application to the script interpreter
  (`0x2630` → `0x1ec0` target) was in progress when this was written.
- **Where an independent integer load lands next to x87 work (M&B `update_camera` 0x4f8c80).** In `((float)(ticks) * 1e-5f) / v; if (num > 0)`, ours loads `num` between the two int64 stores, and the original loads it after the `fild`. Every form that moves the load also emits `fdiv` before `fmul`, which is a different rounding. About 20 other spellings left it alone (locals, helpers, a `Timer` member, compare forms).
- The frame sharing around pushed `double` varargs.
- Six `push`es against `sub esp,0x18` plus six stores for a by-value 6-int block.
- Extended-precision x87 ordering that differs by one ULP.
- The `sub eax,N / je` dispatch chain, where `switch`, if-chain and unsigned
  `switch` all give range compares.
- A callee whose ABI is custom register arguments with no same-TU form to
  imitate.
- Reordering stores or `lea` temporaries to "match" the disassembly made three
  functions worse in one wave; revert any experiment that raises the diff.

---

## 16. What the byte diff cannot see

- A byte comparison masks relocations, so a callee's address and return
  convention are not checked, and the compare of a wrongly declared `__thiscall`
  callee barely moves. The equivalence harness (`tools/difftest/equiv.py`) is what
  finds wrong callees, wrong return conventions, swapped arguments and
  `new`/`new[]` mix-ups.
- Raw match percentage moves with alignment, not semantics: branch labels compare
  as byte offsets, one extra temporary renames every later register, a size
  change relabels every branch, a frame difference shifts every stack operand.
  Compare aligned instructions (`tools/matching/insn_align.py`): on
  `screen_to_world` the byte count ranked the escaping-pointer variant worst (417
  bytes) while alignment showed it among the closest (52 identical, 13 extra
  instructions).
- `cmpdis.py` masks relocations to zero and disassembles the original only up to
  your length, so a size mismatch truncates the tail.
- **Test interactions, not single mutations.** Many exact matches needed several
  coordinated changes that were each neutral or negative alone. A negative sweep
  holds only for its baseline; replay rejected spellings after an ownership or
  lifetime change.

### 16.1 Behaviour bugs only the equivalence harness found (2026-10-29)

Every one of these compiled cleanly, and most had a plausible byte diff. They
surfaced once harness work let inputs reach the code: import models, typed
inputs, fake-call arity. Check for them when writing a function.

- **Constants inside expressions.** `rand() % 15785` where the original divides by
  `0x3dc9` (15817), at four sites in `mesh_3` 0x52f250. The float scale factors
  were already right (0.75/15817), which was the clue.
- **Pointer difference divided twice.** `(e->b - e->a) >> 2 == 0` with `int*`
  members: the subtraction is already a count, so lists of 1 to 3 entries looked
  empty (`ArrHolder::lookup` 0x48d5f0). The original's `sub; sar 2` is the byte
  difference.
- **A missing dereference.** `*(int*)(v + 0xb4 + i*4)` where the original loads
  the array pointer first: `[[v+0xb4] + i*4]` (0x48d636).
- **Swapped if/else arms** (`FieldPanel::update` 0x48d4d2). The `> 0` arm copies
  the stored size into a local; the other arm uses the scene's values.
- **A missing empty-container guard.** A `size()-1` loop bound wrapped on an empty
  list (`map_editor` 0x530d43).
- **A handle cached where the original re-queries.** The original calls
  `GetDlgItem(h, 0x420)` before each of its two `SendMessageA` calls
  (`GameApp::msg_proc` 0x4d4140). Reusing one HWND changes the call trace.
- **Win32 flag constants.** `GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE)` is
  `0x2002`, not `GMEM_ZEROINIT` (`0x42`) (`copy_clipboard` 0x4f34c0). Decode every
  immediate pushed to an import against the SDK header.
- **A missing API check.** `paste_clipboard` 0x4f3c30 calls
  `IsClipboardFormatAvailable(CF_TEXT)` first, and refreshes only after a
  successful read.
- **Results stored in the wrong local.** `note_window` 0x50a1c0 put the links
  text height (`[esp+0x14]`) and the report height (`[esp+0x24]`) into the same
  variable. Map every `mov [esp+N], eax` after a call to a named local before
  writing the code.
- **Uninitialised locals stay uninitialised.** If the original never writes a
  local before reading it (`note_window`'s two heights, 0x50a66d), declare it
  without an initialiser. Zero-initialising it is a behaviour change. The
  remaining equivalence difference is then a documented artifact, a different
  stack leftover in each build, not a bug.
- **A running pointer stepping by the wrong size (M&B `mission` 0x478a10, found 2026-11-10).** A `vector<int>* c`
  advanced with `c += 1` strode 0x10 bytes where the cells are `sizeof(cell)` = 0x34; indexing `cells[c[0]][c[1]]`
  fixed it (FAIL -> PASS, aligned 118 -> 29).
- **A store moved ahead of a read of aliased memory (`PolyMesh::build_geometry` 0x470a00).** `nbr[j] = -1` was written
  before the centroid accumulate that reads the same memory, putting a NaN pattern into the sum; the original stores
  it after. The fix alone made the bytes worse (293 -> 326); the match came with `faces[i].centroid += ...` through an
  inline `operator+=`.

Harness behaviour worth knowing when reading a verdict:

- A row stops after 150 inputs in a row with no new coverage, once 200 inputs are
  valid. Run `--plateau 0` to confirm a fix with every input.
- `--typed-share 0` turns off the type-aware inputs. Use it to tell whether only
  CodeView-typed objects trigger a mismatch.
- ecx is not compared at a call through a generated pointer when the original's
  call cannot be `__thiscall`:
  - `call [ecx+disp]`, where ecx holds the vtable;
  - `call [R+disp]`, where R is the vtable of the first stack argument (COM
    `__stdcall`), unless ecx is itself an object with that vtable;
  - a call whose caller pops the arguments (`__cdecl`).
  Everything else at such a call is still compared.
---

## Appendix: tools

| Need | Tool |
|---|---|
| Annotated disassembly plus decompile | `tools/matching/card.py <va>` |
| Compile once, diff every VA | `tools/matching/chk.py` ("diff" is positional, 0 = MATCH; "aligned" counts only the bytes of differing instructions after aligning the two lists: judge progress by it and record it in the `.nonmatching.txt` column) |
| Rank variants | `tools/matching/try_variants.py` |
| Instruction-level diff (exact mangled name) | `tools/matching/cmpdis.py` |
| Aligned instruction comparison | `tools/matching/insn_align.py <src.cpp> <va> [--show]` |
| The original's instructions | `tools/matching/disasm.py <va> [len]` |
| Final check of a manifest | `tools/matching/run_all.py --manifest ...` |
| Compiler wrapper | `tools/matching/cl.sh` (cl 13.10.3052) |
| Behaviour equivalence (`--fp-tol` for floats) | `tools/difftest/equiv.py` |

## 16. Rules from the 2026-11-11 night wave (each from a traced, byte-exact match; commit in brackets)

Alias and scheduling order
- A store through a pointer is a scheduling dependence for a later load of a named global with external linkage or of
  a non-const float global; not for a literal constant, nor for a file-static whose address is never taken. If the
  original loads a constant/global above a pointer store that ours keeps below, its object was a literal or a static
  (progress_bar::set168 b24a6e6c, Terrain::fill_desc_* 82413a3c).
- A callee that is only declared is assumed to keep a pointer passed to it: the local's home store then depends on
  every pointer access around it, the local cannot share a frame slot, and a value cannot stay in a register across the
  call. Give the unit the callee's body (an `inline`/`__declspec(noinline)` duplicate, as several files already do) and
  those effects vanish (camera_mark_entities 7395564d, PartyMgr::id_remove 70f26887, import_apply d3527887,
  hit_damage c9c89196, plan_order 6e6a1af0). Visible bodies also change the inliner's costs (`/d2inlT` prints them;
  threshold 74): a `found = true; continue;` arm pushed one helper above it and kept the other below.
- Callee summaries work the other way too: if the original keeps a store into a local whose address goes to a
  same-unit callee, that callee read through the parameter (PartyWindow::place_slot 3c200809).
- Scope sets aliasing and frame overlay together: a local declared in an inner block is independent of pointers
  computed before the block; at function scope it aliases but also conflicts with parameter homes in the packer
  (PartyMgr::read a0e6a87b).
- Parentheses around a float argument or right-hand side add an OPPAREN tuple that delays the next x87 instruction
  past neighbouring integer stores/pushes (apply_forces 6aeef0ce, XyzButton::animate e3f372f3, hit_damage). The
  scheduler works in regions of at most 81 tuples; one extra tuple can move a region boundary and with it an order
  far away (DropDown::animate 8860b80f, plan_order).

Shapes that identify the source
- `mov reg2,reg` between a `cmp` and its `je` on a pointer: the null test is inside an inline helper, on its by-value
  parameter (GameApp::FinalCleanup bed82727).
- Unfolded `add reg,base; mov reg,[reg+off]`: the address is shared by several source-level reads, typically an
  accessor called per comparison instead of a cached local (item_slot_source 823d4122).
- Both operands computed, then both stored, with an integer copy between the x87 instructions: a struct temporary
  assigned whole, not per-field stores (ContainerBase::on_child_moved ef16b1f9).
- Element address computed before the argument push: the real `std::vector::operator[]`; after it: `first + i`
  (Game::load_tracks bceaabc4).
- A loop unrolled by 3 with `lea ecx,[eax+2]; lea esi,[ecx-1]` is cl's own unroller, which skips an integer loop whose
  loads equal its stores: read the member in the condition instead of a local copy (import_apply).
- `x + 0.0f` copies that stay (dead integer copies of x/y before a z store): `v += vector4(0, 0, k)` through an inline
  helper (update_controls 77a9259a).
- xor-merge of a colour with the alpha loaded twice: the source is `(a & 0xff000000) | (b & 0xffffff)`, and the flag
  clear is written before it (mesh_hud animate family).
- A reload of `lea reg,[tmp]` instead of reusing a constructor's returned this: a named temporary in its own block
  (`{ tstring s(v); x = s; }`).

Frame slots (tools: `c2trace.py slots --c2 13.10`, `c2slots.py --c2 13.10`)
- Frame below 128 bytes: slots are never sorted by key; they stay in creation order = the packer's visiting order
  (size ascending, uses descending, ties to the first to reach the count). At or above it: key 1000*uses/size.
- One variable reused N times is one element with N times the uses; N separate variables are N elements that can
  share a slot. Both directions occur (XyzButton: three floats; Skin::read 9f146922: one function-scope `int i`).
- A by-value scalar parameter of an inline helper whose address goes to an opaque callee shares one slot across the
  expansions; named locals and const-reference temporaries do not (parse_module_info 2e679497).
- Arguments passed by address to set_size/set_offset/set_xy in the UI code are 8-byte two-float temporaries, one per
  call, never a named vector4.

Symbol keys and operand order (tool: `symkeys`)
- Keys follow first reference: initialisation order of locals decides lea base/index and x87 operand order
  (nearest_face 669df504); a dead early mention of a field moves its field-view key (psys::update 9eb5b08e).
- SIB base/index can also flip from the optimiser's expression numbering: moving an if/else into an inline helper
  shifted one entry and swapped three operands without changing any instruction (FxDef::update 2b523d72).
- Strength reduction anchors the induction pointer at the first store address that survives to that pass
  (Terrain::set_cells: exact only with a self-assignment, not landed).
