// Slice s0049cc60: the mission script-argument validators.
//
// 0x49cc60 (report) formats "%s; LINE NO: %d:" and pushes the result to the message log and the text
// log stream, then raises the script-output global. Every validator below is a tiny `char` guard that
// reads one script argument (the pointer in ECX) and, when it is out of range, prints
// "SCRIPT ERROR ON OPCODE <op>: Invalid <thing> ID: <n>" through report and returns 0.
//
// The limits come from the level/troop/faction/scene/quest tables (via the object pointer at 0x5e2930)
// or from standalone counter globals. A few guards are gated by the "warnings off" switches.
// Names are provisional. The functions whose argument arrives in EAX/EDI/ESI (0x49cda0, 0x49d160,
// 0x49d290, 0x49d360) are called from the interpreter with a non-standard register convention that
// `__fastcall` cannot fully reproduce, so they are faithful but not byte-exact. 0x49cda0 takes the
// type word in ECX and the value pointer in ESI; it is declared with the ECX argument and captures
// ESI with a one-instruction helper (the same trick the engine's own callers require).
#include <windows.h>

// ---- CRT primitive (symbol only) -------------------------------------------------------------------
extern "C" int _sprintf(char*, const char*, ...);       // 0x536f8f

// ---- tstring / message log / text log stream -------------------------------------------------------
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();
    tstring(const char* s);                             // 0x42bbe0
    tstring(const tstring& o);                          // 0x42c110
    ~tstring();                                         // 0x42bc40
};
struct MsgLog { void add(tstring msg, int color); };    // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                                 // 0x5d7080

// text log stream at 0x5a9f60 (see s00418a70): slot 10 (+0x28) is w_cstr
struct stream_base {
    virtual void reset();
    virtual void rewind();
    virtual stream_base& w_stream(void* o) = 0;
    virtual stream_base& w_float(void* v) = 0;
    virtual stream_base& w_bool(void* v) = 0;
    virtual stream_base& w_u64(void* v) = 0;
    virtual stream_base& w_i64(void* v) = 0;
    virtual stream_base& w_uint(void* v) = 0;
    virtual stream_base& w_int(int* v) = 0;
    virtual stream_base& w_byte(void* v) = 0;
    virtual stream_base& w_cstr(const char* s) = 0;
};
struct log_stream : stream_base {
    virtual stream_base& w_stream(void* o);
    virtual stream_base& w_float(void* v);
    virtual stream_base& w_bool(void* v);
    virtual stream_base& w_u64(void* v);
    virtual stream_base& w_i64(void* v);
    virtual stream_base& w_uint(void* v);
    virtual stream_base& w_int(int* v);
    virtual stream_base& w_byte(void* v);
    virtual stream_base& w_cstr(const char* s);
};
extern log_stream g_log;                                // 0x5a9f60

// ---- globals ---------------------------------------------------------------------------------------
extern int   g_lineno;          // 0x5e2598  script line number
extern int   g_script_out;      // 0x5e259c  "script produced output" flag
extern int   g_cur_opcode;      // 0x5e25a0  opcode of the statement being checked
extern char* g_game;            // 0x5e2930  GameData (pointer to the level/troop/faction/... lists)
extern char* g_mission;         // 0x5e292c  current Mission (0 when none)
extern int   g_warn_a;          // 0x5a9fd4  "warnings off" switch A
extern int   g_warn_b;          // 0x5a9fd8  "warnings off" switch B
extern int   g_presentation_n;  // 0xa75f30
extern int   g_mesh_n;          // 0x8ed4f5c
extern int   g_menu_n;          // 0x5e29b4
extern int   g_item_kind_n;     // 0x5e29d4
extern int   g_mission_tpl_n;   // 0x5e2a04
extern int   g_party_tpl_n;     // 0x5e29fc
extern int   g_map_icon_n;      // 0x5e29f4
extern int   g_particle_n;      // 0x8ed5094
extern int   g_scene_prop_n;    // 0x5e29e0
extern int   g_sound_n;         // 0x5aa130
extern int   g_track_n;         // 0xa73a14
extern int   g_anim_n;          // 0xa7662c
extern int   g_string_n;        // 0xa75ea8
extern int   g_qstring_n;       // 0xa75eac
extern int   g_tableau_lo;      // 0xa75f44
extern int   g_tableau_hi;      // 0xa75f48

void __fastcall report(const char* msg);                // 0x49cc60

// ---- report: format + log one script error line ----------------------------------------------------
// @ 0x49cc60
void __fastcall report(const char* msg)
{
    char buf[1024];
    _sprintf(buf, "%s; LINE NO: %d:", msg, g_lineno);
    g_msglog.add(tstring(buf), 0xffff5555);
    g_log.w_cstr(buf);
    g_script_out = 1;
}

// ---- the validators --------------------------------------------------------------------------------
// 2-argument form: ECX = *value, EDX = *limit (the script-parameter bounds test)
// @ 0x49ccc0
char __fastcall check_script_param(int* value, int* limit)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= *limit) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Script Parameter ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49cd00
char __fastcall check_register(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= 0x80) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Register ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49cd50
char __fastcall check_presentation(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= g_presentation_n) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Presentation ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// String arguments: the type word (in ECX) selects which of the three string tables the value (in ESI)
// must index: 3 = string, 0x16 = quick string, 0 = string register.  The caller passes the value
// pointer in ESI (a non-standard register argument), so capture it with a one-instruction helper.
// @ 0x49cda0
char __fastcall check_string_arg(int* type)
{
    char buf[512];
    int t = *type;
    int* value = 0;
    __asm { mov value, esi }
    char r = 1;
    if (t == 3) {
        int v = *value;
        if (v < 0 || v >= g_string_n) {
            _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid String ID: %d", g_cur_opcode, v);
            report(buf);
            r = 0;
        }
    } else if (t == 0x16) {
        int v = *value;
        if (v < 0 || v >= g_qstring_n) {
            _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Quick String ID: %d", g_cur_opcode, v);
            report(buf);
            r = 0;
        }
    } else if (t == 0) {
        int v = *value;
        if (v < 0 || v >= 0x80) {
            _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid String Register ID: %d", g_cur_opcode, v);
            report(buf);
            r = 0;
        }
    }
    return r;
}

// @ 0x49ce40
char __fastcall check_mesh(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= g_mesh_n) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Mesh ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49ce90
char __fastcall check_menu(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= g_menu_n) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Menu ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49cee0
char __fastcall check_troop(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= *(int*)(g_game + 0x13a04)) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Troop ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49cf30
char __fastcall check_quest(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= *(int*)(g_game + 0x13a5c)) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Quest ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49cf80
char __fastcall check_faction(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= *(int*)(g_game + 0x13a4c)) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Faction ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49cfd0
char __fastcall check_item_kind(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= g_item_kind_n) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Item Kind ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49d020
char __fastcall check_scene(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= *(int*)(g_game + 0x13a54)) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Scene ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49d070
char __fastcall check_mission_template(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= g_mission_tpl_n) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Mission Template ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49d0c0
char __fastcall check_party_template(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= g_party_tpl_n) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Party Template ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49d110
char __fastcall check_map_icon(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= g_map_icon_n) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Map Icon ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// ---- the party table (object at g_game + 0xf8) -----------------------------------------------------
struct PartyTable {
    char pad0[0x138d0];
    int* pages;             // +0x138d0  page directory: page = pages[index >> 4]
    char pad138d4[8];
    int  count;             // +0x138dc
    int  index_of(unsigned id);                 // 0x48ad00 (thiscall, ret 4)
};

// Party id: must be a live entry of the party table (field +4 == 1). The id pointer arrives in EAX.
// @ 0x49d160
char __fastcall check_party(int* idp)
{
    char buf[512];
    PartyTable* t = (PartyTable*)(g_game + 0xf8);
    unsigned id = *idp;
    if (id < (unsigned)t->count) {
        int idx = t->index_of(id);
        if (idx >= 0) {
            int base = t->pages[idx >> 4];
            if (*(int*)(base + (idx & 0xf) * 0x5ac8 + 4) == 1)
                return 1;
        }
    }
    _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Party ID: %d", g_cur_opcode, id);
    report(buf);
    return 0;
}

// @ 0x49d1f0
char __fastcall warn_particle_system(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= g_particle_n) {
        _sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid particle_system ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49d240
char __fastcall warn_scene_prop(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= g_scene_prop_n) {
        _sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Scene_prop ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// Scene-prop instance: g_mission must exist and the value must index the prop instance array
// (0x1c8-byte records between the two pointers at mission +0x668c / +0x6690). The value pointer is in EDI.
// @ 0x49d290
char warn_scene_prop_instance(int* idp)
{
    char buf[512];
    int id = *idp;
    if (g_mission && id >= 0) {
        char* a = *(char**)(g_mission + 0x668c);
        char* b = *(char**)(g_mission + 0x6690);
        if (a) {
            if (id < (int)((b - a) / 0x1c8))
                return 1;
        }
    }
    _sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Scene_prop Instance ID: %d", g_cur_opcode, id);
    report(buf);
    return 0;
}

// @ 0x49d310
char __fastcall warn_sound(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= g_sound_n) {
        if (g_warn_b == 0) {
            _sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Sound ID: %d", g_cur_opcode, v);
            report(buf);
        }
        r = 0;
    }
    return r;
}

// Tableau: the value must index the tableau array (0x68-byte records between the two pointers at
// 0xa75f44 / 0xa75f48). The value pointer arrives in EAX.
// @ 0x49d360
char __fastcall warn_tableau(int* idp)
{
    char buf[512];
    int id = *idp;
    if (id >= 0 && g_tableau_lo != 0) {
        if (id < (g_tableau_hi - g_tableau_lo) / 0x68)
            return 1;
    }
    _sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Tableau ID: %d", g_cur_opcode, id);
    report(buf);
    return 0;
}

// @ 0x49d3d0
char __fastcall warn_track(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= g_track_n) {
        if (g_warn_a == 0 && g_warn_b == 0) {
            _sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Track ID: %d", g_cur_opcode, v);
            report(buf);
        }
        r = 0;
    }
    return r;
}

// Agent: g_mission must exist and the value must index the agent table (mission +4 = count, +0x1c = page
// directory, 0x5928-byte records, live flag at +4). The value pointer arrives in EDX (the first
// __fastcall slot is unused).
// @ 0x49d430
char __fastcall warn_agent(int unused, int* idp)
{
    char buf[512];
    char r = 0;
    (void)unused;
    if (g_mission) {
        int id = *idp;
        if (id >= 0 && id < *(int*)(g_mission + 4)) {
            int lo = (id & 0xf) * 0x5928;
            char* page = *(char**)(*(char**)(g_mission + 0x1c) + (id >> 4) * 4);
            if (*(int*)(page + lo + 4) != 0)
                return 1;
        }
    }
    _sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Agent ID: %d", g_cur_opcode, *idp);
    report(buf);
    return r;
}

// @ 0x49d4b0
char __fastcall check_animation(int* value)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= g_anim_n) {
        _sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Animation ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
