# Papercuts

Friction found while matching: tool bugs, unclear docs, tricks that had to be rediscovered. Agents append
entries under **Open**; the orchestrator fixes the tool or folds the lesson into `docs/matching.md`, then
moves the entry to **Resolved** with what was done.

Format (one entry per problem):
```
- <slice id> <YYYY-MM-DD>: <what cost time> — fix idea: <…> — minutes lost: <N>
```

## Open

## Resolved
- pilot 2026-10-20: masked `DIR32` operands hid a wrong float constant (eps 1e-10 vs 0.0) — fixed: the verifiers
  now compare compiler constants (`__real@`, `__int64@`, `??_C@`) with what the original's operand points at.
- pilot 2026-10-20: equivalence harness ran the static CRT down to `kernel32!HeapAlloc` — fixed:
  `machine.py` routes byte-exact-identified CRT functions (`symbols/lib_names.txt` source) to its Python handlers.
- pilot 2026-10-20: harness could not map a constructed vtable — slot 0 is the deleting dtor `??_G…`, which has no
  declaration to annotate — fixed: name it (and the vtable) in `symbols/slices/<id>.txt`.
