// Slice s00502cc0: Mission party-window construction (0x502cc0, 2885 bytes).
// The function builds the prisoner/party management window: it resets the GUI, reads four
// layout floats from the ini, creates the main window (vtable 0x592848) and its mesh, then a
// series of buttons/panels/labels (vtable 0x597050 for the info panel), and finally dispatches
// on the party kind (this+4) to pick the party id, before refreshing the members/window.
//
// Names and layouts are provisional; every offset comes from the disassembly of this slice.
// Signatures of the gui/resmgr/string-table callees were recovered from their call sites.
#include <windows.h>
#include <stdlib.h>
#include <new>
#include <vector>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---------------------------------------------------------------------------------------------------------
// tstring (16 bytes): read cursor, heap buffer, capacity, length.
struct tstring {
    int pos;                                            // +0
    char* buf;                                          // +4
    int cap;                                            // +8
    int len;                                            // +0xc
    tstring() MB_NOTHROW;                               // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;                  // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;               // 0x42c110
    ~tstring() MB_NOTHROW;                              // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// ---------------------------------------------------------------------------------------------------------
// Ini-file table (layout from slice s004bd170): 0x1000 entries of 0x2c bytes, count at +0x2c000.
struct ini_entry {
    tstring key;                                        // +0
    char    pad10[0xc];                                 // +0x10
    tstring value;                                      // +0x1c
};

struct ini_file {
    ini_entry entries[0x1000];                          // +0
    int       count;                                    // +0x2c000
    int   find(const tstring& key);                     // 0x4bd5a0
    float get_float(const tstring& key)                 // 0x4bd5e0 (inlined here)
    {
        int i = find(key);
        if (i < 0)
            return 0.0f;
        return (float)atof(entries[i].value.c_str());
    }
};

ini_file g_config;                                      // 0xa44240

// ---------------------------------------------------------------------------------------------------------
// Localized-string hash table; lookup returns the entry's tstring.
struct string_table { char pad0[0x10]; };
string_table g_strtable;                                // 0x5e2a18
tstring* strtable_lookup(const tstring* key);           // 0x43a1e0

// ---------------------------------------------------------------------------------------------------------
// Resource manager and mesh.
struct mesh { mesh* clone(); };                         // 0x41c6d0
struct resmgr { mesh* get_object(const tstring& name); };  // 0x419540
resmgr g_resmgr;                                        // 0x5d6fa0

// ---------------------------------------------------------------------------------------------------------
// The widget base: vtable slots used here are +0x58, +0x64, +0x80, +0xc8.
struct widget {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c();
    virtual void v50(); virtual void v54();
    virtual void v58(const float* a);                   // +0x58
    virtual void v5c(); virtual void v60();
    virtual void v64(const float* ab);                  // +0x64
    virtual void v68(); virtual void v6c(); virtual void v70();
    virtual void v74(const int* v);
    virtual void v78();
    virtual void v7c(void* p);
    virtual void v80(int n, void* p);                   // +0x80
    virtual void v84(); virtual void v88(); virtual void v8c(); virtual void v90();
    virtual void v94();
    virtual void v98(const int* v);
    virtual void v9c(); virtual void va0();
    virtual void va4(tstring* out, const tstring* s);
    virtual void va8();
    virtual void vac(tstring* out, const float* pos);
    virtual void vb0(); virtual void vb4(); virtual void vb8(); virtual void vbc();
    virtual void vc0(); virtual void vc4();
    virtual void vc8(void* p);                          // +0xc8
};

// The MsgLog / GUI manager global (0x5d7080) and its window stack.
struct msglog {
    void reset();                                       // 0x42d1b0
    void set_window(widget** w);                        // 0x49b120
};
msglog g_msglog;                                        // 0x5d7080
char   g_frame_flag;                                    // 0x5d8788

// Opaque widget constructors / factories.
void    window_ctor(void* p);                           // 0x4931a0
void    panel_ctor(void* p);                            // 0x48ae20
widget* make_button(widget* parent, tstring* text, int a, int b, float h);  // 0x4e5570
widget* make_text  (widget* parent, tstring* text, int a, int b, float h);  // 0x4dce70
widget* make_up    (widget* parent, tstring* text, int a, int b, float h);  // 0x4dcbd0
void    set_label  (widget* w, tstring text);           // 0x4e4c70
void    mb_error(const char* msg);                      // 0x42cc80
void    FUN_004b4700(int v);                            // 0x4b4700

// The main window object: widget + a vector<widget*> at +0x178 + a byte at +0x188.
struct window_obj : widget {
    char pad178[0x178 - 4];
    std::vector<widget*> panels;                        // +0x178 (0x10 bytes)
    char f188;                                          // +0x188
};

// ---------------------------------------------------------------------------------------------------------
// Game data (0x5e2930) and the party manager subobject at +0xf8.
struct party { char pad0[0x8a8]; int f8a8; };

struct party_mgr {
    party* get(int id);                                 // 0x51ee50
    bool   is_slot(int id);                             // 0x4903e0
};

struct game_data {
    char pad00[0x78];
    int  f78;                                           // +0x78
    int  f7c;                                           // +0x7c
    char pad80[0x13a30 - 0x80];
    int  f13a30;                                        // +0x13a30
    int  f13b00;                                        // +0x13b00
};
extern game_data* g_game;                               // 0x5e2930

// ---------------------------------------------------------------------------------------------------------
// The Mission object.
struct Mission {
    char   pad00[0x4];
    int    f4;                                          // +0x04  party kind
    char   pad08[0x10 - 0x8];
    int    f10;                                         // +0x10  party id
    char   pad14[0x2c - 0x14];
    widget* f2c;                                        // +0x2c  main window
    widget* f30;                                        // +0x30
    widget* f34;                                        // +0x34
    widget* f38;                                        // +0x38
    widget* f3c;                                        // +0x3c
    char   pad40[0x44 - 0x40];
    widget* f44;                                        // +0x44
    widget* f48;                                        // +0x48
    widget* f4c;                                        // +0x4c
    widget* f50;                                        // +0x50
    widget* f54;                                        // +0x54
    widget* f58;                                        // +0x58
    char   pad5c[0x7c - 0x5c];
    widget* f7c;                                        // +0x7c
    char   pad80[0x84 - 0x80];
    widget* f84;                                        // +0x84  info panel
    char   pad88[0x94 - 0x88];
    int    f94;                                         // +0x94
    char   pad98[0x4b8 - 0x98];
    float  f4b8;                                        // +0x4b8
    float  f4bc;                                        // +0x4bc
    float  f4c0;                                        // +0x4c0
    float  f4c4;                                        // +0x4c4
    char   pad4c8[0x8c8 - 0x4c8];
    int    f8c8;                                        // +0x8c8
    int    f8cc;                                        // +0x8cc
    int    f8d0;                                        // +0x8d0
    int    f8d4;                                        // +0x8d4
    char   f8d8;                                        // +0x8d8
    char   f8d9;                                        // +0x8d9
    char   f8da;                                        // +0x8da
    char   f8db;                                        // +0x8db
    int    f8dc;                                        // +0x8dc
    int    f8e0;                                        // +0x8e0
    int    f8e4;                                        // +0x8e4
    char   pad8e8[0x390c - 0x8e8];
    int    f390c;                                       // +0x390c

    void open_party_window();                           // 0x502cc0
    void refresh_panel(int page);                       // 0x504ef0
    void FUN_005059c0();                                // 0x5059c0
    void refresh_party_members(int page);               // 0x503ed0
    void refresh_party_window();                        // 0x5063e0
    void FUN_00503a50();                                // 0x503a50
    void FUN_00503810(int page);                        // 0x503810
};

// helper: build a button from a string-table label and an ini height
static widget* button(Mission* self, widget* parent, const char* label_key, const char* cfg_key)
{
    tstring k(label_key);
    tstring h(cfg_key);
    tstring* text = strtable_lookup(&k);
    return make_button(parent, text, 0, 0, g_config.get_float(h));
}

// @ 0x502cc0
void Mission::open_party_window()
{
    g_msglog.reset();
    f94 = 0;
    f390c = 0;
    f7c = 0;

    f4b8 = g_config.get_float(tstring("party_bo_member1_h"));
    f4bc = g_config.get_float(tstring("party_bo_member2_h"));
    f4c0 = g_config.get_float(tstring("party_bo_member1_y"));
    f4c4 = g_config.get_float(tstring("party_bo_member2_y"));

    // main window
    window_obj* win = (window_obj*)::operator new(0x190);
    if (win) {
        window_ctor(win);
        *(void**)win = (void*)0x592848;
        win->f188 = 0;
    }
    f2c = win;

    float pos[2];
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    win->v58(pos);
    pos[0] = 1.0f;
    pos[1] = 0.75f;
    win->v64(pos);

    {
        tstring key("party_window_b");
        mesh* m = g_resmgr.get_object(key);
        mesh* c = m->clone();
        win->v80(0, c);
    }
    g_frame_flag = 1;
    g_msglog.set_window((widget**)&f2c);

    // buttons on the window
    f30 = button(this, win, "ui_done", "party_b_done_h");
    set_label(f30, tstring("party_b_done"));
    f34 = button(this, win, "ui_reset", "party_b_reset_h");
    set_label(f34, tstring("party_b_reset"));

    // info panel
    widget* info = (widget*)::operator new(0x188);
    if (info) {
        window_ctor(info);
        *(void**)info = (void*)0x597050;
    }
    f84 = info;
    set_label(f84, tstring("party_bo_info"));
    win->panels.push_back(f84);

    // text panels inside the info panel
    f7c = (widget*)::operator new(0x178);
    if (f7c) panel_ctor(f7c);
    set_label(f7c, tstring("party_t_selected_troop_name"));
    f84->vc8(f7c);

    f3c = (widget*)::operator new(0x178);
    if (f3c) panel_ctor(f3c);
    set_label(f3c, tstring("party_t_troop_cost"));
    f84->vc8(f3c);

    // talk / release / move buttons
    f44 = button(this, f84, "ui_talk", "party_b_talk_h");
    set_label(f44, tstring("party_b_talk"));

    {
        tstring k("ui_release_one");
        tstring h("party_b_release_h");
        tstring* text = strtable_lookup(&k);
        f50 = make_text(f84, text, 0, 0, g_config.get_float(h));
    }
    set_label(f50, tstring("party_b_release"));

    f54 = button(this, f84, "ui_move_up", "party_b_move_up_h");
    set_label(f54, tstring("party_b_move_up"));
    f58 = button(this, f84, "ui_move_down", "party_b_move_down_h");
    set_label(f58, tstring("party_b_move_down"));

    f8d4 = 0;
    f8d0 = 0;

    // upgrade panel
    f38 = (widget*)::operator new(0x178);
    if (f38) panel_ctor(f38);
    set_label(f38, tstring("party_t_upgrade"));
    f84->vc8(f38);

    {
        tstring k("ui_upgrade_one");
        tstring h("party_b_upgrade1_h");
        tstring* text = strtable_lookup(&k);
        f48 = make_up(f84, text, 0, 0, g_config.get_float(h));
    }
    set_label(f48, tstring("party_b_upgrade1"));

    {
        tstring k("ui_upgrade_one");
        tstring h("party_b_upgrade2_h");
        tstring* text = strtable_lookup(&k);
        f4c = make_up(f84, text, 0, 0, g_config.get_float(h));
    }
    set_label(f4c, tstring("party_b_upgrade2"));

    f8e4 = -1;
    f8dc = -1;
    f8e0 = -1;
    int kind = f4;
    f8cc = 0;

    if (kind == 0 || kind == 5) {
        f8d8 = 1; f8da = 1; f8d9 = 0; f8db = 0;
        f10 = g_game->f7c;
    }
    else if (kind == 1 || kind == 6 || kind == 7) {
        f8d8 = 1; f8da = 1; f8d9 = 1; f8db = 1;
        int x = g_game->f13b00;
        f10 = (x > 0) ? x : g_game->f7c;
    }
    else if (kind == 2) {
        f8d8 = 1; f8da = 1; f8d9 = 0; f8db = 0;
        f10 = g_game->f7c;
        party_mgr* pm = (party_mgr*)((char*)g_game + 0xf8);
        party* p = pm->get(g_game->f7c);
        p->f8a8 = 0;
    }
    else if (kind == 3) {
        f8d8 = 1; f8da = 1; f8d9 = 1; f8db = 0;
        int pid = g_game->f13a30;
        f10 = pid;
        party_mgr* pm = (party_mgr*)((char*)g_game + 0xf8);
        if (!pm->is_slot(pid))
            mb_error("mercenary_source_party not set");
    }
    else if (kind == 4) {
        f8d8 = 1; f8da = 1; f8d9 = 1; f8db = 1;
        int x = g_game->f13b00;
        f10 = (x > 0) ? x : g_game->f7c;
    }

    refresh_panel(0);
    if (f4 == 0 || f4 == 5) {
        f8c8 = 1;
        FUN_005059c0();
    } else {
        f8c8 = 2;
        refresh_panel(1);
    }

    FUN_004b4700(g_game->f78);
    refresh_party_members(0);
    refresh_party_members(1);
    refresh_party_window();
    FUN_00503a50();
    FUN_00503810(0);
    FUN_00503810(1);
}
