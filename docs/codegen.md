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

### 1.3 Smell tests

- `fld st(i); fmul [m]` (a just-computed value stays on the stack and is
  multiplied by memory) is cl 13.10 output; the reverse order,
  `fld [m]; fmul st(i)`, is not.
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
- **A guard on the address of a field of a fresh allocation is real.**
  `lea ecx,[eax+8]; test ecx,ecx; je` comes from `if (&pNode->mValueA)`;
  removing it shrinks the function.

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
  names** (head_rec.h's `slot_empty_1/2/4`, `scale_3/5/6`, `pure_7`,
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
  their thunks stay with the record (`void ItemRec::slot_empty_1() {
  reset_gui(); }`, item_rec.cpp:55).

---

## 6. Register allocation and compiler internals

### 6.1 Whole-function allocation

Allocation is whole-function and keyed by internal ids, not by textual position.
A far-away residual after a local edit is normal. Where the original has a
register swap that no source change moves (which callee-saved register holds
`this` or a loop pointer), see section 15.

### 6.2 Commutative and SIB order keyed by ids

The compiler sorts commutative operands, SIB components and scheduler ties by
internal symbol and IL ids. Any source change that adds or removes named locals,
references, inline formals, operator records or earlier global references can
flip operand order in distant code. Directly commuting one operation's operands
in source is normally neutral.

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

---

## 7. Stack layout

- **`/O2` homes** come from a conflict graph over lifetimes, ordered by increasing
  size then decreasing use count (parameters are scanned separately). Narrow
  scoping, or per-arm short-lived values, lets later values reuse slots and
  shrinks the frame. Local-slot order did **not** depend on declaration order in
  24 shuffled variants of one function (observed in Spore's cl 13.10 code).
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

- `fld st(i); fmul [m]` (keeps a just-computed value on the stack and multiplies
  by memory) is the cl 13.10 shape. Observed in Spore's cl 13.10 code (Havok x87).
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

### 9.5 Reading float mismatches

Compare **aligned instructions**, not differing-byte counts (section 16), and
grade float-only differences with a tolerance rather than by eye. M&B: on a
mismatch `equiv.py --fp-tol` re-runs the original at 64-bit precision (control
word `0x037F`) as the reference. Three failures that had been "justified" by
reading the mismatches turned out to be real bugs under that mode (a code
address written where the original writes a float, a skipped `normalize` call,
seven values left on the x87 stack at return).

### 9.6 Reading `fcomp; fnstsw ax; test ah,M; jcc`

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

## 15. Not source-controllable, or without a known lever

Record the function as complete and move on after two rounds without a diff drop.

- Which callee-saved register holds `this` or a loop pointer (`esi`/`edi`/`ebx`
  swaps), and `edx`/`ecx` swaps in the same instruction stream.
- Several values kept unrounded on the x87 stack across a branch while others
  live in memory (M&B `screen_to_world`: `sy`, `py`, `oy` in registers, `px`, `z`,
  `ox` in memory). Twelve source shapes and four `volatile`/pointer variants moved
  the first statements into line but not this. Accept it as `PASS-FP` when the
  tolerance mode agrees.
- ~~Slot sharing in very large functions (section 7)~~ **solved**: the predicate
  is address-taken-and-read-after-escape block locals, and the fix is the
  function-scope scratch hoist (section 7). Application to the script interpreter
  (`0x2630` → `0x1ec0` target) was in progress when this was written.
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
| Compile once, diff every VA | `tools/matching/chk.py` |
| Rank variants | `tools/matching/try_variants.py` |
| Instruction-level diff (exact mangled name) | `tools/matching/cmpdis.py` |
| Aligned instruction comparison | `tools/matching/insn_align.py <src.cpp> <va> [--show]` |
| The original's instructions | `tools/matching/disasm.py <va> [len]` |
| Final check of a manifest | `tools/matching/run_all.py --manifest ...` |
| Compiler wrapper | `tools/matching/cl.sh` (cl 13.10.3052) |
| Behaviour equivalence (`--fp-tol` for floats) | `tools/difftest/equiv.py` |
