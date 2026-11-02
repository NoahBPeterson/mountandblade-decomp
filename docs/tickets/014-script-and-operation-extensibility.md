# 014 Modding: make the Module System interpreter extensible

Area: modding | Effort: L | Impact: very high | Depends on: 004; benefits from 006

## Summary

Warband modders got extension through external binary patches (WSE-style). With the engine as source, the interpreter should offer a first-class, data-registered operation table, new opcodes without touching a 4,585-line switch,
and a clean place to add game hooks or an embedded scripting language.

## Evidence

- `Operation::execute(__int64* locals, int* context, int* error)` 0x49fa10 (src/game/script/script_interpreter.cpp, 4,585 lines; NOTES.md): `switch (opcode - 30)` over a 2,356-entry jump table (0x4ae458), **591 real cases**; unknown ids hit
  `sprintf("Unrecognized opcode %d.")` (line 4571). Control opcodes < 30 are handled by the block runner 0x4b09e0. Flags: `op::neg` 0x80000000, `op::this_or_next` 0x40000000. Names for every opcode are in
  `src/game/script/script_opcodes.h` (622 lines, `namespace op`).
- Operands are 64-bit values with a type word (`__int64 params[16]`, `int types[16]`); results via `set_lvalue` 0x49d500 (register, global variable or local). **16 operand slots** per operation.
- State: `reg[128]` 64-bit registers and `str[128]` string registers (dialog_manager.h:47-49), 128 position registers of 0x40 bytes (script_interpreter.h:289), dialog lines `list[0x400]`/`sel[0x400]` (dialog_manager.h:75).
- Validators (`check_troop`, `check_faction`, ... 0x49cc60-0x49d4b0) print "script errors" but several are intentionally weak (NOTES.md lists "original behaviour reproduced": e.g. `party_template_set_slot` bounds by the template count).
- Module data are text tables compiled by the Python Module System (`Modules/Native/*.txt`: scripts.txt, triggers.txt, simple_triggers.txt, mission_templates.txt, presentations.txt, menus.txt, ...; 42 files in Native). The engine parses them in `parse_module_info`/`load_module_files` (module_files.cpp, trigger_vec.cpp:793).
- Shader/material technique names and `skeleton_bodies.xml` bone definitions are also data (ticket 009/011) - extension points mods already use.

## Detailed plan

1. **Registry** `ops::register(id, name, arity, flags, handler)` with handler signature `bool (OpContext&)` where `OpContext` gives `p(i)` (typed accessors with range checks), `set(i, v)`, game singletons. Port the 591 cases mechanically
   from the switch into handlers (script: group by the existing opcode ranges; the comment blocks in the source mark them) and keep the old switch only as the fallback during migration (`default:` -> registry lookup, so new opcodes need zero core edits).
2. **Opcode id space**: reserve ranges for mods (e.g. `>= 0x10000`) and ship a generated `header_operations_ext.py` so the stock Module System (python) can emit them; keep ids < 3000 frozen so existing Native/Warband-1.011 mods load unchanged.
3. **Name table**: expose `opcode_name(id)` for error messages (improves "Unrecognized opcode" reports with the operation record index; today `g_cur_opcode` 0x5e25a0 is global state).
4. **Hooks**: events for mission start, agent spawn, damage, item use; a C ABI `mb_mod.h` loaded from `Modules/<name>/plugin.(so|dylib|dll)`; every handler runs on the main thread (012) unless flagged `parallel_safe`.
5. **Optional embedded language** (Lua or WASM) for new logic: expose the same `OpContext`. Decide after 1-4; the registry makes it additive.
6. **Limits that scripts hit**: 128 registers/string registers/pos registers (016) become `std::vector` with a raised ceiling; 16 operands stay (format-bound), but `call_script` params can be raised if the text format allows.
7. Validators move onto the registry as per-op `validate` functions; add a `--check-module` CLI that runs them over a module without starting the game.

## Code sketch

```cpp
// src/game/script/op_registry.h
struct OpContext { __int64* locals; int* ctx; Operation& op; __int64 p(int i) const; void set(int i, __int64 v); };
typedef bool (*OpFn)(OpContext&);
void op_register(int id, const char* name, int min_args, int max_args, OpFn fn);
// migrated case:
static bool op_store_random(OpContext& c) { int hi = (int)c.p(1); c.set(0, hi > 0 ? rand_below(hi) : 0); return true; }
OP_REGISTER(store_random, op::store_random, 2, 2, op_store_random);
```

## Risks

- A 70 KB function split into 591 handlers changes codegen: fine for the portable build, and the MSVC build keeps the faithful switch (the registry replaces only in the portable configuration; keep a test comparing results of both over recorded operation traces).
- Original quirks (see NOTES "reproduced deliberately") are now API: document them or change them under a `compat` flag.
- Mods compiled against the registry become engine-version specific; version the ABI.

## Acceptance

A sample plugin adds `my_store_dist_squared` and a Native-compatible module uses it; all 591 ops pass a recorded-trace regression; `Unrecognized opcode` messages include the module file and line index.

## Effort

L (6-10 wk; porting handlers is mechanical, the hook/plugin API needs design).
