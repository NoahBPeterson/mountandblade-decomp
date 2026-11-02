// rec_slots_tr.cpp: the slot 1/2/3 overrides of the head_rec record classes declared by trigger_records.h (vtables 0x595740..0x596c68).
// The image's tables hold there functions that our source defines under other classes' names (MissionScreen::init_*,
// Hud::on_*, ResultWindow::init, ...), whose `this` is a different view of the record than the 0x30..0x1200-byte record
// that owns the table. Each override is a forwarding stub: it compiles to a bare `jmp` to the function the image's
// slot names (declared by a minimal local view of its class; the class name must match the mangled name). Slot 3's
// base signature is scale_3(const float*) while the real functions take int/float: the stub forwards the 4-byte
// argument unchanged.
#include "./trigger_records.h"
#include "../strat/gui_manager.h"     // g_gui

struct MapDialog {
    void reset_state();
};
struct MissionScreen {
    void init_meeting_window();
    void init_done_window();
    void init_mission_window();
};
struct StartMenu {
    void select(int);
};
struct ConvScreen {
    void open(int, int*);
};
struct Game_s25160 {
    void handle_event(int);
};
struct OptionsMenu {
    void init();
};
struct KeysPage {
    void update(int);
};
struct rec_1ac {
    void release();
};
struct facegen_win {
    void init();
};
struct TerrainPage {
    void update(float);
};
struct DebriefMenu {
    void create_debrief();
    void create_report();
};
struct ContinueMenu {
    void update(int);
};
struct Hud {
    void on_leave(int);
    void on_mission_start(int);
    void on_update(int);
};
struct options_panel {
    void on_key_event(int);
    void import_export(int);
};
struct rec_178 {
    void release();
};
struct FaceGen {
    void init();
};
struct CamView {
    void update_model(float);
};
struct quest_window {
    void __fastcall build();
};
struct MeetingWindow {
    void init();
};
struct LogScreen {
    void build_thunk();
};
struct TickScreen {
    void on_tick(int);
};
struct PartyWindow {
    void create();
};
struct InventoryPanel {
    void refresh_transaction(int);
};
struct OptionsScreen {
    void open();
};
struct QuestPage {
    void update_settings(int);
};
struct OptionsWin {
    void init();
};
struct VideoPage {
    void update(int);
};

// Rec118
void Rec118::slot_empty_1() { g_gui.reset(); overlay = 0; }       // 0x4c6e10 (ICF-folded with ItemA's copy)

// Rec1208
void Rec1208::slot_empty_1() { g_gui.reset(); }

// Rec1bc
void Rec1bc::slot_empty_1() { g_gui.reset(); }

// Rec23c
void Rec23c::slot_empty_1() { ((rec_1ac*)this)->release(); }
void Rec23c::slot_empty_2() { ((facegen_win*)this)->init(); }
void Rec23c::scale_3(float a) { ((TerrainPage*)this)->update(a); }

// Rec34
void Rec34::slot_empty_1() { g_gui.f1709 = 1; }       // the function of slot 1 at 0x4c30b0, which the original's linker folded

// Rec3c
void Rec3c::slot_empty_1() { g_gui.reset(); }

// Rec428
void Rec428::slot_empty_1() { ((rec_178*)this)->release(); }
void Rec428::slot_empty_2() { ((FaceGen*)this)->init(); }
void Rec428::scale_3(float a) { ((CamView*)this)->update_model(a); }

// Rec48
void Rec48::slot_empty_1() { g_gui.reset(); }

// Rec58

// Recacc
void Recacc::slot_empty_1() { g_gui.reset(); }
void Recacc::slot_empty_2() { ((PartyWindow*)this)->create(); }
void Recacc::scale_3(float a) { ((InventoryPanel*)this)->refresh_transaction(*(int*)&a); }

// Recd8
void Recd8::slot_empty_1() { g_gui.reset(); }

// Recf0
void Recf0::slot_empty_1() { g_gui.reset(); }

// Rec38 / Rec44 / Rec454 (their slot 3 is defined in hud.cpp, slot 2 of Rec38 / Rec44 in cls_map.cpp)
void Rec38::slot_empty_1() { g_gui.f1709 = 1; }
void Rec44::slot_empty_1() { g_gui.reset(); }
void Rec454::slot_empty_1() { g_gui.reset(); }
void Rec58::slot_empty_1() { g_gui.reset(); }
