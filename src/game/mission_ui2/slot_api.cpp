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
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../mission.h"
#include "../item_ref.h"
#include "../str_table.h"
#include "../cell.h"
#include "../../include/item_kind.h"
#include "../../include/troop.h"
#include "../../include/party.h"
#include "../../include/rec1c8.h"
#include "../../include/rec78.h"
#include "../../include/shape.h"
#include "../../include/game_data.h"

extern StrTable g_strtable;                     // 0x5e2a18
extern tstring g_result_string;                 // 0xa70464 the last formatted script string
extern tstring g_string_registers[128];         // 0xa72890 the script's string registers
extern LevelGrid* g_level_grid;                 // 0xa732b4
extern Rec78* g_tbl78;                          // 0x5e29dc the scene prop kinds (0x78-byte records)

// the animators fold the clock delta since the last start into the busy time: (g_ticks[clock] - stamp) * 1e-5 + seconds + 1
#define ANIMATE(slot, value)                                                                      \
    ((AnimVt*)widgets[idx])->slot(t, value);                                                      \
    {                                                                                             \
        float done = (float)(g_ticks[clock] - *(__int64*)&stamp_lo) * g_tick_scale + t + 1.0f;   \
        busy_until = (done < busy_until) ? busy_until : done;                                     \
    }

// @ 0x48e5d0
void widget_mgr::animate_pos(int idx, float t, int v)
{
    ANIMATE(v24, v)
}

// @ 0x48e650
void widget_mgr::animate_color(int idx, float t, int rgb)
{
    ANIMATE(v10, rgb)
}

// @ 0x48e6d0
void widget_mgr::animate_alpha(int idx, float t, int a)
{
    ANIMATE(v14, a << 24)
}

// @ 0x48e750
void widget_mgr::animate_hilight_color(int idx, float t, int rgb)
{
    ANIMATE(v18, rgb)
}

// @ 0x48e7d0
void widget_mgr::animate_hilight_alpha(int idx, float t, int a)
{
    ANIMATE(v1c, a << 24)
}

// @ 0x48e850
void widget_mgr::animate_size(int idx, float t, int v)
{
    ANIMATE(v20, v)
}

// @ 0x48e8d0
__declspec(noinline) tstring* NameRec::resolve()
{
    tstring* r = g_strtable.find(&key);
    return r ? r : &name;
}

// @ 0x48e8f0
tstring NameTable::get_string(int index, int kind)
{
    if (kind == 0) {
        if (index >= 0 && index < 0x80)
            return g_string_registers[index];
        return tstring("Invalid String-register!");
    }
    if (kind == 0x16) {
        if (index >= 0 && index < count2) {
            NameRec* rec = &base2[index];
            tstring* r = g_strtable.find(&rec->key);
            return *(r ? r : &rec->name);
        }
        return tstring("Invalid Quick-String!");
    }
    if (index >= 0 && index < count)
        return *(base[index].resolve());
    return tstring("Unexisting String!");
}

// @ 0x48e9e0
tstring GameData::string_value(int index)
{
    g_result_string.assign("");
    if (!emit_event(0x10, index, 0)) {
        char buf[0x200];
        {
            tstring key("ui_value_denars");
            tstring* value = g_strtable.lookup(&key);
            sprintf(buf, value->c_str(), index);
        }
        g_result_string.assign(buf);
    }
    return g_result_string;
}

// @ 0x48ea80
face* GameData::find_at(const float* p)
{
    int idx[2];
    shape_base* cand[0x2000];
    int count;
    float q[2];
    int flag;

    q[0] = p[0];
    q[1] = p[1];
    g_level_grid->locate(q, idx);
    flag = 0;
    cell* c = &g_level_grid->cells[idx[0]][idx[1]];
    c->collect_point(q, &flag, 0x100, cand, &count);
    for (int k = 0; k < count; k++) {
        shape_base* s = cand[k];
        if (s->m4 < 0)
            continue;
        if (((face*)s)->contains_xy(p))
            return (face*)s;
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
            tstring* fmt = g_strtable.lookup(&key);
            sprintf(buf, fmt->c_str(), name);
        }
        {
            tstring msg(buf);
            g_msglog.push(msg, (int)0xff80ff80);
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

// @ 0x48ed60
void Mission::retag_props(int from, int to)
{
    Rec1c8* base = props._Myfirst;
    if (!base)
        return;
    int n = (int)(((char*)props._Mylast - (char*)base) / 0x1c8);
    for (int i = 0; i < n; i++) {
        Rec1c8* r = base + i;
        if (r->destroyed)
            continue;
        if (!r->name.equals(g_tbl78[from].name))
            continue;
        r->kind = to;
        r->name = g_tbl78[to].name;
    }
}

// @ 0x48ee20
int Mission::count_props(int tag)
{
    int count = 0;
    Rec1c8* base = props._Myfirst;
    if (!base)
        return 0;
    unsigned n = (unsigned)(((char*)props._Mylast - (char*)base) / 0x1c8);
    for (unsigned i = 0; i < n; i++) {
        Rec1c8* r = base + i;
        if (!r->destroyed && r->kind == tag)
            count++;
    }
    return count;
}

// @ 0x48ee90
int Mission::find_prop(int tag, int nth)
{
    int found = 0;
    Rec1c8* base = props._Myfirst;
    if (!base)
        return -1;
    unsigned n = (unsigned)(((char*)props._Mylast - (char*)base) / 0x1c8);
    for (unsigned i = 0; i < n; i++) {
        Rec1c8* r = base + i;
        if (!r->destroyed && r->kind == tag) {
            if (found == nth)
                return i;
            found++;
        }
    }
    return -1;
}

// @ 0x48ef20
void* PageTable98::slot(unsigned index)
{
    void** page = &pages[index >> 4];
    return (char*)*page + (index & 0xf) * 0x98;
}
