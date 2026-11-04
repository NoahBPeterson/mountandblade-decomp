// rec_slots_tr.cpp: the trigger records' slot 1 (the screen reset, once per class: the original's linker folded the copies) and the
// slot 2 / 3 overrides still bound by forwarding stubs (a bare `jmp` to the function the image's slot names), see docs/standalone_vtables.md:
// Every other trigger record has its slot 2 / 3 defined as a member of the record in the TU of the original function.
#include "./trigger_records.h"
#include "../strat/gui_manager.h"     // g_gui
#include "../party/party_window_class.h"   // PartyWindow


void Rec118::reset_screen() { g_gui.reset(); overlay = 0; }       // 0x4c6e10 (ICF-folded with ItemA's copy)
void Rec1208::reset_screen() { g_gui.reset(); }
void Rec1bc::reset_screen() { g_gui.reset(); }
void Rec34::reset_screen() { g_gui.f1709 = 1; }       // the function of slot 1 at 0x4c30b0, which the original's linker folded
void Rec3c::reset_screen() { g_gui.reset(); }
void Rec48::reset_screen() { g_gui.reset(); }
void PartyWindow::reset_screen() { g_gui.reset(); }
void Recd8::reset_screen() { g_gui.reset(); }
void Recf0::reset_screen() { g_gui.reset(); }
void Rec38::reset_screen() { g_gui.f1709 = 1; }
void Rec44::reset_screen() { g_gui.reset(); }
void Rec454::reset_screen() { g_gui.reset(); }
void Rec58::reset_screen() { g_gui.reset(); }

