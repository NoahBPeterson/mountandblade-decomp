// mission_pick.cpp: MissionSource::pick (0x4d5410), the item source's per-frame pick of the screen's widgets. Its own object:
// the caller (Hud::handle_buttons, hud.cpp) must not see the body (cl would see that &arg does not escape and turn the
// set_state calls into tail jumps). Names provisional.
#include <windows.h>
#include "../game_globals.h"
#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): scene_counter
#include <math.h>
#include <new>
#include "./mesh_edit.h"
#include "./hud.h"
#include "../main_menu.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
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

// @ 0x4d5410
void MissionSource::pick(int* arg)
{
    float t;
    if (family < 0)                     // 0x4d541b: jl to the epilogue -- nothing else runs without a family
        return;
    {
        __int64 now = g_ticks[slot];
        __int64 d = now - *(__int64*)&tick_lo;      // 0x4d5433: sub/sbb with the 64-bit tick at +0x20
        g_script.param1 = (__int64)(int)((double)d * 1000.0f * 1e-5f);
        play_family_sound(family, -0x3d);         // (char)0xc3: the original pushes the ids sign-extended
        t = (float)(fmod((double)g_ticks[0] * 1e-5f, 1.0) * 40.0f
                    + 100.0f);             // 0x4d5498: fmul, fadd, fstp -- no integer truncation
    }
    if (g_mission_clock) {
        float best_d = 1.0e5f;
        int best = -1;
        for (unsigned i = 0; i < (unsigned)size(); i++) {
            MissionUi* it = begin[i];
            if (it->kind() != 7)
                continue;
            // inside [org + near, org + far) on an axis: 0, else the distance to the nearer edge (0x4d54f5..0x4d55f0)
            float dx;
            if (it->screen[2] + it->e[0] <= g_gui.mouse[0] && g_gui.mouse[0] < it->screen[0] + it->e[0]) {
                dx = 0.0f;
            } else {
                dx = ABS(g_gui.mouse[0] - (it->screen[0] + it->e[0]));
                float alt = ABS(g_gui.mouse[0] - (it->screen[2] + it->e[0]));
                if (alt < dx)
                    dx = alt;
            }
            float dy;
            if (it->screen[3] + it->e[1] <= g_gui.mouse[1] && g_gui.mouse[1] < it->screen[1] + it->e[1]) {
                dy = 0.0f;
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
        if (best >= 0) {
            int idx = best;
            ValueSource* u = item_value(begin[idx]);
            u->on_value_changed((float)g_mission_clock);
            ValueWidget* w = item_value(begin[best]);
            w->set_key_flags(0x10000);
        }
    }

    for (unsigned i = 0; i < (unsigned)size(); i++) {
        // every use re-reads the item from the vector (the original goes through its iterator, 0x534160, each
        // time): a callee may move the storage, so a pointer to begin[i] taken once can go stale
        for (int n = 0; n < 5; n++) {
            if (begin[i]->tracks[n])
                ((mesh_node*)begin[i]->tracks[n])->set_time(&t);
        }
        if (begin[i]->clear_mask(0x10000)) {
            g_script.param1 = i;                 // 0x4d56eb: only the low pair is written here; g_spawn_hi stays stale unless a kind sets it
            int k = begin[i]->kind();
            if (k == 2 || k == 3) {
                g_script.param2 = (__int64)(int)begin[i]->get168();
                play_family_sound(family, -0x3e);
            } else if (k == 5) {
                g_script.param2 = (__int64)begin[i]->get15c();
                play_family_sound(family, -0x3e);
            } else if (k == 6) {
                g_script.str[0] = begin[i]->name;
                play_family_sound(family, -0x3e);
            } else if (k != 8 || ((UiA*)begin[i])->f184 != 1) {
                play_family_sound(family, -0x3e);
            } else {
                g_script.param2 = ((UiA*)begin[i])->f180 != 0;
                play_family_sound(family, -0x3e);
            }
        } else if (begin[i]->clear_mask(2)) {
            g_script.param2 = 0;
            g_script.param1 = i;
            play_family_sound(family, -0x3f);
        } else if (begin[i]->clear_mask(4)) {
            g_script.param2 = 1;
            g_script.param1 = i;
            play_family_sound(family, -0x3f);
        }
    }
}
