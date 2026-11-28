//-----------------------------------------------------------------------------
// File: game_data_party_quest.cpp  (0x490030..0x4958f0)
//
// Desc: the party / quest / experience members of the game object, the container-widget helpers, the scene lists, the
//       message-log quest lines and the neighbouring small objects of translation units 83 (world_2) and 84
//       (game_data_2) of the TU map. Names provisional. Functions in ascending address order. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "platform/image_addrs.h"
#include "game/campaign/game_data_party_quest.h"
#include "game/mission/mission_ctx_rec.h"   // MissionCtx (g_mission_ctx 0xa73300): refresh_rating, select_side
#include "engine/core/crt.h"      // canonical CRT declarations (listed directly: the resolution audit reads only the source's own includes)
#include "game/common/game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)
#include "game/campaign/event.h"              // Event (listed directly: the resolution audit reads only the source's own includes)
#include "engine/core/pool.h"    // Pool<Event>::alloc 0x493700
#include "game/party/party.h"
#include "game/data/troop.h"
#include "game/data/game_recs.h"
#include "game/ui/gui_manager.h"
#include "game/data/str_table.h"
#include "engine/math/vec.h"
#include "game/data/game_data.h"
#include "game/ui/widgets/widget_manager.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "game/ui/widgets/widget_base.h"
#include "game/ui/widgets/mission.h"
#include "game/mission/mission.h"
#include "game/agent/agent_obj.h"
#include "game/data/rec_dtors.h"
#include "game/data/trigger_rec.h"     // TriggerRec: world_free deletes the module triggers (vector deleting destructor 0x431480)
#include "game/mission/placed_obj.h"
#include "game/data/e68.h"
#include "game/campaign/data_files.h"                      // TroopFile / QuestFile / FactionFile: the record arrays free_arrays deletes
#include "game/terrain/scene_file.h"                     // SceneFile: the scene records (map_builder_and_data_load.cpp `new SceneFile[n]`) free_arrays deletes
#include "engine/io/stream_inline.h"   // the stream constructors / destructors inline (the scene record's embedded streams)
#include "engine/render/mesh_parts.h"
#include "engine/render/mesh.h"
#include "engine/scene/world.h"      // rgl::world (ctor 0x478ad0, dtor 0x475d50, init 0x478830, spawn_wind 0x478f30)
#include "game/data/tableau.h"
#include "game/data/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40): the table's members defined here
#include "game/ui/widgets/widget_manager.h"
#include "platform/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)
#include "engine/audio/sound_mgr.h"   // g_sound_mgr 0x5aa128
#include "game/data/item_kind_include.h"   // g_items 0x5e29d0
#include "engine/math/float_consts.h"   // shared float constants kept in memory (0x549cdc ...)
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "game/data/presentation.h"                               // widget_mgr, g_presentation (0x5e299c)


// ==== 0x490030: a random position around a point ===========================================================
// @ 0x490030
void GameData::random_position_around(Vec2 pos, float r, Vec2* out)
{
    char ok = 0;
    *out = pos;
    int iter = 0;
    face* f = find_at(&pos.x);
    if (f == 0)
        return;
    int first = f->m4;
    if (first < 0)
        return;
    while (!ok) {
        if (iter >= 5000)
            return;
        iter++;
        Vec2 d;
        d.x = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        d.y = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        if (sqrt(d.x * d.x + d.y * d.y) <= r) {
            *out = Vec2(pos.x + d.x, pos.y + d.y);
            face* g = find_at(&out->x);
            if (g != 0) {
                int gi = g->m4;
                ok = (char)g_script.level->ground->shares(first, gi);
            }
        }
    }
}

// @ 0x490160
char GameData::position_around(Vec2 pos, float r, Vec2* out, char any)
{
    *out = pos;
    char ok = 0;
    int iter = 0;
    float d[2];
    while (!ok) {
        if (iter >= 50000)
            break;
        iter++;
        d[0] = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        d[1] = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        if (sqrt(d[0] * d[0] + d[1] * d[1]) <= r) {
            *out = Vec2(pos.x + d[0], pos.y + d[1]);
            face* f = find_at(&out->x);
            if (f != 0) {
                int kind = g_script.level->ground->cells[f->m4].kind;
                if (any)
                    ok = (kind & 7) == 0 || kind == 7;
                else
                    ok = ((kind & 7) >= 2 && (kind & 7) < 7) || kind == 7;
            }
        }
    }
    return ok;
}

// ==== 0x4902a0: the stack "DNA" token =========================================================================
// @ 0x4902a0
int GameData::stack_dna(unsigned id, int group, int f14, int f18)
{
    int v = -1;
    if ((int)id >= 0) {
        PartyMgr* m = &parties;
        int i = m->lookup(id);
        v = m->at(i)->stacks[group].troop;
    }
    return v * 0x1bf + (int)id * 0x7bb7 + f14 + 0x45db + f18;
}

// ==== 0x490310: the screen-id deque (std::deque<int>, out-of-line instances) ==================================
// The original object file contains the template instances std::deque<int>::back (0x490310), _Growmap (0x492d80, below)
// and push_back (0x493120, below).
// @ 0x490310
template int& std::deque<int>::back();

// ==== 0x490340: the party manager's page list ==================================================================
// A page is a `new Party[16]` block, freed with delete[] (cl: the array deleting destructor, virtual slot 0 with flag 3,
// or operator delete[] when the cookie count is 0).
// @ 0x490340
void PartyMgr::clear()
{
    for (std::vector<Party*>::iterator i = pages.begin(); i < pages.end(); ++i) {
        Party* e = *i;
        delete[] e;                    // the page of 16 (new Party[16])
        *i = 0;
    }
    pages.clear();      // VC7.1's clear() is _Tidy(): free the buffer, zero the three pointers
    free_ids.clear();
}

// @ 0x4903e0
int PartyMgr::is_live(int id)
{
    if ((unsigned)id < serial) {
        int i = this->lookup(id);
        if (i >= 0)
            return pages[(unsigned)i >> 4][i & 0xf].type == 1;
    }
    return false;
}

// ==== 0x490430: the scene entity a script trigger runs for ======================================================
// @ 0x490430
rgl::strategic_entity* __fastcall trigger_entity(int c)
{
    rgl::strategic_entity* v = 0;
    if (c == -0x28 || c == -0x2b || c == -0x2a) {
        v = static_cast<rgl::strategic_entity*>(g_mission->props[g_game->event_ctx.prop].ent);
    } else if (c == -0x32 || c == -0x33) {
        unsigned idx = (unsigned)g_game->event_ctx.agent;
        unsigned page_idx = idx >> 4;
        unsigned rem = idx & 0xf;
        return g_mission->agents.pages[page_idx][rem].body;
    }
    return v;
}

struct named_object;

// ==== 0x490520: the container widget (children vector at +0x178) ================================================
// The class is game/ui/widgets/container.h's ContainerBase. Its destructor stores the vptr, releases the children (the virtual
// update_children, called non-virtually: 0x48ef40, game/ui/widgets/container_and_tableau_misc.cpp), frees the vector and runs MissionUi's
// destructor 0x487c00. The deleting destructor 0x490930 is the compiler's: the destructor inlined, then operator delete.
// @ 0x490520
ContainerBase::~ContainerBase()
{
    ContainerBase::update_children();
}

// @ 0x490570
void ContainerBase::move_a(const vec2& p)
{
    rel_position.x = p.x;
    rel_position.y = p.y;
    if (p154 != 0) {
        p154->on_child_moved(this);
    } else {
        position.x = p.x;
        position.y = p.y;
    }
    on_moved();    // one shared tail call: an early return in the first arm moved the register allocation
}

// @ 0x4905c0
void ContainerBase::on_child_moved(MissionUi* p)
{
    const vec2& tp = position;
    const vec2& ep = p->rel_position;
    p->position = V2(tp.x + ep.x, tp.y + ep.y);    // both sums into a temporary, then one struct copy (the
                                                                // first store then schedules after the second add)
    p->on_moved();
    if (f104 & 0x100) {
        float v;
        v = p->rect_max.x + p->rel_position.x;
        if (v > rect_max.x)
            rect_max.x = v;
        v = p->rect_min.x + p->rel_position.x;
        if (v < rect_min.x)
            rect_min.x = v;
        v = p->rect_max.y + p->rel_position.y;
        if (v > rect_max.y)
            rect_max.y = v;
        v = p->rect_min.y + p->rel_position.y;
        if (v < rect_min.y)
            rect_min.y = v;
    }
}

// @ 0x4906b0
void ContainerBase::on_moved()
{
    std::vector<MissionUi*>::iterator i = kids.begin();
    while (i != kids.end()) {
        on_child_moved(*i);
        i++;
    }
}

// @ 0x4906e0
void ContainerBase::clear_c8()
{
    std::vector<MissionUi*>::iterator i = kids.begin();
    while (i != kids.end()) {
        (*i)->clear_c8();
        i++;
    }
}

// @ 0x490710
void ContainerBase::render(rgl::scene* sc, float* p3, float* c, float* d)
{
// the two bounds pairs (lo / hi corners); naming them keeps the original's register use
    float* p4 = (float*)c;
    float* p5 = (float*)d;
    if (f104 & 1)
        return;
    struct __declspec(align(8)) Locals {
        float z, y0, hy, ly, mx, my;
    } l;
    MissionUi::render(sc, p3, c, d);   // 0x488720
    l.y0 = p3[0];
    l.mx = p4[0];
    l.my = p4[1];
    l.hy = p5[0];
    l.ly = p5[1];
    float v = rect_min.x + position.x;
    if (l.mx < v) l.mx = v;
    v = rect_max.x + position.x;
    if (l.hy > v) l.hy = v;
    v = rect_min.y + position.y;
    if (l.my < v) l.my = v;
    v = rect_max.y + position.y;
    if (l.ly > v) l.ly = v;
    std::vector<MissionUi*>::iterator i = kids.begin();
    while (i != kids.end()) {
        MissionUi* e = *i;
        if (!(*p5 < e->rect_min.x + e->position.x) &&
            !(p5[1] < e->rect_min.y + e->position.y) &&
            !(e->rect_max.x + e->position.x < p4[0]) &&
            !(e->rect_max.y + e->position.y < p4[1])) {
            l.z = p3[0];
            e->render(sc, &l.z, &l.mx, &l.hy);
            if (l.z > l.y0) l.y0 = l.z;
        }
        i++;
    }
    p3[0] = l.y0;
}
#undef p4
#undef p5

// @ 0x4908c0
void ContainerBase::animate(int p)
{
    if (f104 & 1)
        return;
    std::vector<MissionUi*>::iterator i = kids.begin();
    while (i != kids.end()) {
        (*i)->f100 = 0;
        i++;
    }
    i = kids.begin();
    while (i != kids.end()) {
        (*i)->animate(p);
        i++;
    }
}

// ==== 0x490990: mark the parties of one kind dirty ===============================================================
// PartyMgr::next_party_id (the row is owned elsewhere): a visible duplicate. mark_kind's loop counter stays in a register once cl can see that the callee does not keep its address.
__declspec(noinline) void PartyMgr::next_party_id(int* id)
{
    unsigned i = lookup(*id) + 1;
    while ((int)i < count && (&*pages.begin())[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        *id = serial;
    else
        *id = (&*pages.begin())[i >> 4][i & 0xf].id;
}
// @ 0x490990
void GameData::mark_kind(int kind)
{
    PartyMgr* m = &parties;
    unsigned i = m->first_party_id();
    while (i < parties.serial) {
        if (m->at(m->lookup(i))->faction == kind)
            m->at(m->lookup(i))->label_dirty = 1;
        m->next_party_id((int*)&i);
    }
}

// ==== 0x490a30: take a party out of its leader's follower list ====================================================
// @ 0x490a30
void GameData::detach_party(unsigned id)
{
    PartyMgr* m = &parties;
    if (id >= m->serial)
        return;
    int i = m->lookup(id);
    if (i < 0)
        return;
    if (m->atu((unsigned)i)->type != 1)
        return;
    if (m->at(m->lookup(id))->attached_to < 0)
        return;
    bool done = false;
    std::vector<int>::iterator dst = m->at(m->lookup(m->at(m->lookup(id))->attached_to))->attached.begin();
    do {
        if (dst >= m->at(m->lookup(m->at(m->lookup(id))->attached_to))->attached.end())
            break;
        if ((unsigned)*dst == id) {
            m->at(m->lookup(m->at(m->lookup(id))->attached_to))->attached.erase(dst);
            done = true;
        }
        dst++;
    } while (!done);
    m->at(m->lookup(m->at(m->lookup(id))->attached_to))->dirty = 1;
    m->at(m->lookup(id))->engaged = 0;
    m->at(m->lookup(id))->dirty = 1;
    m->at(m->lookup(id))->attached_to = -1;
}

// ==== 0x490ce0: total a party's stacks (and its followers') into counters ========================================
// @ 0x490ce0
void GameData::count_units(unsigned id, int* a, int* b, int* c, int* d)
{
    PartyMgr* m = &parties;
    if (id >= m->serial)
        return;
    int i = m->lookup(id);
    if (i < 0)
        return;
    if (m->atu((unsigned)i)->type != 1)
        return;
    if (m->at(m->lookup(id))->nstacks > 0) {
        int n = 0;
        do {
            if ((m->at(m->lookup(id))->stacks[n].flags & 1) != 0) {
                *c += m->at(m->lookup(id))->stacks[n].count;
            } else {
                *a += m->at(m->lookup(id))->stacks[n].count;
                int troop = m->at(m->lookup(id))->stacks[n].troop;
                if ((troops[troop].flags30 & 0x10) == 0) {
                    *b = *b + m->at(m->lookup(id))->stacks[n].wounded;
                } else {
                    *d = *d + 1;
                    if (agent_low(m->at(m->lookup(id))->stacks[n].troop)) {
                        *b = *b + 1;
                    }
                }
            }
            n++;
        } while (n < m->at(m->lookup(id))->nstacks);
    }
    for (unsigned j = 0; j < m->at(m->lookup(id))->attached.size(); j++)
        count_units(m->at(m->lookup(id))->attached[j], a, b, c, d);
}

// ==== 0x490fa0: a party and, recursively, its followers ===========================================================
// @ 0x490fa0
void GameData::list_members(unsigned id, int* out, int* n)
{
    PartyMgr* m = &parties;
    if (id >= m->serial)
        return;
    int i = m->lookup(id);
    if (i < 0)
        return;
    if (m->atu((unsigned)i)->type != 1)
        return;
    if (*n >= 0x80)
        return;
    out[*n] = (int)id;
    (*n)++;
    for (unsigned j = 0; j < m->at(m->lookup(id))->attached.size(); j++)
        list_members(m->at(m->lookup(id))->attached[j], out, n);
}

// ==== 0x4910a0: casualties of a battle: turn some of a party's troops into prisoners / dead =====================
// @ 0x4910a0
void GameData::recruit(int id, int count, Party* dest)
{
    PartyMgr* m = &parties;
    if ((unsigned)id >= m->serial)
        return;
    int idx = m->lookup(id);
    if (idx < 0)
        return;
    if (m->atu(idx)->type != 1)
        return;

    int n = 0;
    int members[128];
    unsigned pid = (unsigned)id;
    list_members(pid, members, &n);
    int a = 0, b = 0, c = 0, d = 0;
    count_units(pid, &a, &b, &c, &d);
    if (dest != 0)
        dest->nstacks = 0;
    int total = a - b;

    idx = m->lookup(id);
    Party* p = m->at(idx);
    if (p->stacks[0].troop == player && !agent_low(player))
        total--;

    if (count <= 0)
        return;
    int left = count;
    do {
        if (total > 0) {
            int pick;
            if (total <= 1)
                pick = 0;
            else
                pick = rand() % total;
            int base = 0;
            char found = 0;                    // a byte slot in the original (mov byte ptr [esp+..], 0)
            int j = 0;
            do {
                if (j >= n)
                    break;
                PartyMgr* mm = &parties;
                int mi = mm->lookup(members[j]);
                Party* mp = mm->at(mi);
                int prev = base;
                base += mp->sum_stat(0x134);
                if (pick < base) {
                    found = 1;
                    Stack* st = &mp->stacks[mp->find_worth(pick - prev, 0x134)];
                    int hp = troops[st->troop].level + 10;
                    if ((troops[st->troop].flags30 & 0x10) != 0)
                        hp += 10;
                    if ((float)(rand() % 0x3dc9) * (float)hp * 6.322312e-05f < 2.0f) {
                        if ((troops[st->troop].flags30 & 0x10) != 0) {
                            if (troops[st->troop].hp > g_module_settings.player_wounded_treshold)
                                troops[st->troop].hp = g_module_settings.player_wounded_treshold;
                            troops[st->troop].hp -= 15.0f;
                            if (troops[st->troop].hp < 0.0f)
                                troops[st->troop].hp = 0.0f;
                            if (dest != 0)
                                dest->add_members(st->troop, 1, 0, 1, 0);
                        } else {
                            if ((float)(rand() % 0x3dc9) * 6.322312e-05f < g_k065
                                || (troops[st->troop].flags30 & 0x80) != 0) {
                                st->count--;
                                if (dest != 0)
                                    dest->add_members(st->troop, 1, 0, 0, 0);
                            } else {
                                st->wounded++;
                                if (dest != 0)
                                    dest->add_members(st->troop, 1, 0, 1, 0);
                            }
                        }
                        total--;
                    }
                }
                j++;
            } while (!found);
        }
        left--;
    } while (left != 0);
}

// ==== 0x491430: spread a party's stacks over the members of a battle group =======================================
// @ 0x491430
void GameData::handout(Party* p, int id)
{
    int n = 0;
    int members[128];
    int cum[128];
    list_members(id, members, &n);
    int total = 0;
    PartyMgr* m;
    for (int i = 0; i < n; i++) {
        m = &parties;
        total += m->at(m->lookup(members[i]))->sum_stat(0x100);
        cum[i] = total;
    }
    if (total == 0)
        return;
    for (int i = 0; i < p->nstacks; i++) {
        if ((troops[p->stacks[i].troop].flags30 & 0x10) != 0)
            continue;
        for (int j = 0; j < p->stacks[i].count; j++) {
            int pick;
            if (total <= 1)
                pick = 0;
            else
                pick = rand() % total;
            int k = 0;
            while (k < n && pick >= cum[k])
                k++;
            m->at(m->lookup(members[k]))->add_members(p->stacks[i].troop, 1, p->stacks[i].flags, 0, 0);
        }
    }
}

// ==== 0x4915d0: remove_party a party =================================================================================
// PartyMgr::at spelled out as a file-local helper: the member is not expanded here once the vector size() / operator[] calls
// have used up the inliner's budget (it compiles to a call), the original has the page lookup inline at each store.
static inline Party* wd_at(PartyMgr* m, int i) { return m->pages[i >> 4] + (i & 0xf); }
// @ 0x4915d0
void GameData::remove_party(int id)
{
    if ((unsigned)id >= parties.serial)
        return;
    PartyMgr* m = &parties;
    int idx = m->lookup(id);
    if (idx < 0)
        return;
    if (m->atu(idx)->type != 1)
        return;

    idx = m->lookup(id);
    Party* p = m->at(idx);
    if (p->attached_to >= 0) {
        if (m->is_live(p->attached_to))
            detach_party(id);
        p->attached_to = -1;
    }
    if (p->attached.size() != 0) {
        for (unsigned k = 0; k < p->attached.size(); k++) {
            int& a = p->attached[k];
            if ((unsigned)p->attached[k] >= m->serial)
                continue;
            int j = m->lookup(p->attached[k]);
            if (j < 0)
                continue;
            if (m->atu(j)->type != 1)
                continue;
            wd_at(m, m->lookup(a))->attached_to = -1;
            wd_at(m, m->lookup(p->attached[k]))->engaged = 0;
            wd_at(m, m->lookup(p->attached[k]))->dirty = 1;
        }
        p->attached.clear();
    }
    if (p->owner >= 0)
        g_party_templates[p->owner].num_destroyed += 1;
    p->clear_map_flag();

    idx = m->lookup(id);
    if (idx >= 0 && idx < m->count)
        m->atu(idx)->type = 2;

    unsigned cur = m->first_party_id();
    if (cur < parties.serial) {
        do {
            Party* q = m->at(m->lookup(cur));
            if (q->target == id) {
                q->action = 0;
                q->target = -1;
            }
            if (q->state_target == id) {
                q->state = 0;
                q->state_target = -1;
            }
            m->next_party_id((int*)&cur);
        } while (cur < parties.serial);
    }
}

// ==== 0x4918d0: can a party be merged into another ================================================================
// @ 0x4918d0
bool GameData::can_transfer(unsigned id, int other, char flag)
{
    PartyMgr* mgr = &parties;
    if (id >= parties.serial) return false;
    int idx = mgr->lookup(id);
    if (idx < 0) return false;
    if (mgr->atu((unsigned)idx)->type != 1) return false;
    if (!mgr->is_live(other)) return false;
    int ia = mgr->lookup(id);
    Party* a = mgr->at(ia);
    int ib = mgr->lookup(other);
    Party* b = mgr->at(ib);
    if (b->flags & 0x100000) {
        if (!flag) {
            if (b->sum_head() + a->sum_head() > b->max_members()) return false;
            if (b->sum_flagged() + a->sum_flagged() > b->max_prisoners()) return false;
        } else {
            if (b->sum_head() + a->sum_flagged() > b->max_members()) return false;
            if (b->sum_flagged() + a->sum_head() > b->max_prisoners()) return false;
        }
    }
    int p190 = b->first_flagged();
    int d1 = b->max_stacks() - p190;
    int p510 = b->count_tail();
    int d2 = b->max_stacks() - p510;
    int na = a->nstacks;
    int c1 = 0, c2 = 0;
    for (int k = 0; k < na; k++) {
        bool matched = false;
        if (!(troops[a->stacks[k].troop].flags30 & 0x10)) {
            for (int j = 0; b->nstacks > j; j++) {
                if (a->stacks[k].troop == b->stacks[j].troop) {
                    if (!flag) {
                        if (((a->stacks[k].flags ^ b->stacks[j].flags) & 1) == 0) matched = true;
                    } else {
                        if (((a->stacks[k].flags ^ b->stacks[j].flags) & 1) != 0) matched = true;
                    }
                }
            }
            if (matched) continue;
        }
        if (!flag) { if (a->stacks[k].flags & 1) c2++; else c1++; }
        else       { if (!(a->stacks[k].flags & 1)) { c2++; } else { c1++; } }
    }
    if (c1 > d1 || c2 > d2) return false;
    return true;
}

// ==== 0x491b20: rating of a party against an opponent ===============================================================
// @ 0x491b20
int GameData::gate_rating(unsigned p1, unsigned p2, int p3) {
    PartyMgr* mgr = &parties;
    // the counts are zeroed on each side of the live test (a..d in order on the call path, a chained = on the other),
    // and every party-count test after the first goes through mgr (this is then dead early and ebp is free)
    int a, b, c, d;
    bool ok = false;
    if (p2 < mgr->serial) {
        int i = mgr->lookup(p2);
        if (i >= 0 && mgr->atu((unsigned)i)->type == 1)
            ok = true;
    }
    if (ok) {
        a = 0; b = 0; c = 0; d = 0;
        count_units(p2, &a, &b, &c, &d);
    } else {
        a = b = c = d = 0;
    }
    float f1 = (float)(a - b);
    float f2 = 0.0f;
    if (p1 < mgr->serial) {
        int i = mgr->lookup(p1);
        if (i >= 0 && mgr->atu((unsigned)i)->type == 1) {
            int j = mgr->lookup(p1);
            f2 = (float)mgr->at(j)->sum_stat(0x110);
        }
    }
    float f3 = 0.0f;
    if (p3 >= 0 && mgr->is_live(p3)) {
        a = 0; b = 0; c = 0; d = 0;
        count_units(p3, &a, &b, &c, &d);
        f3 = (float)(a - b);
    }
    float n = f3 + f2;
    float ratio = n / f1;
    if (ratio >= 1.0f) ratio = ratio - 1.0f;
    else ratio = 1.0f - 1.0f / ratio;
    float mag = (n + f1) * 0.02f;
    if (mag < 0.5f) mag = 0.5f;
    else if (mag > 1.0f) mag = 1.0f;
    float prod = mag * ratio;
    float fl = floor(prod);
    int r = (int)fl;
    if (!(prod - fl <= 0.5f)) r++;
    if (p1 < mgr->serial) {
        int i = mgr->lookup(p1);
        if (i >= 0 && mgr->atu((unsigned)i)->type == 1) {
            int j = mgr->lookup(p1);
            r += mgr->at(j)->skill_level(0xf) / 2;
        }
    }
    if (p2 < mgr->serial) {
        int i = mgr->lookup(p2);
        if (i >= 0 && mgr->atu((unsigned)i)->type == 1) {
            int j = mgr->lookup(p2);  // the original looks the party up again here (2nd call 0x48ad00)
            r -= mgr->at(j)->skill_level(0xf) / 2;
        }
    }
    if (r < -0x14) return -0x14;
    if (r > 0x14) return 0x14;
    return r;
}

// ==== 0x491e50: put a party near a point =============================================================================
// @ 0x491e50
void GameData::set_pos(Vec2 at, int id, float radius) {
    random_position_around(at, radius, &at);
    PartyMgr* m = &parties;
    if ((unsigned)id >= m->serial) return;
    int i = m->lookup(id);
    if (i < 0) return;
    if (m->atu((unsigned)i)->type != 1) return;
    i = m->lookup(id);
    *(Vec2*)&m->at(i)->pos = at;       // the pair is written through a Vec2 view of the party's vector4 position
    i = m->lookup(id);
    m->at(i)->place_on_map();
}

// ==== 0x491f20 ======================================================================================================
// @ 0x491f20
int GameData::token_lookup(int a, int b, int c, int d) {
    if (a == main_party) return stack_seeds[b][c];
    return stack_dna(a, b, c, d);
}

// ==== 0x491f50: mission helpers (agent pool walks, ratings) =============================================================
// round to nearest (floor, then +1 above one half), as count_agents inlines it
static __forceinline int wd_rnd(const float* p)
{
    float fl = (float)floor(*p);
    int i = (int)fl;
    if (*p - fl <= 0.5f)
        return i;
    return i + 1;
}
// @ 0x491f50
int Mission::count_agents(int idx)
{
    int total = 0;
    for (int k = 0; k < 8; k++)          // fully unrolled by cl (the last term as (idx+0x3b)*0x16c)
        if (teams[idx].relation[k] > 0)
            total += counts[k][1];
    int res = 0;
    if (total != 0) {
        int cu = player_id;
        if (cu >= 0 && rec((unsigned)cu)->team >= 0) {
            int s = advantage;
            if (teams[idx].relation[rec((unsigned)player_id)->team] < 0) s = -s;
            float v = total * (1.0f / speed_factor((float)s));
            res = wd_rnd(&v);
            if (res <= 0) res = 1;
        }
    }
    return res;
}

// @ 0x4920e0
bool Mission::all_defeated(int a)
{
    if (player_id >= 0) {
        int team = rec((unsigned)player_id)->team;
        bool all = true;
        for (int k = 0; k < 8; k++) {
            if (teams[team].relation[k] == -1 && counts[k][1] > 0) {
                all = false;
                break;
            }
        }
        return all && (float)g_ticks[2] * 1e-05f - changed_at >= (float)a;
    }
    return false;
}

// @ 0x492170
bool Mission::hero_down()
{
    unsigned cu = (unsigned)player_id;
    if (cu == 0xffffffff) return false;
    return rec(cu)->state != 1;
}

// @ 0x4921b0
unsigned Mission::find_agent_by_troop(int id)
{
    unsigned found = 0xffffffff;
    AgentTable* pool = &agents;
    unsigned i = pool->first_used();
    unsigned n = (unsigned)pool->count;
    if (i < n) {
        std::vector<Agent*>::iterator pages = agents.pages.begin();
        do {
            if (pages[i >> 4][i & 0xf].troop == id) found = i;
            pool->next_used(&i);
        } while (i < n);
    }
    return found;
}

// ==== 0x492230: weapon orders of a team =========================================================================
// @ 0x492230
void Mission::set_weapon_order_1(int team, unsigned value)
{
    if (teams[team].cls[0].use != 0)
        teams[team].cls[0].a = value;
    if (teams[team].cls[1].use != 0)
        teams[team].cls[1].a = value;
    if (teams[team].cls[2].use != 0)
        teams[team].cls[2].a = value;

    int n = agents.count;
    unsigned i = 0;
    if (n > 0) {
        do {
            if ((&*agents.pages.begin())[i >> 4][i & 0xf].in_use != 0)
                break;
            i++;
        } while ((int)i < n);
    }
    AgentTable* pool = &agents;
    unsigned j = i;
    for (; j < (unsigned)pool->count; pool->next_used(&j)) {
        Agent* r = &(&*agents.pages.begin())[j >> 4][j & 0xf];                         // read back each iteration (a call follows)
        if (r->state == 1) {
            int k = r->team;
            if (k < 0 || k >= 8)
                k = 7;
            if (k == team) {
                int c = r->agent_class();
                if (teams[team].cls[c].use != 0) {
                    int w = r->weapon_mode;
                    r->weapon_mode = 0;
                    r->set_weapon_mode(w);
                }
            }
        }
    }
}

// @ 0x492350
void Mission::set_weapon_order_2(int team, unsigned value)
{
    if (teams[team].cls[0].use != 0)
        teams[team].cls[0].b = value;
    if (teams[team].cls[1].use != 0)
        teams[team].cls[1].b = value;
    if (teams[team].cls[2].use != 0)
        teams[team].cls[2].b = value;

    int n = agents.count;
    unsigned i = 0;
    if (n > 0) {
        do {
            if ((&*agents.pages.begin())[i >> 4][i & 0xf].in_use != 0)
                break;
            i++;
        } while ((int)i < n);
    }
    AgentTable* pool = &agents;
    unsigned j = i;
    for (; j < (unsigned)pool->count; pool->next_used(&j)) {
        Agent* r = &(&*agents.pages.begin())[j >> 4][j & 0xf];                         // read back each iteration (a call follows)
        if (r->state == 1) {
            int k = r->team;
            if (k < 0 || k >= 8)
                k = 7;
            if (k == team) {
                int c = r->agent_class();
                if (teams[team].cls[c].use != 0) {
                    int w = r->weapon_mode;
                    r->weapon_mode = 0;
                    r->set_weapon_mode(w);
                }
            }
        }
    }
}

// ==== 0x492470: the battle object's rating and side selection ====================================================
// @ 0x492470
void MissionCtx::refresh_rating()
{
    PartyMgr* mgr = &g_game->parties;
    int idx = mgr->lookup(ids[side]);
    int t = mgr->pages[idx >> 4][idx & 0xf].sum_stat(0x10);
    if (t <= 3)
        tier = 0;
    else if (t <= 0xa)
        tier = 1;
    else
        tier = (t > 0x32) + 2;

    int s = side;
    int r = g_game->gate_rating(g_game->main_party, ids[s], ids[1 - s]);
    advantage = r;
    if (r > 6)
        advantage = 6;
    if (advantage < -0x20)
        advantage = -0x20;
}

// ==== 0x492530: move all stacks of one party into another =========================================================
// @ 0x492530
void GameData::transfer_stack(int a, int b, char flag)
{
    WL_MARKER(12)
    PartyMgr* mgr = &parties;
    Party* pa = mgr->at(mgr->lookup(a));
    Party* pb = mgr->at(mgr->lookup(b));
    for (int k = 0; k < pa->nstacks; k++) {
        Stack* s = &pa->stacks[k];
        int f = s->flags;
        if (flag) {
            if (f & 1)
                f &= ~1;
            else
                f |= 1;
        }
        int r = pb->add_members(s->troop, s->count, f, 0, 0);
        if (b == main_party && r >= 0) {
            int shift = s->count;
            if (shift < 0x20) {
                int* d = &stack_seeds[r][shift];
                int* p = stack_seeds[r].v;
                for (int j = 0; j < 0x20 - shift; j++)
                    d[j] = p[j];
            } else {
                shift = 0x20;
            }
            for (int j = 0; j < shift; j++) {
                int v;
                if (a == main_party) {
                    v = stack_seeds[k][j];
                } else {
                    v = -1;
                    if (a >= 0) {
                        Party* px = mgr->at(mgr->lookup(a));
                        v = px->stacks[k].troop;
                    }
                    v = v * 0x1bf + 0x45db + a * 0x7bb7 + j;
                }
                stack_seeds[r][j] = v;
            }
        }
    }
    remove_party(a);
    WL_MARKER(13)
}

// @ 0x492750
void MissionCtx::select_side(int s)
{
    WL_MARKER(12)
    int m = mode;
    if (m != 4)
        player_side = s;
    if (ids[1 - s] >= 0) {
        int flip = 1 - s;
        side = flip;
        mission_flags &= ~0x10;
        side_chosen = 1;
        if (m == 4 && flip == 1)
            mission_flags |= m;
        refresh_rating();
    }
    WL_MARKER(13)
}

// ==== 0x4927d0: std::vector<int>::_Insert_n (with its cold tail 0x492930) ======================================
// The original instance is the pointer-element one (every vector<T*> shares it, /OPT:ICF): the scene list below uses it.
// @ 0x4927d0
template void std::vector<SceneObject*>::_Insert_n(std::vector<SceneObject*>::iterator, size_t, SceneObject* const&);

// ==== 0x492a20: free the game object's record arrays =============================================================
// @ 0x492a20
void GameData::free_arrays()
{
    Troop* s = troops;
    scene_open = 0;
    if (s != 0) {
        delete[] s;
        troops = 0;
    }

    SceneFile* p = scenes;
    if (p != 0) {
        delete[] p;
        scenes = 0;
    }

    FactionRec* t = factions;
    if (t != 0) {
        delete[] t;
        factions = 0;
    }

    QuestRec* q = quests;
    if (q != 0) {
        delete[] q;
        quests = 0;
    }
}

// ==== 0x492b30: std::vector<__int64>::_Insert_n (the quest / slot arrays) =======================================
// The original is the library instance: its STL helpers (_Ucopy / _Ufill / _Xlen) are separate functions elsewhere.
// @ 0x492b30
template void std::vector<__int64>::_Insert_n(std::vector<__int64>::iterator, size_t, const __int64&);

// ==== 0x492d80: std::deque<int>::_Growmap ==========================================================================
// @ 0x492d80
template void std::deque<int>::_Growmap(size_t);

// ==== 0x492ee0: pop the last pointer of a vector, destroying the object ==========================================
// @ 0x492ee0
void TableauTbl::pop_back_delete()
{
    unsigned _n = worlds.size();
    SceneObject* _Ptr = worlds[_n - 1];
    if (_Ptr != 0) {
        _Ptr->~SceneObject();                          // ~world 0x475d50
        ::operator delete(_Ptr);
    }
    worlds.pop_back();
}

// ==== 0x492f40: the game object's destructor =======================================================================
struct CursorObj {
    void reset();   // 0x492a20 (GameData::free_arrays)
};
#include "game/data/trigger_vecs.h"   // SimpleTriggerVec: the simple-trigger list
// @ 0x492f40
void GameData::world_free()
{
    if (scene_open != 0)
        free_arrays();

    {
        tstring* p = talk_lines + 5;
        int n = 5;
        while (n != 0) {
            --p;
            p->~tstring();
            --n;
        }
    }
    background_mesh.~tstring();

    editor.sel.clear();

    debugger.text.~tstring();
    debugger.script.~tstring();

    {
        tstring* p = labels + 50;
        int n = 0x32;
        while (n != 0) {
            --p;
            p->~tstring();
            --n;
        }
    }

    message_log.~tstring();
    t13ab8.~tstring();
    t13aa8.~tstring();
    t13a70.~tstring();

    tracks.clear();

    simple.~SimpleTriggerVec();                                // 0x4317b0 (the simple triggers' delete[] of their records)

    delete[] triggers.data;                                         // ??_ETriggerRec 0x431480 (a call: with a `for` loop before it in the function cl inlines it, see the while loops above)

    parties.clear();
    events.Pool<Event>::~Pool();                                                  // 0x4c6e30 (Pool<Event>::~Pool)
    nameb8.~tstring();

    ents.clear();

    module.~tstring();
    title.~tstring();
}

// ==== 0x493090: std::vector<__int64>::resize (out-of-line instance; SlotArray::grow and read call it) ==============
// @ 0x493090
template void std::vector<__int64>::resize(size_t, __int64);

// ==== 0x493120: std::deque<int>::push_back ============================================================================
// @ 0x493120
template void std::deque<int>::push_back(const int&);

// ==== 0x4931a0 .. 0x493400: the container / list panels =================================================================
// @ 0x4931a0
ContainerBase::ContainerBase()
{
}

// @ 0x4931d0
DropDown::DropDown() throw()
{
    f180 = 0;
    f188 = 0;
    f164 = 0;
    f160 = 0;
    button_size = V2(0.2265f, 0.029f);
    text_size = V2(0.018f, 0.018f);
    button_x = 0.0135f;
    text_pos = V2(0.02f, 0.01f);
}

// @ 0x4932a0
void DropDown::render(rgl::scene* sc, float* count, float* c, float* d)
{
    if (f180 == 0)
        f178 = 1;
    else
        f178 = 0;
    if (f178 != 0) {
        b11c[3] = 0;
        b11c[2] = 1;
        b11c[1] = 0;
    } else {
        b11c[3] = 0;
        b11c[2] = 0;
        b11c[1] = 1;
    }
    MissionUi::render(sc, count, c, d);
}

// @ 0x493300
int DropDown::kind()
{
    return 5;
}

// @ 0x493310
void DropDown::set15c(int v)
{
    int lo = f160;
    v15c = v;
    if (v < lo)
        v15c = lo;
    else if (v > f164)
        v15c = f164;

    int cur = v15c;
    if (cur < lo)
        v15c = lo;
    else if ((unsigned)cur > rows.size())
        v15c = rows.size();

    set_text(rows[v15c]);
}

// @ 0x4933b0
void DropDown::reset_tail()
{
    rows.clear();                                               // _Tidy (0x42d390): destroy the rows, free the buffer
    f164 = 0;
    v15c = 0;
}


// UTF-8 -> code points (0x487b00). The original uses cl's static register convention, so the arguments are ordered
// to put the tstring in EDI and the count out-pointer in EDX.
static __declspec(noinline) void decode_utf8_487b00(const tstring* _s, int* _Out, int* _Buf, const int* _Maxn)
{
    *_Out = 0;
    int _n = _s->len;
    int _i = 0;
    if (_n > 0) {
        do {
            int _cnt = *_Out;
            if (_cnt >= *_Maxn)
                break;
            unsigned char _c = _s->buf[_i];
            int _ch;
            if (_c >= 0xf0 && _c < 0xf5) {
                _ch = ((((((unsigned char)_s->buf[_i] & 7) << 6) | ((unsigned char)_s->buf[_i+1] & 0x3f)) << 6 | ((unsigned char)_s->buf[_i+2] & 0x3f)) << 6) | ((unsigned char)_s->buf[_i+3] & 0x3f);
                _i += 4;
            } else if (_c >= 0xe0 && _c < 0xf0) {
                _ch = (((((unsigned char)_s->buf[_i] & 0xf) << 6) | ((unsigned char)_s->buf[_i+1] & 0x3f)) << 6) | ((unsigned char)_s->buf[_i+2] & 0x3f);
                _i += 3;
            } else if (_c >= 0xc2 && _c < 0xe0) {
                _ch = ((((unsigned char)_s->buf[_i] & 0x1f) << 6) | ((unsigned char)_s->buf[_i+1] & 0x3f));
                _i += 2;
            } else {
                _i++;
                if (_c >= 0x80)
                    continue;
                _ch = (signed char)_c;
            }
            _Buf[_cnt] = _ch;
            *_Out = _cnt + 1;
        } while (_i < _n);
    }
}

// @ 0x493400
void DropDown::append(const tstring* _name)
{
    float _scale = 0.02f;
    int _count;
    int _maxn = 0x800;
    int _buf[0x800];
    decode_utf8_487b00(_name, &_count, _buf, &_maxn);
    float _w = (float)_count * 0.0077999993f;
    if (_w > 0.21f)
        _scale = 0.21f / _w * 0.02f;
    if (_scale < text_scale.x) {
        set_size(V2(_scale, _scale));
    }
    rows.push_back(*_name);   // 0x43f2a0
    ++f164;
    if (rows.size() == 1)
        set15c(0);
}

// ==== 0x4934d0: the party record constructor and its page / hash management ========================================
void __stdcall vector_constructor_iterator(void* first, unsigned size, unsigned count, void* fn);   // 0x401800
void elem_ctor_48c210(void* self);                              // 0x48c210
#include <list>
// std::list<int>::_Buynode (0x46c0c0, protected: reached through a derived class)
namespace std { template<> list<int, allocator<int> >::_Nodeptr list<int, allocator<int> >::_Buynode(); }   // 0x46c0c0
struct ListInt : std::list<int> { using std::list<int>::_Buynode; };

// @ 0x4936a0
void Party::slot1()
{
    init();
}

// @ 0x4934d0 (the constructor: the members' own constructors in declaration order: the names, the 64 Stack records, the
// clock-1 timers and the footstep gui_timer (clock 0), the vectors, the notes with their tableau -1)
Party::Party() throw() : notes(-1)
{
    init();
    clk59b0.rewind();
    f5a04 = 0;
}

// @ 0x4936b0
template void std::vector<int>::push_back(const int&);

// @ 0x493700
template<> int Pool<Event>::alloc()
{
    if (free_ids.size() <= 0) {
        Event* blk = new Event[16];
        pages.push_back(blk);
        for (int i = 0; i < 16; ++i) {
            blk[i].in_use = 0;
            blk[i].pool_id = count + i;
            free_ids.push_back(count + i);
        }
        count += 16;
    }
    unsigned idx = free_ids.back();
    free_ids.pop_back();
    Event* r = pages[idx >> 4] + (idx & 0xf);
    r->in_use = 1;
    r->serial = serial;
    ++serial;
    return idx;
}

// @ 0x493830
PartyMgr* PartyMgr::grow()
{
    Party* blk = new Party[16];
    pages.push_back(blk);

    for (int i = 0; i < 0x10; ++i) {
        blk[i].type = 0;
        free_ids.push_back(count + i);
    }
    count += 0x10;
    return this;
}

// @ 0x493960
int PartyMgr::add_slot()
{

    if (free_ids.size() <= 0)
        grow();

    unsigned idx = free_ids.back();
    free_ids.pop_back();

    Party* p = pages[idx >> 4] + (idx & 0xf);
    p->type = 1;
    p->id = serial;

    int h = serial;
    int slot = h % 0x2717;
    while (tab[slot].key != (unsigned)-1)
        slot = (slot + 0x4f) % 0x2717;
    tab[slot].key = h;
    tab[slot].value = idx;
    ++serial;
    return p->id;
}

// @ 0x493a30
void ContainerBase::add_child(MissionUi* _obj)
{
    kids.push_back(_obj);
    _obj->p154 = this;
    on_child_moved(_obj);
}

// @ 0x493ab0
// The tableau scene's grid: the members' own constructors (the two pools' list heads, the vectors, the view block), then a
// 2000 x 2000 grid of one cell around the origin, no ground / nav mesh / height field, the camera direction (1, 0, 0).
rgl::world::world() : noise_counter(0)
{
    next_id = 1;
    {
        float _x[2] = { 1000.0f, 1000.0f };
        float _y[2] = { -1000.0f, -1000.0f };
        init(_y, _x, 0, 0);
    }
    ground = 0;
    nav = 0;
    hf = 0;
    cam_dir = vector4(1.0f, 0.0f, 0.0f);
    cam_pos = vector4(0.0f, 0.0f, 0.0f);
    ambient = vector4(0.0f, 0.0f, 0.0f);
    sky = 0;
}

// ==== 0x493c40: the slot array's growth ===========================================================================
// @ 0x493c40
void SlotArray::grow(int* _pNewsize)
{
    if ((unsigned)*_pNewsize > size()) {
        int _n = *_pNewsize;
        if (_n < 0x10)          _n = 0x10;
        else if (_n < 0x20)     _n = 0x20;
        else if (_n < 0x40)     _n = 0x40;
        else if (_n < 0x80)     _n = 0x80;
        else if (_n < 0x100)    _n = 0x100;
        else if (_n < 0x200)    _n = 0x200;
        else if (_n < 0x400)    _n = 0x400;
        else if (_n < 0x800)    _n = 0x800;
        else if (_n < 0x1000)   _n = 0x1000;
        else if (_n < 0x2000)   _n = 0x2000;
        else if (_n < 0x4000)   _n = 0x4000;
        else if (_n < 0x8000)   _n = 0x8000;
        else if (_n < 0x10000)  _n = 0x10000;

        int _old = (int)size();
        __int64 _z = 0;
        resize((unsigned)_n, _z);                       // the instance above (0x493090)
        for (int i = _old; i < _n; ++i) {
            (*this)[i] = 0;                               // the new slot's two halves, low word first (two immediate stores)
        }
    }
}

// @ 0x493d70
void SlotArray::set(const int& slot, const __int64& value)
{
    int _n = slot + 1;
    grow(&_n);
    (*this)[slot] = value;
}

// @ 0x493db0
void ScriptState::set_var(int _idx, int _val)
{
    SlotArray* _p = &vars;
    int _n = _idx + 1;
    _p->grow(&_n);
    (*_p)[_idx] = _val;
}

// @ 0x493df0
skin* E68::find_or_create(const int* _key)
{
    skin* _found = 0;
    for (unsigned i = 0; i < v.size(); ++i) {
        if (v[i]->key == *_key) {
            _found = v[i];
            break;
        }
    }
    if (_found != 0)
        return _found;

    skin* _new = make_material(_key);
    v.push_back(_new);
    return _new;
}

// ==== 0x493e90: resource cache groups ==================================================================================
// @ 0x493e90
void E68::purge()
{
    int _i = 0;
    int _last = (int)v.size() - 1;
    if (_last >= 0) {
        do {
            if (v[_i]->refcount <= 0) {
                v[_i]->tableau = -1;
                v[_i]->key = -1;
                g_resmgr.release(v[_i]);   // 0x4175f0
                v[_i] = v[_last];
                --_last;
            } else
                ++_i;
        } while (_i <= _last);
    }
    v.resize(_last + 1, 0);
}

// @ 0x493f30
void TableauTbl::purge_all()
{
    for (unsigned _i = 0; _i < (unsigned)recs.size(); ++_i)
        recs[_i].purge();
}

// ==== 0x493f80, 0x494020: tableau light nodes ============================================================================
// @ 0x493f80
void TableauTbl::add_point_light(const rgl::matrix* p, const rgl::vector4& c)
{
    Light* l = ((rgl::world*)worlds[worlds.size() - 1])->spawn_wind();   // 0x478f30
    l->color = c;
    l->type = 0;
    l->pos = p->o;
    l->base_color = l->color;
    l->range = (l->color.z + l->color.y + l->color.x) * 10.0f;
}

// @ 0x494020
void TableauTbl::add_sun_light(const rgl::matrix* p, const rgl::vector4& c)
{
    Light* l = ((rgl::world*)worlds[worlds.size() - 1])->spawn_wind();   // 0x478f30
    l->color = c;
    l->type = 2;
    l->dir = p->f;
    l->pos = p->o - l->dir * 1000.0f;
    l->base_color = l->color;
}

// ==== 0x494110: add a button widget to the manager ===================================================================
// @ 0x494110
int widget_mgr::create_button_overlay(const tstring& text, unsigned flags)
{
    UiA* w = new UiA;

    w->move_a(V2(0.0f, 0.0f));
    w->set148((flags & 0x181c) | 0x200);
    w->set_hilight_color(0xff55ff50);
    w->set_xy(V2(1.0f, 0.75f));
    w->set_text(text);
    scene->add_child(w);
    widgets.push_back(w);
    return (int)widgets.size() - 1;
}

// ==== 0x494260: open a screen ==========================================================================================
// @ 0x494260
void WindowMgrW::open(int _idx)
{
    g_gui.set_overlay(0, 1);
    dq.push_back(_idx);
    arr[_idx]->init_screen();
    bool v = arr[dq.back()]->always_true();
    if (g_render.dev != 0)
        g_render.dev->ShowCursor(v);
    g_gui.show_cursor = v;
}

// ==== 0x4942f0, 0x494310: the sound player ================================================================================
// @ 0x4942f0
void SoundMgr::play_ui(int _a, unsigned _b)
{
    unsigned _f = _b;
    _f |= 1;
    play(_a, (int)_f, &frame.o, 1.0f, 0);
}

// @ 0x494310
void SoundMgr::stop_all()
{
    if (g_cfg.disable_sound != 0)
        return;

    for (unsigned _i = 0; _i < channels.size(); ++_i)
        FSOUND_StopSound(channels[_i]);

    channels.resize(0, 0);

    rgl::vector4 _pos;
    _pos.x = 0.0f;
    _pos.y = 0.0f;
    _pos.z = 0.0f;
    rgl::mat3 _t;
    rgl::mat3 _rot = *identity3(&_t);
    rgl::matrix _m;
    static_cast<rgl::mat3&>(_m) = _rot;
    _m.o = _pos;
    if (g_cfg.disable_sound == 0) {
        frame = _m;
        FSOUND_3D_Listener_SetAttributes(&frame.o.x, 0, frame.f.x, frame.f.z, frame.f.y,
                               frame.u.x, frame.u.z, frame.u.y);
    }
}

// ==== 0x494410..0x494680: quest lines on the message log ================================================================
extern "C" const char s_ui_quest_completed_value[];             // 0x592650
extern "C" const char s_ui_quest_succeeded_value[];             // 0x59266c
extern "C" const char s_ui_quest_failed_value[];                // 0x592688

static inline bool qbit(unsigned v, int n) { return ((v >> n) & 1) != 0; }

// @ 0x494410
void GameData::quest_done(int _idx)
{
    WL_MARKER(12)
    if (qbit(quests[_idx].flags2, 1) ||
        ((quests[_idx].flags & 0x200) == 0 && (quests[_idx].flags & 0x100) != 0)) {
        QuestRec* _e = &quests[_idx];
        if ((_e->flags & 0x800) == 0) {
            tstring _val(_e->text);
            char _buf[0x200];
            {
                tstring _fmt("ui_quest_completed_value");
                const char* _f = _val.c_str();
                tstring* _s = g_strtable.lookup(_fmt);
                const char* _r = _s->c_str();
                mb_snprintf(_buf, 0x200, _r, _f);
            }
            {
                tstring _txt(_buf);
                g_gui.push(_txt, 0xffd0f0c8);
            }
            g_sound_mgr.play(g_sound_mgr.slot[3], 1, &g_sound_mgr.frame.o, 1.0f, 0);
        }
        if (qbit(_e->flags2, 1))
            _e->f64 = 0;
        else
            _e->flags |= 0x200;
        WL_MARKER(13)
    }
}

// @ 0x494560
void GameData::quest_succeeded(int _idx)
{
    WL_MARKER(12)
    QuestRec* _e = &quests[_idx];
    _e->flags |= 0x1400;
    tstring _val(quests[_idx].text);
    char _buf[0x200];
    {
        tstring _fmt("ui_quest_succeeded_value");
        const char* _f = _val.c_str();
        tstring* _s = g_strtable.lookup(_fmt);
        const char* _r = _s->c_str();
        mb_snprintf(_buf, 0x200, _r, _f);
    }
    {
        tstring _txt(_buf);
        g_gui.push(_txt, 0xff90ff28);
    }
    g_sound_mgr.play(g_sound_mgr.slot[4], 1, &g_sound_mgr.frame.o, 1.0f, 0);
    WL_MARKER(13)
}

// @ 0x494680
void GameData::quest_failed(int _idx)
{
    WL_MARKER(12)
    QuestRec* _e = &quests[_idx];
    _e->flags |= 0x1800;
    tstring _val(quests[_idx].text);
    char _buf[0x200];
    {
        tstring _fmt("ui_quest_failed_value");
        const char* _f = _val.c_str();
        tstring* _s = g_strtable.lookup(_fmt);
        const char* _r = _s->c_str();
        mb_snprintf(_buf, 0x200, _r, _f);
    }
    {
        tstring _txt(_buf);
        g_gui.push(_txt, 0xffd02028);
    }
    g_sound_mgr.play(g_sound_mgr.slot[6], 1, &g_sound_mgr.frame.o, 1.0f, 0);
    WL_MARKER(13)
}

// ==== 0x4947a0, 0x4948c0: quest concluded / cancelled ====================================================================
// the ring-buffer append inlined by this TU
static __forceinline void log_add(const tstring& s)
{
    if (g_gui.enabled && g_gui.ring.count < 32) {
        int idx = (g_gui.ring.head + g_gui.ring.count) % 32;
        g_gui.ring.names[idx] = s;
        g_gui.ring.vals[idx] = 0xffffffff;       // 0x494e0b stores -1
        g_gui.ring.count++;
    }
}

// @ 0x4947a0
void GameData::set_quest_concluded(int _idx)
{
    WL_MARKER(12)
    QuestRec* _e = &quests[_idx];
    _e->flags |= 0x1000;
    tstring _val(quests[_idx].text);
    char _buf[0x200];
    {
        tstring _fmt("ui_quest_concluded_value");
        const char* _f = _val.c_str();
        tstring* _s = g_strtable.lookup(_fmt);
        const char* _r = _s->c_str();
        mb_snprintf(_buf, 0x200, _r, _f);
    }
    {
        tstring _txt(_buf);
        g_gui.push(_txt, 0xffd02028);
    }
    g_sound_mgr.play(g_sound_mgr.slot[5], 1, &g_sound_mgr.frame.o, 1.0f, 0);
    WL_MARKER(13)
}

// @ 0x4948c0
void GameData::set_quest_cancelled(int _idx)
{
    WL_MARKER(12)
    QuestRec* _e = &quests[_idx];
    _e->flags = 0;
    tstring _val(quests[_idx].text);
    char _buf[0x200];
    {
        tstring _fmt("ui_quest_cancelled_value");
        const char* _f = _val.c_str();
        tstring* _s = g_strtable.lookup(_fmt);
        const char* _r = _s->c_str();
        mb_snprintf(_buf, 0x200, _r, _f);
    }
    {
        tstring _txt(_buf);
        g_gui.push(_txt, 0xffd02028);
    }
    g_sound_mgr.play(g_sound_mgr.slot[7], 1, &g_sound_mgr.frame.o, 1.0f, 0);
    WL_MARKER(13)
}


// ==== 0x4949d0, 0x494ad0: party objectives =================================================================================
// @ 0x4949d0
void GameData::set_objective(int party, int value)
{
    PartyMgr* m = &parties;
    Party* pp = m->at(m->lookup(party));
    if (pp->attached_to >= 0)
        detach_party(party);
    Party* pp2 = m->at(m->lookup(party));
    pp2->attached_to = value;
    Party* pv = m->at(m->lookup(value));
    pv->attached.push_back(party);
    m->at(m->lookup(value))->dirty = 1;
}

// @ 0x494ad0
void GameData::switch_objective(int party, int mode)
{
    if (party == attach_party)
        return;
    if (party == attach_scene)
        return;
    if (party == main_party)
        return;
    if (!f13a18)
        return;
    if (((Party*)parties.get(party))->attached_to >= 0)
        detach_party(party);
    if (mode == 0)
        set_objective(party, main_party);
    else if (mode == 1)
        set_objective(party, attach_party);
    else if (mode == 2)
        set_objective(party, attach_scene);
    ((Party*)parties.get(party))->engaged = 1;
}

// ==== 0x494b60: share an experience amount over a party ==================================================================
// round to nearest through the CRT floor (0x536b10): an inline helper, its i and the result are separate values (mov ecx,eax; mov esi,ecx; lea esi,[ecx+1])
static inline int round_half_up(float x)
{
    float f = (float)floor(x);
    int i = (int)f;
    if (x - f <= 0.5f)
        return i;
    return i + 1;
}
// @ 0x494b60
void GameData::share_experience(int party, float amount)
{
    PartyMgr* pm_ = &parties;
    if (party >= pm_->serial)
        return;
    int idx = pm_->lookup(party);
    if (idx < 0)
        return;
    if (pm_->atu(idx)->type != 1)
        return;
    Party* p = pm_->at(pm_->lookup(party));
    int total = 0;
    int i = 0;
    for (; i < p->nstacks; i++) {
        if (p->stacks[i].flags & 1)
            break;
        int troop = p->stacks[i].troop;
        char hero = (troops[troop].flags30 & 0x10) != 0;
        if (hero) {
            if (i == 0)
                total = (int)((float)total + 10.0f);
            else
                total = (int)((float)total + 3.0f);
        } else
            total += p->stacks[i].count;
    }
    if (total <= 0)
        return;                                 // 0x494c69: the whole function ends here, no message either
    {
        float per = amount / (float)total;
        for (int j = 0; j < p->nstacks; j++) {
            if (p->stacks[j].flags & 1)
                break;
            int troop = p->stacks[j].troop;
            char hero = (troops[troop].flags30 & 0x10) != 0;
            if (hero) {
                if (j == 0)
                    GameData::add_experience(troop, (int)(per * 10.0f));
                else
                    GameData::add_experience(troop, (int)(per * 3.0f));
            } else
                p->add_xp_to_stack(j, (int)((float)p->stacks[j].count * per));
        }
    }
    int n = round_half_up(amount);
    if (n != 0 && party == main_party) {
        char buf[512];
        {
            tstring key("ui_shared_number_experience_within_party");
            sprintf(buf, g_strtable.lookup(key)->c_str(), n);
        }
        {
            tstring msg(buf);
            log_add(msg);
        }
    }
}

// ==== 0x494e40: award experience to the troop stacks of a party and fold the share-in counters ===========================
// @ 0x494e40
void GameData::award_experience(int party, float amount, int mode)
{
    PartyMgr& m2 = parties;
    if ((unsigned)party >= m2.serial)
        return;
    int idx = m2.lookup(party);
    if (idx < 0)
        return;
    if (m2.atu((unsigned)idx)->type != 1)
        return;
    idx = m2.lookup(party);                      // the original looks the party up a second time
    Party* p = &((Party*)(m2.pages[idx >> 4]))[idx & 15];
    mode = mode == 0 ? rand() % 2 : mode - 1;

    int total = 0;
    for (int i = 0; i < p->nstacks; i++) {
        if (p->stacks[i].flags & 1)
            break;
        if (troops[p->stacks[i].troop].flags30 & 0x10) {
            if (i == 0)
                total = (int)((float)total + 10.0f);
            else
                total = (int)((float)total + 3.0f);
        } else
            total += p->stacks[i].count;
    }
    float per = amount / (float)total;

    for (int i = 0; i < p->nstacks; i++) {
        if (p->stacks[i].flags & 1)
            break;
        int troop = p->stacks[i].troop;
        if (troops[troop].flags30 & 0x10) {
            int r = rand() % 15817;
            // no float temporary: the original keeps the product on the x87 stack (0x494fd1 .. 0x494fdb) and truncates it from there
            if (i == 0)
                GameData::add_experience(troop, (int)(((float)r * (1.0f / 31634.0f) + 0.75f) * 10.0f * per));
            else
                GameData::add_experience(troop, (int)(((float)r * (1.0f / 31634.0f) + 0.75f) * 3.0f * per));
        } else {
            int r = rand() % 15817;
            p->add_xp_to_stack(i, (int)((((float)r * (1.0f / 15817.0f)) * 0.5f + 0.75f) * (float)p->stacks[i].count * per));
        }
    }

    for (int i = 0; i < p->nstacks; i++) {
        if (p->stacks[i].flags & 1)
            break;
        if (troops[p->stacks[i].troop].flags30 & 0x10)
            continue;
        Stack& s = p->stacks[i];
        if (s.f14 <= 0)
            continue;
        int lead = troops[s.troop].upgrade_troop[0];
        if (mode == 1 && (unsigned)troop_f54(&troops[s.troop]) > 0)
            lead = troop_f54(&troops[s.troop]);

        int n = 0;
        for (int j = 0; j < p->nstacks; j++) {
            if (p->stacks[j].flags & 1)
                break;
            n++;
        }
        int k = -1;
        for (int j = 0; j < n; j++)
            if (p->stacks[j].troop == lead)
                k = j;

        if (k >= 0) {
            p->stacks[k].count += s.f14;
            int extra = (s.f14 - s.count) + s.wounded;
            extra = extra < 0 ? 0 : extra;
            p->stacks[k].wounded += extra;
            if (s.f14 == s.count) {
                p->remove_row(i, 0);
                i--;
            } else {
                s.count -= s.f14;
                int c = s.count;
                if (s.wounded < c)
                    c = s.wounded;
                s.wounded = c;
                s.f14 = 0;
            }
        } else {
            if (s.f14 == s.count) {
                s.troop = lead;
                s.f14 = 0;
                s.xp = 0;
            } else {
                int n2 = 0;
                for (int j = 0; j < p->nstacks; j++) {
                    if (p->stacks[j].flags & 1)
                        break;
                    n2++;
                }
                if (n2 < p->max_stacks()) {
                    int x = (s.f14 - s.count) + s.wounded;
                    x = x < 0 ? 0 : x;
                    p->add_members(lead, s.f14, 0, x, 0);
                    s.count -= s.f14;
                    int c = s.count;
                    if (s.wounded < c)
                        c = s.wounded;
                    s.wounded = c;
                    s.f14 = 0;
                }
            }
        }
    }
}

// ==== 0x4952a0: award a random fraction of an amount to the main hero's party ========================================
// @ 0x4952a0
int GameData::add_random_experience(int amount)
{
    int r = rand() % 15817;
    return add_experience(player, (int)(((float)r * (1.0f / 15817.0f) + 0.5f) * amount));
}

// ==== 0x4952f0: put a light / wind object at the trigger frame ======================================================
// @ 0x4952f0
void Mission::add_wind_effect(float a1, float a2)
{
    Light* o = scene->spawn_wind();   // 0x478f30
    o->flicker_magnitude = a1;
    o->flicker_interval = a2;
    o->color = g_game->current_color;
    o->type = 0;
    rgl::vector4 v;
    rgl::vector4& t0 = g_game->trigger_frame.s;
    rgl::vector4& t1 = g_game->trigger_frame.f;
    rgl::vector4& t2 = g_game->trigger_frame.u;
    rgl::vector4& t3 = g_game->trigger_frame.o;
    v.x = g_game->trigger_frame.u.x * g_game->position_delta.z +
          g_game->trigger_frame.f.x * g_game->position_delta.y +
          t0.x * g_game->position_delta.x +
          t3.x;
    v.y = t0.y * g_game->position_delta.x + t2.y * g_game->position_delta.z +
          t1.y * g_game->position_delta.y + t3.y;
    v.z = t0.z * g_game->position_delta.x + t2.z * g_game->position_delta.z +
          t1.z * g_game->position_delta.y + t3.z;
    o->pos = v;
    o->base_color = o->color;
    o->range = (o->color.z + o->color.y + o->color.x) * 10.0f;
}

// ==== 0x495430: show / hide the carried meshes of an agent's four slots ========================================================
// cdecl; arg1 = the agent body (skeleton at +0x304), arg2 = four carried item refs, arg3 = the three per-slot mesh arrays.
// @ 0x495430
void __cdecl place_scene_agents(rgl::strategic_entity* holder, ItemRef* table, CarryCtx* ctx)
{
    char placed[11];
    memset(placed, 0, sizeof(placed));
    for (int i = 3; i >= 0; i--) {
        int id = table[i].item;
        if (id < 0)
            continue;
        ItemKind* it = &g_items[id];
        if ((it->flags & 0x7f0000000ui64) == 0)
            continue;
        rgl::matrix M;
        int attach;
        int bone;
        it->get_carry_pos(&M, &attach, &bone);
        if (placed[bone] != 0 || ((it->type_flags & 0xff) == 7 && (int)(table[i].m4 & 0xffff) <= 0xa)) {
            if (ctx->c3[i] != 0)
                ctx->c3[i]->mask = 0;
            continue;
        }
        placed[bone] = 1;
        if (ctx->c3[i] != 0) {
            ctx->c3[i]->mask = 0xffff;
            continue;
        }
        if (ctx->c1[i] != 0) {
            ctx->c1[i]->mask = 0xffff;
            rgl::lod_mesh* c = ctx->c1[i];
            FxDef* sk = holder->fx;
            sk->remove_group(c);
            c->frame = M;
            sk->elems[attach].add_group(c);
        }
        if (ctx->c2[i] != 0) {
            ctx->c2[i]->mask = 0xffff;
            rgl::lod_mesh* c = ctx->c2[i];
            FxDef* sk = holder->fx;
            sk->remove_group(c);
            c->frame = M;
            sk->elems[attach].add_group(c);
        }
    }
}

// ==== 0x4955e0: register an emitter in a table and record its index ==============================================
// @ 0x4955e0
void Party::add_emitter(int e)
{
    if (model == 0) return;
    int owner = 0;
    rgl::vector4 pos;                                           // a zeroed position (stores are immediates, not a shared register; w is never set)
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    rgl::psys* em = g_emit_defs.spawn(&e, &pos, &owner);   // 0x479d20
    if (em != 0) {
        model->add_emitter(em);
        deferred.push_back(e);
    }
}

// ==== 0x495650: append a fresh scene object ====================================================================
// @ 0x495650
SceneObject* TableauTbl::push()
{
    SceneObject* o = new SceneObject;
    worlds.push_back(o);
    return worlds[worlds.size() - 1];
}

// ==== 0x495700: leave the screen state, tear down the open-screen deque ==========================================
// @ 0x495700
void WindowMgrW::set_state(int idx)
{
    widget_mgr* g = g_presentation;
    if (g != 0) {
        g->release();
        g_presentation = 0;
    }
    g_gui.set_overlay(0, 1);
    if (arr[idx]->pure_7()) {
        while (dq.size() != 0) {
            arr[dq.back()]->reset_screen();
            dq.pop_back();
        }
        g_gui.reset();
    }
    open(idx);
}

// ==== 0x4957d0: spread an experience reward over the main party's heroes ====================================================
// @ 0x4957d0
void GameData::spread_party_experience(int amount)
{
    WL_MARKER(12)
    GameData::add_experience(player, amount);
    PartyMgr* m = &parties;
    Party* p = m->at(m->lookup(main_party));
    int live = 0;
    for (int i = 0; i < p->nstacks; i++) {
        if (p->stacks[i].flags & 1)
            break;
        live++;
    }
    int heroes = 0;
    if (live > 1) {
        for (int i = 1; i < live; i++) {
            int troop = p->stacks[i].troop;
            if (troops[troop].flags30 & 0x10)
                heroes++;
        }
        if (heroes) {
            for (int i = 1; i < live; i++) {
                int troop = p->stacks[i].troop;
                if (troops[troop].flags30 & 0x10)
                    GameData::add_experience(troop, amount / heroes);
            }
        }
    }
    WL_MARKER(13)
}
