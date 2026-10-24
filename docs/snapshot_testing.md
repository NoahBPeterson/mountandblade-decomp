# Snapshot-driven equivalence testing (design)

Status: design, 2026-10-24. Nothing below is built yet.

## Why

`equiv.py` tests a non-exact function by running the original and ours under Unicorn on random
inputs ("pointer soup" objects in a lazily generated pool). That proves a lot for leaf code and very
little for code that walks real game objects. Verdicts on the module `nonmatching.txt` rows
(free-text notes, so approximate):

| verdict | rows | |
|---|---:|---|
| PASS | ~190 | outputs matched, every edge covered |
| WEAK | ~330 | outputs matched, some edges never reached |
| UNSUPPORTED | ~360 | could not be run |
| unparsed | ~230 | note does not state a verdict in the usual form |

Causes of the UNSUPPORTED rows:

| cause | rows | fixed by snapshots? |
|---|---:|---|
| invalid inputs (`_purecall` on soup vtables, faults, pool limit, fake-arity, budget) | 128 | yes, if the game executes the function |
| imports (FMOD, D3D, file IO) | 8 | yes, with recorded import results (phase 2) |
| harness/resolution (resolution audit misaligns, own-copy vtables/constants, unresolved symbols) | 118 | no: harness work, tracked separately |
| not callable (catch funclets, cold tails, covered by the parent) | 73 | no: reclassify, they are not test failures |
| other | 37 | case by case |

The fix for the first two rows, and for most WEAK rows, is realistic input: real objects, real
vtables, real globals. The original game produces them for free.

## Idea

1. Run the unpacked original under CrossOver with a breakpoint at the entry of every target function.
2. On a hit, record the CPU state (registers, x87 stack and control word, TEB) and the memory the
   call can reach, then resume. That record is a **snapshot**.
3. In `equiv.py`, a snapshot becomes an input: map its pages, set the registers, and run both the
   original and our compiled function from it. Compare exactly as today (return values, memory
   writes, call trace, imports).
4. Mutate snapshots (flip a flag, change a count, perturb a float in `this` or an argument object)
   to reach the branches the recorded session did not.

The original game is the oracle for state; Unicorn stays the arbiter of equivalence. Our build does
not need to link or run for any of this.

## Facts that shape the design

- **Most game state is in the image.** `.data` has a virtual size of 0x893db94 (143 MB, mostly
  `.bss`): the engine keeps troops, parties, items, scenes, etc. in fixed global arrays. A snapshot
  that captures the written pages of `.data` already holds most of the state a function reads.
  Untouched `.bss` pages are zero and cost nothing.
- **The infrastructure mostly exists.** `tools/hybrid/build.py` makes the unpacked image runnable
  (OEP restored, no Steam check after unpacking, `docs/PLAN.md`) and already has a `--coverage`
  mode: a one-shot `int3` at every function entry, handled by a CRT-free runtime (`cov.c`) that
  installs a vectored exception handler and reaches every API through the image's own IAT. That
  runtime exists in spore-decomp (`tools/hybrid/cov.c`) but was **not ported**: `build.py
  --coverage` here would fail on a missing file. The capture runtime is the same shape.
- **Callees are the original's own code on both sides**, so a replay can execute them for real
  instead of faking them. That keeps state consistent after a call (a callee that fills a buffer
  really fills it). What cannot execute is code outside the image: wine's DLLs, d3d9, FMOD, Newton.
- **The machine is short on disk** (60 GB free, 94% used). The snapshot store needs a hard size cap.
- **Hybrid boot is unverified on this project.** `work/hybrid/run*.log` are empty and no boot result
  is recorded. Step 0 confirms it.

## Step 0: coverage run (small, tells us the yield)

Port `cov.c` (VC7.1, this image's IAT slots), build a coverage image, and play a session: main
menu, start or load a campaign, world-map travel, a town, a battle, the character, party and
inventory screens. `covread.py --tiers` then reports which of the ~1,100 equivalent-only functions
the game executes. Only those can get snapshots. This bounds the win before any capture code is
written, and it is useful on its own: "UNSUPPORTED, and never executed in play" is a lower-risk
category than "UNSUPPORTED, runs every frame".

Who plays: a human session (15-20 minutes) gives the broadest coverage. A scripted boot-to-menu run
(`winshot.py`, keystrokes) gives a repeatable regression run. Do both.

## Step 1: capture runtime (`tools/hybrid/snap.c`)

Built into the image by `build.py --snapshot TARGETS` (TARGETS: one VA per line, default every
equivalent-only function the coverage run saw). The same CRT-free rules as `cov.c` apply.

On a breakpoint hit:

1. **Sampling.** Decide whether to capture this hit. Per function: the first hit, then hits 2, 4,
   8, 16, ... (exponential, so a hot function spreads its samples across the session), and also the
   first hit from each new caller (the return address at `[esp]`). Cap at K captures per function
   (start with K = 16). Not capturing costs one exception, and the breakpoint stays armed: re-armed
   by single-stepping one instruction (trap flag) and putting the `int3` back.
2. **Freeze.** Suspend the other threads (toolhelp thread list through `GetProcAddress`) so memory
   cannot change under the copy. FMOD's and wine's threads do not touch game state, but the copy
   must not tear.
3. **CPU state.** The CONTEXT from the exception: general registers, eflags, `FloatSave` (x87 stack,
   control and status words). The control word matters: Direct3D can switch the x87 to 24-bit
   precision on the device thread, and the harness currently assumes `FPCW = 0x007F` for every
   function. A snapshot carries the real value.
4. **Memory, bounded closure.** Capture:
   - every page of the image's `.data` that is not all zeros;
   - the thread's stack from `esp` up to the stack base, plus the TEB and its TLS slots;
   - pages reachable from the registers and the first 16 stack dwords: every dword that points
     into committed memory pulls in its page, transitively, to a depth of D hops (start D = 3) and a
     budget of B MB per capture (start B = 8). `.data` pages count as roots too, but only at depth 1:
     following every global pointer transitively would copy the whole heap.

   Store pages content-addressed (hash of the 4 KB page, compressed), so unchanged texture or array
   pages cost nothing in the next capture. A snapshot file is a small manifest: registers, x87
   state, TEB address, the region map (`VirtualQuery` results, for address-space layout), and a list
   of (page address, page hash). Store cap: 5 GB, then sampling stops.
5. **Resume** the other threads, re-arm, continue.

A one-shot **return breakpoint** (`int3` at the caller's return address, keyed by `esp`) records
what the real call returned: `eax`, `edx`, `st0` and the final `esp`. Replaying the original from
the snapshot must reproduce those values exactly. That is a fidelity check on the replay itself
(missing pages, wrong control word, a wrong import model), separate from the orig-vs-ours check.

### Alternative to measure: full dumps

Instead of the bounded closure, capture every committed private page, deduplicated by hash. It is
complete by construction, so replays never miss a page, but a busy session might write tens of MB
per second. Measure both on the step-0 session (bytes of new pages per capture) and pick one. The
closure is the default because of the disk cap.

## Step 2: imports with recorded results

During a captured call (from entry to the return breakpoint, on that thread only), route the IAT
through logging thunks: import name, arguments, return value, and for a handful of APIs with output
buffers (`ReadFile`, `GetFileSize`, `QueryPerformanceCounter`, `timeGetTime`, the FMOD getters) the
bytes they wrote. In replay, those imports return the recorded results in order. An unrecorded call
falls back to today's import handlers.

COM calls into d3d9/d3dx9 go through vtables in DLL memory, not the IAT, so they are not logged. In
replay, a call whose target is outside the image and our object becomes a fake external call: the
same return value on both sides, with arguments compared in the call trace, which is what fake
methods do today. That is enough to test the game code around rendering calls, not the calls
themselves.

## Step 3: `equiv.py` integration

`--snapshots DIR` (or automatic, when `work/snapshots/<va>/` exists) adds snapshot inputs next to
the random ones.

- **Address space.** Snapshot pages map at their real addresses, faulted in lazily from the page
  store by the existing unmapped-memory hook. The harness's fixed regions (pool `0x40000000`, dyn
  heap `0x50000000`, stack `0x7E000000`, stubs `0x7F000000`, misc `0x7F800000`) can collide with
  wine's layout, so `Machine` takes their bases as parameters, and snapshot mode picks free holes
  from the snapshot's region map.
- **Entry.** Registers, `esp` and the x87 state come from the snapshot. The return address at
  `[esp]` is replaced by the sentinel on both sides. FS points at a copy of the captured TEB.
- **Callees.** Image functions execute for real on snapshot inputs (same code on both sides). The
  depth-1 call trace is still recorded and compared, so a wrong argument is caught at the call,
  not only through its effects.
- **Comparison.** Unchanged: return registers, written memory (dirty-page tracking already covers
  lazily mapped pages), the call trace, imports. Stack-pointer normalisation uses the captured
  frame instead of `esp_entry`.
- **Misses.** A read of a page the snapshot did not capture is a discard, counted as
  `snapshot-miss`. A function with many misses needs a deeper capture (raise D or B for it alone).
- **Mutation.** Each snapshot also seeds M mutants. One mutant changes one dword of a page the run
  read, chosen from the pages the original actually read in the unmutated replay. A mutant counts
  only if both sides complete without a discard.
- **Verdicts.** Unchanged names (PASS, WEAK, FAIL, UNSUPPORTED, and the FP variants). The ledger
  adds the number of snapshot inputs and mutants, and the cache key includes the snapshot set's
  hash. A row's note says `snap N/M` so the evidence is visible.
- **Fidelity.** Before testing, replay the original on each snapshot and check it against the
  recorded return values. A mismatch marks the snapshot unusable; it is never counted against ours.

## Step 4: rollout

1. Sweep every equivalent-only row the coverage run saw, with snapshots. Before/after verdict table.
2. Rewrite the row notes, `snap` evidence included. The FAIL policy is unchanged: any FAIL gets
   root-caused before the row is touched.
3. Reclassify the 73 not-callable rows: funclets and cold tails covered by their parent's test
   belong in `symbols/not_functions.txt` or under the parent, not in UNSUPPORTED.
4. The 118 harness/resolution rows are a separate task (resolver and own-copy constant handling).

## Expected result

Expect most of the 128 invalid-input UNSUPPORTED rows, and a large share of the ~330 WEAK rows, to
gain real evidence, but only for functions the session executes. Step 0 turns that into a number.
Functions gameplay never runs (error paths, editor-only code, unused opcodes) stay on random inputs.

## Later: whole-program check

Once our sources link into a complete exe, the same recorded session (inputs and timing) can drive
both executables, and their outcomes can be diffed. That tests everything at once, but needs a
deterministic replay of the game's inputs, which is its own project.

## Risks

- Wine/CrossOver differences: vectored handlers and thread suspension work under wine, but the
  timing differs from Windows. Captures do not depend on timing.
- SteamStub: hooks go into the unpacked image, which already runs without Steam.
- A capture inside a callback (e.g. a window procedure called from user32) has wine frames above it
  on the stack. Those are only read if the function walks up past its own arguments, which it does
  not.
- Disk: enforce the cap in `snap.c`, not afterwards.
