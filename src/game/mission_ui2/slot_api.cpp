//-----------------------------------------------------------------------------
// File: slot_api.cpp  (0x48e5d0..0x48ef40)
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
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../ui/ui_common.h"
#include "../ui/item_ref.h"
#include "../ui/mission_ui.h"
#include "../config/widget_mgr.h"
#include "./slot_api.h"
#include "../battle_rec.h"      // BattleTable::get 0x48ef20
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../mission.h"
#include "../item_ref.h"
#include "../str_table.h"

#include "../../include/item_kind.h"
#include "../../include/troop.h"
#include "../../include/party.h"
#include "../placed_obj.h"
#include "../../include/rec78.h"
#include "../../include/shape.h"
#include "../../include/game_data.h"
#include "../../include/level_global.h"      // g_level (0xa732b4)
#include "../../include/clock_table.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object


// the animators fold the clock delta since the last start into the busy time: (g_ticks[clock] - stamp) * 1e-5 + seconds + 1
#define ANIMATE(slot, value)                                                                      \
    ((AnimVt*)widgets[idx])->slot(t, value);                                                      \
    {                                                                                             \
        float done = (float)(g_ticks[clock] - *(__int64*)&stamp_lo) * 1e-5f + t + 1.0f;   \
        busy_until = (done < busy_until) ? busy_until : done;                                     \
    }

// @ 0x48e5d0
void widget_mgr::animate_pos(int idx, float t, int v)
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
void widget_mgr::animate_size(int idx, float t, int v)
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

// @ 0x48ea80
face* GameData::find_at(const float* p)
{
    int count;
    int flag;
    int idx[2];
    float q[4];
    shape_base* cand[0x2000];

    rgl::world* level = g_script.level;
    q[0] = p[0];
    q[1] = p[1];
    q[2] = 0;                                      // 0x48ea80 builds a 3-float point with z = 0
    flag = 0;
    level->to_cell(q, idx);
    count = 0;
    rgl::cell* row = level->cells[idx[0]];
    row[idx[1]].collect_point(q, &flag, 0x100, cand, &count);
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
    Troop* agent = &troops[player];
    int n = agent->inv_count();
    int free_slot = -1;
    int i;
    for (i = 0; i < n; i++) {
        if (agent->inv[i].item == -1) {
            free_slot = i;
            break;
        }
    }

    if (free_slot >= 0) {
        agent->set_slot(item, free_slot, amount);
        if (f13a34 == 0)
            return;

        char buf[0x200];
        {
            tstring key("ui_got_item_value");
            tstring* nm = g_strtable.find(&g_items[item].key);
            const char* name = (nm ? nm : (tstring*)&g_items[item])->c_str();
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
    unsigned flags = g_items[item].type_flags;
    if (!((flags >> 12) & 1))
        return;

    int replace = -1;
    int m = agent->inv_count();
    for (i = 0; i < m; i++) {
        if (replace != -1)
            break;
        unsigned f = g_items[agent->inv[i].item].type_flags;
        if (!((f >> 12) & 1))
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
    return v._Myfirst ? (int)(((char*)v._Mylast - (char*)v._Myfirst) / (int)sizeof(PlacedObj)) : 0;
}

// @ 0x48ed60
void Mission::retag_props(int from, int to)
{
    for (unsigned i = 0; i < prop_count(props); i++) {
        PlacedObj* r = &props._Myfirst[i];
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
        PlacedObj* r = &props._Myfirst[i];
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
        PlacedObj* r = &props._Myfirst[i];
        if (!r->kind && r->id == tag) {
            if (found == nth)
                return i;
            found++;
        }
    }
    return -1;
}

// @ 0x48ef20
BattleRec* BattleTable::get(unsigned index)
{
    std::vector<BattleRec*>& pg = *(std::vector<BattleRec*>*)((char*)this + 0x14);
    return pg[index >> 4] + (index & 0xf);
}
