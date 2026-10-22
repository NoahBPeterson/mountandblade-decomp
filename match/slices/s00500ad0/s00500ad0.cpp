// Slice s00500ad0: InventoryPanel::create (0x500ad0) -- the factory that builds the inventory screen.
// It picks the troop to show from the mode, allocates the inventory window (0x190-byte container, vptr
// 0x592848), gives it a background and a title, then adds four hinges, the character views, the two
// inventory areas (0x4a8-byte 0x49deb0 objects, each with its own scroll widget from 0x4f6160), the
// gold/name labels, the leave button, the total-cost label and the armour labels; finally it walks the
// player party's item grid for the equipped marker (+0xabc), registers the window with the gui manager
// and turns on the inventory tutorial.
//
// Widget slot offsets and the engine object layouts follow the verified neighbours (s004dfd50,
// s004dd830, s0049deb0, s004dcc50): +0x58 move_a, +0x64 move_b, +0x7c set_2, +0x80 set_string,
// +0xa4 set_text(out, tstring*), +0xc8 add_child.  The settings record at 0xa44240 is 0x2c bytes with
// its key tstring at +0 and the value pointer / fallback-address pair at +0x14.  `this` is provisional.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <new>
#include "rgl.h"

// ---- engine string (0x10 bytes) -------------------------------------------------------------------
struct tstring {
    int pos;                                                        // +0x00
    char* buf;                                                      // +0x04
    int cap;                                                        // +0x08
    int len;                                                        // +0x0c
    tstring(const char* s);                                         // 0x42bbe0 (ret 4)
    tstring();                                                      // 0x42bbd0
    tstring(const tstring& o);                                      // 0x42c110 (ret 4)
    ~tstring();                                                     // 0x42bc40
    void operator=(const tstring& o);                               // 0x42bc50 (ret 4)
    char equals(const tstring* o);                                  // 0x42bdd0 (ret 4, ecx = a)
};

// the string table at 0x5e2a18 (lookup never returns null)
struct strtable {
    tstring* lookup(const tstring* key);                            // 0x43a1e0 (ret 4)
};
extern strtable g_strtable;                                         // 0x5e2a18

// ---- resources -------------------------------------------------------------------------------------
struct mesh {
    mesh* clone();                                                  // 0x41c6d0
};
struct resmgr {
    mesh* get_object(const tstring& name);                          // 0x419540 (ret 4)
};
extern resmgr g_resmgr;                                             // 0x5d6fa0

// ---- settings table --------------------------------------------------------------------------------
struct config_entry {
    tstring key;                                                    // +0x00
    int     value_len;                                              // +0x10
    char*   value;                                                  // +0x14 (or the address itself)
    char    pad[0x2c - 0x18];
};
extern int g_config_count;                                          // 0xa70240
#define CONFIG_ENTRIES ((config_entry*)0xa44240)

extern "C" int    __cdecl sprintf(char*, const char*, ...);         // 0x536f8f
extern "C" double __cdecl atof(const char*);                        // 0x537352

// ---- widgets ---------------------------------------------------------------------------------------
struct vector2 {
    float x, y;
    vector2(float a, float b) { x = a; y = b; }
};

struct Widget {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21();
    virtual void move_a(vector2* p);                                // +0x58
    virtual void d23(); virtual void d24();
    virtual void move_b(vector2* p);                                // +0x64
    virtual void d26(); virtual void d27(); virtual void d28(); virtual void d29();
    virtual void d30();
    virtual void set_2(int a, void* b);                             // +0x7c
    virtual void set_string(int a, void* p);                        // +0x80
    virtual void d33(); virtual void d34(); virtual void d35(); virtual void d36(); virtual void d37();
    virtual void d38(); virtual void d39(); virtual void d40();
    virtual void set_text(tstring* out, void* name);                // +0xa4
    virtual void d42(); virtual void d43(); virtual void d44(); virtual void d45(); virtual void d46();
    virtual void d47(); virtual void d48(); virtual void d49();
    virtual void add_child(void* c);                                // +0xc8
    virtual void d51(); virtual void d52(); virtual void d53(); virtual void d54(); virtual void d55();
    virtual void d56(); virtual void d57(); virtual void d58(); virtual void d59(); virtual void d60();
    virtual void d61(); virtual void d62(); virtual void d63(); virtual void d64(); virtual void d65();
};

// the 0x188-byte text/label widget (base ctor 0x48ae20)
struct ULabel : Widget {
    char pad0[0xe8 - 4];
    int f_e8;                                                       // +0xe8
    int f_ec;                                                       // +0xec
    int f_f0;                                                       // +0xf0
    char padf4[0x178 - 0xf4];
    int f178;                                                       // +0x178
    char pad17c[0x180 - 0x17c];
    int f180;                                                       // +0x180
    int f184;                                                       // +0x184
    void ctor();                                                    // 0x48ae20
};

// the 0x190-byte container (ctor 0x4931a0, vptr 0x592848)
struct UContainer : Widget {
    char pad0[0x188 - 4];
    char f188;                                                      // +0x188
    void ctor();                                                    // 0x4931a0
};

// the 0x4a8-byte inventory view (ctor 0x49deb0, vptr 0x5931a8)
struct InvView : Widget {
    char pad0[0x104 - 4];
    unsigned flags;                                                 // +0x104
    char pad108[0xe4];
    vector2 pos;                                                    // +0xe8
    vector2 size;                                                   // +0xf0
    char pad100[0x484 - 0x100];
    void* scroll;                                                   // +0x484
    char pad488[0x490 - 0x488];
    int content_h;                                                  // +0x490
    char pad494[0x4a8 - 0x494];
    void ctor();                                                    // 0x49deb0
};

// the gui manager at 0x5d7080
struct gui_manager {
    void reset();                                                   // 0x42d1b0
    void push_back(Widget* w);                                      // 0x49b120 (ret 0)
};
#define GUI ((gui_manager*)0x5d7080)

// ---- game globals ----------------------------------------------------------------------------------
extern char* g_game;                                                // 0x5e2930 (pointer global)
struct troop_record {
    char pad0[0x20];
    int  name;                                                      // +0x20 (a tstring)
    char pad24[0xac - 0x24];
    int  type;                                                      // +0xac
    char padb0[4];
    int  grid[0x60];                                                // +0xb4, stride 8
};

struct party_record {
    char pad0[0xac];
    char grid[0x8a8 - 0xac];                                        // +0xac, stride 0x20
    int  count;                                                     // +0x8a8
};

struct InventoryPanel;
int party_slot(int party);                                          // 0x48ad00 (ecx = manager)

// ---- other callees ---------------------------------------------------------------------------------
int  inv_rows(InventoryPanel*, int);                                // 0x502320 (ret 4)
void inv_rows2(InventoryPanel*, int);                               // 0x501940
void inv_bottom(InventoryPanel*, int);                              // 0x501a20
void inv_finish(InventoryPanel*);                                   // 0x5025a0
void inv_character(InventoryPanel*, int* a, int b, int c);          // 0x5024e0 (ret 0x10)
int  troop_member_count(troop_record* t);                           // 0x462b60 (ecx = record)
Widget* make_scroll(Widget* parent, const tstring& name);           // 0x4f6160 (ret 4)
void label_colors(Widget* l, const tstring& s);                     // 0x4e4c70
void view_scene(InvView* v);                                        // 0x488d50
Widget* make_button(Widget* parent, const tstring& text, int a, int b, float c, const tstring& d,
                    tstring* out, float e, float f, float g);       // 0x4dcc50 (ret 4)
void help_open(void* help, int zero);                               // 0x494260 (ecx = 0xa73980)

extern float g_inv_row_scale;                                       // 0x599ae8
extern int   g_5a9fac;                                              // 0x5a9fac (tutorial done)

// a numeric settings value, or 0 when the key is missing
static float config_float(const char* key)
{
    tstring k(key);
    int idx = -1;
    int i = 0;
    config_entry* e = CONFIG_ENTRIES;
    while (i < g_config_count) {
        if (e->key.equals(&k))
            idx = i;
        e++;
        i++;
        if (idx != -1)
            break;
    }
    float f = 0.0f;
    if (idx >= 0) {
        config_entry* c = CONFIG_ENTRIES + idx;
        f = (float)atof(c->value ? c->value : (char*)&c->value);
    }
    return f;
}

struct InventoryPanel {
    void* vptr;                                                     // +0x00
    int  mode;                                                      // +0x04
    char pad08[0x14 - 0x08];
    int  troop;                                                     // +0x14
    char pad18[0x2c - 0x18];
    int  scale[2];                                                  // +0x2c
    InvView* view[2];                                               // +0x34
    char pad3c[0x44 - 0x3c];
    ULabel* gold[2];                                                // +0x44
    ULabel* name[2];                                                // +0x4c
    ULabel* hinge[4];                                               // +0x54
    char pad64[0x964 - 0x64];
    int  view_count;                                                // +0x964
    int  bottom_count;                                              // +0x968
    UContainer* window;                                             // +0x96c
    char pad970[0xa60 - 0x970];
    ULabel* total_cost;                                             // +0xa60
    ULabel* armor[4];                                               // +0xa64
    ULabel* armor2[4];                                              // +0xa74
    int  f_a84;                                                     // +0xa84
    int  f_a88;                                                     // +0xa88
    int  f_a8c;                                                     // +0xa8c
    ULabel* leave;                                                  // +0xa90
    int  f_a94;                                                     // +0xa94
    int  f_a98;                                                     // +0xa98
    int  f_a9c;                                                     // +0xa9c
    int  f_aa0;                                                     // +0xaa0
    int  f_aa4;                                                     // +0xaa4
    int  f_aa8;                                                     // +0xaa8
    int  f_aac;                                                     // +0xaac
    int  f_ab0;                                                     // +0xab0
    int  f_ab4;                                                     // +0xab4
    int  f_ab8;                                                     // +0xab8
    char pabc;                                                      // +0xabc
    char padabd[0x3];
    int  f_ac0;                                                     // +0xac0
    int  f_ac4;                                                     // +0xac4
    ULabel* equip_bg;                                               // +0xac8
    void create();
};

// @ 0x500ad0
void InventoryPanel::create()
{
    f_ac0 = 0;
    f_ac4 = 0;
    int* page = (int*)this;
    for (int k = 0; k < 0x60; k++) {
        *(int*)((char*)this + 0x364 + k * 4) = 0;
        *(int*)((char*)this + 0x4e4 + k * 4) = 0;
    }

    if (mode == 1) {
        int t = *(int*)(g_game + 0x13af0);
        troop = t;
        if (t <= 0)
            troop = *(int*)(g_game + 0x90);
    } else if (mode == 0) {
        troop = *(int*)(g_game + 0x90);
        troop_record* rec = (troop_record*)(g_game + 0x13a00 + troop * 0x508);
        int* troop_addr = &rec->grid[0];
        int* base = troop_addr;
        for (int k = 0; k < 0x60; k++)
            base[k * 2] = -1;
    }

    f_a8c = 0;
    f_a98 = 0;
    GUI->reset();

    f_ab0 = -1;
    f_ab4 = -1;
    f_ab8 = -1;
    f_a84 = 0;
    f_a88 = 0;

    UContainer* win = (UContainer*)::operator new(0x190);
    if (win != 0) {
        win->ctor();
        *(void**)win = (void*)0x592848;
        win->f188 = 0;
    }
    window = win;
    {
        vector2 pos(0.0f, 0.0f);
        win->move_a(&pos);
    }
    {
        vector2 size(1.0f, 0.75f);
        win->move_b(&size);
    }
    {
        tstring n("inventory_window_b");
        mesh* m = g_resmgr.get_object(n);
        win->set_string(0, m->clone());
    }

    if (mode == 4) {
        ULabel* bg = (ULabel*)::operator new(0x178);
        if (bg != 0)
            bg->ctor();
        else
            bg = 0;
        equip_bg = bg;
        {
            tstring n("inv_i_equipment_pangel_bg");
            label_colors(bg, n);
        }
        {
            tstring n("inventory_equipment_panel");
            mesh* m = g_resmgr.get_object(n);
            bg->set_2(0, m->clone());
        }
        window->add_child(bg);
    }

    for (int k = 0; k < 4; k++) {
        ULabel* h = (ULabel*)::operator new(0x178);
        if (h != 0)
            h->ctor();
        else
            h = 0;
        hinge[k] = h;
        char buf[0x58];
        sprintf(buf, "inv_hinge%d", k + 1);
        {
            tstring n(buf);
            label_colors(h, n);
        }
        {
            tstring n("inventory_window_hinge");
            mesh* m = g_resmgr.get_object(n);
            h->set_2(0, m->clone());
        }
        window->add_child(h);
    }
    {
        tstring n("ui_inv_bg");
        tstring out;
        window->set_text(&out, &n);
    }
    {
        tstring n("inv_i_character");
        inv_character(this, &f_a8c, *(int*)(g_game + 0x8c), 0);
    }
    if (mode == 4) {
        tstring n("inv_i_character_2");
        inv_character(this, &f_a98, troop, 0);
    }

    int m = mode;
    view_count = 1;
    bottom_count = 1;
    if (m == 0 || m == 2 || m == 1 || m == 3)
        view_count = 2;
    if (m == 4)
        bottom_count = 2;

    for (int i = 0; i < view_count; i++) {
        scale[i] = inv_rows(this, i);
        int other = i == 0 ? *(int*)(g_game + 0x8c) : troop;
        float area;
        if (other == *(int*)(g_game + 0x90) && mode == 0)
            area = 9.0f;
        else {
            troop_record* rec = (troop_record*)(g_game + 0x13a00) + other;
            area = (float)troop_member_count(rec) * g_inv_row_scale;
        }
        float rowsf = (float)((scale[i] + 2) / 3);
        float y = config_float("inv_i_inventory_area");
        InvView* v;
        int sc;
        if (i == 0) {
            tstring n("inv_s_inventory1");
            sc = (int)make_scroll(window, n);
            v = (InvView*)::operator new(0x4a8);
            if (v != 0)
                v->ctor();
            else
                v = 0;
            v->flags |= 0x20;
            if (sc != 0)
                v->scroll = (void*)sc;
            v->content_h = *(int*)&rowsf;
            window->add_child(v);
            {
                tstring t("inv_sc_inventory1_outer");
                label_colors(v, t);
            }
            view[0] = v;
        } else {
            tstring n("inv_s_inventory2");
            sc = (int)make_scroll(window, n);
            v = (InvView*)::operator new(0x4a8);
            if (v != 0)
                v->ctor();
            else
                v = 0;
            v->flags |= 0x20;
            if (sc != 0)
                v->scroll = (void*)sc;
            v->content_h = *(int*)&rowsf;
            window->add_child(v);
            {
                tstring t("inv_sc_inventory2_outer");
                label_colors(v, t);
            }
            view[i] = v;
        }
        float top = rowsf > area ? rowsf : area;
        float bot = rowsf > area ? area : rowsf;
        {
            vector2 p(v->pos.x, v->pos.y - top);
            v->move_a(&p);
        }
        {
            vector2 s(v->size.x, bot);
            v->move_b(&s);
        }
        view_scene(v);
        window->add_child(view[i]);
        ULabel* g = (ULabel*)::operator new(0x178);
        if (g != 0)
            g->ctor();
        else
            g = 0;
        gold[i] = g;
        if (i == 0) {
            tstring n("inv_t_gold1");
            label_colors(g, n);
        } else {
            tstring n("inv_t_gold2");
            label_colors(g, n);
        }
        window->add_child(g);
        if (mode != 0 || other != *(int*)(g_game + 0x90))
            inv_rows2(this, i);
        ULabel* nm = (ULabel*)::operator new(0x178);
        if (nm != 0)
            nm->ctor();
        else
            nm = 0;
        name[i] = nm;
        if (i == 0) {
            tstring n("inv_t_name1");
            label_colors(nm, n);
            {
                troop_record* rec = (troop_record*)(g_game + 0x13a00);
                tstring* t = g_strtable.lookup((tstring*)&rec->name);
                tstring out;
                nm->set_text(&out, t);
            }
        } else {
            tstring n("inv_t_name2");
            label_colors(nm, n);
            if (mode == 2 || mode == 4) {
                troop_record* rec = (troop_record*)(g_game + 0x13a00) + troop;
                tstring* t = g_strtable.lookup((tstring*)&rec->name);
                tstring out;
                nm->set_text(&out, t);
            } else if (mode == 0) {
                tstring k("inv_nobody");
                tstring out;
                nm->set_text(&out, g_strtable.lookup(&k));
            }
        }
        window->add_child(nm);
    }
    for (int i = 0; i < bottom_count; i++)
        inv_bottom(this, i);

    {
        tstring leave_name("inv_b_leave");
        tstring ret("ui_return");
        float y = config_float("ui_return");
        tstring* label = g_strtable.lookup(&ret);
        tstring up("short_button");
        tstring down("short_button_down");
        tstring out;
        leave = (ULabel*)make_button(window, up, 0, 0, y, down, &out, 3.0f, 0.09375f, 0.046875f);
        (void)leave_name;
        (void)label;
    }
    {
        tstring n("inv_b_leave_h");
        label_colors(leave, n);
    }
    total_cost = (ULabel*)::operator new(0x178);
    if (total_cost != 0)
        total_cost->ctor();
    else
        total_cost = 0;
    {
        tstring n("inv_t_total_cost");
        label_colors(total_cost, n);
        tstring k("ui_no_cost");
        tstring out;
        total_cost->set_text(&out, g_strtable.lookup(&k));
    }
    window->add_child(total_cost);

    for (int r = 0; r < 4; r++) {
        ULabel* a = (ULabel*)::operator new(0x178);
        if (a != 0)
            a->ctor();
        else
            a = 0;
        armor[r] = a;
        {
            tstring n("inv_t_armor_base");
            label_colors(a, n);
        }
        float step = config_float("inv_t_armor_y");
        float ry = (float)r;
        float y = (float)a->f_ec - step * ry;
        {
            vector2 p((float)a->f_e8, y);
            a->move_a(&p);
        }
        window->add_child(a);
        if (mode == 4) {
            ULabel* a2 = (ULabel*)::operator new(0x178);
            if (a2 != 0)
                a2->ctor();
            else
                a2 = 0;
            armor2[r] = a2;
            {
                tstring n("inv_t_armor_base2");
                label_colors(a2, n);
            }
            float step2 = config_float("inv_t_armor_y");
            float y2 = (float)a2->f_ec - step2 * ry;
            {
                vector2 p2((float)a2->f_e8, y2);
                a2->move_a(&p2);
            }
            window->add_child(a2);
        }
    }

    pabc = 0;
    if (mode == 2) {
        int n = 1;
        int off = 0x20;
        for (;;) {
            int party = *(int*)(g_game + 0x78);
            char* mgr = g_game + 0xf8;
            int slot = party_slot(party);
            int* pages = *(int**)(mgr + 0x138d0);
            party_record* rec = (party_record*)(char*)(*(int*)(pages + (slot >> 4)) + (slot & 0xf) * 0x5ac8);
            int count = rec->count;
            int c = 0;
            if (count > 0) {
                char* p = (char*)rec + 0xb0;
                int j = 0;
                do {
                    if ((*p & 1) != 0)
                        break;
                    c++;
                    j++;
                    p += 0x20;
                } while (j < count);
            }
            if (n >= c)
                break;
            party = *(int*)(g_game + 0x78);
            slot = party_slot(party);
            pages = *(int**)(mgr + 0x138d0);
            rec = (party_record*)(char*)(*(int*)(pages + (slot >> 4)) + (slot & 0xf) * 0x5ac8);
            if (troop == *(int*)((char*)rec + 0xac + off))
                pabc = 1;
            n++;
            off += 0x20;
        }
    }

    {
        tstring n("ui_return");
        *(int*)(g_game + 0x13ae8) &= ~1;
        GUI->push_back(window);
        inv_finish(this);
    }

    if ((*(unsigned*)&g_game[0x13ae8] & 1) == 0 && mode == 2 && g_5a9fac == 0) {
        tstring t1("ui_tutorial_inventory");
        tstring t2("ui_tutorial");
        tstring* a = g_strtable.lookup(&t1);
        tstring* b = g_strtable.lookup(&t2);
        int* help = *(int**)((char*)0xa73980);
        *(tstring*)((char*)help + 0x68) = *b;
        *(tstring*)((char*)help + 0x1c) = *a;
        help[0x4c / 4] = 0;
        help[0x50 / 4] = 0;
        help[4 / 4] = 0;
        help_open((void*)0xa73980, 0);
        *(int*)(g_game + 0x13ae8) |= 1;
    }
}
