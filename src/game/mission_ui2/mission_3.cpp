//-----------------------------------------------------------------------------
// File: mission_3.cpp  (0x48c330..0x48e5d0)
//
// Desc: the party's stack editors (remove_slot, consume_stack, add_members), the game data's loot generators (free
//       inventory slots, give / pick / spawn items, the event string), the item reference setters, the script triggers'
//       lookups, the battle-scene field panel, the string-resolving accessors of the quest / faction / troop / party records,
//       the force label, and the widget setters of the script's overlay operations. Names provisional. Functions in
//       ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP.
//
// Boundary: class changes only (Troop | Party at 0x48c330, Party | SlotAPI at 0x48e5d0); no forward call in or out.
//-----------------------------------------------------------------------------
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../ui/ui_common.h"
#include "../ui/item_ref.h"
#include "../ui/mission_ui.h"
#include "../config/widget_mgr.h"
#include "./stat_panel_x.h"
#include "./mission_3.h"
#include "./field_panel.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../mission.h"
#include "../item_ref.h"
#include "../slot_array.h"
#include "../str_table.h"
#include "../../include/i64_vec.h"
#include "../../include/rec1c8.h"
#include "../../include/item_kind.h"
#include "../../include/troop.h"
#include "../../include/party.h"
#include "../../include/game_recs.h"
#include "../../include/game_data.h"

extern StrTable g_strtable;                     // 0x5e2a18
#include "../game_globals.h"
struct PartyModelV { virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c(); virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c(); virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c(); virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c(); virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c(); virtual void v50(); virtual void v54(); virtual void v58(); virtual void v5c(); virtual void v60(); virtual void v64(); virtual void release(); };   // the party's map model: slot +0x68 (0x48d7e0)
extern unsigned __int64 g_bit[];                // 0x5a8e68 g_bit[i] == 1 << i
extern int g_item_kind_count;                   // 0x5e29d4 (item_kind.h)

// custom-ABI helpers and other-object callees
extern char g_script[]; // 0xa70440

// @ 0x48c330
void Party::remove_slot(int index)
{
    // a prisoner stack (flag bit 0), or the first stack, stays where it is; otherwise the stack is swapped up past every
    // prisoner stack in front of it
    if ((stacks[index].flags & 1) || index <= 0) {
        dirty = 1;
        return;
    }
    int i = index;
    do {
        if (!(stacks[i - 1].flags & 1)) {
            dirty = 1;
            return;
        }
        Stack t = stacks[i - 1];
        stacks[i - 1] = stacks[i];
        stacks[i] = t;
        i--;
    } while (i > 0);
    dirty = 1;
}

// @ 0x48c3c0
int Party::consume_stack(int index, int amount, int* rows, char keep)
{
    Stack* e = &stacks[index];
    int r = 0;
    e->count -= amount;
    int v = e->count;
    if (keep) {
        e->wounded -= amount;
        e->wounded = e->wounded > 0 ? e->wounded : 0;
    } else {
        if (e->wounded > v)
            e->wounded = v;
    }
    if (e->f14 > v)
        e->f14 = v;
    if (stacks[index].count <= 0) {
        Stack out;
        remove_row((int*)&out, index, rows);
        r = 1;
    }
    dirty = 1;
    return r;
}

// @ 0x48c460
int material_type()
{
    return 4;
}

// @ 0x48c470
tstring GameData::event_string(int id)
{
    (*(tstring*)((char*)&g_script + 0x24)).assign("");
    emit_event(0xf, id / 0x18, id);
    return (*(tstring*)((char*)&g_script + 0x24));
}

// @ 0x48c4c0
void GameData::init_slots(int troop, int want)
{
    Troop* t = &troops[troop];
    int n = t->inv_count();
    int filled = 0, i;
    for (i = 0; i < n; i++)
        if (t->inv[i].item == -1)
            filled++;
    for (i = 0; i < 200 && filled < want; i++) {
        int pick = n > 1 ? rand() % n : 0;
        int item = t->inv[pick].item;
        if (item != -1 && !(g_items[item].type_flags & 0x1000)) {
            t->inv[pick].item = -1;
            filled++;
        }
    }
    t->inv[0].compact(t->inv_count());
}

// @ 0x48c570
struct Rec8 { int a, b; };
Rec8* __stdcall copy_records8(Rec8* first, Rec8* last, Rec8* dst)
{
    int n = last - first;
    return (Rec8*)memmove(dst, first, n * sizeof(Rec8)) + n;
}

// @ 0x48c5a0
void ItemRef::set_item(int item_)
{
    item = item_;
    m4 = 0;
    reduce(0);
}

// @ 0x48c5c0
void ItemRef::randomize_modifier(float quality)
{
    if (item < 0)
        return;
    if ((float)(rand() % 15817) * 6.322312e-05f < 0.1f)
        return;
    int found = -1;
    float base_in = 1.0f, base_out = 1.0f;
    float total = speed_scale(&base_out, &base_in, quality);
    float ref = total;
    unsigned lo = g_items[item].f1e0;
    unsigned hi = g_items[item].f1e4;
    for (int i = 0; i < 0x2b; i++) {
        if ((lo & (unsigned)g_bit[i]) != 0 || (hi & (unsigned)(g_bit[i] >> 32)) != 0)
            total = speed_scale(&g_sort_val[i * 5].pad, &g_sort_val[i * 5].v, quality) + total;
    }
    float target = (float)(rand() % 15817) * total * 6.322312e-05f;
    if (target > ref) {
        total = ref;
        for (int i = 0; i < 0x2b; i++) {
            if (found != -1)
                break;
            if ((g_items[item].f1e0 & (unsigned)g_bit[i]) != 0 ||
                (g_items[item].f1e4 & (unsigned)(g_bit[i] >> 32)) != 0) {
                total = speed_scale(&g_sort_val[i * 5].pad, &g_sort_val[i * 5].v, quality) + total;
                if (target < total)
                    found = i;
            }
        }
    }
    if (found < 0)
        return;
    b.mod = 0;
    m4 |= (unsigned)found << 24;
    reduce(0);
    if (item > 0) {
        unsigned t = g_items[item].type_flags & 0xff;
        if (t == itp_type_thrown || t == itp_type_arrows || t == itp_type_bolts ||
            t == itp_type_pistol || t == itp_type_musket || t == itp_type_crossbow ||
            t == itp_type_bullets) {
            b.amount = 0;
        } else if (t == itp_type_bow || t == itp_type_one_handed_wpn || t == itp_type_book) {
            int v = stat_1e8();
            *(unsigned short*)&m4 = 0;
            m4 |= (unsigned)v;
        }
    }
}

// @ 0x48c7f0
void Troop::set_slot(int item, int idx, int mod)
{
    ItemRef* r = &inv[0] + idx;
    r->set_item(item);
    if (mod >= 0 && mod < 0x2b) {
        r->b.mod = 0;
        r->m4 |= (unsigned)mod << 24;
        r->reduce(0);
    }
}

// @ 0x48c840
int Party::add_members(int troop, int count, int flags, int wounded, char force)
{
    if (count == 0)
        return 0;
    int idx = find_or_add(troop, flags, force == 0);
    if (idx < 0) {
        dirty = 1;
        return -1;
    }
    if (idx == nstacks) {
        Stack* e = &stacks[nstacks];
        e->count = 0;
        e->flags = 0;
        e->f14 = 0;
        e->spawned = 0;
        e->wounded = 0;
        e->f18 = 0;
        e->troop = -1;
        stacks[nstacks].troop = troop;
        stacks[nstacks].count = count;
        stacks[nstacks].wounded = wounded;
        stacks[nstacks].f14 = 0;
        stacks[nstacks].f18 = 0;
        stacks[nstacks].flags = flags;
        nstacks++;
        if (!(flags & 1)) {
            remove_slot(nstacks - 1);
            dirty = 1;
            return idx;
        }
    } else {
        stacks[idx].count += count;
        stacks[idx].wounded += wounded;
    }
    dirty = 1;
    return idx;
}

// @ 0x48c9b0
void GameData::give_item(int troop, int item, int count)
{
    int left = count;
    int n = (troops + troop)->inv_count();
    for (int i = 0; i < n; i++) {
        if (left <= 0)
            return;
        ItemRef* r = (ItemRef*)((char*)troops + 0xb4) + (troop * 0xa1 + i);
        if (r->item == -1) {
            left--;
            r->set_item(item);
        }
    }
}

// 0x48ca30 calls ItemRef::stat_204 (0x462a10, src/game/ui/item_ref.cpp; the raw 0x435090 of agent.cpp is ItemRef::accuracy),
// the canonical member of item_ref.h. The declaration here repeats its address for the resolution tools.
namespace icf_twin {
struct ItemRef {
    int stat_204();   // 0x462a10
};
}

// @ 0x48ca30
int GameData::pick_random_item(int t0, int t1, int t2, int t3)
{
    for (;;) {
        float prob = 1.0f;
        int idx;
        if (g_item_kind_count <= 1)
            idx = 0;
        else
            idx = rand() % g_item_kind_count;
        ItemKind* k = &g_items[idx];
        ItemRef r;
        r.item = idx;
        r.m4 = 0;
        if (idx > 0) {
            unsigned t = k->type_flags & 0xff;
            if (t == itp_type_thrown || t == itp_type_arrows || t == itp_type_bolts ||
                t == itp_type_pistol || t == itp_type_musket || t == itp_type_crossbow ||
                t == itp_type_bullets)
                r.m4 = 0;
            else if (t == itp_type_shield || t == itp_type_horse || t == itp_type_animal)
                r.m4 = (unsigned)r.stat_1e8();
        }
        unsigned tf = k->type_flags;
        unsigned t = tf & 0xff;
        unsigned f = tf >> 12;
        if ((tf & 0x10000) != 0 && (f & 1) == 0 &&
            (t == (unsigned)t0 || t == (unsigned)t1 || t == (unsigned)t2 || t == (unsigned)t3)) {
            float v = (float)troops[player].level;
            prob = (v * 30.0f + 200.0f) / ((float)k->value + 200.0f);
            if (t == itp_type_head_armor || t == itp_type_body_armor ||
                t == itp_type_foot_armor || t == itp_type_hand_armor) {
                int a = r.stat_204();
                int b = r.stat_1fc();
                int c = r.stat_200();
                if (c > b) {
                    if (c > a)
                        a = c;
                } else if (b > a) {
                    a = b;
                }
                if (a <= 4)
                    prob = 0.0f;
            } else {
                unsigned u = k->type_flags & 0xff;
                if (u == itp_type_one_handed_wpn || u == itp_type_two_handed_wpn ||
                    u == itp_type_polearm) {
                    int a = r.stat_21x(1);
                    int b = r.stat_21x(0);
                    if (b <= a)
                        b = a;
                    if (b <= 0xa)
                        prob = 0.0f;
                }
            }
        } else {
            idx = -1;
        }
        float roll = (float)(rand() % 0x3dc9) * 6.322312e-05f;
        if (roll >= prob || idx == -1)
            continue;
        return idx;
    }
}

// @ 0x48cc40
void GameData::spawn_items(int type, int want, int troop)
{
    Troop* ag = &troops[troop];
    int got = 0;
    for (int tries = 0; tries < 10000 && got < want; tries++) {
        int idx;
        if (g_item_kind_count <= 1)
            idx = 0;
        else
            idx = rand() % g_item_kind_count;
        ItemKind* k = &g_items[idx];
        unsigned tf = k->type_flags;
        if ((tf & 0xff) != (unsigned)type || (tf & 0x1000) != 0 || (tf & 0x10000) == 0)
            continue;
        float chance = (float)k->f220 * k->f224 * 0.01f;
        if (chance < 0.0f)
            chance = 0.0f;
        else if (chance >= 20.0f)
            chance = 20.0f;
        float roll = (float)(rand() % 0x3dc9) * 6.322312e-05f;
        for (; chance > roll && got < want; got++) {
            chance -= 1.0f;
            int n = ag->inv_count();
            int slot = n > 1 ? rand() % n : 0;
            ItemRef* e = &ag->inv[slot];
            if (e->item == -1 || (g_items[e->item].type_flags & 0x1000) == 0) {
                e->item = idx;
                e->m4 = 0;
                if (idx > 0) {
                    unsigned t = k->type_flags & 0xff;
                    if (t == itp_type_thrown || t == itp_type_arrows || t == itp_type_bolts ||
                        t == itp_type_pistol || t == itp_type_musket || t == itp_type_crossbow ||
                        t == itp_type_bullets) {
                        e->b.amount = 0;
                    } else if (t == itp_type_shield || t == itp_type_horse || t == itp_type_animal) {
                        e->b.amount = 0;
                        e->m4 |= (unsigned)e->stat_1e8();
                    }
                }
                e->randomize_modifier(f13a38);
            }
        }
    }
    ag->inv[0].compact(ag->inv_count());
}

// @ 0x48ce90
ItemRef* __stdcall copy_refs(ItemRef* dst, unsigned n, const ItemRef* src)
{
    ItemRef* p = dst;
    if (n > 0) {
        unsigned i = n;
        do {
            *p = *src;
            p++;
        } while (--i);
    }
    return dst + n;
}

// @ 0x48cec0
int __fastcall trigger_item(int key)
{
    int r = 0;
    if (key == -0x28 || key == -0x2b || key == -0x2a) {
        int i = ((SiteKeys*)g_game)->cur;
        if (i >= 0) {
            Rec1c8* m = &g_mission->props._Myfirst[i];
            PropEnt::Vec* v = &((PropEnt*)m->ent)->lods;
            if (v->first != 0) {
                unsigned n = v->last - v->first;
                if (n > 0)
                    r = *((PropEnt*)m->ent)->lods.first;
            }
        }
    } else if (key == -0x32 || key == -0x33)
        r = ((SiteKeys*)g_game)->key_a;
    return r;
}

// @ 0x48cf40
int __fastcall trigger_mesh_obj(int key)
{
    int r = 0;
    if (key == -0x28) {
        int i = ((SiteKeys*)g_game)->cur;
        if (i >= 0) {
            Rec1c8* m = &g_mission->props._Myfirst[i];
            PropEnt::Vec* v = &((PropEnt*)m->ent)->meshes;
            if (v->first != 0) {
                unsigned n = v->last - v->first;
                if (n > 0)
                    r = *((PropEnt*)m->ent)->meshes.first;
            }
        }
    } else if (key == -0x32 || key == -0x46)
        r = ((SiteKeys*)g_game)->key_b;
    return r;
}

// @ 0x48cfc0
void ListHolder::set_owner(void* owner)
{
    int i = 0;
    if (this->count > 0) {
        char* p = (char*)this + 0x3c;
        do {
            int** it = *(int***)(p - 4);
            for (; it < *(int***)p; it++) {
                Owned* obj = *(Owned**)(it);
                if (obj->owner != 0)
                    obj->owner->refs--;
                obj->owner = (Owned*)owner;
                if (owner != 0)
                    ((Owned*)owner)->refs++;
            }
            i++;
            p += 0x10;
        } while (i < this->count);
    }
}

// The bounds are folded with the windef.h-style min / max (a > b ? a : b) into two float pairs that cl builds as temporaries.
static __forceinline float fmax2(float a, float b) { return (a > b) ? a : b; }
static __forceinline float fmin2(float a, float b) { return (a < b) ? a : b; }

// @ 0x48d020
void FieldPanel::update_bounds()
{
    FieldV2 mx(0.0f, 0.0f);
    FieldV2 mn(999999.0f, 999999.0f);
    if (shape(0)) {
        shape(0)->compute_bounds();
        mx = FieldV2(fmax2(mx.x, shape(0)->bounds.max.x), fmax2(mx.y, shape(0)->bounds.max.y));
        mn = FieldV2(fmin2(mn.x, shape(0)->bounds.min.x), fmin2(mn.y, shape(0)->bounds.min.y));
    }
    if (shape(2)) {
        shape(2)->compute_bounds();
        mx = FieldV2(fmax2(mx.x, shape(2)->bounds.max.x), fmax2(mx.y, shape(2)->bounds.max.y));
        mn = FieldV2(fmin2(mn.x, shape(2)->bounds.min.x), fmin2(mn.y, shape(2)->bounds.min.y));
    }
    if (shape(1)) {
        shape(1)->compute_bounds();
        mx = FieldV2(fmax2(mx.x, shape(1)->bounds.max.x), fmax2(mx.y, shape(1)->bounds.max.y));
        mn = FieldV2(fmin2(mn.x, shape(1)->bounds.min.x), fmin2(mn.y, shape(1)->bounds.min.y));
    }
    if (shape(3)) {
        shape(3)->compute_bounds();
        mx = FieldV2(fmax2(mx.x, shape(3)->bounds.max.x), fmax2(mx.y, shape(3)->bounds.max.y));
        mn = FieldV2(fmin2(mn.x, shape(3)->bounds.min.x), fmin2(mn.y, shape(3)->bounds.min.y));
    }
    if (sqrt(mx.x * mx.x + mx.y * mx.y) > 0.0f) {
        set_xy(&mx.x);
        screen[2] = mn.x;
        screen[3] = mn.y;
    }
}

// @ 0x48d350
void FieldPanel::set_color(unsigned rgb)
{
    FieldVisual* v = scene()->view[0];
    rgb = rgb | (v->color & 0xff000000);
    scene()->view[0]->set_color(&rgb);
    float out[2];
    scene()->view[0]->get_state(out);
}

// @ 0x48d3c0
void FieldPanel::set_alpha(unsigned a)
{
    unsigned u = a << 24;
    a = (scene()->view[0]->color & 0xffffff) | u;
    scene()->view[0]->set_color(&a);
    float out[2];
    scene()->view[0]->get_state(out);
    mirror()->tracks[0]->color = (tracks[0]->color & 0xffffff) | u;
    mirror()->tracks[2]->color = (tracks[2]->color & 0xffffff) | u;
}

// @ 0x48d470
void FieldPanel::update()
{
    FieldVisual* v = scene()->view[0];
    float p[2];
    p[0] = v->f0;
    p[1] = v->f4;
    scene()->set_xy(p);

    if (sqrt(f49c * f49c + f4a0 * f4a0) > 0.0f) {
        float q[2];                     // a copy of the stored size (0x48d4d4)
        q[0] = f49c;
        q[1] = f4a0;
        set_xy(q);
    } else {
        float q[2];                     // none stored: the scene's width, our height (0x48d4f6)
        q[0] = scene()->screen[0];
        q[1] = screen[1];
        set_xy(q);
    }

    if (!(scene()->screen[1] < screen[1])) {        // fcomp; test ah,5; jp (0x48d528): not shorter than us
        float z[2];
        z[0] = 0.0f;
        z[1] = 0.0f;
        v->set_pos(z);
    } else {
        float z[2];
        z[0] = 0.0f;
        z[1] = screen[1] - scene()->screen[1];
        v->set_pos(z);
        float w[2];
        w[0] = scene()->screen[0];
        w[1] = screen[1];
        scene()->set_xy(w);
    }

    float m[2];
    m[0] = screen[0] + e[0] + 0.01f;
    m[1] = e[1];
    mirror()->d22(m);
    draw();
}

// @ 0x48d5f0
int ArrHolder::lookup(int index, int arg)
{
    int i = ((ArrElem*)((char*)base + index * 0x1dc))->v08(arg);
    if (i < 0)
        return 0;
    ArrElem* e = (ArrElem*)((char*)base + index * 0x1dc);     // base re-read after the call (0x48d613)
    int* a = e->a;
    if (!a)
        return 0;
    if (e->b - a == 0)          // an empty list (pointer difference is already a count: no >> 2)
        return 0;
    int* v = (int*)a[0];
    return (*(int**)((char*)v + 0xb4))[i];     // +0xb4 holds a pointer to the int array (0x48d636)
}

// @ 0x48d650
// Not reproducible byte-exactly with cl 13.10 /O2 (see the note in mission_3.nonmatching.txt): the body is
// identical except that the compiler puts `_Mylast - _Myfirst` in ecx (dead `this`) where the original uses
// eax. Every source form of the same conditions (/O2) picks ecx; /O1 and /O2 /Os pick eax but emit the
// size-optimised shared epilogue (a `jmp` past the failure block) instead of the original's duplicated tails.
__int64 SlotArray::get(const int& slot)
{
    I64Vec* v = (I64Vec*)this;
    unsigned i = slot;
    if ((int)i < 0 || !v->_Myfirst || i >= (unsigned)(v->_Mylast - v->_Myfirst))
        return 0;
    return v->_Myfirst[i];
}

// @ 0x48d690
int Cont690::get(unsigned index)
{
    Pair690 r;
    if ((int)index >= 0 && base != 0 && index < (unsigned)((end - base) >> 3))
        r = *(Pair690*)(base + index * 8);
    else {
        r.a = 0;
        r.b = 0;
    }
    return r.a;
}

// @ 0x48d6d0
tstring* TroopRec::plural_name()
{
    tstring* r = g_strtable.find(&(key + tstring("_pl")));
    return r ? r : &plural;
}

// @ 0x48d730
tstring* QuestRec::raw_text()
{
    tstring* r = g_strtable.find(&key);
    return r ? r : &name;
}

// @ 0x48d750
tstring* QuestRec::raw_title()
{
    tstring* r = g_strtable.find(&(key + tstring("_text")));
    return r ? r : &desc;
}

// @ 0x48d7b0
tstring* Party::display_name()
{
    if (g_show_party_ids_instead_of_names)
        return &key;
    if (flag == 0) {
        tstring* r = g_strtable.find(&key);
        if (r)
            return r;
    }
    return &name;
}

// @ 0x48d7e0
void Party::clear_particle_systems()
{
    if (deferred.first)
        operator delete(deferred.first);
    deferred.first = 0;
    deferred.last = 0;
    deferred.end = 0;
    if (model)
        ((PartyModelV*)model)->release();
}

static __declspec(noinline) void unit_class_name(unsigned kind, char* out);

// @ 0x48d830
static void unit_class_name(unsigned kind, char* out)
{
    if (kind == 0) {
        tstring key("ui_infantry");
        strcpy(out, g_strtable.lookup(&key)->c_str());
    } else if (kind == 2) {
        tstring key("ui_cavalry");
        strcpy(out, g_strtable.lookup(&key)->c_str());
    } else if (kind == 1) {
        tstring key("ui_archers");
        strcpy(out, g_strtable.lookup(&key)->c_str());
    } else {
        tstring key("ui_everyone");
        strcpy(out, g_strtable.lookup(&key)->c_str());
    }
}

// @ 0x48d940
void ForceLabel::make_label(char* out)
{
    FieldGroup* g = (FieldGroup*)((char*)this + 0x4c);
    char all = 1;
    if (g->n == 0)
        all = 0;
    if (n1 == 0)
        all = 0;
    if (n2 != 0 && all) {
        tstring key("ui_everyone");
        strcpy(out, g_strtable.lookup(&key)->c_str());
        return;
    }

    char part[0x200];
    char acc[0x200];
    char first_done = 1;
    for (int i = 0; i < 3; i++) {
        if (g[i].n == 0)
            continue;
        char grouped = 1;
        for (int j = i + 1; j < 3; j++)
            if (g[j].n != 0)
                grouped = 0;
        unit_class_name(i, part);
        if (first_done) {
            strcpy(out, part);
            first_done = 0;
        } else {
            strcpy(acc, out);
            if (grouped) {
                tstring key("ui_str_and_str");
                sprintf(out, g_strtable.lookup(&key)->c_str(), acc, part);
            } else {
                tstring key("ui_str_comma_str");
                sprintf(out, g_strtable.lookup(&key)->c_str(), acc, part);
            }
        }
    }
}

// @ 0x48db00
tstring* FactionRec::display_name()
{
    if (flag == 0) {
        tstring* r = g_strtable.find(&key);
        if (r)
            return r;
    }
    return &name;
}

// @ 0x48db20
void* widget_mgr::set_mesh_matrix(int idx, const void* src)
{
    rgl::matrix m;
    rgl::identity_pos(&m);
    memcpy(&m, src, 0x30);
    OvNode** nodes = (OvNode**)widgets._Myfirst;
    if (nodes[idx]->s0) memcpy((char*)nodes[idx]->s0 + 0xcc, &m, 0x40);
    if (nodes[idx]->s2) memcpy((char*)nodes[idx]->s2 + 0xcc, &m, 0x40);
    if (nodes[idx]->s1) memcpy((char*)nodes[idx]->s1 + 0xcc, &m, 0x40);
    if (nodes[idx]->s3) memcpy((char*)nodes[idx]->s3 + 0xcc, &m, 0x40);
    if (nodes[idx]->s4) memcpy((char*)nodes[idx]->s4 + 0xcc, &m, 0x40);
    return &nodes[idx];
}

// the widget `idx` is re-read from the manager's vector at every use (cl keeps no copy across the virtual calls)
#define OW(idx) ((OvObj*)widgets[idx])

// @ 0x48dc30
void widget_mgr::set_color(int idx, unsigned argb)
{
    if (OW(idx)->type() != 0 && OW(idx)->type() != 1 && OW(idx)->type() != 9 && OW(idx)->type() != 5 &&
        OW(idx)->type() != 6) {
        if (OW(idx)->type() == 4) {
            OW(idx)->m108->color = (OW(idx)->m108->color & 0xff000000) | argb;
        } else if (OW(idx)->type() == 8) {
            OW(idx)->m110->color = (OW(idx)->m110->color & 0xff000000) | argb;
            OW(idx)->m10c->color = (OW(idx)->m10c->color & 0xff000000) | argb;
        } else if (OW(idx)->type() == 7) {
            ((FieldPanel*)(void*)OW(idx))->set_color(argb);
            return;
        } else {
            return;
        }
        unsigned c = (OW(idx)->color14c & 0xff000000) | argb;
        OW(idx)->set_color(&c);
    } else {
        unsigned c = (OW(idx)->color14c & 0xff000000) | argb;
        OW(idx)->set_color(&c);
        float out[2];
        OW(idx)->get_state(out);
    }
}

// @ 0x48de00
void widget_mgr::set_alpha(int idx, unsigned a)
{
    if (OW(idx)->type() == 0 || OW(idx)->type() == 1) {
        unsigned c = (OW(idx)->color14c & 0xffffff) | (a << 24);
        OW(idx)->set_color(&c);
        float out[2];
        OW(idx)->get_state(out);
        return;
    }
    if (OW(idx)->type() == 4) {
        OW(idx)->m108->color = (OW(idx)->m108->color & 0xffffff) | (a << 24);
    } else if (OW(idx)->type() == 8 || OW(idx)->type() == 9) {
        OW(idx)->m110->color = (OW(idx)->m110->color & 0xffffff) | (a << 24);
        OW(idx)->m10c->color = (OW(idx)->m10c->color & 0xffffff) | (a << 24);
    } else if (OW(idx)->type() == 2) {
        OW(idx)->m108->color = (OW(idx)->m108->color & 0xffffff) | (a << 24);
        OW(idx)->m110->color = (OW(idx)->m110->color & 0xffffff) | (a << 24);
    } else if (OW(idx)->type() == 3) {
        OW(idx)->m108->color = (OW(idx)->m108->color & 0xffffff) | (a << 24);
        OW(idx)->m110->color = (OW(idx)->m110->color & 0xffffff) | (a << 24);
        OW(idx)->f1a8 = a << 24;
        OW(idx)->f194 = 0xcb3c57cb;
    } else if (OW(idx)->type() == 5 || OW(idx)->type() == 6) {
        OW(idx)->m110->color = (OW(idx)->m110->color & 0xffffff) | (a << 24);
        OW(idx)->m10c->color = (OW(idx)->m10c->color & 0xffffff) | (a << 24);
        if (OW(idx)->type() == 5)
            OW(idx)->m114->color = (OW(idx)->m114->color & 0xffffff) | (a << 24);
        unsigned c = (OW(idx)->color14c & 0xffffff) | (a << 24);
        OW(idx)->set_color(&c);
        float out[2];
        OW(idx)->get_state(out);
        return;
    } else if (OW(idx)->type() == 7) {
        ((FieldPanel*)(void*)OW(idx))->set_alpha(a);
        return;
    } else {
        return;
    }
    unsigned c = (OW(idx)->color14c & 0xffffff) | (a << 24);
    OW(idx)->set_color(&c);
}

// @ 0x48e1a0
void widget_mgr::set_hilight_color(int idx, unsigned rgb)
{
    if (OW(idx)->type() != 0 && OW(idx)->type() != 1 && OW(idx)->type() != 9 && OW(idx)->type() != 5 &&
        OW(idx)->type() != 6)
        return;
    unsigned c = (OW(idx)->color14c & 0xff000000) | rgb;
    OW(idx)->set_color2(&c);
    float out[2];
    OW(idx)->get_state(out);
}

// @ 0x48e270
void widget_mgr::set_hilight_alpha(int idx, unsigned a)
{
    if (OW(idx)->type() != 0 && OW(idx)->type() != 1 && OW(idx)->type() != 9 && OW(idx)->type() != 5 &&
        OW(idx)->type() != 6)
        return;
    unsigned c = (OW(idx)->color150 & 0xffffff) | (a << 24);
    OW(idx)->set_color2(&c);
    float out[2];
    OW(idx)->get_state(out);
}

// @ 0x48e340
void widget_mgr::set_val(int idx, int v)
{
    if (OW(idx)->type() == 2 || OW(idx)->type() == 3) {
        OW(idx)->set_value_f((float)v);
        if (OW(idx)->v168 < OW(idx)->lo16c) {
            OW(idx)->v168 = OW(idx)->lo16c;
            return;
        }
        if (OW(idx)->v168 > OW(idx)->hi170)
            OW(idx)->v168 = OW(idx)->hi170;
        return;
    }
    if (OW(idx)->type() == 5) {
        if (v < OW(idx)->min160) {
            OW(idx)->set_value(OW(idx)->min160);
            return;
        }
        if (v > OW(idx)->max164)
            v = OW(idx)->max164;
        OW(idx)->set_value(v);
        return;
    }
    if (OW(idx)->type() != 8)
        return;
    if (OW(idx)->kind184 != 1)
        return;
    if (v < 0)
        v = 0;
    else if (v > 2)
        v = 2;
    OW(idx)->set_value(v);
    OW(idx)->on180 = v != 0;
}

// @ 0x48e4c0
void widget_mgr::set_limits(int idx, int lo, int hi)
{
    if (OW(idx)->type() != 2 && OW(idx)->type() != 3)
        return;
    OW(idx)->lo16c = (float)lo / (*(int*)((char*)&g_script + 0x20));
    OW(idx)->hi170 = (float)hi / (*(int*)((char*)&g_script + 0x20));
    if (OW(idx)->v168 < OW(idx)->lo16c) {
        OW(idx)->v168 = OW(idx)->lo16c;
        return;
    }
    if (OW(idx)->v168 > OW(idx)->hi170)
        OW(idx)->v168 = OW(idx)->hi170;
}

// @ 0x48e590
void widget_mgr::add_item(int idx, int v)
{
    if (OW(idx)->type() == 5)
        OW(idx)->vc8(v);
}
