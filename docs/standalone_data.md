# Recreating the original's static data in source (standalone link)

Goal: `tools/relink/glue.py` must stop copying the original's `.rdata/.data/.bss` into `mb_data.obj`. Every data
label the link still needs from that copy is a failure. This file is how a datum goes from "label in the copy" to
"definition in our source". Pilot module: `cls_map` (66 labels, see the end).

## 0. First look at the label

`work/relink/standalone/build.json` and `indep_list.json` list a label by mangled name. The same address is often
referenced under several names with different types (`g_menus` is a `void*` here, a `GameMenu*` in gui_edit, a
`char*` in agent_hud). Run the alias listing before touching anything: every name at one address is the *same* datum,
and defining it in one module while another keeps its own `extern` makes two storage locations (the data gate fails,
and writers and readers stop seeing each other). So there are four cases:

| case | what the original had | what to write |
|---|---|---|
| fake constant | `.rdata` float/dword used as `x * g_01` | delete the extern, write the literal (`x * 0.1f`). Code bytes stay equal: relocations are masked and cl pools the same `__real@` constant. Do it even when other modules share the label; only your reference goes. |
| string label | `.rdata` string used as `tstring k(g_key_x)` | the literal `tstring k("ui_x")` |
| global used only here | `.data/.bss` object | a definition in the module, `// 0xADDR` annotated, declared once in a header |
| field of a bigger object | `g_cfg + 0x54` named `g_bpp`, `g_options + 0x1c` named `g_char_detail`, `g_a73314` (= `g_mission_ctx.mode`) | no label at all: use the member of the canonical object (`g_cfg.display_bits`, `g_options.i1c`, `g_mission_ctx.mode`); add the member to the shared struct if the padding hides it |

## 1. Where definitions live, names, annotation

- One TU defines, one shared header declares. The definition goes in the module that uses the datum most (or owns
  the object's code); put it in a `---- data this module owns ----` block right after the includes. The declaration
  goes in the module's header, or in the object's canonical header when one exists (`src/game/menu_rec.h`,
  `src/game/game_globals.h`, `src/include/game_recs.h`, `src/game/config/core.h`, `src/rgl/options.h`). Delete every
  per-module `extern` of the same global; include the canonical header instead (listed directly in the .cpp: the
  resolution audit reads only a source's own includes).
- Keep the name the repository already uses (the resolver also maps by name). A definition always carries the
  annotation on its own line: `float g_5a9f24 = 0.65f;   // 0x5a9f24 (what it is)`. The harness
  (`tools/difftest/resolve.py`) and the data gate (`data_size_audit.py`) read `// 0xADDR` to bind name -> address.
- Never write the address as a number in code, never a placeholder, never a glue alias.

```cpp
// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
int g_video_played;                              // 0x5e2a10 (.bss: no initialiser, size from the code)
float g_5a9f24 = 0.65f;                          // 0x5a9f24
float g_5a9f28[2] = { 0.0f, 0.21450001f };       // 0x5a9f28 {x, y}; [0] is set by static_init_547a60
```

## 2. gen_data.py

`.venv/bin/python tools/relink/gen_data.py SYMBOL|0xADDR [--type T] [--header H] [--name N] [--const] [--extent]`

It reads the image once (`tools/difftest/resolve.py Image`, bss reads as zeros) and prints the definition line to
paste (its output is reviewed source; nothing is read at link time).

```
$ gen_data.py 0x5a9f28 --type 'float[2]' --extent
float g_5a9f28[2] = {0.0f, 0.21450001f};  // 0x5a9f28
// item 0x5a9f28..0x5a9f30 (8 bytes, borrowed from image); instruction uses at +0x0 +0x4
$ gen_data.py 0x59736c --type 'char[]'          ->  char g_59736c[] = "ui_return_to_game";  // 0x59736c
$ gen_data.py 0x5aa008 --type 'struct options' --header src/rgl/options.h --name g_options
options g_options = {/*i00*/ 3, /*i04*/ 3, /*f08*/ 0.4f, ...};  // 0x5aa008
$ gen_data.py 0x5924a0 --type 'void*[4]'         (pointers print as the symbol our source defines there)
$ gen_data.py --items 0xa732f0 0xa73450          (items of an address range with the offsets the code uses)
```

- Types: scalars, `T*`, `T[N]`, `char[]` (one string), `struct NAME` from `--header` (simple `T name;` members,
  `int a, b;`, arrays, nested structs, methods skipped), `raw[N]` (dwords; only where the original itself was a number
  table). Floats print as the shortest literal that round-trips through float32 (`0.21450001f`, not `0.2145f`:
  the check is on the bits). A dword in the image with no symbol of ours at that address prints
  `/* TODO: no symbol for 0x... */ nullptr` and the tool exits 2: define the target (or fix its annotation) and
  regenerate. A `TODO` is never committed.
- Member-function pointers and vtable slots print as `&Class::name`; cast them for the table's element type.
- `--extent` shows the data_coverage.py item (`data_items()` in `tools/relink/data_coverage.py`): start, end, class and
  every offset inside it that an instruction of the original uses. Items are cached in
  `work/relink/data_items.json`.
- A value read from the image is the *pre-initialisation* value. Where the original's dynamic initialiser overwrote
  it (`static_init_*`), see section 4.

## 3. Neighbour indexing

Code that reaches one global through the address of another (`g_a73348 + kb`, `OPT_F(o)` over `g_music_volume`,
`g_a732fc[side]`) only works while the copy keeps the original's adjacency. The rule: the offsets the code
uses (`--extent`, `--items`) must lie inside ONE object whose members are real. Examples done in `cls_map`:

- `g_ragdolls .. g_particle`, `g_scale_x/y/z`, the `orig::g_music_volume ..` family: all fields of `options g_options`
  (`g_options.i1c`, `g_options.f80`).
- `g_a73314/g_a73348/g_a73428/g_a7342c/g_a73430/g_a73438`: members of `MissionCtx` (`mode`, `grid[0x38]`, `cols`,
  `depth`, `rows`, `side`); `g_a732fc[side]` = `party[side - 1]`; `g_mission_ctx[i]` becomes `((int*)&g_mission_ctx)[i]`
  (an honest table view of the context, not a second label).
- `g_hinstance`, `g_init_flag`, `g_fullscreen_flag`, `g_res_scale`: `Core` members at +0x358, +0x354, +0x346, +0x008
  (`using CD3DApplication::m_bStartFullscreen;` publishes a protected framework member, the pattern core.h already
  uses).
- `g_bpp` is `g_cfg.display_bits`.

If a struct's padding hides a member, replace `char padN[..]` with the members (keep the offsets: the comment
`// +0x48` and the following member's offset are the check; `data_size_audit.py` flags a definition smaller than a
declaration of its address).

## 4. Constructors and static_init

`src/game/static_init` hand-decompiles the original's dynamic initialisers (`static_init_5479c0` ... run from the
CRT initialiser table). The idiomatic end state is the real definition `Foo g_foo;` with its real constructor and the
hand stub gone. Never define it and keep the stub (double construction). In the pilot this applies to:

- `g_game_keys` (0xa73730, `memset(.., 0xff, 0x4a*4)` + `fn_4c2ea0`), `g_mission_ctx` (0xa73300, `MetaMission::init`),
  `g_presentations` + `g_main_menu_kind` + `g_menu_kind_count` (0xa75f2c.., `rec_table` ctor + `RefList::retire` dtor):
  constructed by `static_init_*`. Left for the static_init owner; cls_map only declares them.
- `g_5a9f28[0]`: the original computes `1.0f - g_5a9f20` at startup (`static_init_547a60`). cls_map defines the pair
  (`{0.0f, 0.2145f}`), static_init keeps writing element 0 through the shared declaration. The static_init owner
  should turn it into `float g_5a9f28[2] = { 1.0f - g_5a9f20, 0.2145f };` and delete the stub.
- Objects with constructors that static_init does not construct (`tstring g_meeting_name`, 0xa70464, four other names
  for the same string in other modules): a plain definition would add a ctor call the original does not run at that
  point; resolve together with the owners of the other names.

## 5. Pitfalls met

- Same address, several mangled names: unify through the canonical header first (case table above). `indep_list.json`'s
  `mods == ['cls_map']` lists *names*; the alias listing shows which of them are truly sole.
- A const table defined in the TU that reads it can be constant-folded by cl: check `chk.py` before and after.
- Floats that are a computed constant: `rand() % 0x3dc9 * (1.0f / 0x3dc9)` is the literal 0x54a6bc; reproduce the
  expression the programmer plausibly wrote when the bits match (`1.0f / 15817`).
- `sed` on macOS: use python for multi-line edits; a scripted rename must not touch `orig::` prefixes of other
  modules' aliases blindly.
- bss has no bytes in the image (`Image.read` is short); gen_data zero-fills. For bss the right size comes from the code
  (`--extent` and the type), not from the image.
- When a gate fails with "two storage locations", search the other names at the address
  (`python3 scratchpad alias listing` = group `indep_list.json` by `addr`).

## 6. Procedure per module (what the fan-out does)

1. `grep` the module's labels in `indep_list.json`; group by address and list every other name at that address.
2. Fake constants / strings: literal. Fields of canonical objects: member access. Sole globals: definition.
3. `run_all.py --manifest <module manifest>` and `chk.py <cpp> <nonmatching vas>` before and after; no byte-exact row may
   be lost, chk counts may not rise.
4. `build.py --out <scratch>`: the glue data-label count in `build.json` drops by what you converted; the data gate
   passes.
5. Spot-check `equiv.py <module> <va> --src ...` for two or three touched functions.
