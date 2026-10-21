# dxframework migration (0x401000-0x407c0f)

The 13 wave-1 slices of the binary's first region (`s00401010` ... `s00407250`, five
independent authors) are merged into the six translation units of the DirectX 9 SDK sample
framework that Mount & Blade 1.011 links. The slices under `match/slices/` are unchanged and stay
the reference.

All six modules build with the default flags `/O2 /MT /Gy /TP`. None of the region's functions has
an EH frame (no `fs:[0]` anywhere in the region), so the `/EHsc` that seven slices used is dropped.
Each module defines `_MBCS` (the project setting: `TCHAR` = `char`, `_tcsncpy` = `_mbsnbcpy`,
`_tcsstr` = `_mbsstr`, `_tcsncat` = `_mbsnbcat`), which is what the slices' direct `_mbs*` calls showed.
Real headers only (`<windows.h>`, `<windowsx.h>`, `<mmsystem.h>`, `<d3d9.h>`, `<d3dx9.h>`); the
hand-written `d3dmin.h` / `d3ddlg.h` / per-slice Win32 and D3D stubs are gone.

Verification: `chk.py` on every module VA, then `run_all.py --manifest` on each
`<module>.manifest.txt`, and the equivalence harness on the functions that were nonmatching or partial in
their slice (results below).

## Summary

| module | functions | byte-exact before (slices) | byte-exact after | improved | regressed |
|---|---|---|---|---|---|
| d3dapp.cpp | 27 | 21 of 26 (0x401000 was in no slice) | 27 | 6 | 0 |
| d3denumeration.cpp | 15 | 15 | 15 | 0 | 0 |
| d3dfont.cpp | 10 | 9 | 10 | 1 | 0 |
| d3dsettings.cpp | 22 | 20 | 22 | 2 | 0 |
| d3dutil.cpp | 1 | 1 | 1 | 0 | 0 |
| dxutil.cpp | 7 | 3 | 7 | 4 | 0 |
| **total** | **82** | **69 of 81** | **82** | **13** | **0** |

There are no `nonmatching.txt` or `partial.txt` rows, so those files were not created: every VA of
the region is a row of its module's `manifest.txt`.

Improved (slice status, then module status):
- 0x401000 `WndProc`: not in any slice (the index treated it as a label). Now MATCH.
- 0x401070 `CD3DApplication::Create`: nonmatching (398 bytes) -> MATCH. The default-size computation
  is `fx = 4*CXSIZEFRAME; fy = CYMENU + 4*CYSIZEFRAME + CYCAPTION; w = CXMAXIMIZED - fx;
  SetRect(.., w, CYMAXIMIZED - fy)`. With `w` as a named temporary and the height computed inline,
  cl reuses the hInstance home slot exactly as the original does.
- 0x401910 `CD3DApplication::CD3DApplication`: partial (9 bytes, scheduling) -> MATCH. The fix is
  `g_pD3DApp` being file-`static`, as in the SDK. The pause counter stays a function-local static of
  the out-of-line `Pause` (SDK), inlined into the constructor.
- 0x4027c0 `Initialize3DEnvironment`: nonmatching (580) -> MATCH (same-TU callees; SDK source).
- 0x402be0 `ToggleFullscreen`: nonmatching (188) -> MATCH. cl now sees that
  `BuildPresentParamsFromSettings` keeps ECX, so it drops the ECX reload before `Reset3DEnvironment`.
  The repeated `IsWindowed` reads come from the SDK's inline `AdapterOrdinal()`/`DevType()`; the slice's
  `volatile` trick is gone.
- 0x403120 `MsgProc`: nonmatching (46) -> MATCH (SDK source; slice-local stubs gone).
- 0x404c00 `CD3DFont::InitDeviceObjects`: nonmatching (435) -> MATCH (SDK body: `while (MOREDATA ==
  (hr = PaintAlphabet()))`, locals declared at their SDK positions).
- 0x405e00 `AdapterFormatChanged`: nonmatching (317) -> MATCH (SDK source with the combo-box helpers).
- 0x406a90 `MultisampleTypeChanged`: nonmatching (311) -> MATCH (`m_d3dSettings.SetMultisampleType()`).
- 0x407660 `DXUtil_Timer`: nonmatching (611) -> MATCH. This is the SDK source: FLOAT return,
  function-local statics, `m_llStopTime` re-read after `QueryPerformanceCounter`.
- 0x407a70 `CArrayList::CArrayList`: nonmatching (16) -> MATCH (SDK: `if (Type == AL_REFERENCE)
  BytesPerEntry = sizeof(void*)`).
- 0x407ab0 `CArrayList::Add`: nonmatching (5) -> MATCH (SDK, `new BYTE[]` / `delete[]`).
- 0x407bc0 `CArrayList::Contains`: nonmatching (2) -> MATCH (SDK, through `GetPtr()`).

Regressions: none. Every function that was byte-exact in its slice is byte-exact in its module.

## Removed slice-local tricks
- `static __declspec(noinline) GetColorChannelBits` (s00404140) and the dummy caller
  `color_bits_pair` (s00403120). The SDK's plain `static UINT ColorChannelBits()` is not inlined into
  `Enumerate` and gets the EAX convention by itself. `AlphaChannelBits`, `DepthBits` and `StencilBits`
  are the SDK statics; they inline as the slices' hand-inlined switches did.
- The local copy of `Pause` in s00401910, made only so the constructor could inline it.
- The `volatile bool` read in s00402be0's `cur_mode()`.
- The hand-expanded copies of `DisplayErrorMsg` in `Cleanup3DEnvironment` (s00402280) and
  `UserSelectNewDevice` (s00402be0). The SDK calls `DisplayErrorMsg(D3DAPPERR_NONZEROREFCOUNT, ...)` /
  `(D3DAPPERR_RESETFAILED, ...)`, and cl inlines exactly those two calls. The latch
  `s_bFatalErrorReported` (0x5d6f48) is a file-static: the inlined copies use the same variable.
- The hand-expanded `ComboBoxCount` / `ComboBoxSomethingSelected` / `ComboBoxAdd` / `ComboBoxClear`
  bodies and the `ItemMsg` helper in the dialog slices. The SDK helpers are defined once, and cl's own
  inlining reproduces the original's mix of inlined and called helpers in every handler.
- The flag trick `(mode != 3) - 1 & 0x50` in `ConfirmDeviceHelper` (the SDK's if-chain compiles to the
  same `setne/dec/and`).

## Compiler-generated code kept as stand-ins (no SDK source exists)
The linker placed these COMDATs in this region, but none is SDK code. Each is reproduced by a clearly
marked stand-in at its address, so the bytes stay verified (all MATCH):
- 0x401800 `` `vector constructor iterator' `` and 0x401830 `` `vector destructor iterator' ``
  (d3dapp.cpp, stand-ins `vector_constructor_iterator` / `vector_destructor_iterator`). They are
  instantiated by some array construction/destruction in d3dapp.obj that the SDK source does not have
  (M&B code in the TU or in a header it includes; not identified).
- 0x401870 `` `vector deleting destructor' `` of a 0x34-byte game record (two string members with dtor
  0x42bc40, an array pointer at +0x30), called only from 0x4d3731. `MBRecord34_DeleteArray` is a dummy
  that exists only to instantiate `??_EMBRecord34`, because the `delete[]` that produced it in the
  original is not in the framework source. Removing it would drop a byte-exact function, so it stays.
  Record names are provisional.
- 0x404590 out-of-line `inline float ceil(float)` from VC7.1 `<math.h>` (d3dfont.cpp, stand-in
  `ceil_float_comdat`). Every use inside d3dfont.cpp is inlined (`PaintAlphabet`). The only call is
  from game code at 0x4ad066, and the linker resolved it to d3dfont.obj's copy. That copy presumably
  came from a non-inlined call in an SDK d3dfont function the game does not link (dropped by
  /OPT:REF).
- 0x401a30 `CD3DApplication`'s scalar deleting destructor is generated naturally from
  `virtual ~CD3DApplication() {}`. No stand-in is needed.

## Boundaries
The boundaries in the brief were right. One placement decision:
- `MultisampleTypeToString` (0x4055e0) sat in the font slice s00404f00 but belongs to
  d3dsettings.cpp, as in the SDK. M&B gives it external linkage: it is called with the stack
  convention, not EAX, although its only caller is in this TU.
- The functions inside each file appear in the binary's order, which is cl's bottom-up emission order
  (callees first), not the SDK's source order. Each .cpp lists its functions by ascending address.

## Canonical names
| slices' names | canonical (SDK) |
|---|---|
| `Application`, `App` | `CD3DApplication` (vtable 0x549c60, 0x348 bytes) |
| `Enumeration`, `D3DEnumeration`, `D3DEnum`, `DeviceEnum`, `ModeFilter` | `CD3DEnumeration` |
| `List`, `ArrayList`, `CArrayList`, `dyn_array`, `UIntVec`, `IntVec`, `VecItem` | `CArrayList` |
| `DeviceCombo`, `FmtInfo`, `DeviceComboDtor` | `D3DDeviceCombo` |
| `DeviceInfo`, `ModeRec` | `D3DDeviceInfo` |
| `AdapterInfo` | `D3DAdapterInfo` |
| `SettingsBlock`, `D3DSettingsBlock`, `SettingsSet` (half) | `D3DSettings` |
| `D3DSettingsDlg`, `CSettingsDialog`, `SettingsDlg` | `CD3DSettingsDialog` |
| `g_app` | `g_pD3DApp` (static) |
| `g_errorShown`, `g_reset_msg_shown` | `s_bFatalErrorReported` (static) |
| `g_settingsDlg`, `g_pSettingsDialog` | `s_pSettingsDialog` |
| `timer_op`, `GetTimer` | `DXUtil_Timer` (`TIMER_COMMAND`) |
| `d3dfmt_name`, `FormatToString` | `D3DUtil_D3DFormatToString` |
| `safe_strncpy`, `CopyString` | `DXUtil_ConvertAnsiStringToGenericCch` |
| `app_confirm_device` | `ConfirmDeviceHelper` |
| `wndproc` | `CD3DApplication::MsgProc`; `MBWndProc` -> `WndProc` |
| `handle_size_change` / `toggle_fullscreen` / `run` / `initialize` | `ToggleFullscreen` / `UserSelectNewDevice` / `Run` / `Create3DEnvironment` (M&B, provisional) |
| `reset_a/b/c/d`, `size_changed`, `fail`, `check` | `Cleanup3DEnvironment` / `Initialize3DEnvironment` / `BuildPresentParamsFromSettings` / `Reset3DEnvironment`, `HandlePossibleSizeChange`, `DisplayErrorMsg`, `FindBestWindowedMode` |
| `ComboAdd`, `ComboSelectIndex`, `ComboSelectedData`, `ComboContains`, `ComboSelectData` | `ComboBoxAdd`, `ComboBoxSelectIndex`, `ComboBoxSelected`, `ComboBoxContainsText`, `ComboBoxSelect` |
| `On*Changed`, `OnRefreshRateChanged`/`UpdateRefreshRate`, `DlgProc`, `SettingsDlgProcThunk`, `ShowD3DSettingsDialog` | `*Changed` (SDK names), `RefreshRateChanged`, `DialogProc`, `DialogProcHelper`, `CD3DSettingsDialog::ShowDialog` |
| `GetColorChannelBits`, `BuildConflictList`, `CeilF` | `ColorChannelBits`, `BuildDSMSConflictList`, stand-in `ceil_float_comdat` |

M&B-specific members (provisional names): `CD3DApplication::m_bQuitRequested` (+4, Run() closes the
window when it is set), `m_fMinFrameTime` (+8, the frame limiter in Render3DEnvironment), `m_dw334`
(+0x334, zeroed by the constructor), the virtual `Create3DEnvironment(int)` (slot 13: the device
half of the SDK's Create), and `g_iWindowWidth` / `g_iWindowHeight` (0x5a9fe4 / 0x5a9fe8, game
globals read by Create).

## M&B changes kept (binary differs from the stock SDK)
- vtable: `FindBestFullscreenMode` is virtual (slot 0); slot 13 is new; slots 4-11 are the stock
  overridables (all ICF-folded to 0x408020).
- `Create` only registers a Unicode window class (`"MB Window"`) and creates the window
  (default size from the work area or the game's globals, centred). Run uses `PeekMessageW`/
  `GetMessageW`/`DispatchMessageW` and honours the quit flag. `Reset3DEnvironment` returns early when
  `TestCooperativeLevel` reports a lost device. `FindBest*Mode` rank the depth/stencil formats
  (`DepthStencilFormatRank`). `BuildPresentParamsFromSettings` sets `BackBufferCount` to 2 unless the
  interval is IMMEDIATE. `Render3DEnvironment` has a frame limiter. `UpdateStats` prints only
  `"%.02f fps"`. `DisplayErrorMsg` adds `D3DERR_DRIVERINTERNALERROR` (always fatal). `MsgProc`'s
  WM_CLOSE calls `FinalCleanup` first.
- `WindowedFullscreenChanged` lists only fullscreen device combos' adapter formats.
- `MultisampleTypeToString` has external linkage.

## Header conflicts resolved
- `D3DDeviceCombo` size: s00401010.h gave it `pAdapterInfo`/`pDeviceInfo` at 0x2c/0x30 (0x34 bytes),
  while s00403ab0 (byte-exact `EnumerateDeviceCombos`, `operator new(0x2c)`) proves 0x2c. The canonical
  struct is the SDK's 0x2c layout; the two extra fields are gone.
- `CArrayList` has 5 dwords (`m_NumEntriesAllocated` at +0x10, from s00403ab0/s00407250). The 4-field
  `List` of s00401010.h was a prefix of it.
- The dialog's `D3DEnumeration::allowVertexProc` (+0x24, s00405890/s004061c0) is
  `CD3DEnumeration::AppUsesDepthBuffer` (it gates the depth/stencil combo, as in the SDK).
- `AdapterInfo` in d3ddlg.h split the identifier into `identifier[0x200]` + `description[0x200]`;
  that is `D3DADAPTER_IDENTIFIER9::Driver` / `::Description`.
- The timer was declared `float` (s00401010.h) and `double` (s00402be0, s00403120, s00407250). It is
  the SDK's `FLOAT __stdcall` (callers are byte-exact with it, and so is the timer itself).
- `CArrayList::~CArrayList` and `Add` free with `operator delete[]` (0x53570d, what the original
  calls), not `free` as in s00407250 (that difference only hid behind the masked call).
- `ShowD3DSettingsDialog(HWND)` (`__stdcall`, void) is the thiscall `CD3DSettingsDialog::ShowDialog`.
  It does not use `this`, and it returns the `DialogBoxParamA` result (`INT_PTR`), which
  `UserSelectNewDevice` compares with `IDOK`.
- `BuildPresentIntervalList` was written `__stdcall` (s00403ab0). It is the private thiscall member
  (it ignores `this`; the caller loads ECX). The bytes are the same.
- `CD3DFont::DrawText` compiles as `?DrawTextA@CD3DFont@@...`, because `<windows.h>` defines a
  `DrawText` macro (the real SDK build behaves the same way). chk.py pairs it by size; the manifest row
  names the symbol.
- `SM_CXMAXIMIZED`/`SM_CYMAXIMIZED` (61/62) are what `Create` passes; the slice had them as raw numbers.

## Equivalence harness
There are no nonmatching rows, so the harness was run as an extra identity check, mostly on the
functions that were nonmatching or partial in their slice. Logs: `work/match/scratch_mig_equiv*.log`.
Two harness details matter:
- Pass `--flags "/O2 /MT /Gy /TP"`. Otherwise equiv.py compiles the module with the flags from the
  *slice* manifest, and seven slices used `/EHsc`. `/EHsc` gives `EnumerateDeviceCombos` an EH frame
  around `new CArrayList`, which is a different function: it FAILs with `/EHsc` and PASSes with the
  module flags.
- The modules include their headers as `"./x.h"`. `equiv.py` reads `// 0x...` annotations only from
  headers included by relative path, so a plain `"x.h"` left `DXUtil_Timer`, `CArrayList::GetPtr` and
  the others unresolved.

Results (module flags):
- PASS: 0x403640, 0x4036e0, 0x403720, 0x403770, 0x4039b0, 0x403ab0, 0x403b40, 0x403bf0, 0x403c50,
  0x403f90, 0x4040c0, 0x404480, 0x407250, 0x407a30, 0x407a70, 0x407aa0, 0x407ab0, 0x407ba0, 0x402430
  (Render3DEnvironment).
- WEAK, no mismatch: 0x407660 DXUtil_Timer (58.1% coverage; it was also WEAK in its slice), 0x407bc0
  (189 valid inputs; it PASSed with 246 in another run), 0x401070 Create (GetSystemMetrics/
  GetWindowLong discards), 0x404140 Enumerate (5% coverage, D3D vtables).
- UNSUPPORTED because of the harness (user32/gdi32 imports, D3D/device vtables, faults on random
  pointers, or ABI): 0x401010, 0x401290, 0x402100, 0x402be0, 0x402d40, 0x402ee0, 0x403010, 0x403120,
  0x4027c0, 0x4038c0, 0x404c00, 0x405920, 0x405e00, 0x406a90, 0x406d30, 0x407220. 0x4035d0 is
  UNSUPPORTED because its argument arrives in EAX.
- FAIL from statics the harness cannot map, not from the code: 0x401910 (constructor), 0x401480
  (Pause) and 0x402580 (UpdateStats). In each, the only differing state is the static: the original
  writes 0x5d6f40/0x5d6f44 (or 0x5d6f4c/0x5d6f50), while ours writes the object's own copy, so the
  original addresses read 0 on our side. The statics are file-static `g_pD3DApp` and the
  function-local statics of `Pause`/`UpdateStats`. Making them extern globals that the harness can map
  changes the code (the constructor goes from 0 to 181 differing bytes). The same limitation is noted
  in the s00401910 slice's partial row.
  For the timer, the function-local statics were hoisted into annotated globals in a scratch copy
  (`work/match/scratch_mig_eq/dxutil.cpp`, still byte-exact). That copy runs with no mismatch (WEAK,
  coverage only).

Symbols the harness would need in a shared file (not added here: only src/dxframework was in scope):
`00549c60 ??_7CD3DApplication@@6B@`, `00401a30 ??_GCD3DApplication@@UAEPAXI@Z`, `005d6f40
?g_pD3DApp@@3PAVCD3DApplication@@A`, plus the function-local statics listed in the sources' comments.
