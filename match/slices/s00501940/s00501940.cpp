// Slice s00501940: the party-window inventory screens -- the money label refresh (0x501940), the
// inventory "equipment menu" builder (0x501a20), a mouse-event hook (0x5021b0), the destroying
// destructor of the menu block (0x5021f0), the inventory slot factory (0x502230) and a refresh
// event (0x5022c0).  `this` is the party-window object (layout from s00502300: the gui_manager at
// +0x2c, the widget lists at +0x3c, the troop index at +0x14).  Widget slot offsets, the tstring /
// resource-manager / config-table / string-table layouts follow the verified neighbours
// (s004dfd50, s004dd830).  Names are provisional.
#include <windows.h>
#include <new>
#include "rgl.h"

using rgl::vector4;

// ---- engine types ------------------------------------------------------------------------------------
namespace rgl { struct mesh; }
using rgl::mesh;

// 0x10-byte tstring: { pos, buffer, capacity, length }.
struct tstring {
    int pos;                                                // +0
    char* buf;                                              // +4
    int cap;                                                // +8
    int len;                                                // +0xc
    tstring(const char* s);                                 // 0x42bbe0 (ret 4)
    tstring() {}                                            // default: uninitialised (0x42bbd0 is the from-string path)
    tstring(const tstring& o);                              // 0x42c110 (ret 4)
    ~tstring();                                             // 0x42bc40
};

// resource manager at 0x5d6fa0: the mesh lookup passes the name by pointer (ret 4).
struct resmgr {
    mesh* get_object(const tstring* name);                  // 0x419540 (ret 4)
};
extern resmgr g_alloc;                                      // 0x5d6fa0

namespace rgl {
struct mesh {
    mesh* clone();                                          // 0x41c6d0
    void translate(const vector4* v);                       // 0x44f240 (ret 4)
};
}
using rgl::mesh;

// settings table at 0xa44240: find() returns an entry index or -1; entries are 0x2c bytes starting at
// 0xa4425c with the value string pointer at +4 (null means the pointer field itself is the string).
struct config_table {
    int find(const tstring& key);                           // 0x4bd5a0 (ret 4)
};
extern config_table g_config;                               // 0xa44240

struct config_entry {
    int f0;                                                 // +0
    char* value;                                            // +4
    char pad[0x2c - 8];
};
#define CONFIG_ENTRIES ((config_entry*)0xa4425c)            // 0xa4425c

// string table at 0x5e2a18: lookup returns a pointer to its named table entry.
struct strtab {
    tstring* lookup(const tstring& name);                   // 0x43a1e0 (ret 4)
};
extern strtab g_strtab;                                     // 0x5e2a18
// the same table reached through a pointer, as 0x501a20 does
static strtab* const g_strtab_p = (strtab*)0x5e2a18;        // 0x5e2a18

// Troop record (stride 0x508); only the count accessor is needed here.
struct TroopRec {
    int count();                                            // 0x462b60
};

// The world / game-data object at 0x5e2930.
struct World {
    char pad8c[0x8c];
    int   cur_troop;                                        // +0x8c
    char pad90[0x13a00 - 0x90];
    TroopRec* troops;                                       // +0x13a00 (stride 0x508)
    tstring* string_value(tstring* out, int id);            // 0x48e9e0 (hidden return pointer, ret 8)
};
extern World* g_game;                                       // 0x5e2930
extern int g_a181a8;                                        // 0xa181a8 (non-zero: no food slot)
extern float g_layout_unit;                                 // 0x599ae8 (0.088f: one row of the 3-column grid)
extern float g_layout_half;                                 // 0x599cc8 (0.176f = 2 * 0x599ae8)
extern float g_layout_three;                                // 0x599cc4 (0.264f = 3 * 0x599ae8)
extern float g_panel_off;                                   // 0x599bd8
extern float g_panel_dy;                                    // 0x599bd4

// ---- widgets (slot numbers from the call sites) -------------------------------------------------------
struct UiText {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20();
    virtual void d21();
    virtual void set_xy(float* v);                          // +0x58 (slot 22)
    virtual void d22();
    virtual void d23();
    virtual void set_scale(float* v);                       // +0x64 (index 24)
    virtual void d25();
    virtual void d26();
    virtual void d27();
    virtual void set_xyz(float* v);                         // +0x74 (index 28, third component)
    virtual void d29();
    virtual void add_image(int idx, void* m);               // +0x7c (index 30, inventory slot icon)
    virtual void add_image2(int idx, void* m);              // +0x80 (index 31)
    virtual void d32(); virtual void d33(); virtual void d34(); virtual void d35(); virtual void d36();
    virtual void d37(); virtual void d38(); virtual void d39();
    virtual void set_text(tstring* out, const tstring* name);// +0xa4 (index 40)
    virtual void d41(); virtual void d42(); virtual void d43(); virtual void d44(); virtual void d45();
    virtual void d46(); virtual void d47(); virtual void d48();
    virtual void add_child(void* child);                    // +0xc8 (index 49)
    virtual void d50(); virtual void d51(); virtual void d52(); virtual void d53(); virtual void d54();
    char  pad0[0xe0 - 4];                                   // data members start after the vptr
    float x0, y0;                                           // +0xe0, +0xe4
};

// plain text widget (ctor 0x48ae20, size 0x178)
struct UiText2 : UiText {
    char pad[0x178 - 0xe8];
    UiText2();                                              // 0x48ae20
    virtual ~UiText2();                                     // 0x487c00
};

// the inventory-slot widget (vptr 0x599d28, ctor 0x48ae20, size 0x188)
struct UiSlot : UiText2 {
    UiSlot();
};
// written inline so the ctor is emitted here: the 0x48ae20 ctor, then the class vptr
inline UiSlot::UiSlot()
{
    *(void**)this = (void*)0x599d28;
}

// the menu block / panel (vptr 0x599bf8, ctor 0x4931a0, size 0x188)
struct UiPanel : UiText2 {
    char pad2[0x188 - 0x178];
    UiPanel();                                              // 0x4931a0
};

// the container the party window owns (vptr 0x5924a0, ctor 0x4931a0, dtor 0x48ef40, size 0x188)
void __fastcall vec_clear(int* v);                          // 0x41f4b0
struct UiFrame : UiText2 {
    int f178;                                               // +0x178
    UiFrame();                                              // 0x4931a0
    void release();                                         // 0x48ef40
    // written out by hand: cl elides the compiler-generated scalar deleting destructor for this
    // class (the game sets the vptr manually), and 0x5021f0 is that destructor.
    void* destroy(unsigned flags);                          // 0x5021f0 (ret 4)
};

struct PartyWindow {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17();
    virtual void on_input(unsigned mask);                   // +0x48
    PartyWindow();                                          // ends the vtable (0x48-byte prefix)
    char  pad04[0x14 - 4];
    int   troop;                                            // +0x14
    virtual void d19(); virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23();
    virtual void d24(); virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28();
    virtual void d29(); virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33();
    virtual void d34(); virtual void d35();
    virtual void refresh();                                 // +0x90
    virtual void d37(); virtual void d38(); virtual void d39(); virtual void d40(); virtual void d41();
    virtual void d42(); virtual void d43(); virtual void d44(); virtual void d45(); virtual void d46();
    virtual void d47(); virtual void d48(); virtual void d49(); virtual void d50(); virtual void d51();
    virtual void d52(); virtual void d53(); virtual void d54(); virtual void d55(); virtual void d56();
    virtual void d57(); virtual void d58(); virtual void d59(); virtual void d60(); virtual void d61();
    virtual void d62(); virtual void d63(); virtual void d64(); virtual void d65(); virtual void d66();
    virtual void d67(); virtual void d68(); virtual void d69(); virtual void d70(); virtual void d71();
    virtual void d72(); virtual void d73(); virtual void d74(); virtual void d75(); virtual void d76();
    virtual void d77(); virtual void d78(); virtual void d79(); virtual void d80(); virtual void d81();
    virtual void d82(); virtual void d83(); virtual void d84(); virtual void d85(); virtual void d86();
    virtual void d87(); virtual void d88(); virtual void d89(); virtual void d90(); virtual void d91();
    virtual void d92(); virtual void d93(); virtual void d94(); virtual void d95(); virtual void d96();
    virtual void d97(); virtual void d98(); virtual void d99(); virtual void d100(); virtual void d101();
    virtual void d102(); virtual void d103(); virtual void d104(); virtual void d105(); virtual void d106();
    virtual void d107(); virtual void d108(); virtual void d109(); virtual void d110(); virtual void d111();
    virtual void d112(); virtual void d113(); virtual void d114(); virtual void d115(); virtual void d116();
    virtual void d117(); virtual void d118(); virtual void d119(); virtual void d120(); virtual void d121();
    virtual void d122(); virtual void d123(); virtual void d124(); virtual void d125(); virtual void d126();
    virtual void d127(); virtual void d128(); virtual void d129(); virtual void d130(); virtual void d131();
    virtual void d132(); virtual void d133(); virtual void d134(); virtual void d135(); virtual void d136();
    virtual void d137(); virtual void d138(); virtual void d139(); virtual void d140(); virtual void d141();
    virtual void d142(); virtual void d143(); virtual void d144(); virtual void d145(); virtual void d146();
    virtual void d147(); virtual void d148(); virtual void d149(); virtual void d150(); virtual void d151();
    virtual void d152(); virtual void d153(); virtual void d154(); virtual void d155(); virtual void d156();
    virtual void d157(); virtual void d158(); virtual void d159(); virtual void d160(); virtual void d161();
    virtual void d162(); virtual void d163(); virtual void d164(); virtual void d165(); virtual void d166();
    virtual void d167(); virtual void d168(); virtual void d169(); virtual void d170(); virtual void d171();
    virtual void d172(); virtual void d173();
    virtual void on_message(int a, int b);                  // +0x2b0
    char  pad18[0x2c - 0x18];
    UiFrame* frame;                                         // +0x2c
    char  pad30[0x3c - 0x30];
    void* lists[10];                                        // +0x3c (menu buttons / panels)
    char            pad64[0x96c - 0x64];
    void**          close_btn;                              // +0x96c
    void  money_label(void* unused);                        // 0x501940 (ret 4)
    void* make_inventory_menu(int idx);                     // 0x501a20 (ret 4)
    char  mouse_event(unsigned mask);                       // 0x5021b0 (ret 4)
    UiText2* make_slot(void* slot_record);                  // 0x502230 (ret 4)
    bool  base_mouse_event(unsigned mask);                  // 0x42cf50 (ret 4, widget base)
    char  refresh_event(unsigned mask);                     // 0x5022c0 (ret 4)
};

// ---- external callees ----------------------------------------------------------------------------------
void  open_menu(void* w);                                   // 0x4e4c70
char  widget_mouse_event(void* w, unsigned mask);           // 0x42cf50 (ret 4)
UiText2* make_inventory_menu(PartyWindow* o, void* slot_record);   // 0x502230 (ret 4)
extern "C" int sprintf(char* dst, const char* fmt, ...);    // 0x536f8f
extern "C" double atof(const char* s);                      // 0x537352

// ---- code ----------------------------------------------------------------------------------------------

// the set_text entry of the widget class reached through the list
struct QString { virtual void t00(); virtual void t01(); virtual void t02(); virtual void t03();
    virtual void t04(); virtual void t05(); virtual void t06(); virtual void t07(); virtual void t08();
    virtual void t09(); virtual void t10(); virtual void t11(); virtual void t12(); virtual void t13();
    virtual void t14(); virtual void t15(); virtual void t16(); virtual void t17(); virtual void t18();
    virtual void t19(); virtual void t20(); virtual void t21(); virtual void t22(); virtual void t23();
    virtual void t24(); virtual void t25(); virtual void t26(); virtual void t27(); virtual void t28();
    virtual void t29(); virtual void t30(); virtual void t31(); virtual void t32(); virtual void t33();
    virtual void t34(); virtual void t35(); virtual void t36(); virtual void t37(); virtual void t38();
    virtual void t39(); virtual void t40();
    virtual void set_text(tstring* out, const tstring* name);      // +0xa4
};

// @ 0x501940
void PartyWindow::money_label(void* arg)
{
    int idx = g_game->cur_troop;
    if (arg != 0)
        idx = troop;
    int money = *(int*)((char*)g_game->troops + idx * 0x508 + 0x3b4);

    char buf[512];
    {
        tstring val;
        tstring key("ui_money_value");
        tstring* id = g_game->string_value(&val, money);
        char* b = id->buf ? id->buf : (char*)&id->buf;
        tstring* pv = g_strtab_p->lookup(key);
        char* a = pv->buf ? pv->buf : (char*)&pv->buf;
        sprintf(buf, a, b);
    }
    {
        tstring s(buf);
        QString* w = *(QString**)((char*)this + 0x44 + (int)arg * 4);
        char out[16];
        w->set_text((tstring*)out, &s);
    }
}

// the config lookup every equipment key goes through: a missing key yields `def`
static __forceinline float config_float(const char* name, float def)
{
    tstring n(name);
    int i = g_config.find(n);
    float r = def;
    if (i >= 0)
        r = (float)atof(CONFIG_ENTRIES[i].value ? CONFIG_ENTRIES[i].value : (char*)&CONFIG_ENTRIES[i].value);
    return r;
}

// the string-table lookup with a named key
static __forceinline tstring* lookup(const char* name)
{
    tstring k(name);
    return g_strtab_p->lookup(k);
}

// (x, y) pair handed to the widget methods
struct v2 {
    float x, y;
    v2(float a, float b) : x(a), y(b) {}
};

// @ 0x501a20
void* PartyWindow::make_inventory_menu(int idx)
{
    float f_1 = 0.0f;
    if (idx == 1)
        f_1 = -config_float("inv_equipment_menu_x_a", f_1);
    float f_x = config_float("inv_equipment_x", 0.0f);
    float f_y = config_float("inv_equipment_y", 0.0f);
    float f_ax = config_float("inv_armor_x", 0.0f);
    float f_hy = config_float("inv_horse_y", 0.0f);

    float p[20];
    p[0]  = f_y;
    p[1]  = f_x;
    p[2]  = f_y - g_layout_unit;
    p[3]  = f_x;
    p[4]  = f_y - g_layout_half;
    p[5]  = f_x;
    p[6]  = f_y;
    p[7]  = f_x - g_layout_unit;
    p[8]  = f_y;
    p[9]  = f_x - g_layout_half;
    p[10] = f_y;
    p[11] = f_x - g_layout_three;
    p[12] = f_ax;
    p[13] = f_x - g_layout_unit;
    p[14] = f_ax;
    p[15] = f_x - g_layout_half;
    p[16] = f_ax - g_layout_unit;
    p[17] = f_x;
    p[18] = f_hy;
    p[19] = f_x;

    UiPanel* panel = new UiPanel;
    lists[idx] = panel;
    open_menu(panel);
    {
        v2 v(f_x + panel->x0, panel->y0);
        panel->set_xy(&v.x);
    }
    {
        tstring e("");
        tstring out;
        panel->set_text(&out, &e);
    }

    {
        UiText2* w = new UiText2;
        {
            v2 v(p[18] - g_panel_off, p[19] + g_panel_dy);
            w->set_xy(&v.x);
        }
        {
            v2 v(0.088f, 0.04f);
            w->set_scale(&v.x);
        }
        {
            v2 v(16.0f, 0.0f);
            w->set_xyz(&v.x);
        }
        {
            tstring out;
            w->set_text(&out, lookup("ui_outfit"));
        }
        panel->add_child(w);
    }
    {
        UiText2* w = new UiText2;
        {
            v2 v(p[6], p[7] + g_panel_dy);
            w->set_xy(&v.x);
        }
        {
            v2 v(0.088f, 0.04f);
            w->set_scale(&v.x);
        }
        {
            v2 v(16.0f, 0.0f);
            w->set_xyz(&v.x);
        }
        {
            tstring out;
            w->set_text(&out, lookup("ui_arms"));
        }
        panel->add_child(w);
    }
    {
        UiText2* w = new UiText2;
        {
            v2 v(p[12], p[13] + g_panel_dy);
            w->set_xy(&v.x);
        }
        {
            v2 v(0.088f, 0.04f);
            w->set_scale(&v.x);
        }
        {
            v2 v(16.0f, 0.0f);
            w->set_xyz(&v.x);
        }
        {
            tstring out;
            w->set_text(&out, lookup("ui_horse"));
        }
        panel->add_child(w);
    }
    if (idx == 0 && g_a181a8 == 0) {
        UiText2* w = new UiText2;
        {
            v2 v(p[16], p[17] + g_panel_dy);
            w->set_xy(&v.x);
        }
        {
            v2 v(0.088f, 0.04f);
            w->set_scale(&v.x);
        }
        {
            v2 v(16.0f, 0.0f);
            w->set_xyz(&v.x);
        }
        {
            tstring out;
            w->set_text(&out, lookup("ui_food"));
        }
        panel->add_child(w);
    }

    int troop_idx = g_game->cur_troop;
    if (idx != 0)
        troop_idx = troop;
    for (int i = 0; i < 10; i++) {
        if ((idx < 1 && g_a181a8 == 0) || i != 9) {
            void* slot = make_slot((char*)g_game->troops + troop_idx * 0xa1 * 8 + 0x4a4 + i * 8);
            *(void**)((char*)this + 0x970 + (idx * 10 + i) * 4) = slot;
            if (i != 9)
                *(char*)((char*)slot + 0x11c) = 0;
            {
                v2 v(p[i * 2], p[i * 2 + 1]);
                ((UiText*)slot)->set_xy(&v.x);
            }
            ((UiText*)lists[idx])->add_child(slot);
            *(int*)((char*)this + (i * 4 + idx * 40 + 0x9c0)) = 2;
        } else {
            *(int*)((char*)this + idx * 40 + 0x994) = 0;
            *(int*)((char*)this + idx * 40 + 0x9e4) = 0;
        }
    }
    ((UiText*)close_btn)->add_child(lists[idx]);
    return 0;
}

// @ 0x5021b0
char PartyWindow::mouse_event(unsigned mask)
{
    bool r = false;
    if (mask == 0x10) {
        r = true;
        on_input(0x10000);
    }
    if (base_mouse_event(mask))
        return true;
    return r;
}

// @ 0x5021f0
void* UiFrame::destroy(unsigned flags)
{
    *(void**)this = (void*)0x5924a0;
    release();
    vec_clear(&f178);
    UiText2::~UiText2();
    if (flags & 1)
        ::operator delete(this);
    return this;
}

// @ 0x502230
UiText2* PartyWindow::make_slot(void* slot_record)
{
    UiSlot* w = new UiSlot;
    {
        float v2[2];
        v2[0] = 0.088f;
        v2[1] = 0.088f;
        w->set_scale(v2);
    }
    {
        tstring n("inv_slot");
        w->add_image(0, g_alloc.get_object(&n)->clone());
    }
    return w;
}

// @ 0x5022c0
char PartyWindow::refresh_event(unsigned mask)
{
    char r = 0;
    if (mask == 0x10) {
        r = 1;
        on_input(0x10000);
    }
    on_input(mask);
    refresh();
    return r;
}
