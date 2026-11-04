// rec_slots_tr.cpp: the trigger records' slot 1 (the screen reset, once per class: the original's linker folded the copies) and the
// slot 2 / 3 overrides still bound by forwarding stubs (a bare `jmp` to the function the image's slot names), see docs/standalone_vtables.md:
//   Rec23c  (terrain page)   slot 2 facegen_win::init 0x4f1460, slot 3 TerrainPage::update 0x4f2d70
//   Rec428  (face generator) slot 2 FaceGen::init 0x4f49d0,     slot 3 CamView::update_model 0x4ea8e0   (two views of one record, char_sheet)
// Every other trigger record has its slot 2 / 3 defined as a member of the record in the TU of the original function.
#include "./trigger_records.h"
#include "../strat/gui_manager.h"     // g_gui
#include "../party/party_window_class.h"   // PartyWindow

struct facegen_win { void init(); };
struct TerrainPage { void update(float); };
struct FaceGen { void init(); };
struct CamView { void update_model(float); };

void Rec118::slot_empty_1() { g_gui.reset(); overlay = 0; }       // 0x4c6e10 (ICF-folded with ItemA's copy)
void Rec1208::slot_empty_1() { g_gui.reset(); }
void Rec1bc::slot_empty_1() { g_gui.reset(); }
void Rec34::slot_empty_1() { g_gui.f1709 = 1; }       // the function of slot 1 at 0x4c30b0, which the original's linker folded
void Rec3c::slot_empty_1() { g_gui.reset(); }
void Rec48::slot_empty_1() { g_gui.reset(); }
void PartyWindow::slot_empty_1() { g_gui.reset(); }
void Recd8::slot_empty_1() { g_gui.reset(); }
void Recf0::slot_empty_1() { g_gui.reset(); }
void Rec38::slot_empty_1() { g_gui.f1709 = 1; }
void Rec44::slot_empty_1() { g_gui.reset(); }
void Rec454::slot_empty_1() { g_gui.reset(); }
void Rec58::slot_empty_1() { g_gui.reset(); }

void Rec23c::slot_empty_2() { ((facegen_win*)this)->init(); }
void Rec23c::scale_3(float a) { ((TerrainPage*)this)->update(a); }
void Rec428::slot_empty_2() { ((FaceGen*)this)->init(); }
void Rec428::scale_3(float a) { ((CamView*)this)->update_model(a); }
