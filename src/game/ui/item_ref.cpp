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
#include "../tstring.h"
#include "./ui_common.h"
#include "./item_ref.h"
#include "../item_ref.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/item_kind.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/clock_table.h"

static __forceinline void restart_inl(Timer& t) { t.start = g_ticks[t.clock]; }     // Timer::restart (0x432bf0) as the callers inline it

static __forceinline void set_delay_inl(Timer& t, float seconds) { t.start = g_ticks[t.clock] - (int)(seconds * 100000.0f); }     // Timer::set_delay (0x42cd20) as randomize inlines it

// @ 0x4625d0
void snd_slot::update(int arg)
{
    if (state == 2) {
        if (hold_time <= (float)(g_ticks[hold.clock] - hold.start) * 1e-5f) {
            state = 0;
            restart_inl(gap);
        }
    }
    if (state == 0) {
        if (min_gap <= -0.1f) {
            if (round_float(&min_gap) == arg && first.play(arg)) {
                second.play(arg);
                restart_inl(hold);
                state = 2;
                return;
            }
        } else if (arg >= 0) {
            if ((float)(g_ticks[gap.clock] - gap.start) * 1e-5f < min_gap) return;
            restart_inl(gap);
            if (first.play(arg)) {
                restart_inl(play);
                state = 1;
            }
        }
    }
    if (state != 1) return;
    if (!(play_time <= (float)(g_ticks[play.clock] - play.start) * 1e-5f)) return;
    second.play(arg);
    restart_inl(hold);
    state = 2;
}

// @ 0x462760
void snd_bank::update_all(int arg)
{
    for (int i = 0; i < count; i++)
        slots[i].update(arg);
}

// @ 0x462790
void snd_bank::randomize()
{
    for (int i = 0; i < count; i++) {
        snd_slot* s = &slots[i];
        set_delay_inl(s->gap, (float)(rand() % 15817) * s->min_gap * 6.32231167e-05f);
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
int ItemRef::stat_1fc()
{
    if (item < 0) return 0;
    int v = g_items[item].f1fc;
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
int ItemRef::stat_204()
{
    if (item < 0) return 0;
    int v = g_items[item].f204;
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

