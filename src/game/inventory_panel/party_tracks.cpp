//-----------------------------------------------------------------------------
// File: party_tracks.cpp  (0x518680..0x5191e0)
//
// Desc: the party-track ageing (track_parties), the track ribbons and the town training camp of the game object.
//       Split from game_tick.cpp: track_parties 0x518680 was compiled with the Map10007::lookup body visible
//       (it keeps main_party in a register across the lookup call), update_cursor 0x5179f0 in game_tick.cpp without it
//       (docs/codegen.md 16, "a same-unit callee body can hide a translation-unit boundary"); train_troops 0x518d70
//       also gains from not seeing game_tick's bodies (aligned 1098 -> 514). Same macros and flags as game_tick.cpp
//       ("T regime": the file keeps the binary order). Names provisional. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./game_tick.h"
#include "./inv_common.h"      // (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../str_table.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../strat/gui_manager.h"
#include "../../rgl/mesh.h"
#include "../rec5c.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../config/config.h"
#include "../shared_vars.h"
#include "../key_names.h"
#include "../game_globals.h"
#include "../../include/render_flags.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "../party2/party_model.h"   // strategic_entity::set_fade 0x4b1800 (Party::model)


// ---- globals and callees of other translation units ----------------------------------------------------------------
extern const float g_k002;       // 0x590cd0  marker decay per second

#include "../game_globals.h"      // g_module_settings.disable_food_slot, g_module_settings.player_wounded_treshold (canonical)
void set_clock_step(int* clock, float step);            // 0x461420
bool is_daytime();                                          // 0x435700 (true in the 4..20 h window; game/agent/agent.cpp)
// GameData::remove_party 0x4915d0 (game/world_data)
#include "../res_mgr.h"                         // g_resmgr: get_object 0x419540 (rgl/core/resmgr.h)
extern "C" int __cdecl sprintf(char*, const char*, ...);   // 0x536f8f
#define GAMMA (0.45454543828964233f)                    // 0x5a9f1c  1 / 2.2 (the compiler's rounding)

// the iterator helper on a bare base pointer
static Rec5c* rec_at(Rec5c* base, int idx)
{
    ItemArray a;
    a.base = base;
    return a.at(idx).p;
}

// the id lookup: visible here too, as a header inline in the original (the same definition as ui/mission_ui.cpp; the row
// owner wins the link). With it track_parties keeps main_party in a register across the lookup call (0x518694).
inline int Map10007::lookup(unsigned id)
{
    if (id < 0x2717)
        return tab[id].value;
    int h = id % 0x2717;
    int n = 0;
    if (tab[h].key != id) {
        do {
            if (n >= 1000)
                return -1;
            h = (h + 0x4f) % 0x2717;
            n++;
        } while (tab[h].key != id);
    }
    if (n < 1000)
        return tab[h].value;
    return -1;
}

// @ 0x518680
void GameG::track_parties(float dt)
{
    PartyV4 pos = player_party()->pos;
    int skill = player_party()->skill_level(0x10);
    float radius = ((float)skill * 0.125f + 1.0f) * g_module_settings.track_spotting;
    if (!is_daytime())
        radius *= 0.3f;
    float threshold = radius * radius;
    if (player_party()->skill_level(0x10) == 0)
        threshold = -0.1f;

    const int SEEN = 0x20000000;
    Rec5c* rec = track_vec()._Myfirst;
    while (rec < track_vec()._Mylast) {
        rec->f14 += dt;
        if ((rec->flags & SEEN) == 0) {
            float dx = rec->x - pos.x;
            float dy = rec->y - pos.y;
            float dz = rec->z - pos.z;
            if (dx * dx + dz * dz + dy * dy < threshold)
                rec->flags |= SEEN;
        }
        rec++;
    }

    float sf = skill_plus_8();
    int last = track_vec().size() - 1;
    int i = 0;
    while (i <= last) {
        Rec5c* cur = rec_at(track_vec()._Myfirst, i);
        unsigned char b = cur->f1a;
        float t = (float)((b & 0xf) + 0x10) * 0.03125f;
        t *= sf;
        if (rec_at(track_vec()._Myfirst, i)->f14 > t) {
            *rec_at(track_vec()._Myfirst, i) = *rec_at(track_vec()._Myfirst, last);
            last--;
        } else {
            i++;
        }
    }
    if (last < track_vec().size() - 1) {
        Rec5c* dummy;
        track_vec().shift_over(&dummy, track_vec()._Myfirst + (last + 1), track_vec()._Mylast);
    }
}

// @ 0x518940
float GameG::skill_plus_8()
{
    int s = player_party()->skill_level(0x10);
    return (float)s + 8.0f;
}

// @ 0x518990
Rec5c** Vec5c::shift_over(Rec5c** out, Rec5c* dst, Rec5c* src)
{
    if (dst != src) {
        Rec5c* end = _Mylast;
        Rec5c* p = dst;
        while (src != end) {
            *p = *src;
            ++p;
            ++src;
        }
        _Mylast = p;
    }
    *out = dst;
    return out;
}

// @ 0x5189e0
void GameG::update_tracks()
{
    map_track->clear_geometry();

    rgl::mesh* track;
    {
        tstring n("track");
        track = g_resmgr.get_object(&n);
    }
    rgl::mesh* battle;
    {
        tstring n("battle_track");
        battle = g_resmgr.get_object(&n);
    }

    float denom = (float)party_at(&parties, parties.lookup(main_party))->skill_level(0x10) + 8.0f;

    for (Rec5c* p = track_vec()._Myfirst; track_vec()._Mylast > p; ++p) {
        if (!reveal_all && !(p->flags & 0x20000000))
            continue;

        int first = (int)map_track->vertices.size();
        rgl::mesh* m = track;
        if (p->flags & 0x10000000)
            m = battle;
        map_track->append_skinned(m, (rgl::matrix*)((char*)p + 0x1c), -1, -1, 0);

        float t = p->f14 / denom;
        t = (t < 1.0f) ? t : 1.0f;
        t *= 3.0f;

        // the ramp is two vector4 products summed through the inline operators (0x518b53 / 0x518bee): the right
        // operand's product is built first, each product and the sum in their own temporaries, then c is assigned whole
        rgl::vector4 c;
        if (t < 1.0f) {
            float u = 1.0f - t;
            c = rgl::vector4(0.9f, 0.8f, 0.0f) * t + rgl::vector4(1.2f, 0.0f, 0.0f) * u;
        } else {
            float u = t - 1.0f;
            if (u > 1.0f)
                u = 1.0f;
            float v = 1.0f - u;
            c = rgl::vector4(0.0f, 0.6f, 0.8f) * u + rgl::vector4(0.9f, 0.8f, 0.0f) * v;
        }

        unsigned col = rgl::pack_color(&c);
        if (party_at(&parties, parties.lookup(main_party))->skill_level(0x10) < 2)
            col = 0xffa8a8a8;
        col |= 0xff000000;

        for (int i = first; i < map_track->vertices.size(); i++)
            map_track->vertices[i].color = col;
    }

    float d = 1000000.0f;
    track_entity->set_fbc(&d);
}

// @ 0x518d70
void GameG::train_troops()
{
    for (unsigned id = parties.first_party_id(); id < parties.serial; parties.next_party_id((int*)&id))
        party_at(&parties, parties.lookup(id))->dirty = 1;

    Party* p = party_at(&parties, parties.lookup(main_party));

    int gain[64];
    for (int i = 0; i < p->nstacks; i++)
        gain[i] = 0;

    int limit[11] = {0, 4, 10, 0x10, 0x17, 0x1e, 0x26, 0x2e, 0x37, 0x41, 0x50};
    int total = 0;

    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if (s.flags & 1)
            continue;
        if (!(troops[s.troop].flags30 & 0x10))
            continue;
        if (s.troop == player ? troops[s.troop].hp < g_module_settings.player_wounded_treshold : troops[s.troop].hp < g_module_settings.hero_wounded_treshold)
            continue;
        int lvl = skill_level(s.troop, 0x11);
        if (lvl > 10)
            lvl = 10;
        int cost = limit[lvl];
        if (!cost)
            continue;

        int j = 0;
        int n = p->nstacks;
        if (n > 0) do {
            Stack& q = p->stacks[j];
            if (j == i)
                continue;
            if (q.flags & 1)
                continue;
            if (troops[s.troop].level <= troops[q.troop].level)
                continue;
            if (troops[q.troop].flags30 & 0x10) {
                if (q.troop == player ? troops[q.troop].hp < g_module_settings.player_wounded_treshold : troops[q.troop].hp < g_module_settings.hero_wounded_treshold)
                    continue;
                gain[j] += cost;
                total += cost;
            } else {
                if (!troops[q.troop].upgrade_troop[0])
                    continue;
                int ready = q.count - q.wounded;
                gain[j] += cost * (ready > 0 ? ready : 0);
                total += cost * (ready > 0 ? ready : 0);
            }
        } while (++j < n);
    }

    if (total != 0) {
        g_gui.add(*g_strtable.lookup(tstring("ui_days_training_is_complete")), -1);
        char buf[512];
        sprintf(buf, g_strtable.lookup(tstring("ui_total_experience_gained_through_training_number"))->c_str(), total);
        g_gui.add(tstring(buf), -1);
    }

    bool changed = false;
    for (int i = 0; i < p->nstacks; i++) {
        Stack& q = p->stacks[i];
        if (troops[q.troop].flags30 & 0x10) {
            add_experience(q.troop, gain[i]);
        } else {
            int old = q.f14;
            p->add_xp_to_stack(i, gain[i]);
            if (q.f14 > old)
                changed = true;
        }
    }
    if (changed)
        g_gui.add(*g_strtable.lookup(tstring("ui_some_soldiers_are_ready_to_upgrade")), -1);
}
