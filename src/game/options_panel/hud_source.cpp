// hud_source.cpp: the mission item source's per-frame handler and the menu records' slot 3 (0x4d5410..0x4d5ba0). The file boundary
// at 0x4d5410 is the original's: its caller, ItemRec::frame_update 0x4d5290 (hud.cpp), compiled with only a declaration of pick.
// Names provisional.
#include <windows.h>
#include "../game_globals.h"
#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): scene_counter
#include <math.h>
#include <new>
#include "./mesh_edit.h"
#include "./hud.h"
#include "../window_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mission.h"                               // Mission::destroy 0x4d51d0 (the world object 0x5e292c is the mission)
#include "../mission_source.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../screen_records.h"   // Rec38 / Rec44 / Rec454: the records whose slot 3 these are (vtables 0x595828 / 0x5957e0 / 0x595894)
#include "../triggers/item_rec.h"   // ItemRec: slot 3 is handle_buttons (vtable 0x596124)
#include "quest_page.h"        // QuestPage: Hud is the quest page screen (QuestPage::sel is Hud::f250), fill() 0x4d5ba0
#include "../shared_vars.h"
#include "../key_names.h"
#include "../ui_globals.h"      // UI / core globals

// mesh_node (rgl/xmlcfg/game_util.h, whose shape.h clashes with this TU's headers): the animation node's time setter.
struct mesh_node {
    void set_time(const float* t);   // 0x433190
};

// @ 0x4d5940
__declspec(noinline) void ValueSource::on_value_changed(float arg)
{
    ((UiD*)(void*)this)->set_value((step / slot[idx] * arg) * 0.0005f + ((UiD*)(void*)this)->value());
}

// vector<MissionUi*>::operator[] as the original compiles it in the key loop: begin() + i through the iterator's
// operator+, which cl left out of line here (the COMDAT 0x534160, defined in map_editor/list_screens.cpp; the same
// stand-in as inventory_panel/mission_ctx.cpp's). Each use has its own result temporary: the original's ten slots.
struct VecIter {
    int* p;
    __declspec(noinline) int** advance(int** out, int n);   // 0x534160 (ret 8)
};
static __forceinline MissionUi* item_at(MissionSource* s, unsigned i)
{
    VecIter it;
    it.p = (int*)s->begin;
    int* out;
    return *(MissionUi**)*it.advance(&out, i);
}
struct MissionSourcePick : MissionSource {          // local view: the clock timer at +0x18 (tracks)
    Timer tracks;                                   // +0x18
    void pick(int* arg);
};
// @ 0x4d5410
void MissionSourcePick::pick(int* arg)
{
    if (family < 0)
        return;
    float t;
    {
        int clock = slot;
        __int64 d = g_ticks[clock] - *(__int64*)&tick_lo;
        g_script.param1 = (__int64)(int)((float)d * 1000.0f * 1e-5f);
        play_family_sound(family, -0x3d);
        t = (float)(fmod((double)g_ticks[0] * 1e-5f, 1.0) * 40.0f + 100.0f);
    }
    if (g_gui.wheel) {
        float best_d = 1.0e5f;
        int best = -1;
        for (unsigned i = 0; i < (unsigned)size(); i++) {
            MissionUi* it = begin[i];
            if (it->kind() == 7) {
                float dx;
                if (it->screen[2] + it->e[0] <= g_gui.mouse[0] && g_gui.mouse[0] < it->screen[0] + it->e[0]) {
                    dx = 0;
                } else {
                    dx = ABS(g_gui.mouse[0] - (it->screen[0] + it->e[0]));
                    float alt = ABS(g_gui.mouse[0] - (it->screen[2] + it->e[0]));
                    if (alt < dx)
                        dx = alt;
                }
                float dy;
                if (it->screen[3] + it->e[1] <= g_gui.mouse[1] && g_gui.mouse[1] < it->screen[1] + it->e[1]) {
                    dy = 0;
                } else {
                    dy = ABS(g_gui.mouse[1] - (it->screen[1] + it->e[1]));
                    float alt = ABS(g_gui.mouse[1] - (it->e[1] + it->screen[3]));
                    if (alt < dy)
                        dy = alt;
                }
                float dist = (float)sqrt(dx * dx + dy * dy);
                if (dist < best_d) {
                    best = i;
                    best_d = dist;
                }
            }
        }
        if (best >= 0) {
            ValueSource* u = item_value(begin[best]);
            u->on_value_changed((float)g_gui.wheel);
            item_value(begin[best])->set_key_flags(0x10000);
        }
    }
    for (unsigned i = 0; i < (unsigned)size(); i++) {
        for (int n = 0; n < 5; n++) {
            if (begin[i]->tracks[n])
                ((mesh_node*)begin[i]->tracks[n])->set_time(&t);
        }
        if (item_at(this, i)->clear_mask(0x10000)) {
            g_script.param1 = i;
            if (item_at(this, i)->kind() == 2) {
                g_script.param2 = (__int64)(int)item_at(this, i)->get168();
            } else if (item_at(this, i)->kind() == 3) {
                g_script.param2 = (__int64)(int)item_at(this, i)->get168();
            } else if (item_at(this, i)->kind() == 5) {
                g_script.param2 = (__int64)item_at(this, i)->get15c();
            } else if (item_at(this, i)->kind() == 6) {
                g_script.str[0] = item_at(this, i)->name;
            } else if (item_at(this, i)->kind() == 8 && ((UiA*)item_at(this, i))->f184 == 1) {
                g_script.param2 = ((UiA*)item_at(this, i))->f180 != 0;
            }
            play_family_sound(family, -0x3e);
        } else if (item_at(this, i)->clear_mask(2)) {
            g_script.param2 = 0;
            play_family_sound(family, -0x3f);
            g_script.param1 = i;
        } else if (item_at(this, i)->clear_mask(4)) {
            g_script.param2 = 1;
            g_script.param1 = i;
            play_family_sound(family, -0x3f);
        }
    }
    }

// @ 0x4d5980
void Rec44::frame_update(float)
{
    g_mission->scene_counter = g_mission_ctx.scene_counter;
    g_mission_ctx.scene_counter = g_mission_ctx.scene_counter + 1;
    g_mission->start_setup();
    slot_as<MenuScreen>(14)->v4();                                 // +0x10 (0x4d59ad)
    slot_as<MenuScreen>(14)->f18 = w18;
    slot_as<int>(11)[1] = 0;
    slot_as<int>(11)[0x18 / 4] = w18;
    g_windows.set_state(0xe);
}

// @ 0x4d59f0
void Rec38::frame_update(float)
{
    if (((MissionUi*)(void*)f34)->clear_mask(0x10000))
        g_windows.set_state(7);
}

#define W(x) ((MissionUi*)(void*)(x))
// @ 0x4d5a20
void Rec454::frame_update(float)
{
    if (g_gui.wheel) {
        ValueWidget* u = item_value((MissionUi*)(void*)list);
        float v = (u->f168 - u->f16c) / (u->f170 - u->f16c) * (u->slot[u->idx] - u->step);
        u->set_value((float)g_gui.wheel / u->slot[u->idx] * u->step * 0.0005f + v);
        item_value((MissionUi*)(void*)list)->set_key_flags(0x10000);
    }
    for (int i = 0; i < count; i++) {
        if (W(rows[i])->clear_mask(0x10000)) {
            f250 = i;
            ((QuestPage*)(void*)this)->fill();
        }
    }
    if (W(active_tab)->clear_mask(0x10000)) {
        if (w04 == 0) {
            W(active_tab)->set15c(1);
            return;
        }
        w04 = 0;
        g_windows.set_state(0xd);
    } else if (W(finished_tab)->clear_mask(0x10000)) {
        if (w04 == 1) {
            W(finished_tab)->set15c(1);
            return;
        }
        w04 = 1;
        g_windows.set_state(0xd);
    } else if (W(done)->clear_mask(0x10000) || (!g_keyboard.down[0x1] && g_keyboard.prev[0x1])) {
        g_windows.set_state(w18);
    }
}
#undef W

