# Papercuts

Friction found while matching: tool bugs, unclear docs, tricks that had to be rediscovered. Agents append
entries under **Open**; the orchestrator fixes the tool or folds the lesson into `docs/matching.md`, then
moves the entry to **Resolved** with what was done.

Format (one entry per problem):
```
- <slice id> <YYYY-MM-DD>: <what cost time> — fix idea: <…> — minutes lost: <N>
```

## Open
- G6-char-sheet 2026-10-24: the resolution audit / equiv read address annotations only from the headers a .cpp includes itself, so moving `Animator` into src/game/animator.h left `set_anim_a` unresolved in mission5/troop_rec.cpp (equiv WEAK -> UNSUPPORTED) until the header was also listed in the .cpp — fix idea: follow nested includes (or warn when a nested header carries `// 0x...` annotations) — minutes lost: 20
- G6-char-sheet 2026-10-24: cl 13.10 gives overloads of one virtual name consecutive vtable slots in REVERSE declaration order (probe: `ov(int)`, `ov(const V2&)`, `ov()` -> +0x3c, +0x38, +0x34); the "families numbered by parameter type" theory in terrain_page_update.h was this effect. Give every virtual its own name and take offsets from the call sites, not from counting placeholders — minutes lost: 30
- G6-char-sheet 2026-10-24: a codegen_dump fingerprint can change with no change in the function's own bytes (a referenced out-of-line function's scheduling moved when a header added types); `work/headers2/G6-char-sheet/cgdbg.py` style walks show which referenced symbol differs — fix idea: add a `--why ROW` option to codegen_dump.py — minutes lost: 25
- wave1/s00409500 2026-10-20: `d3d9.h` does not compile with the VC7.1 toolchain (`specstrings.h` needs `sal.h`, which the Toolkit lacks), so D3D device calls need a local vtable stub (slot indices: SetRenderState 57, SetTexture 65, DrawPrimitiveUP 83, SetVertexShader 92) — fix idea: ship a stub `sal.h` in `match/include` (or a `d3d9_min.h` with IDirect3DDevice9 slots) — minutes lost: 10
- wave1/s00409500 2026-10-20: equiv.py reported `unresolved reference ?normalize@rgl@@...` for a callee declared in rgl.h whose `// 0x407e00` is not at the start of the comment (it sits after prose); and a second declaration with `// 0x407e00` only fixed it for ONE of two functions in the file. Adding `00407e00 <mangled>` to symbols/slices/<id>.txt fixed both — fix idea: have the resolver also accept an address anywhere in the declaration comment (and make rgl.h comments `// 0x...` first), or fall back to symbols/ of every slice — minutes lost: 15
- wave1/s00409500 2026-10-20: cmpdis.py / try_variants.py give no hint when `work/match/*.obj` was not produced (cc.sh style wrappers print nothing on success); other agents share `work/match/` so generic scratch names (s1.obj) can collide — fix idea: doc that scratch objects should be named `scratch_<id>_*.obj` — minutes lost: 5
- wave1/s00409500 2026-10-20: x87 equivalence mismatches that are really float-vs-extended rounding (e.g. an intermediate the original `fstp`s to float but our build keeps in a register) are only found via replay; a `volatile float tmp` trick in the nonmatching source makes equiv PASS but is not a codegen hint — fix idea: mention in matching.md that equiv failures on denormal/"smallint" inputs usually mean a missing float rounding point — minutes lost: 20
- wave1/s00409500 2026-10-20: pilot's rgl.h-like `sphere` class in s00407c10 has v20/v30/m40 members, but the copy-ctor at 0x40a580 shows the base is only 0x20 bytes (vtable + 7 ints) and the 100-byte derived class (vtable 0x54a540, vectors at +0x20/+0x30/+0x40/+0x50, float at +0x60) owns them — fix idea: split sphere into base + derived (`capsule`, provisional) when merging — minutes lost: 0
- wave1/s00407250 2026-10-20: `#include <windows.h>` / `<d3d9.h>` fail ("sal.h" missing from the vc71tk include path although matching.md says the SDK headers work), so Win32 imports and the D3D9 device had to be declared by hand — fix idea: ship a stub `sal.h` (all SAL macros empty) on the include path, or document the hand-declaration route — minutes lost: 10
- wave1/s00408110 2026-10-20: `cmpdis.py` is not given `/Imatch/include`, so recompiling by hand with `cl.sh` for a diff fails silently when the file includes rgl.h; also try_variants/cmpdis output lists a function's out-of-line copy of `__forceinline` helpers as an "ambiguous" symbol — fix idea: let cmpdis take the slice id and compile itself — minutes lost: 8
- wave1/s00408110 2026-10-20: lessons worth folding into matching.md: (a) an inlined callee in the same TU gives the caller interprocedural register knowledge, so a call to an out-of-TU function must be declared on a stub type with no body (`pos_ext`) or the caller keeps values in caller-saved regs; (b) `__declspec(noinline)` is accepted by cl 13.10 and is the way to force the original's `call` to a small helper; (c) a base class with an inline ctor (virtual dtor declared, never defined) reproduces "vptr stored last"; (d) a `float pts[N]` local sets the exact `__chkstk` frame size (N*4 + 0x58 here); (e) a pointer-walking `if (n>0) { p=...; do {..} while }` loop reproduces `add esi,K` inside the guard, a plain for loop hoists it; (f) when every source spelling of an x87 sum compiles identically, the original probably used a different operand form (reference vs value, vector4& param instead of float*) — `void f(const vector4& t){ vector4* o=&cur.o; o->x += t.x; ...}` fixed an asymmetric fld order that float* spellings could not — minutes lost: 0 (notes)
- wave1/s00402be0 2026-10-20: `#include <windows.h>` fails under cl 13.10 (`specstrings.h` needs `sal.h`, absent in vc71tk) — had to hand-declare every user32 import and MSG/POINT — fix idea: add a shim `sal.h` (empty SAL macros) to `src/include` or ship a trimmed `winapi.h` in match/include — minutes lost: 15
- wave1/s00403120 2026-10-20: file-local (`static`) functions use cl's custom register convention (argument in EAX, e.g. 0x4035d0) and only get emitted when a caller exists in the TU; the trick (static fn + dummy caller) matched 112/112 bytes but is not in matching.md — fix idea: document "EAX-arg / ECX-preserved function = static in the original TU; define it static plus a caller" — minutes lost: 20
- wave1/s00402be0 2026-10-20: same mechanism explains `call A; call B` with no `mov ecx,this` between (cl knows static A preserves ECX); not reproducible without A's full body in the same TU (0x402be0 stays +2 bytes) — fix idea: note in matching.md — minutes lost: 10
- wave1/s00403120 2026-10-20: much of this code is the DirectX 9 SDK sample framework (d3dapp.cpp / d3denumeration.cpp, CArrayList); transcribing the SDK's literal source form (comparison order, `bool` temporaries, one `case` per value instead of grouped labels to get a direct jump table) matched where "natural" spellings did not — fix idea: tell agents to recall SDK sample source first — minutes lost: 30
- wave1/s00402be0 2026-10-20: equiv.py reports UNSUPPORTED for any function calling user32 imports (SetWindowPos, MessageBoxA, LoadAcceleratorsA, DialogBoxParamA...: `discarded import:...`), so framework/window code cannot get PASS; also ~10 min per function — fix idea: stub common user32 imports in machine.py — minutes lost: 25
- wave1/s00405890 2026-10-20: equiv.py cannot run anything that calls user32 imports (GetDlgItem/SendMessageA/IsDlgButtonChecked: "discards: import:..."), so the whole D3D settings dialog region (0x405890-0x407230) is UNSUPPORTED — fix idea: model the Win32 dialog/message imports as stubs (return handles/ints, record calls) — minutes lost: 15
- wave1/s00405890 2026-10-20: `#include <windows.h>` fails in cl.sh (SDK 6.0A specstrings.h needs `<sal.h>`, absent from vc71tk and vc9sp1 dirs); had to hand-declare GetDlgItem/SendMessageA/... with `__declspec(dllimport)` in a slice header (`match/slices/s00405890/d3ddlg.h`) — fix idea: ship a minimal `sal.h` (or all-empty SAL macros) in `src/include`, which is already on INCLUDE but does not exist — minutes lost: 10
- wave1/s00405890 2026-10-20: try_variants.py needs the variants as free functions in one file and has no way to swap one code fragment; wrote /tmp-style helper that clones a member function N times as methods of a derived class (needed so `this` stays in ecx) — fix idea: a `--subst old.txt new.txt...` mode — minutes lost: 15
- wave1/s00405890 2026-10-20: cmpdis.py diff is unreadable when one early difference reorders a whole function (register renames cascade); a "ignore reg names" or per-instruction-opcode-only diff would show structural diffs sooner — minutes lost: 20
- s00404140 2026-10-20: `windows.h` / `d3d9.h` (SDK 6.0A) fail under cl 13.10 (C1083 `sal.h`: `specstrings.h` includes it by angle brackets and VC9's sal.h is not on INCLUDE; pre-including it by path then breaks in `driverspecs.h`), so every D3D/GDI slice has to hand-declare Win32 and the D3D9 vtables (I wrote `match/slices/s00404140/d3dmin.h`: GDI, user32 dialog calls, IDirect3D9/Device9/Texture9/VertexBuffer9/StateBlock9 with real slot numbers) — fix idea: ship a shared `match/include/win32min.h` + `d3d9min.h` (or a stub `sal.h` that defines the SAL macros empty and `-D_PREFAST_` off) — minutes lost: 25
- s00404140 2026-10-20: the default flags include `/EHsc`, but the original has no SEH/EH prologue anywhere in this region (a local with a non-trivial destructor plus `new` compiles to `push -1 / fs:[0]` frames with it); with `--flags "/O2 /MT /Gy /TP"` everything matched. matching.md should say "try without /EHsc first for C++ with locals that have destructors" — fix idea: default the flags to no `/EHsc` or note it — minutes lost: 15
- s00404140 2026-10-20: a `static` helper compiled in the same TU gets cl 13.10's register convention automatically (0x4035d0 takes its argument in EAX and was matched by defining `static __declspec(noinline) int GetColorChannelBits(D3DFORMAT)`); callers of "usercall" functions in the original should declare the callee as static in the slice instead of giving up — fix idea: add to matching.md Lessons — minutes lost: 20
- s00404140 2026-10-20: scratch files named `work/match/cardN.txt` collide across parallel agents (another agent overwrote mine mid-read); `card.py` output is only safe to redirect into the scratchpad or into a per-slice-named file — fix idea: tell agents to use `work/match/scratch_<id>_*` for card dumps too — minutes lost: 5
- s00404a00 2026-10-20: `equiv.py` marks any function that calls user32/gdi32 imports (CreateCompatibleDC, GetTextExtentPoint32A, GetDlgItem/SendMessage...) UNSUPPORTED, so GDI/dialog-heavy functions cannot be verified at all — fix idea: add simple Python models for the common GDI/user32 imports (return a fresh handle, record the arguments) — minutes lost: 10
- wave1/s00401010 2026-10-20: `windows.h` + `d3d9.h` DO compile under cl 13.10 if the header starts with `#pragma include_alias(<sal.h>, <Z:/…/work/toolchain/vc9sp1/VC/include/sal.h>)` (VC9's sal.h is empty-macro in non-PREFAST mode; pre-including it by path or defining SPECSTRINGS_H both fail later in driverspecs.h). With real SDK types (D3DCAPS9 at +0xfc, D3DSURFACE_DESC at +0x22c, D3DPRESENT_PARAMETERS, D3DADAPTER_IDENTIFIER9) the framework layout falls out and vtable calls are exact — fix idea: put that pragma (or a one-line `sal_shim.h`) in `match/include` and say so in matching.md — minutes lost: 25
- wave1/s00401010 2026-10-20: `/EHsc` in the default flags changes the codegen of `delete[] p` for a class with a destructor (calls the `__ehvec_dtor` helper instead of the inline loop with the dtor body inlined), so `??_E` vector-deleting destructors only match WITHOUT `/EHsc`; the whole TU (all 25 functions here) matched with `--flags "/O2 /MT /Gy /TP"`, and the manifest rows carry the flags — fix idea: default to no `/EHsc`, or tell agents to try that first — minutes lost: 20
- wave1/s00401010 2026-10-20: compiler-generated `??_E`/`??_G` deleting destructors have no source line to put a `// @ 0x` marker on: write `void f(T* p){ delete[] p; }` (or a class with `virtual ~T()` and a ctor) and chk.py reports the generated symbol as "other ... MATCH"; a manifest row must name `??_ERec34@@QAEPAXI@Z`, not the wrapper — not documented in matching.md — minutes lost: 15
- wave1/s00401010 2026-10-20: `cmpobj.constant_diffs` reads the original at the raw operand address and ignores the DIR32 addend stored in our code bytes, so `strcpy(buf, "Not enough memory.")` (cl copies it as dword loads from literal+0, +4, +8 …) reports 5 false constant mismatches and the function can never print MATCH although every code byte is identical (DisplayErrorMsg 0x4014f0) — fix idea: compare `read(va - addend, len)` with the addend taken from our section bytes — minutes lost: 20
- wave1/s00401010 2026-10-20: register promotion of a global (load hoisted before the stores, one store at the end) only happens when the counter is a FILE-LOCAL static (`static DWORD dwAppPausedCount` inside `Pause`, inlined into the constructor); an extern/global of the same address gives 277 instead of 263 bytes. But resolve.py cannot map a file-local static (class-3 symbol) to its original address (only extern symbols go through symbols//annotations, function-local statics through alignment which fails when the function is not yet byte-exact), so equiv FAILs on the very function that needs the trick — fix idea: let a `// 0x…` annotation or symbols/ line bind class-3 data symbols too — minutes lost: 40
- wave1/s00401010 2026-10-20: resolve.by_symbols requires `plausible()` = the VA is in `img.start_set`; the 5-byte libcmt thunks `operator delete[]` (0x53570d: `jmp +0` into 0x535712) are not function starts, so `??_V@YAXPAX@Z` falls through to alignment and equiv FAILs ("orig calls 53570d, ours 535712") on a byte-exact `??_E` — fix idea: accept any VA that decodes as `jmp rel32` inside .text, or seed start_set from lib_names/symbols/slices — minutes lost: 15
- wave1/s00401010 2026-10-20: `source_annotations` only reads `<id>.cpp`/`<id>.h` of the slice under test, so a header shared with another slice dir (`#include "../s00401010/s00401010.h"`) silently loses all its `// 0x…` annotations (equiv then says "map that symbol" or aligns wrongly); I had to copy the header into every slice dir under its own name — fix idea: also scan quote-included headers, or document "one header per slice" — minutes lost: 15
- wave1/s00401010 2026-10-20: lessons for matching.md: (a) this binary's D3D layer is the DirectX 9 SDK common framework (CD3DApplication/CD3DEnumeration/D3DSettings, d3dapp.cpp ~Dec 2002): after recalling the SDK source literally (init order `Width,Height,Format,RefreshRate` of a D3DDISPLAYMODE, `if (a > b)` operand order, `D3DSettings::DisplayMode()` returning a struct by value, `static` locals for Pause/UpdateStats) five of the seven "hard" functions matched on the first or second try; (b) cl 13.10 rejects `__thiscall` as a function-pointer attribute (C4234) — `__fastcall` with one argument loads only ECX and is identical for `__vec_ctor`/`__vec_dtor`-style loops; (c) a `switch` over sparse HRESULTs reproduces the original's decision tree and jump table only if every `case` appears in the original's body order, and a label shared by two cases (`0x8200000b`/`0x80070002`) must be listed together; (d) an `if (a) {X} else {Y}` that ends both arms in a call with the same argument registers is tail-merged by cl, so write the call in both arms (ClipCursor(&rc) / ClipCursor(0)) rather than computing the argument first — minutes lost: 0 (notes)
- orchestrator 2026-10-21 (s00401010 0x401870, byte-exact): equiv FAIL "orig calls 0x42bc40 (element dtor), ours
  calls operator delete": with identical bytes this means one of OUR relocations is resolved to the wrong callee
  (likely an unannotated element destructor), not the "plausible() rejects jmp thunks" diagnosis in the agent's
  report. Matters for the relink: a byte-exact row can still name the wrong callee — fix idea: a relocation-target
  audit (our reloc symbol -> resolved VA vs. the original's operand) over every manifest row — minutes lost: 10
- resaudit-fixes 2026-10-22: resaudit.our_insns decoded the unrelocated object, so `jmp [eax*4 + table]` had disp 0 and
  recursive descent never reached a switch's case bodies; the audit then paired the default case with the original's first
  case and reported every switch-returning-strings function as "messages shifted by one" (0x4014f0, 0x4055e0, 0x406370,
  0x407250: all false). Fixed in resaudit.py (applies same-section DIR32 relocs before decoding); resolve.our_refs has the
  same blind spot (case-body references are never aligned) — fix idea: same patch in resolve.our_refs — minutes lost: 20
- resaudit-fixes 2026-10-22: resolve.by_crt keyed lib_names by `name.lstrip("_")`, folding `_exit` (exit, 0x537ced) and
  `__exit` (_exit, 0x537cfe) into one key; the later line won, so exit(-1) ran _exit. Fixed (exact C name first).
  Same family: cl 13.10 only emits `??_U`/`??_V` (operator new[]/delete[]) when `<new>` is included; without it `new T[n]`
  and `delete[] p` call plain operator new/delete with identical bytes. Every slice using array new/delete needs `<new>` —
  fix idea: put `#include <new>` in a common header or say so in matching.md — minutes lost: 10
- src/tinyxml 2026-10-22: resaudit_all.py on `src:` module rows ignores the module's `<module>.manifest.txt` symbol: `slice.find_symbol` only reads `match/slices/<sid>/manifest.txt`, so it falls back to `// @ 0x` markers. A marker followed by a comment instead of a definition (out-of-line copies of inline members, compiler-generated `??_G`) silently resolved to the NEXT definition in the file, giving 3 false `disagree` and 7 `unverified` rows; `??4`/`??Y`/`??_G` have no name the marker/`named` patterns can express. Worked around with plain `// 0x...:` comments (no `@`) plus `symbols/slices/<module>.txt` lines. — fix idea: have find_symbol read the `src/**/<psid>.manifest.txt` row for module sources, and make `_def_name_after` stop at a blank line / the next marker before any definition — minutes lost: 15
- src/game/script 2026-10-22: cl 13.10 stops packing block-scope locals and temporaries into shared frame slots once a function has a few hundred candidates (synthetic switch: ~600 cases with three address-taken locals pack into 4 slots, ~700 cases into none; the 70 KB interpreter 0x49fa10 packs any 300 of its cases but not all 591), so every stack displacement of a huge function differs although the original packs fine. Routing the ~220 `set_lvalue(locals, <temporary>)` sites through one function-scope `__int64` cut the frame from 0x2d18 to 0x2630 and 6 KB of code — fix idea: note in matching.md that in very large functions by-value temporaries / named locals passed by address each cost a slot, and that shared function-scope scratch variables are the lever — minutes lost: 60
- src/game/script 2026-10-22: resolve.py ignores annotation addresses >= 0x1900000 (the .bss globals at 0x8ed4f58..0x8ee30ec: g_dialogs and its fields, g_meshes, g_skin_count, g_in_meta_mission) and then takes the NEXT address of the same comment (a use-site citation) and reports `annotation-implausible <use site>`; such globals can only be resolved through symbols/ lines, which a module agent may not write — fix idea: accept data addresses up to SizeOfImage and only the first address of a declaration comment — minutes lost: 15
- src/game/script 2026-10-22: equiv.prepare reads annotations only from the source and the headers IT includes with a relative path, not transitively (the module header's own includes of dialog_manager.h / agent.h were invisible: g_script, g_strtable, ItemRef::stat_21x unresolved until re-included from the .cpp), and a one-line `struct S { void f(); };   // 0x...` is not an annotation of f — fix idea: follow relative includes recursively; document both in matching.md — minutes lost: 15
- src/game/script 2026-10-22: resaudit on a 70 KB function reports ~20 false `disagree` rows with `pairs: 1`: neighbouring cases of identical shape (complete/succeed/fail/cancel_quest, check_faction/check_scene, overlay creators) are paired off by one; each had to be checked by hand against the case's own call — fix idea: give single-pair evidence its own status (weak), or anchor the alignment on the jump-table targets of a switch — minutes lost: 20
- src/game/script 2026-10-22: chk.py cannot attribute `// @ 0x` markers placed before macro-generated definitions (`SI_RANGE_CHECK(check_mesh, ...)`); it falls back to `~size` and scores a same-size sibling (all ten 65-byte validators reported as check_presentation) — fix idea: mention in chk's output when a marker exists but no definition was found after it; expand such macros — minutes lost: 10
- reaudit-b07 2026-10-23: a struct with a hand-written vtable base (`struct dev_item : dev_rec`) puts its own virtual slots AFTER the base's, so "slot 0x04" methods compiled to +0x4c; byte diffs hid it, only the harness saw it (fake-arity). Likewise a stray padding field (one missing/extra int before a std::vector member, or `int f78` overlapping the last array element's spare word) shifts every later offset; check offsetof against the disassembly's last access, not just the first. VC7.1 std::vector / std::deque members start with an allocator word (first at +4), which is why `size()` shows as `first ? (last-first)/N : 0` and reproduces the original's reloads — fix idea: add a line to matching.md 'Writing matchable source' — minutes lost: 40
- headers G3 2026-10-24: splitting a `char padN[..]` array of a shared class (GameData::pad8 -> pad8 + module + pad2c) changed the register allocation of unrelated NONMATCHING functions that merely include the header (party_screen 0x5059c0: two loads swapped, terrain_b 0x46e590: 18 bytes), even with a plain `char[16]` as the new member; the codegen_dump fingerprint flags them and equiv verdicts do not move, but a header-only layout merge is not always code-neutral — fix idea: none known (cl 13.10 numbering of members); to attribute a changed row, compile it in a `git archive HEAD` tree and diff the function bytes + relocation names with local labels (`$T123`, `$L45`) normalised — minutes lost: 25

- names-n2 2026-10-24: `resolve.source_annotations` ignores `// 0x...` addresses >= 0x1900000, so globals at 0x08ed4f58..0x08ee3c48 resolve only through symbols/ (mangled name); renaming such a global (g_panel -> g_app_obj, g_anim_table) silently turned its relocations into unresolved names until a symbols/slices line for the new mangled name was added (symbols/slices/names_n2.txt) — fix idea: raise the range check to the image end (0x9000000) — minutes lost: 30
- names-n2 2026-10-24: a codegen_dump row (nonmatching) changed only because a one-word rename of an UNUSED declaration in another header (`g_item_count` -> `g_item_kind_count` in ui/item_ref.h) reordered two loads in party_screen 0x5059c0: cl 13.10's register choice depends on the identifier set of the TU, so a pure rename is not always codegen-neutral — fix idea: gate renames with equiv (done), not with the fingerprint alone — minutes lost: 40
- names-n2 2026-10-24: `resolve.source_annotations` ignores `// 0x...` addresses >= 0x1900000, so globals at 0x08ed4f58..0x08ee3c48 resolve only through symbols/ (mangled name); renaming such a global (g_panel -> g_app_obj, g_anim_table) silently turned its relocations into unresolved names until a symbols/slices line for the new mangled name was added (symbols/slices/names_n2.txt) — fix idea: raise the range check to the image end (0x9000000) — minutes lost: 30
- names-n2 2026-10-24: a codegen_dump row (nonmatching) changed only because a one-word rename of an UNUSED declaration in another header (`g_item_count` -> `g_item_kind_count` in ui/item_ref.h) reordered two loads in party_screen 0x5059c0: cl 13.10's register choice depends on the identifier set of the TU, so a pure rename is not always codegen-neutral — fix idea: gate renames with equiv (done), not with the fingerprint alone — minutes lost: 40
- names-n2 2026-10-24: an `_ext`/`_x`/`box3_b` twin declaration exists to give the caller an opaque callee (bound with /alternatename) in the same TU as the real definition; renaming it to the canonical name makes cl inline or re-bind the call and changes the caller (shapes.cpp: 15 exact rows), and renaming a stub's constructor (`text_stream()` -> `tstream3()`) silently turns it into a plain member — fix idea: mark twins with a `// twin` tag the rename tooling can skip — minutes lost: 30
- head_rec family 2026-10-28: two doc/tool hazards found while matching the family's ctor rows (mission_items 0x4ce960..). (a) `_vtable_match`'s docstring says unverifiable slots are reported, but they are *skipped* (the row can PASS on the remaining slots agreeing); combined with the family's never-constructed records (slot 0 = the deleting dtor, an external special member nothing can annotate), every such row passes with the whole table's slot 0 unchecked — the "never constructed -> rows compare vtable bytes" note in head_rec.h cites docs/Papercuts.md but no entry described what is actually verified, which cost a full mechanism trace. (b) `resolve.slot_symbol` (equiv.py:1274) compares `sy.secno` (1-based) against a 0-based section index, so it always returns None and a vtable slot whose target is a *local* symbol is unverifiable instead of agreeing; the head_rec rows currently depend on that skip (ItemRec2's stub slots pass through it), so it is documented, not fixed — fixing it would flip ItemRec2 to FAIL until its stub slots are annotated too — fix idea: fix slot_symbol's index base and re-annotate the affected rows in the same change — minutes lost: 120

## Resolved
- pilot 2026-10-20: masked `DIR32` operands hid a wrong float constant (eps 1e-10 vs 0.0) — fixed: the verifiers
  now compare compiler constants (`__real@`, `__int64@`, `??_C@`) with what the original's operand points at.
- pilot 2026-10-20: equivalence harness ran the static CRT down to `kernel32!HeapAlloc` — fixed:
  `machine.py` routes byte-exact-identified CRT functions (`symbols/lib_names.txt` source) to its Python handlers.
- pilot 2026-10-20: harness could not map a constructed vtable — slot 0 is the deleting dtor `??_G…`, which has no
  declaration to annotate — fixed: name it (and the vtable) in `symbols/slices/<id>.txt`.
- s004f73c0 2026-10-21: `cmpdis.py` silently diffs a STALE object when `cl.sh` failed on the current source (errors are printed but the old `.obj` from a previous compile is still on disk, so the diff looks like a codegen result), and its output shows `call 9` for unresolved (external) call targets, which reads like a bogus offset — fix idea: cmpdis should compile or at least warn when the obj is older than the source, and print `<ext>` for relocations — minutes lost: 10
- s004e65e0 2026-10-22: writing a transform-ish expression with a local `vector4 operator+` (one temp per operator) blew the frame
  from 0x11c to 0x26c and the diff from 993 to 2088 bytes; spelling the same maths scalar-per-component (letting cl CSE the
  shared `w * k` temporaries) brought the function to within 10 bytes of the original. minutes lost: 10
- s004e65e0 2026-10-22: `(unsigned)((char*)end - (char*)begin) / 0x18` compiles to an unsigned `mul`/`shr` division, while the
  original's `imul`/`sar`/sign-fix sequence shows a *signed* division whose result is then compared unsigned — cast the result
  (`(unsigned)(diff / 0x18)`), not the numerator. minutes lost: 5
- s005134c0 2026-10-22: `equiv.py` compares a `char`-returning function's result as `eax&0xffff`, so a reconstruction
  that differs only in the *undefined* high byte of EAX is reported FAIL 5/5 (`orig 00008d00 / ours 00000000`; the
  original just leaves an earlier `imul` product there) and the row has to be filed as partial even though AL matches
  on every input — fix idea: for sub-dword returns compare only the declared width (AL), or grade it
  `weak(garbage-high-bits)` — minutes lost: 15
- s005134c0 2026-10-22: two `MsgLog` methods at 0x452c00 and 0x4334b0 take the same operands in *opposite* order
  (`add(int, tstring)` vs `add(tstring*, int)`); `chk.py` cannot see it (call targets are masked) and the first
  declaration made the whole 718-byte flush look wrong, while equiv pinned it to the two arguments in one line
  (`orig args=[5fe878, ffbbffbb], ours=[ffbbffbb, 5fe878]`) — declare the overload to match the original's push
  order, not the plausible C++ one — minutes lost: 20
- s005134c0 2026-10-22: spelling `Agent* a = (Agent*)(g_game->chars + idx * 0x508);` once per branch costs 249 bytes
  of diff (706 -> 457) against writing the accessor inline for every use (`g_game->troop(idx)->items[i]`): the local
  forces `add base, idx*0x508` plus a separate `lea +0xb4`, while the inlined accessor lets cl fold it into one
  `lea [chars + idx*0x508 + 0xb4]` — and that change alone flipped the register allocation to the original's
  (this->edi, id->ebx) — minutes lost: 15
- s004875e0 2026-10-22: `card.py 0x4875e0` annotated the wrong function (it labelled the 0x487520-ish helper, not `Channel::play`), so the first pass coded the wrong body — fix idea: have card.py key its annotation on the requested VA exactly — minutes lost: 20
- s004885e0 2026-10-22: a free function declared to stand in for a `__thiscall` callee (`extern void helper413750(Track*, const void*, const void*)`) silently compiles as `__cdecl`, so the caller pushes `this` and the callee faults at its first `[eax+0x1a8]`; `cmpdis`/`chk` barely notice — fix idea: matching.md already says to check the callee convention, but add "a stub *method* on the owner class is the only way to get ecx=this for a free-function-looking callee" — minutes lost: 45
- s004885e0 2026-10-22: equiv.py's "memory pool @<addr>" first-mismatch only prints the address and both values, not which side wrote (or that the other side never wrote); combined with the harness's `args [...]` list showing *harness* arguments (not the callee's stack slots) it took a `--replay -v` and manual `[esp+N]` arithmetic to see that the reads at `[esp+0x68]`/`[esp+0x6c]` in `Effect::update` happen after an intervening `push edx` but before its `add esp,4`, so `a`=param_1 and `t`=param_2 (not param_2/param_3) — fix idea: have equiv print "wrote here/never wrote" per differing region — minutes lost: 60
- s00487c40 2026-10-22: resaudit.py pairs a `g_time` `+4` operand as a `disagree` when instruction order differs between our build and the original, even though both read the same `__int64` high word — fix idea: normalise `[sym]`/`[sym+4]` pairs for `__int64` accesses before pairing — minutes lost: 10
- s004885e0 2026-10-22: `cmpobj.py --len` crashes with `IndexError` when the requested length exceeds our symbol's own size, which is exactly the case when checking whether our block is *shorter* than the original's — fix idea: clamp `--len` to min(len, symbol size) or report "our block is N bytes shorter" — minutes lost: 10
- s004885e0 2026-10-22: the orchestrator rejects ad-hoc `.venv/bin/python <script>` probes even for scripts inside allowed paths (`work/match/*.py`, `match/slices/*/scratch_*.py`, heredocs, `-m py_compile` all rejected); only `.venv/bin/python tools/**` and side-effect-free `-c` one-liners run — fix idea: document the exact policy in matching.md so agents do not burn rounds discovering it — minutes lost: 15

- headers G2-party-script-agent 2026-10-24: `tools/difftest/resaudit_all.py --help` is not parsed: it starts the full audit (every match/slices and src row) and truncates work/resaudit/all.jsonl — fix idea: argparse with --help — minutes lost: 3
- headers G2-party-script-agent 2026-10-24: sweeps/equiv started in two processes at once race on work/difftest/obj/*.part.obj (FileNotFoundError on the rename -> row reported UNSUPPORTED "harness error"); rerunning the row alone gives the real verdict — fix idea: unique temp names (pid) for the .part files — minutes lost: 10
- headers G2-party-script-agent 2026-10-24: moving a declaration with its `// 0x...` annotation into a header that the .cpp only reaches through another header makes the equiv/audit rows UNSUPPORTED ("unresolved reference") without any compile error; the audit scans only the .cpp and the headers it includes directly by relative path — fix idea: let source_annotations follow the quoted includes of the directly included headers (or print which header would have supplied the address) — minutes lost: 25

- equiv.py and run_all.py need `.venv/bin/python` (system python3 lacks capstone); a full nine-VA equiv run exceeds the 120 s tool timeout, so it must go through run_in_background.
- chk.py takes a slice id or VA list only; module stems such as `config` are rejected.
- The marker parser takes the first identifier before `(`, so `static __declspec(noinline) void f(` is read as `__declspec`; forward-declare with the attribute and put the marker on a plain `static void f(` definition line.
- equiv.py resolves a same-TU custom-ABI static to its own copy ("local copies: 1") and reports FAIL even though the bytes are exact (0x48d940 calling 0x48d830).

- 0x4d9aa0 2026-10-25: `tools/hybrid/capture.py` on a *hooked hybrid* silently destroys the hooked function:
  `build()` fills the runtime's restore-byte table from `R.Image()` (the untouched analysis image), so when a
  target's int3 fires it rewrites the first byte with the **original** opcode (`81` = `sub esp,`) over the
  hook's `e9` jump, leaving bytes 1-4 of the jmp in place — the function is garbage from the second call on and
  the game dies (log stops after "Starting new log file."). Three capture runs produced only the loader-entry
  snapshot because of it. Fix idea: `--base EXE` that reads the restore bytes from the base image, or read them
  from the file being patched. Probe side: `work/probe/cap_hybrid.py` + `work/probe/patch_cap_orig.py`
  — minutes lost: 90

- 0x4f65f0 2026-10-25: two `struct RowRec` definitions with *different* sizes (0x20 as `: named_vec` in
  `inventory_panel/widget_text.cpp`, and a modelled `char pad[0x1c8]` in `agent5/agent_vec.cpp`) share every
  mangled name, so the linker folds `std::vector<RowRec>::_Uninit_copy` onto one body. The 0x1c8 body then ran
  inside the 0x20-byte row vector: `Entries::add` grew the skeleton list, `_Uninit_copy` stepped 0x1c8 bytes
  per element, read past the single-element buffer into reused heap holding text, and the record copy ctor
  faulted on `tstring{buf=0x20353066, len=" 2 1"}`. Fix: model the size correctly (`tstring` + 4-dword vector);
  the folded stride is visible as `add edi/esi, 0x20` in our `_Uninit_copy` against the original's `0x4ebe10`.
  Lesson: a "size only" modelling struct is only safe if *every* TU that names it agrees on the size — otherwise
  any `std::vector<T>`/`_Uninit_copy<T>`/`_Ufill<T>` COMDAT folds across them. Cross-check the stride in the
  emitted code, not just the row's byte match (`_Insert_n` was byte-exact the whole time; its *callee* was not).
  — minutes lost: 150

- 0x4d10f0 2026-10-26: `item_modifier_array_dtor` was modelled on `module_load.h`'s 0x30-byte `item_modifier`
  (`tstring name; tstring value; char hidden; int count; int* ids; int* values`), freeing `ids` at +0x28. The real
  function (0x4d10f0) steps **0x50** and frees the block at +0x44 (the bone record's `vector<R44>` data), then
  destroys the string at +0. It is the array destructor of the **bone lists** (`widget_text`/`skeleton_load`
  `destroy_items` -> `Entries`, 32 x 0x50 records in the dump), not of the item-modifier table (`item_mod`, 43 x 0x28).
  At 0x30 stride the loop walked into the middle of each record, read `socket_dir`/`rotate_*` floats as a `tstring`
  and freed `0x3f800000` (`1.0f`) — the second "Start New Game" crash, in `tstring::~tstring <- item_modifier_array_dtor
  <- destroy_range <- _Insert_n <- Entries::add`. Fix: model the record as `tstring` + 0x34 pad + three dwords at
  +0x44. Lesson: an opaque callee's model must match the *caller's* element type, and "the name says modifier" is not
  evidence — read the stride (`add esi,0x50`) and the offsets it frees (`[+0x44]`) off the original every time.
  — minutes lost: 70

- 0x520c00 2026-10-26: `map.txt` carries no neighbour list for a map group. `read_map` (0x520c00) fills `count` (+0),
  the group's vertex ids (+4), `mask` (+0x24) and +0x68; `Group::Group` (0x4246f0) initialises +0x24, +0x48/+0x4c/+0x50,
  +0x64, +0x68/+0x6c/+0x70 and +0x84 — but **not** +0x28 (`nb[4]`) or +0x38 (`weights[4]`). Those reach the 62 408
  elements only through `vec136::resize(n2, Group())`, i.e. as whatever bytes the temporary's stack slot held. In our
  build that slot held heap pointers, so `score_triggers` (0x51dac0, gate `nb[k] != -1`) treated a heap address as a
  node id and `trigger_score` (0x51db70) indexed `nodes + nb*0x88` — page fault at 0x4be8a7 on Start-New-Game.
  Fix: build the resize template explicitly with `nb = -1`, `weights = 0` (game_2.cpp `read_map`). Probes showed the
  parse itself is correct: 31 307 vertices / 62 408 groups, first six groups all `count=3 mask=2` (true in the file),
  ids at +4 varying (0,1 / 173,1 / 172,1 …). Lesson: a vector filled by `resize(n, template)` inherits *every*
  uninitialised byte of the template; if the original only survives because its stack junk is benign, initialise.
  — minutes lost: 95

- 0x5213d0 2026-10-26: `build_map_trees`'s scratch buffer is sized by the *fixed offset* `scan_arrays` writes, not by the
  point count. `scan_arrays` (0x523630) writes one dword at `buffer + 0xc3500` (constant `0xcb189680`, not per element),
  and the original's buffer is the ~0xc3540-byte block at `[esp+0xc3e00]`. We allocated `new int[npts]` (125 KB on a
  31 307-point map), so the store landed past the block: page fault at 0x49c86d on Start-New-Game. Fix: `new char[0xc3600]`.
  Lesson: when a callee indexes by a fixed offset rather than by `i`, size the caller's buffer from that offset.
  — minutes lost: 25

- 0x5213d0 (open) 2026-10-26: after the map-group `nb[]` initialisation (see the 0x520c00 entry) Start-New-Game reaches
  "map mesh built." and then faults in the edge-relaxation loops of `Game::build_map_trees` (our `+0x3c5` / `+0x467`,
  `fld [pts + 9*a*4]`) on an edge whose `a` is **-1** (fault address = `pts - 36`). Probe of the edge vector:
  `npts=31307 nedges=93744 edges=27810020`, first five entries valid (`a/b` = 0/1, 1/173, 173/0, 1/172, 172/173) and
  every sampled record has `+0x20 = -1`, so some slots in `edges[]` were never written by `PolyGeom::build` (0x47ee60)
  while the walk trusts `(edges_end - edges)/0x24`. Next: count `bad_a/bad_b` over the whole vector (probe ready,
  `edge_probe.txt`) and find which append path leaves `f0c = -1`.
  Blocker for measurement: the click-through is flaky (fraction-based clicks land on "Restore a Saved Game" and die in
  `SavePanel::update` 0x44cc0c); the deterministic button position in windowed 1280x720 is `(876, 631)` and the log
  must be checked for `map mesh built.` to confirm the right item was hit.
  — minutes lost: 60 (ongoing)

- 0x4ea290 (open) 2026-10-26: an intermittent, *input-free* crash a few seconds after the main menu appears:
  `tstring::operator=`+0x24 (`0x44f9a4`, `rep movsl (%esi),%es:(%edi)` with EDI=0) writing 0x2a dwords into a destination
  whose `buf` is null but whose `len` is 42. Backtrace (stack walk): `SavePanel::update`+0x53c (0x44cc0c) <-
  `GuiScreen::process`+0x449 (0x44e519) <- `Slot::read_keys`+0x37 (0x49b4f7) <- `GameApp::Update`. So `update()` is called
  on the menu's save panel every frame and takes a branch that assigns from a computed pointer
  (`*(int*)(g_game+0x8c)*0x508 + *(int*)(g_game+0x13a00)`); the panel's `deleting` at +0x234 (or that pointer) is junk.
  Earlier the *same* instruction was reached by clicking "Restore a Saved Game", which is why it was mistaken for a
  misclick. Next: diff our `SavePanel::update` against the original 0x4ea290 (field offsets + branch order) and check
  whether the panel passed by `GuiScreen::process` is the same object our model assumes.
  — minutes lost: 35 (ongoing)

  Follow-up on the same crash: the faulting assignment is `g_main_menu.answer_no = title2` (compiled as
  `ecx = 0xa97988` + `call tstring::operator=` 0x44f980, the caller's return address 0x44cc0c). Our `operator=` is
  byte-identical to the original (0x42bc50) and calls `reserve(len)`; the destination `answer_no` has `buf == 0` with
  `cap` already >= len+1, so `reserve` (0x44f880, `cmp [edi+8], n+1; jae done`) returns without allocating and the
  `rep movsl` writes to null. So the global's tstring is inconsistent *before* the assignment: its `cap` is junk while
  `buf` is null, i.e. `g_main_menu` (ours at 0xa97900, the original's 0xa73980 via the glue) was written by something
  that sets `+8` without `+4`, or its static construction never ran. Next: check the resolve_map/glue entry for
  `?g_main_menu@@3Umain_menu@@A` and whether our static-init list constructs its tstrings.

- 0xa739f8 (open) 2026-10-26: the input-free menu crash *after* the tstring one (fixed in save_panel.cpp) is the screen
  deque: `GameApp::Update+0x1ed` (0x46f6ed) reads `edx=[0xa9797c]` (blocks), `eax=[edx+eax*4]` (a block pointer, 0) then
  `[eax+ecx*4]` -> read of 0x0000000c. The object at 0xa739f8 is the `std::deque<int>` seen as `g_screen_stack`
  (script_interpreter.h), `g_menu_vars` (facegen_misc.cpp: {pad, rows +4, count +8}) and `g_id_stack` (id_stack.h:
  {f0, blocks +4, block_count +8, a +0xc, b +0x10}); its blocks array exists but the current block is null, i.e. the
  deque was never constructed - the same missing-static-constructor pattern as g_main_menu. Next: construct it in place
  once (first frame of GameApp::Update) with our std::deque<int> and check the engine's layout (0x28?) matches, or call
  the original's deque ctor at its address if one exists.

  RESOLVED 2026-10-26 (the entry above guessed "never constructed"; it was constructed and then destroyed):
  the deque (`0xa739f8` = `g_windows + 0x78`) is healthy at every `WindowMgrW::open`/`set_state` call — probe
  `open(2/4/21) after: map=0beefef8 ms=8 off=0 size=1` — and all-zero in the `winedbg` dump at the fault (`x /6x
  0xa98978` = zeros, while `0xa98900` still held the 30 screen pointers). The writer came from probing
  `GuiScreen::process` entry/exit: healthy at one frame's exit, zero at the next entry, i.e. zeroed between the input
  pass and the following frame. Disassembling every reference to `0xa98978` in the built image gave the culprit:
  `SavePanel::update` runs `tstring_ctor` on `0xa98954/0xa98968/0xa98978/0xa98988` — the menu record's
  `title`/`+0x54`/`header`/`answer_yes`/`answer_no` — and a `tstring` ctor zeroes four dwords, exactly the damage.
  Those offsets belong to the dialog *record* (`g_main_menu.rec`; the original's `ask_yes_no` 0x48a3d0 clears `this+0x68`
  and assigns `this+0x1c/0x78/0x88`), **not** to the screen manager, whose `+0x78` is the deque (the original's
  `open` 0x494260: `lea esi,[edi+0x78]` then the deque's push_back/back). Our `main_menu` model inlined the record's
  fields into the manager, so constructing or assigning them landed on the deque. Fix: all record-field uses go through
  `g_main_menu.rec` (save_panel.cpp, cam_view.cpp, video_page.cpp). Verified: four runs, deque never zeroed, no fault
  (`menu after 28-30 s, fault: none, alive: 3`). Lesson: one global modelled as several types needs every field offset
  kept on its own object; a stray `tstring` write looks exactly like a `std::deque` reset.

- 0x513710 (open) 2026-10-26: the next crash, once a game is actually reached (`launch complete.`): `Mgr513710_s25160::m`
  (`0x513710`) does `call dword ptr [[0xa9795c] + 0x10]` and jumps to `0x3ea66666` (the float 0.325f). The object's vptr
  is our `??_7ItemA@@6B@` (built at `0x55585c`) and slot 4 holds `0x3ea66666`; `resolve_map` matched that class to
  `0x596190`, which is **not** a vtable (its slots point into the middle of unrelated functions and slots 10/11 are the
  floats 0.325f / 0.24f), so the fix-up wrote the garbage dword into our slot. Next: find the real `ItemA` vtable (all
  slots function starts) or make `fix_vtables` reject a candidate whose slots are not `img.start_set` functions.

- 0x513710 (fixed) 2026-10-26: virtual calls that land in *float constants* (0x3ea66666 = 0.325f from
  `call [[0xa9795c]+0x10]`; 0x3cb923a2 = 0.009f later, inside `GameScreen::build` -> `text_box::add_child` ->
  `ContainerW::track_bounds`) are **short vtables**: our source declares fewer virtuals than the original class, so
  the compiled table ends early and a call through a later slot reads whatever .rdata follows (a string, a float).
  `??_7ItemA@@6B@` had 1 slot against the original's 10 (`ItemA::ItemA` 0x4ceb30 stores 0x596190); `fix_vtables`
  reported "0 of 1 slots agree" and never redirected it. Fix (tools/relink/build.py): for every vtable whose
  original table starts with more function-start dwords than ours has slots and whose address is inside the copied
  .rdata, redirect our vptr stores to the glue's copy of the original table, whose slots are all relocated to our
  functions: 51 classes, 461 stores (`vtable_report.redirected_short`). Verified: no float-jump in the menu, in-game
  init (`launch complete.`) and the HUD build path for four runs. Lesson: a vtable that is too short is worse than a
  wrong one - the failure surfaces as an "impossible" EIP.

- 0x4ee910 (fixed) 2026-10-26: the RGL ERROR dialog "get_object failed for: ui_scrollbar" (the user's screenshot):
  `make_scroll_track` asked for `ui_scrollbar` / `ui_scrollbar_handle`, but the original pushes 0x592748 = "scrollbar"
  and 0x592734 = "scrollbar_handle" (0x4ee9a3 / 0x4eea05), and `CommonRes/core_ui_meshes.brf` holds those names with
  no `ui_` prefix. Audit of all 71 `get_object*` call sites with a literal: only this one named a string absent from
  the original exe.

- 0x488ad0 (fixed) 2026-10-26: `make_scroll_track` called vtable slot +0x80 (`MissionUi::set_track(int i, mesh* p)`,
  `ret 8`) with **one** argument through the cdecl `VCALL1` macro, so the object landed in `i` and the callee faulted
  at `[esi + edi*4 + 0x108]`. The original passes both (`0x4eea5e`: push edi / push 0; `0x4eea6b`: push ebx / push 2).
  Fix: typed `set_track(0, m1)` / `set_track(2, m2)` (the declaration was added to cs_common.h's MissionUi view).
  The `VCALL0/1/P/2` macros still cast to cdecl: MSVC 7.1 rejects `__thiscall` in a pointer cast (C4234), so a
  thiscall slot cannot be spelled there - call the class's method instead when it is known.

- 0x43ac03 (naming) 2026-10-26: `*(tstring*)((char*)g_game + 0x1a210) = *(tstring*)(rec + 0x28)` is
  `g_game->mesh1a210 = MenuRec::mesh`: `rec` is a 0x60-byte menus.txt record (`g_menus + idx*0x60`, `+0x28` is its
  `mesh` tstring), and `Game+0x1a210` is a tstring that `Block::read` (0x51d100) constructs and 0x492f6f resets - it
  is part of the serialised game block. Named `Game::mesh1a210` and `MenuRec::mesh` at the call site.

- 0x43ac03 (open, cleanup) 2026-10-26: **idiomaticize the offset views.** The reconstructions read objects through
  raw offsets and casts (`*(tstring*)((char*)g_game + 0x1a210)`, `GAME_AT(int, 0x13aec)`, `rec + 0x28`,
  `*(int*)((char*)self + 0x178)`, `VCALL1(self, 0x80, 0)`), which is what let the same offset mean two different
  objects in different files (the `g_main_menu`/deque collision, the `MenuRec::mesh` line above). Work to do, in
  order of payoff:
    1. one header per object with the *complete* field list (Game/GameData still has 20+ `padXXXX` runs and unnamed
       `fXXXX` fields between named ones; MenuRec is done),
    2. take the field's own type (tstring, vector, deque) from the original's constructor/reader, never from a
       `char*` cast,
    3. replace the `VCALLn` macros (cdecl casts, wrong for thiscall slots) with the class's method when the class is
       known, and a *typed* slot view (`struct Slot80 { virtual void set_track(int, rgl::mesh*); }`) when it is not,
    4. name the remaining `f1a2xx`-style fields from their users (`Block::read`/`Block::write` order is a good
       oracle: every serialised field appears there).

- 0x48f020 (fixed) 2026-10-26: `FieldPanel::relayout` crashed with EIP = a *float* (0x3cb923a2 = 0.00905f) right
  after `relayout` returned. The original reserves `sub esp,0x10` and the trailing `get_state(&v[2])` writes **two**
  floats at `v+8`, filling all 16 bytes; our `float v[3]` (12 bytes) let the second one land on the saved frame, so
  the return address became that float's bit pattern. Fix: `float v[4]` with the frame size in the comment. Lesson:
  match the original's *local buffer* size, not just its call count - a callee that writes a 2-float state can be one
  float wider than the source's array.

- NavData MapEdge (fixed) 2026-10-26: `build_map_trees` crashed in the relaxation passes (`fld [pts+idx*4]`, idx -1).
  `struct MapEdge` padded `0x24-0x18` after `b`, which is 0x0c bytes - the record is 0x24, so the tail must be
  `0x24-0x14`. `sizeof` was 0x20 while `nedges` divides by 0x24, so the loop advanced 4 bytes per edge, drifted off
  the array and read a record whose `a` was -1. Fix: `pad14[0x24-0x14]` plus `chk_map_edge_size`. Lesson: a `padX`
  whose length is written as `TOTAL - wrong_offset` silently changes `sizeof`; assert every documented size.

- Widget slots +0xb8/+0xbc (fixed) 2026-10-26: `ui_widget.h` had `enable()` at +0xb8 and `disable()` at +0xbc, but
  0x488cb0 (+0xb8) *sets* bit 1 of the flag word at +0x104 and 0x488cc0 (+0xbc) *clears* it. So every `->enable()`
  disabled its widget and every `->disable()` enabled it, which is why no menu item responded to a click.
  `MissionUpdater::update` happened to be self-consistent (it wants "clear disabled" then "disable the flagged
  rows"), so flip both the header and its two call sites.

- build_map_trees cdecl casts (fixed) 2026-10-26: five virtual calls were spelled
  `(*(void(**)(...))((char*)*(void**)obj + N))(...)`, i.e. *cdecl* function-pointer casts of `__thiscall` slots. The
  callee gets `ecx` = the vtable pointer, so `entity::set_fbc` wrote to 0x5635e8 (inside .rdata) and the game died on
  the first map-tree cell. The original always does `mov ecx, obj; push arg; call [vt+N]`
  (0x521c90 slot +0x18 with `&zero`, 0x52308a slot +0x10, 0x5231d7 the scalar deleting destructor with `push 1`).
  Fix: `MapObjView` (a typed thiscall view) instead of the casts. Same class as the 0x488ad0 `set_track` bug.

- 0x413c50 add_vertex (fixed) 2026-10-26: `build_map_trees` called
  `add_vertex(idx, uv, &col, prec, 0)` - the last parameter is a `const int*` that the callee dereferences, so the
  literal 0 crashed at `mov ecx,[ecx]` (EIP 0x5295d7). Every other call site passes `&zero`. Lesson: a `0` in a
  pointer parameter is not "unused" - check the callee before passing it.

- 0x49bea0 add_order_arrow (fixed) 2026-10-26: `game_2.h` declared `add_order_arrow(int, int)` and callers passed
  `(id, 0)`, but the original is `ret 4` (one int). The 2-arg spelling resolved to a *different* mangled symbol, so no
  warning fired; the callee also dispatched `create_entity` through a virtual slot where the original calls 0x428440
  directly (`mov ecx,[0xa732b4]; push 9; call 0x428440`), and the object at g_level is not polymorphic. Fix: one
  argument and the direct call. Lesson: argument count must be checked against the callee's `ret N`, not the header.

- config table (fixed) 2026-10-26: **every UI offset was 0** because the settings table was read through the
  *original's address* instead of the symbol. `config_table.h`'s `CONFIG_ENTRIES`, `char_sheet.cpp`'s
  `config_text`/`config_index`, `cs_panels.h`'s `config_float` and `note_window.cpp`'s `CONFIG` all spelled
  `0xa44240` / `0xa4425c`, but the relink places `g_game_vars` at **0xa691c0** (`mb_standalone.map`), so the
  lookups hit a dead copy of the table: `find()` still returned the key's index (831 entries load fine) while
  `atof(value)` saw an empty buffer. Symptom: the whole character sheet stacked at y=0 and every menu row, slider
  and hint collapsed to the origin. Fix: view the *symbol* (`&g_game_vars`, `&g_config + 0x1c`). Verified by probe:
  `char_b_skills_y_a` went from `v=0` to `v=0.0175` and the sheet's rows, attributes, skill names and points all
  render at their configured positions. Lesson: a hard-coded original address in a *macro* escapes
  literal_addrs.py's fix-up; grep for `0xa4xxxx` literals when a table looks empty.

- string keys (fixed) 2026-10-26: 13 of the 541 `"ui_*"` keys our source passes to `g_strtable.lookup` do not
  exist in the game's language files (`languages/*/ui.csv` + `Data/*.csv`). The visible one was
  `ui_skillts` -> the sheet printed the raw key in the corner; the real key is `ui_skill_points`. Also fixed
  `ui_health_pyer` -> `ui_health_player`, `ui_order_dismount` -> `ui_order_dismount_toggle`,
  `ui_order_hold_position` -> `ui_order_hold`, `ui_order_mount` -> `ui_mount`,
  `ui_need_to_learn_prisoner_mgmt` -> `ui_need_to_learn_prisoner_management`, `ui_factions_report` -> `ui_factions`.
  Remaining suspects (`ui_font`, `ui_item_shadow`, `ui_font_background`, `ui_create_brf`, `ui_audio_options`) are
  resource names, not strings. Repeatable check: the one-liner in tools/matching/README (collect keys from
  `game/languages/*/*.csv` + `Data/*.csv`, compare with the `"ui_..."` literals in `src/`).

- signature drift vs the original (tool) 2026-10-26: `tools/matching/slot_sig_audit.py` (textual slot scan) is too
  noisy to act on (1693 drift rows, all alias spellings). `tools/matching/sig_vs_original.py` is the usable
  version: it pairs our mangled name with the original's at the same VA via `resolve_map.json`, and MSVC expands
  typedefs in mangling, so `mat3*` vs `rgl::mat3*` cannot produce a row. Output is bucketed VIRTUAL / CONVENTION /
  ARITY / RETURN / OTHER: currently **20 rows**, of which 2 were real: `~mesh` was compiled non-virtual
  (`??1mesh@rgl@@QAE@XZ`) while the original's is `??1mesh@rgl@@UAE@XZ` - `strat/gui_manager.h`'s view declared it
  without `virtual`; fixed by making that view polymorphic while folding the vptr into `d0` so every offset stays
  put (`char d0[0x10c - 4]`), and the row disappeared on the next run. `~RecR` (same class of issue in
  `inventory_panel/map_info.cpp`) is still flagged and left documented.

- vtable slot loss (tool + fixed for Rec428/FaceGen) 2026-10-26: a class whose source declares fewer virtuals than
  the original compiles a *short* vtable, and every slot past its end is unreachable - the relink can only patch
  slots that exist, so the functions those slots point at never run. The visible case: the table at 0x595c68 (our
  `Rec428`, the face-generator / character-creation record) has nine entries, and slot +0x08 is `FaceGen::init`
  (0x4f49d0), slot +0x0c `CamView::update_model` (0x4ea8e0). Our source declared one virtual, so `init` never ran,
  ~40 widget fields stayed zero and the first slider read crashed (`0x44a09d`, reading `0x3f800000` = 1.0f) the
  moment the character sheet opened; `sl_hair_color`/`sl_age` are not null, they are the *screen's* fields at those
  offsets, so null guards do not help. Fix: declare all nine slots on `Rec428` (placeholders; the relink writes the
  original's own functions into them) and give it the constructor the original has (`new Rec428` left the body as raw
  heap, so slot +0x04 `release` freed garbage at +0x178 and `~tstring` crashed on 0x3c6d9166). Check:
  `tools/relink/reach_audit.py` - it reads each original table (bounded by the next known table) and lists the slots
  our build is missing, with `--strict` for a build gate. Before the fix: 38 tables / 289 slots. After: 38 / 281.
  Declaring the nine slots on the shared base (`head_rec.h`, 0x5947d8) instead fixes ~20 tables at once but pushed
  startup into a *new* crash (`options_panel::export_path` called with a null `this`), so that is left for later and
  the wide short-table family (`item_head`/`item_base`/`ItemRec`/`MissionBlocker`/`Buffer`/`UiC`) is still open.

- vtable slot declaration (2026-10-27): every vtable the original has is now declared at its full length; the
  patch on `??_7C4c3290@@6B@` fixed the last 23 record tables (the `head_rec` family: `head_cam`, `head_obj2`, 15
  `Rec*`, 6 `C4c*`), each 7 -> 9 slots. `tools/relink/reach_audit.py` reports **0 tables / 0 slots unreachable** and the
  game reaches `Finished All...`. What was fixed and verified (game runs): `??_7Buffer@@6B@` (widget base) 29->49,
  `NodeA` 50->51, `Event` 2->3, `PlacedObj_s2f250` 2->3, `strategic_entity` 28->29, `ItemA` 9->10,
  `item_head`/`item_base`/`ItemD_vt` 1->9.
  The base record table `0x5947d8` is **seven** slots, not nine: entry 7 is the next table's first dword (a CRT
  function, not a slot) and declaring eight makes the engine call it and die inside `TiXmlBase::StringEqual`; seven
  runs. `reach_audit.py` carries that as `VERIFIED_SLOTS = {0x5947d8: 7}`. Each *derived* table does have nine entries
  (dtor, release, init, update_model, ..., always1 twice).
  The reason the 46 extra declarations used to crash, found by bisection to `C4c3290` alone: a record table's match is
  *suspect* under build.py's agreement test (our declared virtuals sit in a different slot order), so its slots are
  never rewritten; the fallback that then points the vptr stores at the glue copy of the original table only fired when
  our table was **shorter** than the original's (`e["slots"] < n_orig`). With the full length declared, our unpatched
  table was used instead: its slot 7/8 returned nothing where the original's `always1` returns 1, the menu state machine
  took the wrong branch and nothing ever pushed a root widget, so `GuiScreen::process` ran on an empty vector
  (`vec_back` reads +4/+8 = end/cap) and faulted. Fix: in the suspect branch, redirect to the copy whenever the
  original's table is real, regardless of our table's length.

- vtable "match suspect" (2026-10-27): `build.py`'s agreement test compares, slot by slot, the address in our compiled
  table with the address of the function we define under the *original slot's* name. Every record/vtable family whose
  slots hold per-class functions (the 23 record tables, `Buffer`, `Item*`, `Iface*`: 49 tables) therefore reads "1 of 9"
  and is classed suspect - the slot *positions* are right (vfunc + d01..d06, derived d07/d08), but the placeholder bodies
  cannot be address-identical to `reset_gui`, `init2@MeetingWindow`, `always1`, ... since one base declaration cannot
  carry 23 different per-class names. So this is a naming artifact, not a reordering, and reordering cannot fix it: the
  slots a class overrides would have to be *our reconstructions of that class's* functions under the original names.
  Correctness is unaffected because such tables are pointed at the glue copy of the original table with every slot
  relocated to our definition (the redirect now applies whether or not our table is short).

- New Game crash in a tstring destructor (2026-10-27): `Start a New Game` died in `tstring::~tstring` -> `free(0x3c6d9166)`
  (an uninitialised pointer, the same value the menu-state FaceGen::init case produced). The bt names `rec_178::release`,
  but that function is a `return;` stub, so the frame is a stale stack value. `~tstring` now checks that `buf` looks like a
  heap address, logs `tstr_bad.txt` and skips the free: with it, New Game runs through character creation to the world map
  (reached gender -> background -> ...). This is a diagnostic, not the fix: the real cause is a tstring member that is
  destroyed without its constructor having run, still to be pinned down by the `this` in `tstr_bad.txt`.

- byte-exact wall: WinLicense tags (2026-10-27): `CharSheet::update` (0x4eeb10) cannot be byte-exact: the original
  brackets its two game-key blocks with four 18-byte WinLicense records (`EB 10 "WL  "`, ids 0x0c/0x0d) at 0x4ef333,
  0x4ef36d, 0x4ef3a9, 0x4ef3eb. cl 13.10 emits those bytes only through `_emit`, which asm_audit forbids (four slices
  already record the same blocker). With the tags hand-emitted in scratch the remainder is the original's *shared zero
  register* (`xor ebp,ebp` at entry, reused by four comparisons and a `push`), which cl 13.10 only materialises for the
  attribute loop - the same wall as s00462e20 / s004529e0. Measured: 678/686 instructions identical, 6 changed,
  3 orig-only, 2 ours-only; equiv WEAK (403 inputs, no mismatch).
- equiv.py seeding: the input generator is seeded with the slice-id *string*, so the same source+VA can flip between
  "UNSUPPORTED 0/363 valid" and "WEAK 403 valid". Running the old batch id with `--src match/slices/<id>/<id>.cpp`
  restores usable inputs.

- integration builds during a slice wave (2026-10-27): `build.py` links every compiled object, `match/slices/*/<id>.cpp`
  included, and resolves duplicate definitions "from its row's module". While slices are being written, a slice that is
  only half verified can therefore replace the src version of a function in the game exe (10 such duplicates in one
  wave build); the run then hangs at the character sheet with 0% CPU. Playthrough verification must be done either
  before a wave starts or after it lands, or with the slice objects removed from `work/difftest/obj/`.

- `--uninit-mem` (default ON) excused a _missing zeroing store_ (2026-10-27). `FILL_BYTES` was derived mechanically as
  `p & 0xFF for p in FILL_PATTERNS`, so the sentinel `0x0BB0BB00` contributed `0x00`, and the partial-copy branch of
  `unobserved_fill` then excused any dword whose differing bytes all happened to be fill bytes. Measured:
  `unobserved_fill(v=0x00000000, base=0xDEADBEEF) == True` — i.e. "the original zeroes this and our build never stores
  it", one of the commonest real divergences, was dropped from the comparison. This shipped default-ON, so it could
  turn a real FAIL into WEAK (not into PASS: the dword still has to differ and our side still has to hold the input).
  Fixed by narrowing the predicate rather than the bookkeeping — see the two positions below.

  **Position A (narrow the predicate; keep one merged flag set).** The defect is what counts as evidence: `0x00` is not
  evidence of non-initialisation, it is the most common intended value in the binary; nor are `0x0D`/`0x1D` from
  `BAADF00D`/`0x0D1D0D1D`. The only measured case that needs the partial-copy branch is `movb 0xf(%esp),%cl;
  movb %cl,0x44(%esi)` — an uninitialised `0xCC` stack byte copied over one byte of a field (`cls_map 0x4d99d0`,
  `config 0x4bd4d0`). So: treat a dword as a leftover only when it is one of `FILL_PATTERNS`, or when exactly ONE byte
  changed and that byte is an MSVC stack-fill byte (`0xCC/0xCD/0xDD/0xFD`). Nothing else. On the flags: the refusal
  test already requires our side to hold the baseline (`y == t0`), so the hole was in the fill predicate, not in the
  merge — two bitmaps would add state and a class of "which side's flags are live at the moment of the comparison"
  bugs for no measurable gain. Landed: `FILL_BYTES = {0xCC,0xCD,0xDD,0xFD}` plus a single-differing-byte rule
  (`len(diff) == 1`).

  **Position B (the reviewer's: drop the bad bytes _and_ split the flags per side).** Agree on the byte set (and go
  further: drop `0x00`, and arguably `0x0D`/`0x1D`), but also replace the single merged `write_flags` bitmap with two
  (orig-wrote / ours-wrote), so an original-side write whose value merely looks fill-like can never be excused.
  Not required for soundness under position A, and deferred: if we later want it, its value is in the *reason string*
  ("ours wrote it, the original did not") — a diagnostic — rather than in the verdict rule.

  Evidence for both: `python -c "import sys; sys.path.insert(0,'tools/difftest'); import equiv as E;
  print(E.unobserved_fill(0, 0xDEADBEEF), E.unobserved_fill(0xBF5B80CC, 0xBF5B8056))"` → `False True` after the fix
  (`True True` before). Guard test: a mutant of `Font::Font` that *writes* `0xCCCCCCCC` into the very dword the rule
  exists to excuse still FAILs.
