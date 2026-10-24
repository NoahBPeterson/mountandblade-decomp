# M4 mission_ui2 migration (0x48b930..0x490030)

Moved from match/slices into 8 translation-unit modules (flags `/O2 /MT /Gy /TP`, cl 13.10). match/slices is unchanged.
86 functions (85 distinct symbols plus the Ghidra index artifact 0x48c849).

| module | range | functions | exact before | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| ui_d_value | 0x48b930 | 1 | - | 1 | 0 | 0 |
| ui_d_key | 0x48b960 | 1 | - | 0 | 1 | 0 |
| stat_panel_x | 0x48be00..0x48c330 | 8 | - | 2 | 6 | 0 |
| mission_3 | 0x48c330..0x48e5d0 | 41 | - | 18 | 23 | 0 |
| slot_api | 0x48e5d0..0x48ef40 | 15 | - | 9 | 6 | 0 |
| mission_4 | 0x48ef40..0x48f790 | 12 | - | 4 | 8 | 0 |
| party | 0x48f790..0x48fd60 | 5 | - | 3 | 2 | 0 |
| game_data | 0x48fd60..0x490030 | 3 | - | 2 | 1 | 0 |

0x48baa0..0x48be00 is already migrated (src/game/config/ui_widgets_c) and was skipped.
Byte-exact in the slices: 20 in total (per-module split not tracked) (all still exact); now 39. No regressions, nothing partial.

## Boundary evidence
- 0x48b930 | 0x48b960: UiD::key calls value() out of line with registers kept; in one TU cl inlines value (+13 bytes). Split like ui_slider / ui_slider_key.
- 0x48be00 (class change UiE | ItemRef), 0x48c330 (Troop | Party), 0x48e5d0, 0x48ef40, 0x48f790, 0x48fd60: class changes, no forward call across them.
- Emission order is bottom-up (callees first); no forward calls inside a module.
- 0x48c849 is not a function: Ghidra splits Party::add_members (0x48c840, 357 bytes, byte-identical to our symbol) there. Both rows are nonmatching with that note. Suggest adding 0x48c849 to symbols/not_functions.txt.

## Tricks kept
- "External view" structs (TroopExt::set_attr, ItemRefExt::compact) so cl treats an in-TU callee as external.
- `__declspec(noinline)` on NameRec::resolve and unit_class_name.
- goto structure in count_troops; `party_of` page-pointer shape (canonical get_party gives different codegen).
- OW(idx) macro for widget access; Party::init stores through offset macros in disassembly order.
- Local views (ItemRefStat204, FieldVisual, FieldHost) where a header annotation maps a member to another address.

## Class identities found (slice names were mostly wrong)
Agent = Troop; World/Game/InvOwner = GameData; Mission (dc30/e5d0) = widget_mgr (Presentation); Mission (d3c0/ca30) = FieldPanel : MissionPanel;
0x48f180 = Party::init; 0x48f450.. = TableauStack; 0x48e5d0.. = widget_mgr::animate_*. script_interpreter.h also defines Presentation/TableauVec/StringTable (not includable), so local names differ (TableauStack, NameTable).

## Canonical headers extended (pure additions)
- troop.h, item_ref.h, party.h, mission.h, game_data.h: method declarations.
- item_kind.h: f1e0, f1e4 (+0x1e0/+0x1e4) and f220 (+0x220), from padding.
- game_data.h: float f13a38 (+0x13a38), from padding.
- config/widget_mgr.h: clock (+0x18), stamp_lo/stamp_hi (+0x20), busy_until (+0x28); pad split.
- symbols/slices: stat_panel_x.txt (g_skin_cap, Event vtable), mission_3.txt (unit_class_name).

## Verification
chk.py per module; run_all per manifest (all exact); compile_all (8/8); resaudit 39 rows, all agree; sweeps in work/migrate (scratch). Equiv: no FAIL except SlotArray::get 0x48d650, a benign untouched-ecx clobber; UNSUPPORTED (harness fake-arity) for 0x48d350 0x48d3c0 0x48d470 0x48d5f0 0x48dc30 0x48de00 0x48e340 0x48f020 0x48f120 0x48f6b0, 0x48c0c0 (harness error), 0x48c840/0x48c849 (index split).
Corrections found: set_limits divides by g_fixed_point_multiplier (0xa70460); 0x549d78 is -0.5f.
