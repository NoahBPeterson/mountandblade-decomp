//-----------------------------------------------------------------------------
// File: item_ref.cpp  (0x4625d0..0x462ac0)
//
// Desc: the sound-slot scheduler (per-slot state machine driven by the 1e-5 s clock table), the item mesh lookup
//       by class and modifier bit, and the modifier-adjusted item stats of an (item, modifier) pair. Names
//       provisional. Functions in ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP.
//
// Codegen levers found here: the slot's three Timer members are reached through Timer& (Timer::restart inlined), which
// keeps the stamp stores and clock-index loads in source order; `float secs = r * gap * c1; (int)(secs * c2)` keeps two multiplies where one
// expression would be reassociated.
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include "engine/core/tstring.h"
#include "game/ui/widgets/ui_common.h"
#include "game/data/item_ref_game_ui.h"
#include "game/data/trigger_rec.h"   // TriggerRec: update 0x4625d0
#include "game/data/trigger_vecs.h"             // TriggerVec: update_all 0x462760, randomize 0x462790
#include "game/data/item_ref.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/item_kind_include.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "engine/core/clock_table.h"

static __forceinline void restart_inl(Timer& t) { t.start = g_ticks[t.clock]; }     // Timer::restart (0x432bf0) as the callers inline it

static __forceinline void set_delay_inl(Timer& t, float seconds) { t.start = g_ticks[t.clock] - (int)(seconds * 100000.0f); }     // Timer::set_delay (0x42cd20) as randomize inlines it

// @ 0x4625d0
void TriggerRec::update(int arg)
{
    if (state == 2) {
        if (rearm_interval <= (float)(g_ticks[rearm_timer.clock] - rearm_timer.start) * 1e-5f) {
            state = 0;
            restart_inl(check_timer);
        }
    }
    if (state == 0) {
        if (check_interval <= -0.1f) {
            if (round_float(&check_interval) == arg && cond.play(arg)) {
                cons.play(arg);
                restart_inl(rearm_timer);
                state = 2;
                return;
            }
        } else if (arg >= 0) {
            if ((float)(g_ticks[check_timer.clock] - check_timer.start) * 1e-5f < check_interval) return;
            restart_inl(check_timer);
            if (cond.play(arg)) {
                restart_inl(delay_timer);
                state = 1;
            }
        }
    }
    if (state != 1) return;
    if (!(delay_interval <= (float)(g_ticks[delay_timer.clock] - delay_timer.start) * 1e-5f)) return;
    cons.play(arg);
    restart_inl(rearm_timer);
    state = 2;
}

// @ 0x462760
void TriggerVec::update_all(int arg)
{
    for (int i = 0; i < count; i++)
        data[i].update(arg);
}

// @ 0x462790
void TriggerVec::randomize()
{
    for (int i = 0; i < count; i++) {
        TriggerRec* s = &data[i];
        set_delay_inl(s->check_timer, (float)(rand() % 15817) * s->check_interval * 6.32231167e-05f);
    }
}


// @ 0x462820
tstring ItemKind::mesh_name(unsigned __int64 cls, int bit)
{
    tstring name;
    unsigned __int64 mask = 1i64 << bit;
    bit = -1;
    int i = 0;
    if (0 < mesh_count) {
        while (i < mesh_count) {
            unsigned __int64 c = meshes[i].flags & 0xF000000000000000ui64;
            if (c != 0 && c == cls) {
                if (bit == -1) bit = i;
                if (meshes[i].flags & mask) bit = i;
            }
            i++;
        }
        if (bit != -1) name = meshes[bit].name;
    }
    return name;
}

// @ 0x462900
tstring ItemRef::mesh_name(unsigned __int64 cls)
{
    if (item >= 0 && item < g_item_kind_count)
        return g_items[item].mesh_name(cls, b.mod);
    return tstring("");
}

// @ 0x462960
int ItemRef::head_armor()
{
    if (item < 0) return 0;
    int v = g_items[item].head_armor;
    if (v != 0) {
        switch (b.mod) {
        case imod_rusty:
        case imod_tattered: v -= 3; break;
        case imod_crude: v -= 1; break;
        case imod_sturdy: v += 1; break;
        case imod_thick: v += 2; break;
        case imod_heavy:
        case imod_hardened: v += 3; break;
        case imod_cracked: v -= 4; break;
        case imod_battered:
        case imod_ragged: v -= 2; break;
        case imod_reinforced: v += 4; break;
        case imod_lordly: v += 6; break;
        }
        if (v < 0) v = 0;
    }
    return v;
}

// @ 0x462a10
int ItemRef::leg_armor()
{
    if (item < 0) return 0;
    int v = g_items[item].leg_armor;
    if (v != 0) {
        switch (b.mod) {
        case imod_rusty:
        case imod_tattered: v -= 3; break;
        case imod_crude: v -= 1; break;
        case imod_sturdy: v += 1; break;
        case imod_thick: v += 2; break;
        case imod_heavy:
        case imod_hardened: v += 3; break;
        case imod_cracked: v -= 4; break;
        case imod_battered:
        case imod_ragged: v -= 2; break;
        case imod_reinforced: v += 4; break;
        case imod_lordly: v += 6; break;
        }
        if (v < 0) v = 0;
    }
    return v;
}

