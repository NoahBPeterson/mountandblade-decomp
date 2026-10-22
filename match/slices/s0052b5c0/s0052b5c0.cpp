// Slice s0052b5c0: the mission order and cursor/terrain helpers.
//   0x52b5c0  Mission::order_agent   -- give an item/order to one of an agent's four order slots
//   0x52ba10  Mission::update_ui     -- per-frame UI/cursor state machine
//   0x52ba1d  interior continuation of 0x52ba10 (EBX/ZF preset) -- translated in full
//   0x52bc80  Mission::update_cursor -- ray-pick the ground and drive the terrain brush
//   0x52bdb0  Mission::find_position -- screen ray -> world position through the collision world
// Layouts and callee signatures are the ones verified in the neighbouring slices
// (s0052ae70 Mission/Screen, s0052f250 MapEditor, s005191e0 collision shapes, s0052c2b0 brush).
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) -------------------------------------------------------------------------
struct tstring {
    int pos;                                            // +0x00
    char* buf;                                          // +0x04
    int cap;                                            // +0x08
    int len;                                            // +0x0c
    tstring(const char* s);                             // 0x42bbe0
    tstring(const tstring& o);                          // 0x42c110
    ~tstring();                                         // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct StrTable {
    tstring* lookup(const tstring& key);                // 0x43a1e0 (thiscall, ret 4)
};
extern StrTable g_strtable;                             // 0x5e2a18

// ---- gui / message log (one big object at 0x5d7080) ---------------------------------------------------
struct Gui {
    void set_o568(void* obj, int on);                   // 0x42cdb0
    void set_view(const float* v);                      // 0x4c5190
    void add(tstring text, int flag);                   // 0x452c00
};
extern Gui g_gui;                                       // 0x5d7080

// ---- item kinds / order references --------------------------------------------------------------------
struct ItemKind {
    char pad[0x1c8];
    unsigned kind;                                      // +0x1c8
    char pad2[0x238 - 0x1cc];
};
extern ItemKind* g_items;                               // 0x5e29d0

struct ItemRef {
    int id;                                             // +0x00
    int count;                                          // +0x04
    int max_amount();                                   // 0x434b70 (thiscall)
    void name(char* out);                               // 0x512730 (thiscall, ret 4)
};

// ---- agents -------------------------------------------------------------------------------------------
struct AgentFrame {
    char pad[0x58];
    matrix m;                                           // +0x58
};

struct Agent {
    char pad0[0x1d0];
    ItemRef orders[4];                                  // +0x1d0
    char pad1f0[0x30];
    char active[4];                                     // +0x220
    char pad224[8];
    int f22c;                                           // +0x22c
    int f230;                                           // +0x230
    char pad234[0x1c];
    AgentFrame* frame;                                  // +0x250
    char pad254[0x5928 - 0x254];
    void on_wield_reset();                              // 0x445a20 (thiscall)
};
void __stdcall agent_refresh(Agent* agent);             // 0x4f7e00

// ---- collision world (mission->f0) --------------------------------------------------------------------
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1),
                   min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}
    virtual int shape_type();                           // slot 1, 0x408020
    void intersect(vector4* out, const vector4* a, const vector4* b);   // 0x51a2d0
};
struct capsule : shape_base {
    char geom[0x44];
    capsule(float r, const vector4* a, const vector4* b);               // 0x433310
    void transform(const matrix* m);                                    // 0x408770
};
int shape_contact_count(shape_base* a, shape_base* b, int max, void* out);  // 0x40bfe0
matrix* identity(matrix* out);                                              // 0x40cd00

struct Scene {
    char pad[0x64];
    int f64;                                            // +0x64
    void query_segment(const float* a, const float* b, unsigned mask,
                       shape_base** out, int* count);   // 0x439870
};

struct Camera {
    void screen_to_world(vector4* near_pt, vector4* far_pt);    // 0x4bd2f0
};

// ---- terrain (0xa76630) -------------------------------------------------------------------------------
struct Terrain {
    char pad0[0x60];
    int f60;                                            // +0x60
    char pad64[0x278 - 0x64];
    int f278;                                           // +0x278
    int f27c;                                           // +0x27c
    char pad280[0x288 - 0x280];
    char cells[0xc3b0dc - 0x288];                       // +0x288
    void* f_c3b0dc;                                     // +0xc3b0dc
    float f_c3b0e0;                                     // +0xc3b0e0
    char pad_c3b0e4[0x845d984 - 0xc3b0e4];
    int f845d984;                                       // +0x845d984
    int f845d988;                                       // +0x845d988
    char pad845d98c[0x845e030 - 0x845d98c];
    char f845e030[0x64];                                // +0x845e030
    char f845e094[0x10000];                             // +0x845e094
    void clear();                                       // 0x52c290
    void update(int* objs, vector4* pos, int sel, int mode, int flag);  // 0x52bf40
    void reset();                                       // 0x4c8630
    void build_chunk(int layer, int blend, int cx, int cy, float h);     // 0x47ff60
    void gen_object(int* objs, int index);                              // 0x467250
    void brush(int* objs, int* sel, int* col, float* d, int* mode, int* flag, char* place); // 0x52c2b0
};
extern Terrain g_terrain;                               // 0xa76630

// ---- the mission / screen object (0x5e292c) -----------------------------------------------------------
struct UiObj {
    char pad[0x140];
    char f140;                                          // +0x140
};

struct CampaignGame {
    char pad0[0x1a094];
    int game_mode;                                      // +0x1a094
    int f1a098;                                         // +0x1a098
    vector4 color;                                      // +0x1a09c
    int f1a0ac;                                         // +0x1a0ac
    char pad1a0b0[0x1a0c8 - 0x1a0b0];
    char f1a0c8;                                        // +0x1a0c8
    char pad1a0c9[0x1a0d4 - 0x1a0c9];
    float f1a0d4;                                       // +0x1a0d4
    float f1a0d8;                                       // +0x1a0d8
    float f1a0dc;                                       // +0x1a0dc
    char pad1a0e0[0x1a1cc - 0x1a0e0];
    int f1a1cc;                                         // +0x1a1cc
    char pad1a1d0[0x1a20c - 0x1a1d0];
    int f1a20c;                                         // +0x1a20c
};
extern CampaignGame* g_game;                            // 0x5e2930

struct Mission {
    Scene* f0;                                          // +0x00
    char pad4[0x18 - 4];
    std::vector<Agent*> pages;                          // +0x18 (data pointer at +0x1c)
    char pad28[0x60 - 0x28];
    void* view;                                         // +0x60
    char pad64[0x2b8 - 0x64];
    int f2b8;                                           // +0x2b8
    int f2bc;                                           // +0x2bc
    long long f2c0;                                     // +0x2c0
    int f2c8;                                           // +0x2c8
    int f2cc;                                           // +0x2cc
    char pad2d0[0x2f0 - 0x2d0];
    char f2f0;                                          // +0x2f0
    char pad2f1[0x63d0 - 0x2f1];
    char f63d0[0x651c - 0x63d0];                        // +0x63d0 object array
    int f651c;                                          // +0x651c
    char pad6520[0x65d0 - 0x6520];
    UiObj* f65d0;                                       // +0x65d0
    char pad65d4[8];
    UiObj* f65dc;                                       // +0x65dc
    char pad65e0[4];
    UiObj* f65e4;                                       // +0x65e4
    char pad65e8[0x6738 - 0x65e8];
    UiObj* f6738;                                       // +0x6738

    bool order_agent(unsigned agent_id, ItemRef* order); // 0x52b5c0
    void update_ui();                                   // 0x52ba10
    void update_ui_continue();                          // 0x52ba1d (interior continuation)
    void update_cursor();                               // 0x52bc80
    void find_position(char* ok, vector4* pos);         // 0x52bdb0
    int add_placed(const int* kind, const int* id, const int* value,
                   const matrix* m, int* spawn_id);     // 0x44f870
    void update_map();                                  // 0x52f250
    void update_dialog();                               // 0x5305b0
    void update_mode4();                                // 0x52cf60
    void pick_entity();                                 // 0x4ed8d0
};
extern Mission* g_mission;                              // 0x5e292c

struct Editor {
    void update_option_checks();                        // 0x4bf640
    void refresh();                                     // 0x4c77c0
};
extern Editor g_editor;                                 // 0x8ee3c48

extern long long g_timestamps[];                        // 0x5e2948

extern int g_a17160;                                    // 0xa17160
extern int g_a17560;                                    // 0xa17560
extern int g_a171b8;                                    // 0xa171b8
extern int g_a175b8;                                    // 0xa175b8
extern int g_a17458;                                    // 0xa17458
extern int g_a1745c;                                    // 0xa1745c
extern int g_a17858;                                    // 0xa17858
extern int g_a1785c;                                    // 0xa1785c
extern float g_a178d8;                                  // 0xa178d8
extern float g_a178d4;                                  // 0xa178d4
extern int g_5e28fc;                                    // 0x5e28fc

// =====================================================================================================
//  Mission::order_agent
// =====================================================================================================
// @ 0x52b5c0
bool Mission::order_agent(unsigned agent_id, ItemRef* order)
{
    Agent* agent = pages[agent_id >> 4] + (agent_id & 0xf);
    ItemKind* ok = &g_items[order->id];
    int chosen = -1;

    for (int i = 0; i < 4 && chosen == -1; i++) {
        ItemRef slot = agent->orders[i];
        if (slot.id < 0) {
            chosen = i;
            continue;
        }
        unsigned k = g_items[slot.id].kind & 0xff;
        if (k == 5 || k == 6 || k == 0x12 || k == 0xa) {
            if (slot.max_amount() - (slot.count & 0xff) <= 0)
                chosen = i;
        } else if (k == 7) {
            if ((slot.count & 0xffff) == 0)
                chosen = i;
        }
    }

    if (chosen == -1) {
        unsigned k = ok->kind & 0xff;
        int cur;
        if (k == 2 || k == 3 || k == 4 || k == 8 || k == 9 || k == 0xa ||
            k == 0x10 || k == 0x11)
            cur = agent->f22c;
        else if (k == 7)
            cur = agent->f230;
        else
            goto L_rot;
        if (cur >= 0 && cur != -1) {
            chosen = cur;
            goto L_done;
        }
    L_rot:
        {
            int d = agent->f22c;
            int e = (d + 1) % 4;
            int f = agent->f230;
            if (e == f)
                e = (f + 1) % 4;
            if (e == d)
                e = (f + 2) % 4;
            chosen = e;
        }
    L_done: ;
    }

    if (chosen < 0) {
        tstring key("ui_unable_to_take");
        g_gui.add(*g_strtable.lookup(key), -1);
        return false;
    }

    ItemRef* slot = &agent->orders[chosen];
    char newname[128];
    char oldname[128];
    char buf[512];

    if (slot->id < 0) {
        order->name(newname);
        {
            tstring key("ui_picking_up");
            sprintf(buf, g_strtable.lookup(key)->c_str(), newname);
        }
        g_gui.add(tstring(buf), -1);
    } else {
        slot->name(oldname);
        order->name(newname);
        {
            tstring key("ui_dropping_picking_up");
            sprintf(buf, g_strtable.lookup(key)->c_str(), oldname, newname);
        }
        g_gui.add(tstring(buf), -1);

        unsigned k = g_items[slot->id].kind & 0xff;
        bool place = false;
        if (k == 7)
            place = ((slot->count & 0xffff) > 1);
        else if (k == 2 || k == 3 || k == 4 || k == 8 || k == 9 ||
                 k == 0x10 || k == 0x11)
            place = true;
        else
            place = (slot->max_amount() - (slot->count & 0xff) > 0);

        if (place) {
            matrix m = agent->frame->m;
            m.o.y += 0.1f;
            int mode = 6;
            add_placed(&mode, &slot->id, &slot->count, &m,
                       (int*)((char*)agent + 8));
        }
    }

    slot->id = order->id;
    slot->count = order->count;
    agent->active[chosen] = 0;

    unsigned k = g_items[order->id].kind & 0xff;
    if (k == 2 || k == 3 || k == 4 || k == 8 || k == 9 || k == 0xa ||
        k == 0x10 || k == 0x11)
        agent->f22c = chosen;
    else if (k == 7)
        agent->f230 = chosen;

    agent_refresh(agent);
    agent->on_wield_reset();
    return true;
}

// =====================================================================================================
//  Mission::update_ui
// =====================================================================================================
// @ 0x52ba10
void Mission::update_ui()
{
    if (g_a17160 != 0 && g_a17560 == 0)
        g_game->f1a20c = (g_game->f1a20c == 0);

    g_mission->f65d0->f140 = 0;
    g_mission->f6738->f140 = 0;

    int m = g_game->game_mode;
    if (m != 0) {
        if (m == 1 || m == 3 || m == 2) {
            g_mission->f65dc->f140 = 0;
            g_mission->f65e4->f140 = 0;
            this->update_cursor();
        } else if (m == 4) {
            g_mission->f6738->f140 = 1;
            this->update_mode4();
        }
        goto L_bbc5;
    }

    if (g_game->f1a20c != 0)
        return;

    if (g_a171b8 == 0 || g_a175b8 != 0)
        goto L_bb21;

    g_game->f1a098 = 1 - g_game->f1a098;
    {
        vector4 v;
        v.x = 1.0f;
        v.y = 1.0f;
        v.z = 1.0f;
        g_game->color = v;
    }
    g_game->f1a0ac = 0;
    g_editor.update_option_checks();
    if (g_game->f1a098 == 0) {
        g_editor.refresh();
        goto L_bb21;
    }

L_bb4b:
    g_mission->f65dc->f140 = 0;
    this->update_map();
    goto L_bbc5;

L_bb21:
    if (g_game->f1a098 == 0 && g_game->f1a0c8 == 0) {
        g_mission->f65e4->f140 = 0;
        this->update_dialog();
        goto L_bbc5;
    }
    goto L_bb4b;

L_bbc5:
    if (g_game->game_mode == 0 && g_game->f1a098 == 0) {
        long long now = g_timestamps[this->f2b8];
        long long prev = this->f2c0;
        if ((float)(now - prev) * 1e-05f > 0.05f) {
            this->pick_entity();
            this->f2c0 = g_timestamps[this->f2b8];
        }
    }
    if (this->f2cc == -1) {
        g_gui.set_o568(0, 1);
        float v[2] = { 0.5f, 0.5f };
        g_gui.set_view(v);
    }
}

// @ 0x52ba1d
// The original 0x52ba1d is the 601-byte interior continuation of update_ui: it begins
// at `push esi` with EBX=0 and the ZF from the `cmp eax,ebx` in the 13-byte prologue at
// 0x52ba10, and returns through the caller's epilogue -- it is not a callable function
// on its own.  A standalone C definition cannot reproduce an interior entry point, so
// this is the continuation body translated in full as its own function (own prologue /
// epilogue); the whole state machine is emitted once at 0x52ba10 in Mission::update_ui.
void Mission::update_ui_continue()
{
    if (g_a17160 != 0 && g_a17560 == 0)
        g_game->f1a20c = (g_game->f1a20c == 0);

    g_mission->f65d0->f140 = 0;
    g_mission->f6738->f140 = 0;

    int m = g_game->game_mode;
    if (m != 0) {
        if (m == 1 || m == 3 || m == 2) {
            g_mission->f65dc->f140 = 0;
            g_mission->f65e4->f140 = 0;
            this->update_cursor();
        } else if (m == 4) {
            g_mission->f6738->f140 = 1;
            this->update_mode4();
        }
        goto L_bbc5;
    }

    if (g_game->f1a20c != 0)
        return;

    if (g_a171b8 == 0 || g_a175b8 != 0)
        goto L_bb21;

    g_game->f1a098 = 1 - g_game->f1a098;
    {
        vector4 v;
        v.x = 1.0f;
        v.y = 1.0f;
        v.z = 1.0f;
        g_game->color = v;
    }
    g_game->f1a0ac = 0;
    g_editor.update_option_checks();
    if (g_game->f1a098 == 0) {
        g_editor.refresh();
        goto L_bb21;
    }

L_bb4b:
    g_mission->f65dc->f140 = 0;
    this->update_map();
    goto L_bbc5;

L_bb21:
    if (g_game->f1a098 == 0 && g_game->f1a0c8 == 0) {
        g_mission->f65e4->f140 = 0;
        this->update_dialog();
        goto L_bbc5;
    }
    goto L_bb4b;

L_bbc5:
    if (g_game->game_mode == 0 && g_game->f1a098 == 0) {
        long long now = g_timestamps[this->f2b8];
        long long prev = this->f2c0;
        if ((float)(now - prev) * 1e-05f > 0.05f) {
            this->pick_entity();
            this->f2c0 = g_timestamps[this->f2b8];
        }
    }
    if (this->f2cc == -1) {
        g_gui.set_o568(0, 1);
        float v[2] = { 0.5f, 0.5f };
        g_gui.set_view(v);
    }
}

// =====================================================================================================
//  Mission::update_cursor
// =====================================================================================================
// @ 0x52bc80
void Mission::update_cursor()
{
    if (g_mission->f0->f64 == 0)
        return;

    vector4 pos;
    char ok;
    // the original stores this flag as a byte (setne / mov [esp+8],cl) but passes
    // the whole 4-byte stack slot to terrain.update (mov ecx,[esp+8]); a bool/int
    // union reproduces both the byte store and the dword load.
    union { bool b; int i; } f;

    if (g_a17458 == 0 && g_a1745c == 0)
        goto L_bd6f;

    if (g_a178d8 != 0.0f || g_a178d4 != 0.0f) {
        this->find_position(&ok, &pos);
        f.b = (g_a1745c != 0);
        if (ok) {
            int mode = 0;
            if (g_game->game_mode == 2)
                mode = 1;
            else if (g_game->game_mode == 3)
                mode = 2;
            if (mode != 0)
                this->f2f0 = 1;
            g_terrain.update((int*)((char*)g_mission + 0x63d0), &pos,
                             g_mission->f651c, mode, f.i);
            g_game->f1a1cc = 1;
        }
    }

    if (g_a17458 != 0)
        goto L_bd78;

L_bd6f:
    if (g_a17858 != 0)
        goto L_bd8a;
L_bd78:
    if (g_a1745c != 0)
        return;
    if (g_a1785c == 0)
        return;
L_bd8a:
    if (this->f2f0 != 0) {
        g_terrain.reset();
        this->f2f0 = 0;
    }
}

// =====================================================================================================
//  Mission::find_position
// =====================================================================================================
// @ 0x52bdb0
void Mission::find_position(char* ok, vector4* pos)
{
    *ok = 0;

    vector4 near_pt, far_pt;
    ((Camera*)this->view)->screen_to_world(&near_pt, &far_pt);

    capsule cap(0.0f, &near_pt, &far_pt);
    matrix m;
    cap.transform(identity(&m));

    vector4 p;
    p.x = near_pt.x;
    p.y = near_pt.y;
    float b[3];
    b[0] = far_pt.x;
    b[1] = far_pt.y;

    float best = 1000000.0f;
    shape_base* shapes[8191];
    int count;
    g_mission->f0->query_segment((const float*)&p, b, 0x100, shapes, &count);

    for (int i = 0; i < count; i++) {
        shape_base* s = shapes[i];
        if (s->shape_type() == 2 && shape_contact_count(s, &cap, 0, 0)) {
            s->intersect(&p, &near_pt, &far_pt);
            float dx = p.x - near_pt.x;
            float dy = p.y - near_pt.y;
            float dz = p.z - near_pt.z;
            float d = (float)sqrt(dx * dx + dy * dy + dz * dz);
            if (d < best) {
                best = d;
                *pos = p;
                *ok = 1;
            }
        }
    }
}


