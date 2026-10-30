// rec_slots_tr.cpp: the slot 1/2/3 overrides of the head_rec record classes declared by trigger_records.h (vtables 0x595740..0x596c68).
// The image's tables hold there functions that our source defines under other classes' names (MissionScreen::init_*,
// Hud::on_*, ResultWindow::init, ...), whose `this` is a different view of the record than the 0x30..0x1200-byte record
// that owns the table. Each override is a forwarding stub: it compiles to a bare `jmp` to the function the image's
// slot names (declared by a minimal local view of its class; the class name must match the mangled name). Slot 3's
// base signature is scale_3(const float*) while the real functions take int/float: the stub forwards the 4-byte
// argument unchanged.
#include "./trigger_records.h"

void set_flag_5d8789();     // 0x4c30b0 (rec_tables.cpp)

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
void Rec118::slot_empty_1() { ((MapDialog*)this)->reset_state(); }
void Rec118::slot_empty_2() { ((MissionScreen*)this)->init_meeting_window(); }
void Rec118::scale_3(const float* a) { ((StartMenu*)this)->select((int)a); }

// Rec1208
void Rec1208::slot_empty_1() { reset_gui(); }
__declspec(naked) void Rec1208::slot_empty_2() { __asm jmp ConvScreen::open }
void Rec1208::scale_3(const float* a) { ((Game_s25160*)this)->handle_event((int)a); }

// Rec1bc
void Rec1bc::slot_empty_1() { reset_gui(); }
void Rec1bc::slot_empty_2() { ((OptionsMenu*)this)->init(); }
void Rec1bc::scale_3(const float* a) { ((KeysPage*)this)->update((int)a); }

// Rec23c
void Rec23c::slot_empty_1() { ((rec_1ac*)this)->release(); }
void Rec23c::slot_empty_2() { ((facegen_win*)this)->init(); }
void Rec23c::scale_3(const float* a) { ((TerrainPage*)this)->update(*(float*)&a); }

// Rec34
void Rec34::slot_empty_1() { set_flag_5d8789(); }
void Rec34::slot_empty_2() { ((DebriefMenu*)this)->create_debrief(); }
void Rec34::scale_3(const float* a) { ((ContinueMenu*)this)->update((int)a); }

// Rec38
void Rec38::slot_empty_1() { set_flag_5d8789(); }
void Rec38::slot_empty_2() { ((DebriefMenu*)this)->create_report(); }
void Rec38::scale_3(const float* a) { ((Hud*)this)->on_leave((int)a); }

// Rec3c
void Rec3c::slot_empty_1() { reset_gui(); }
void Rec3c::slot_empty_2() { ((MissionScreen*)this)->init_done_window(); }
void Rec3c::scale_3(const float* a) { ((options_panel*)this)->on_key_event((int)a); }

// Rec428
void Rec428::slot_empty_1() { ((rec_178*)this)->release(); }
void Rec428::slot_empty_2() { ((FaceGen*)this)->init(); }
void Rec428::scale_3(const float* a) { ((CamView*)this)->update_model(*(float*)&a); }

// Rec44
void Rec44::slot_empty_1() { reset_gui(); }
void Rec44::slot_empty_2() { ((MissionScreen*)this)->init_mission_window(); }
void Rec44::scale_3(const float* a) { ((Hud*)this)->on_mission_start((int)a); }

// Rec454
void Rec454::slot_empty_1() { reset_gui(); }
void Rec454::slot_empty_2() { ((quest_window*)this)->build(); }
void Rec454::scale_3(const float* a) { ((Hud*)this)->on_update((int)a); }

// Rec48
void Rec48::slot_empty_1() { reset_gui(); }
void Rec48::slot_empty_2() { ((MeetingWindow*)this)->init(); }
void Rec48::scale_3(const float* a) { ((options_panel*)this)->import_export((int)a); }

// Rec58
void Rec58::slot_empty_1() { reset_gui(); }
void Rec58::slot_empty_2() { ((LogScreen*)this)->build_thunk(); }
void Rec58::scale_3(const float* a) { ((TickScreen*)this)->on_tick((int)a); }

// Recacc
void Recacc::slot_empty_1() { reset_gui(); }
void Recacc::slot_empty_2() { ((PartyWindow*)this)->create(); }
void Recacc::scale_3(const float* a) { ((InventoryPanel*)this)->refresh_transaction((int)a); }

// Recd8
void Recd8::slot_empty_1() { reset_gui(); }
void Recd8::slot_empty_2() { ((OptionsScreen*)this)->open(); }
void Recd8::scale_3(const float* a) { ((QuestPage*)this)->update_settings((int)a); }

// Recf0
void Recf0::slot_empty_1() { reset_gui(); }
void Recf0::slot_empty_2() { ((OptionsWin*)this)->init(); }
void Recf0::scale_3(const float* a) { ((VideoPage*)this)->update((int)a); }
