//-----------------------------------------------------------------------------
// File: widget_overlay_api.cpp  (0x48e5d0..0x48ef40)
//
// Desc: the widget manager's animated setters (overlay_animate_to_*), the string record tables, the game data's script
//       string and inventory-add operations, the grid query for a point, the scene props' retag / count / find, and the
//       page table of 0x98-byte records. Names provisional. Functions in ascending address order (cl's bottom-up emission).
//       Flags: /O2 /MT /Gy /TP.
//
// Boundary: class changes only (Mission | SlotAPI at 0x48e5d0, InvOwner | Mission at 0x48ef40); no forward call in or out.
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include "engine/core/tstring.h"
#include "engine/math/rgl_math.h"
#include "engine/render/mesh.h"
#include "game/ui/widgets/ui_common.h"
#include "game/data/item_ref_game_ui.h"
#include "game/ui/widgets/widget_base.h"
#include "game/ui/widgets/widget_manager.h"
#include "game/ui/widgets/widget_overlay_api.h"
#include "game/campaign/event.h"           // Event: the records of Pool<Event>::get 0x48ef20
#include "game/campaign/event.h"           // Event, the record of Pool<Event>
#include "engine/core/pool.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "game/mission/mission.h"
#include "game/data/item_ref.h"
#include "game/data/str_table.h"

#include "game/data/item_kind_include.h"
#include "game/data/troop.h"
#include "game/party/party.h"
#include "game/mission/placed_obj.h"
#include "game/mission/mission_obj.h"      // MissionObj: the scene prop kind (g_propkinds)
#include "engine/physics/shape.h"
#include "game/data/game_data.h"
#include "game/common/level_global.h"      // g_level (0xa732b4)
#include "engine/core/clock_table.h"
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object


// the animators extend the presentation to the animation's end: (g_ticks[tick.clock] - tick.start) * 1e-5 + seconds + 1
#define ANIMATE(slot, value)                                                                      \
    widgets[idx]->slot(t, value);                                                      \
    {                                                                                             \
        float done = (float)(g_ticks[tick.clock] - tick.start) * 1e-5f + t + 1.0f;   \
        duration = (done < duration) ? duration : done;                                     \
    }

// @ 0x48e5d0
void widget_mgr::animate_pos(int idx, float t, const vec2& v)
{
    ANIMATE(animate_pos, v)
}

// @ 0x48e650
void widget_mgr::animate_color(int idx, float t, int rgb)
{
    ANIMATE(animate_color, rgb)
}

// @ 0x48e6d0
void widget_mgr::animate_alpha(int idx, float t, int a)
{
    ANIMATE(animate_alpha, a << 24)
}

// @ 0x48e750
void widget_mgr::animate_hilight_color(int idx, float t, int rgb)
{
    ANIMATE(animate_hilight_color, rgb)
}

// @ 0x48e7d0
void widget_mgr::animate_hilight_alpha(int idx, float t, int a)
{
    ANIMATE(animate_hilight_alpha, a << 24)
}

// @ 0x48e850
void widget_mgr::animate_size(int idx, float t, const vec2& v)
{
    ANIMATE(animate_size, v)
}

// @ 0x48e8d0
__declspec(noinline) tstring* StringTable::Str::resolve()
{
    tstring* r = g_strtable.find(&key);
    return r ? r : &name;
}

// @ 0x48e8f0
tstring StringTable::get_string(int index, int kind)
{
    if (kind == 0) {
        if (index >= 0 && index < 0x80)
            return g_script.str[index];
        return tstring("Invalid String-register!");
    }
    if (kind == 0x16) {
        if (index >= 0 && index < quick_count) {
            Str* rec = &quick[index];
            tstring* r = g_strtable.find(&rec->key);
            return *(r ? r : &rec->name);
        }
        return tstring("Invalid Quick-String!");
    }
    if (index >= 0 && index < count)
        return *(strings[index].resolve());
    return tstring("Unexisting String!");
}

// @ 0x48e9e0
tstring GameData::string_value(int index)
{
    g_script.result_string.assign("");
    if (!emit_event(0x10, index, 0)) {
        char buf[0x200];
        {
            tstring key("ui_value_denars");
            tstring* value = g_strtable.lookup(key);
            sprintf(buf, value->c_str(), index);
        }
        g_script.result_string.assign(buf);
    }
    return g_script.result_string;
}

bool __cdecl in_box(const vector4* lo, const vector4* hi, const vector4* p);   // 0x4336d0
// rgl::cell::collect_point (0x439780, game/agent/agent_stats.cpp) with its body visible, as the original's unit saw it: the
// candidate array and the counts then do not escape, so the row pointer stays a register (mov esi,[edx+eax*4]; add ecx,esi)
// and the loop bound is loaded straight into ebx
inline __declspec(noinline) void rgl::cell::collect_point(const vector4* p, int* use3d, unsigned mask, shape_base** out, int* count)
{
    unsigned i = 0;
    *count = 0;
    while (i < a.size()) {
        shape_base** q = &a[i];
        shape_base* sh = *q;
        if (sh->m8 & mask) {
            i++;
            continue;
        }
        vector4 tv[2];
        const vector4* smax = sh->get_max(&tv[0]);
        const vector4* smin = sh->get_min(&tv[1]);
        bool hit;
        if (*use3d == 0)
            hit = smin->x <= p->x && p->x <= smax->x && smin->y <= p->y && p->y <= smax->y;
        else
            hit = in_box(smin, smax, p);
        if (!hit) {
            i++;
            continue;
        }
        out[*count] = *q;
        ++*count;
        i++;
    }
}

// @ 0x48ea80
face* GameData::find_at(const float* p)
{
    int count;
    int flag;
    int idx[2];
    vector4 q;
    shape_base* cand[0x2000];

    rgl::world* level = g_script.level;
    q.x = p[0];
    q.y = p[1];
    q.z = 0;                                      // 0x48ea80 builds a 3-float point with z = 0
    flag = 0;
    level->to_cell(&q.x, idx);
    count = 0;
    rgl::cell* row = level->cells[idx[0]];
    row[idx[1]].collect_point(&q, &flag, 0x100, cand, &count);
    int n = count;
    for (int k = 0; k < n; k++) {
        shape_base* s = cand[k];
        if (s->m4 < 0)
            continue;
        if (((face*)s)->contains_xy(p))
            return (face*)cand[k];
    }
    return 0;
}

// @ 0x48eb50
void GameData::add_inventory(int item, int amount)
{
    char buf[0x200];
    Troop* agent = troops + player;
    int n = agent->inv_count();
    int free_slot = -1;
    int i;
    for (i = 0; free_slot == -1 && i < n; i++) {
        if (agent->inv[i].item == -1)
            free_slot = i;
    }

    if (free_slot >= 0) {
        agent->set_slot(item, free_slot, amount);
        if (f13a34 == 0)
            return;

        {
            tstring key("ui_got_item_value");
            ItemKind* k = &g_items[item];
            tstring* nm = g_strtable.find(&k->key);
            const char* name = (nm ? nm : (tstring*)k)->c_str();
            tstring* fmt = g_strtable.lookup(key);
            sprintf(buf, fmt->c_str(), name);
        }
        {
            tstring msg(buf);
            g_gui.push(msg, (int)0xff80ff80);
        }
        return;
    }

    // No free slot: only a unique item may displace a non-unique one.
    bool unique = (g_items[item].type_flags >> 12) & 1;
    if (!unique)
        return;

    int m = agent->inv_count();
    int replace = -1;
    for (i = 0; replace == -1 && i < m; i++) {
        bool u = (g_items[agent->inv[i].item].type_flags >> 12) & 1;
        if (!u)
            replace = i;
    }
    if (replace == -1) {
        mb_error("All items in inventory are unique!");
        return;
    }

    // The original indexes the old row with the loop counter (one past `replace`).
    int old_item = agent->inv[i].item;
    ItemRef* r = &agent->inv[replace];
    r->item = item;
    r->m4 = 0;
    r->reduce(0);
    r->b.mod = 0;
    r->reduce(0);
    add_inventory(old_item, amount);
}

// the number of 0x1c8-byte prop records: a null vector holds none (the original guard, 0x48ed70/0x48ee33)
static inline int prop_count(const std::vector<PlacedObj>& v)
{
    return (int)v.size();
}

// @ 0x48ed60
void Mission::retag_props(int from, int to)
{
    for (unsigned i = 0; i < prop_count(props); i++) {
        PlacedObj* r = &props[i];
        if (r->kind)
            continue;
        if (!r->name.equals(g_propkinds[from].name))
            continue;
        props[i].id = to;
        props[i].name = g_propkinds[to].name;
    }
}

// @ 0x48ee20
int Mission::count_props(int tag)
{
    int count = 0;
    for (unsigned i = 0; i < prop_count(props); i++) {
        PlacedObj* r = &props[i];
        if (!r->kind && r->id == tag)
            count++;
    }
    return count;
}

// @ 0x48ee90
int Mission::find_prop(int tag, int nth)
{
    int found = 0;
    for (unsigned i = 0; i < prop_count(props); i++) {
        PlacedObj* r = &props[i];
        if (!r->kind && r->id == tag) {
            if (found == nth)
                return i;
            found++;
        }
    }
    return -1;
}

// @ 0x48ef20
template<> Event* Pool<Event>::get(unsigned index)
{
    return pages[index >> 4] + (index & 0xf);
}
