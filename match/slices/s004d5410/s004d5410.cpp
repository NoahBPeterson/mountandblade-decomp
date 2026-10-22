// Slice s004d5410: mission-item source callbacks (the object at Hud+0x30 seen as 0x4d5410's `this`).
//   0x4d5410  MissionSource::pick   start-of-frame bookkeeping (spawn tick global + family sound), the
//             nearest kind-7 item under the mouse (distance to the rectangle [x0,y0,x0+w,y0+h], two
//             fabs variants), the same item's value handler, then every item's mesh animation clocks and
//             the item reactions to "used" (kind 2/3 float, 5 int, 6 title string, 8 flag), "pressed 2"
//             and "pressed 4".
//   0x4d5940  ValueSource::on_value_changed  advance the progress-bar source by one step.
//   0x4d5980  Hud::on_mission_start  stamp the spawn tick, set up the mission, switch the screen, open the menu.
//   0x4d59f0  Hud::on_leave         the "leave" button opens menu option 7.
//   0x4d5a20  Hud::on_update        advance the progress-bar source by the mission clock, poll the page
//             list, the two page buttons and the "menu" button.
// (0x4d5290 Hud::handle_buttons lives in slice s004d4b80, which owns its VA.)
// Names are provisional; the layouts are offset stubs taken from this range's accesses. 0x495700 is the
// scene-slot removal of s00494e40, but in every caller here it is a menu choice on the object at 0xa73980.
#include <windows.h>
#include <math.h>
#include "rgl.h"

#define ABS(x) ((float)fabs(x))

// ---- engine strings / tables ---------------------------------------------------------------------------
// 0x495700 (thiscall, ret 4) on 0xa73980: pick a main-menu option.
struct MainMenu {
    char pad0[0x18];
    int  sel;                                       // +0x18
    void choose(int option);                        // 0x495700
};
extern MainMenu g_main_menu;                        // 0xa73980

// the widgets the menu options write back into (each int* is a widget: +0x4, +0x18 index, +0x8c)
extern int* g_menu_w3;                              // 0xa739ac

struct MenuScreen {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    char pad14[0x18 - 0x14];
    int  f18;                                       // +0x18
};
extern MenuScreen* g_screen;                        // 0xa739b8

extern void* g_world;                               // 0x5e292c
extern int   g_u32;                                 // 0xa735c0
void mission_start_setup(void* m);                  // 0x484590 (this = g_world)
extern float g_char_scale;                          // 0x590d7c

// the strings the mission items write and the numbers their sounds depend on
extern rgl::string g_mission_title;                 // 0xa72890
extern __int64 g_spawn_lo;                          // 0xa70448
extern __int64 g_spawn_hi;                          // 0xa70450
extern float g_mouse_x;                             // 0x5d7094
extern float g_mouse_y;                             // 0x5d7098
extern double g_tick_scale;                         // 0x54a6d0
extern double g_day_scale;                          // 0x591834
extern double g_day_period;                         // 0x54a5d8
extern float g_hour_scale;                          // 0x590c90
extern float g_hour_base;                           // 0x54ab28
extern int g_ticks_[2];                             // 0x5e2948 (the 64-bit tick clock)
extern int g_mission_clock;                         // 0x5d8790 (mission time; 0 = no mission)
extern int g_a170d8;                                // 0xa170d8
extern int g_a174d8;                                // 0xa174d8

extern "C" double __cdecl fmod(double x, double y); // 0x536a6a
extern "C" int   __cdecl round_top(float v);        // 0x535a64 (the x87 int conversion used here)
void timer_set(void* node, const float* t);         // 0x433190 (mesh_node::clamp_c8)
void string_assign(rgl::string* thisp, const rgl::string* src); // 0x42bc50

// sound holder inside a MenuKind (s0048add0): count of 0x30-byte children at +0, the child array at +4.
// update(0xc3 / 0xc2 / 0xc1) advances the children with that flag.
struct TimerList {
    int  count;                                     // +0x0
    void* items;                                    // +0x4
    void update(int flag);                          // 0x48add0
};
struct MenuKind {                                   // 0x1c bytes (table at 0xa75f2c)
    char      pad0[0x14];
    TimerList sound;                                // +0x14
};
extern MenuKind* g_menu_kinds;                      // 0xa75f2c (menu-kind table, element stride 0x1c)
inline void play_family_sound(int family, int id)
{
    g_menu_kinds[family].sound.update(id);
}

// ---- widgets -------------------------------------------------------------------------------------------
// control widget: "activate" at vtable +0x2c, "value" at +0x30, "float" at +0x38, "used" at +0x44,
// "key flags" at +0x48, "kind" at +0xc0 (slots cross-checked with s004cf7b0 / s004dd830).
struct Widget {
    virtual void m00(); virtual void m01(); virtual void m02(); virtual void m03();
    virtual void m04(); virtual void m05(); virtual void m06(); virtual void m07();
    virtual void m08(); virtual void m09(); virtual void m0a();
    virtual void activate(int v);                   // +0x2c
    virtual void m0c();
    virtual int  get_value();                       // +0x30
    virtual void m0e(); virtual void m0f();
    virtual float get_float();                      // +0x38
    virtual char used(int flag);                    // +0x44
    virtual void set_key_flags(int f);              // +0x48
    virtual void m13(); virtual void m14();
    virtual void m15(); virtual void m16(); virtual void m17(); virtual void m18();
    virtual void m19(); virtual void m1a(); virtual void m1b(); virtual void m1c();
    virtual void m20(); virtual void m21(); virtual void m22(); virtual void m23();
    virtual void m24(); virtual void m25(); virtual void m26(); virtual void m27();
    virtual void m28(); virtual void m29(); virtual void m2a(); virtual void m2b();
    virtual void m2c(); virtual void m2d(); virtual void m2e(); virtual void m2f();
    virtual int  kind();                            // +0xc0
};

// the progress-bar data source (see s0048b760): +0x168..+0x170 the current value and its range,
// +0x178 the slot index, +0x17c the step, +0xf0 the per-slot screen values.
struct ValueSource {
    void* vptr;                                     // +0x00
    char  pad04[0xf0 - 0x04];
    float slot[1];                                  // +0xf0
    char  pad1[0x168 - 0xf4];
    float f168;                                     // +0x168 (current value)
    float f16c;                                     // +0x16c (range low)
    float f170;                                     // +0x170 (range high)
    char  pad2[0x178 - 0x174];
    int   idx;                                      // +0x178
    float step;                                     // +0x17c
    void  set_value(float v);                       // 0x48baa0
    float value();                                  // 0x48b930
    void  on_value_changed(float arg);              // 0x4d5940
};
// kept out of line as in the original (0x48b930 / 0x48baa0 are real functions)
__declspec(noinline) void ValueSource::set_value(float v) { f168 = v; }
__declspec(noinline) float ValueSource::value()
{
    return (f168 - f16c) / (f170 - f16c) * (slot[idx] - step);
}

// value source reached through an item pointer (Hud+0x30 sources): only the vtable slots matter
struct ValueWidget : ValueSource {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v0a(); virtual void v0b();
    virtual void v0c(); virtual void v0d(); virtual void v0e(); virtual void v0f();
    virtual void v10(); virtual void v11();
    virtual void set_key_flags(int f);              // +0x48
};

// an item record: a Widget with the offset vector at +0xe0 (x0,y0,w,h), the mesh animation nodes at
// +0x108, a name at +0x128 (string) and two item flags at +0x180/+0x184.
struct MissionItem : Widget {
    char         pade0[0xe0 - sizeof(Widget)];
    float        rect[4];                           // +0xe0 x0, +0xe4 y0, +0xf0 w, +0xf4 h
    char         padf8[0x108 - 0xf8];
    void*        nodes[5];                          // +0x108 .. +0x118
    char         pad11c[0x128 - 0x11c];
    rgl::string  name;                              // +0x128
    char         pad138[0x180 - 0x138];
    char         f180;                              // +0x180
    char         pad181[3];
    int          f184;                              // +0x184
};

// the per-item value source sits at item+0x484
inline ValueWidget* item_value(MissionItem* it) { return (ValueWidget*)((char*)(void*)it + 0x484); }

// the item source (0x4d5410's `this`, Hud+0x30): an STL vector of MissionItem* at +0x4/+0x8/+0xc, the
// item family at +0x10 (its sound is the family-th ItemSound), the current clock slot at +0x18 and the
// 64-bit tick the slot was entered at (+0x20/+0x24).
struct MissionSource {
    char           pad0[4];
    MissionItem**  begin;                           // +0x4
    MissionItem**  end;                             // +0x8
    MissionItem**  cap;                             // +0xc
    int            family;                          // +0x10
    char           pad14[0x18 - 0x14];
    int            slot;                            // +0x18
    char           pad1c[0x20 - 0x1c];
    int            tick_lo;                         // +0x20
    int            tick_hi;                         // +0x24

    // VC7.1 vector<MissionItem*>::size(): empty vector is NULL in this build
    unsigned size() const { return begin ? (unsigned)(end - begin) : 0; }

    void pick(int arg);                             // 0x4d5410
};

// ---- the screen object owning the callbacks -------------------------------------------------------------
struct Hud {
    char    pad00[0x4];
    int     f04;                                    // +0x4 (page mode)
    char    pad08[0x10 - 8];
    int     f10;                                    // +0x10
    char    pad14[0x18 - 0x14];
    int     f18;                                    // +0x18
    char    pad1c[0x30 - 0x1c];
    char    src30[0x4];                             // +0x30 (the item source; passed as `this` to 0x4d5410)
    Widget* f34;                                    // +0x34
    Widget* f38;                                    // +0x38
    char    pad3c[0x88 - 0x3c];
    int     f88;                                    // +0x88
    char    pad8c[0x24c - 0x8c];
    int     f24c;                                   // +0x24c
    int     f250;                                   // +0x250
    Widget* wi[1];                                  // +0x254 (page list)

    void on_mission_start(int arg);                 // 0x4d5980
    void on_leave(int arg);                         // 0x4d59f0
    void on_update(int arg);                        // 0x4d5a20
};

// A page object of the list at +0x254 (0x4d5ba0 fills it; only the call matters here).
void quest_page_fill(Hud* page);                    // 0x4d5ba0

// @ 0x4d5940
void ValueSource::on_value_changed(float arg)
{
    float v = value();
    set_value(step / slot[idx] * arg * g_char_scale + v);
}

// @ 0x4d5980
void Hud::on_mission_start(int arg)
{
    *(int*)((char*)(void*)g_world + 0x5270) = g_u32;
    g_u32 = g_u32 + 1;
    mission_start_setup((void*)(void*)g_world);
    g_screen->v2();
    g_screen->f18 = f18;
    g_menu_w3[1] = 0;
    g_menu_w3[0x18 / 4] = f18;
    g_main_menu.choose(0xe);
}

// @ 0x4d59f0
void Hud::on_leave(int arg)
{
    if (f34->used(0x10000))
        g_main_menu.choose(7);
}

// @ 0x4d5a20
void Hud::on_update(int arg)
{
    if (g_mission_clock) {
        MissionItem* it = ((MissionSource*)((char*)(void*)this + 0x30))->begin[0];
        if (it) {
            item_value(it)->on_value_changed((float)g_mission_clock);
            ValueWidget* u = item_value(((MissionSource*)((char*)(void*)this + 0x30))->begin[0]);
            u->set_key_flags(0x10000);
        }
    }
    {
        Hud* self = this;
        int n = self->f24c;
        for (int i = 0; i < n; i++) {
            if (self->wi[i]->used(0x10000)) {
                self->f250 = i;
                quest_page_fill((Hud*)(void*)self->wi[i]);
            }
        }
    }
    if (f34->used(0x10000)) {
        if (f04 == 0) {
            f34->activate(1);
            return;
        }
        f04 = 0;
        g_main_menu.choose(0xd);
    } else if (f38->used(0x10000)) {
        if (f04 == 1) {
            f38->activate(1);
            return;
        }
        f04 = 1;
        g_main_menu.choose(0xd);
    } else if (((Widget*)(void*)((MissionSource*)((char*)(void*)this + 0x30))->begin[0])->used(0x10000)) {
        g_main_menu.choose(f18);
    }
}

// 0x4d5410
void MissionSource::pick(int arg)
{
    float t;
    if (family >= 0) {
        __int64 now = *(__int64*)(g_ticks_ + slot * 2);
        __int64 d = now - ((__int64)tick_hi << 32) - (unsigned)tick_lo;
        g_spawn_lo = (__int64)(int)((double)d * g_tick_scale * g_day_scale);
        play_family_sound(family, 0xc3);
        t = (float)(int)(fmod((double)*(__int64*)g_ticks_ * g_tick_scale, g_day_period) * g_hour_scale
                         + g_hour_base);
    }
    if (g_mission_clock) {
        float best_d = 1.0e5f;
        int best = -1;
        for (unsigned i = 0; i < (unsigned)size(); i++) {
            MissionItem* it = begin[i];
            if (it->kind() != 7)
                continue;
            float dx;
            if (it->rect[0] + it->rect[2] <= g_mouse_x && g_mouse_x < it->rect[0] + it->rect[2]) {
                dx = 0.0f;
            } else {
                dx = ABS(g_mouse_x - (it->rect[0] + it->rect[2]));
                float alt = ABS(g_mouse_x - (it->rect[0] + it->rect[2]));
                if (alt < dx)
                    dx = alt;
            }
            float dy;
            if (it->rect[1] + it->rect[3] <= g_mouse_y && g_mouse_y < it->rect[1] + it->rect[3]) {
                dy = 0.0f;
            } else {
                dy = ABS(g_mouse_y - (it->rect[1] + it->rect[3]));
                float alt = ABS(g_mouse_y - (it->rect[1] + it->rect[3]));
                if (alt < dy)
                    dy = alt;
            }
            float dist = (float)sqrt(dx * dx + dy * dy);
            if (dist < best_d) {
                best = i;
                best_d = dist;
            }
        }
        if (best >= 0) {
            int idx = best;
            ValueSource* u = item_value(begin[idx]);
            u->on_value_changed((float)g_mission_clock);
            ValueWidget* w = item_value(begin[best]);
            w->set_key_flags(0x10000);
        }
    }

    for (unsigned i = 0; i < (unsigned)size(); i++) {
        MissionItem* it = begin[i];
        MissionItem** p = &begin[i];
        for (int n = 0; n < 5; n++) {
            if ((*p)->nodes[n])
                timer_set((*p)->nodes[n], &t);
        }
        if ((*p)->used(0x10000)) {
            g_spawn_lo = i;
            g_spawn_hi = 0;
            int k = it->kind();
            if (k == 2 || k == 3) {
                g_spawn_hi = (__int64)(int)it->get_float();
                play_family_sound(family, 0xc2);
            } else if (k == 5) {
                g_spawn_hi = (__int64)it->get_value();
                play_family_sound(family, 0xc2);
            } else if (k == 6) {
                string_assign(&g_mission_title, &it->name);
                play_family_sound(family, 0xc2);
            } else if (k != 8 || it->f184 != 1) {
                play_family_sound(family, 0xc2);
            } else {
                g_spawn_hi = it->f180 != 0;
                play_family_sound(family, 0xc2);
            }
        } else if (it->used(2)) {
            g_spawn_hi = 0;
            g_spawn_lo = i;
            play_family_sound(family, 0xc1);
        } else if (it->used(4)) {
            g_spawn_hi = 1;
            g_spawn_lo = i;
            play_family_sound(family, 0xc1);
        }
    }
}
