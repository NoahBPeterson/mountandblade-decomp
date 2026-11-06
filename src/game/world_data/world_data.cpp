//-----------------------------------------------------------------------------
// File: world_data.cpp  (0x490030..0x4958f0)
//
// Desc: the party / quest / experience members of the game object, the container-widget helpers, the scene lists, the
//       message-log quest lines and the neighbouring small objects of translation units 83 (world_2) and 84
//       (game_data_2) of the TU map. Names provisional. Functions in ascending address order. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "../../include/image_addrs.h"
#include "./world_data.h"
#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx (g_mission_ctx 0xa73300): refresh_rating, select_side
#include "../../include/crt.h"      // canonical CRT declarations (listed directly: the resolution audit reads only the source's own includes)
#include "../game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)
#include "../rec98.h"             // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../../include/game_recs.h"
#include "../strat/gui_manager.h"
#include "../str_table.h"
#include "../../include/vec.h"
#include "../../include/game_data.h"
#include "../config/widget_mgr.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../ui/mission_ui.h"
#include "../ui/mission.h"
#include "../mission.h"
#include "../agent_obj.h"
#include "../../include/rec_dtors.h"
#include "../placed_obj.h"
#include "../../include/i64_vec.h"
#include "../../include/e68.h"
#include "../mission5/data_files.h"                      // TroopFile / QuestFile / FactionFile: the record arrays free_arrays deletes
#include "../../include/mesh_parts.h"
#include "../../rgl/mesh.h"
#include "../../rgl/scene/world.h"      // rgl::world (ctor 0x478ad0, dtor 0x475d50, init 0x478830, spawn_wind 0x478f30)
#include "./tableau.h"
#include "../../include/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40): the table's members defined here
#include "./widget_mgr_w.h"
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128
#include "../../include/item_kind.h"   // g_items 0x5e29d0
#include "../float_consts.h"   // shared float constants kept in memory (0x549cdc ...)
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
struct Presentation;
extern Presentation* g_presentation;                        // 0x5e299c


// ==== 0x490030: a random position around a point ===========================================================
// @ 0x490030
void GameW::random_position_around(Vec2 pos, float r, Vec2* out)
{
    char ok = 0;
    *out = pos;
    int iter = 0;
    FaceId* f = (FaceId*)find_at(&pos.x);
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
            FaceId* g = (FaceId*)find_at(&out->x);
            if (g != 0) {
                int gi = g->m4;
                ok = (char)g_script.level->ground->shares(first, gi);
            }
        }
    }
}

// @ 0x490160
char GameW::position_around(Vec2 pos, float r, Vec2* out, char any)
{
    Vec2 np;
    float dx, dy;
    *out = pos;
    char ok = 0;
    int iter = 0;
    while (iter < 50000) {
        iter++;
        dx = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        dy = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        if (sqrt(dx * dx + dy * dy) > r)
            continue;
        np = Vec2(pos.x + dx, pos.y + dy);
        *out = np;
        FaceId* f = (FaceId*)find_at(&out->x);
        if (f == 0)
            continue;
        int kind = g_script.level->ground->cells[f->m4].ids[8];
        int low = kind & 7;
        if (any)
            ok = low == 0 || kind == 7;
        else
            ok = (low >= 2 && low < 7) || kind == 7;
        if (ok)
            break;
    }
    return ok;
}

// ==== 0x4902a0: the stack "DNA" token =========================================================================
// @ 0x4902a0
int GameW::stack_dna(unsigned id, int group, int f14, int f18)
{
    int v = -1;
    if ((int)id >= 0) {
        PartyMgr* m = &pm();
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
// A page is a `new Party[16]` block (cookie word in front); the original frees it through the array deleting
// destructor of the party record (virtual slot 0 with flag 3), or plain operator delete[] when the cookie is 0.
// _Tidy is protected: reached through a derived class (the same device as SceneObjVec / int_stdvec)
struct PartyVecTidy : std::vector<Party*> { using std::vector<Party*>::_Tidy; };
struct IntVecTidy : std::vector<int> { using std::vector<int>::_Tidy; };

// @ 0x490340
void PartyMgr::clear()
{
    Party** i = pages._Myfirst;
    while (i < pages._Mylast) {
        Party* e = *i;
        if (e != 0) {
            int* p = (int*)((char*)e - 4);
            if (*p != 0)
                ((PartyVt*)e)->v00(3);
            else
                operator delete[](p);
        }
        *i = 0;
        i++;
    }
    ((PartyVecTidy&)pages)._Tidy();
    ((IntVecTidy&)free_ids)._Tidy();
}

// @ 0x4903e0
int PartyMgr::is_live(int id)
{
    if ((unsigned)id < n_parties) {
        int i = this->lookup(id);
        if (i >= 0)
            return pages[(unsigned)i >> 4][i & 0xf].type == 1;
    }
    return false;
}

// ==== 0x490430: the scene entity a script trigger runs for ======================================================
// @ 0x490430
int __fastcall trigger_entity(int c)
{
    int v = 0;
    if (c == -0x28 || c == -0x2b || c == -0x2a) {
        v = (int)g_mission->props[((GameW*)g_game)->cur_party_idx()].ent;
    } else if (c == -0x32 || c == -0x33) {
        unsigned idx = ((GameW*)g_game)->cur_agent_idx();
        unsigned page_idx = idx >> 4;
        unsigned rem = idx & 0xf;
        char* page = (char*)g_mission->agents.pages[page_idx];
        return *(int*)(page + rem * 0x5928 + 0x250);
    }
    return v;
}

// @ 0x4904a0
int trigger_item_mesh(int c)
{
    GameW* g = (GameW*)g_game;
    if (trigger_entity(c) == 0)
        return -1;
    if (c != -0x32 && c != -0x33)
        return -1;
    if (g->free_slot() < 0)
        return -1;
    unsigned idx = g->cur_agent_idx();
    int off = (idx & 0xf) * 0xb25 + g->free_slot();
    int id = *(int*)((char*)g_mission->agents.pages[idx >> 4] + off * 8 + 0x1d0);
    if (id < 0)
        return -1;
    return g_items[id].ammo_skill();
}

struct named_object;
void __stdcall clear_f14(named_object* o);                      // 0x4175f0 (the resource manager's clear_f14; named_object is rgl/resource.h's)

// ==== 0x490520: the container widget (children vector at +0x178) ================================================
// The class is ui/container.h's ContainerBase. Its destructor stores the vptr, releases the children (the virtual
// update_children, called non-virtually: 0x48ef40, mission_ui2/mission_4.cpp), frees the vector and runs MissionUi's
// destructor 0x487c00. The deleting destructor 0x490930 is the compiler's: the destructor inlined, then operator delete.
// @ 0x490520
ContainerBase::~ContainerBase()
{
    ContainerBase::update_children();
}

// @ 0x490570
void ContainerBase::move_a(const float* p)
{
    int* a = (int*)p;
    WD_FLD(int, 0xe8) = a[0];
    WD_FLD(int, 0xec) = a[1];
    if (p154 != 0) {
        p154->on_child_moved(this);
    } else {
        WD_FLD(int, 0xe0) = a[0];
        WD_FLD(int, 0xe4) = a[1];
    }
    on_moved();    // one shared tail call: an early return in the first arm moved the register allocation
}

// @ 0x4905c0
void ContainerBase::on_child_moved(MissionUi* p)
{
    char* e = (char*)p;
    float n[2];
    float* ep = (float*)(e + 0xe8);
    n[0] = *(float*)((char*)this + 0xe0) + ep[0];
    *(int*)(e + 0xe0) = *(int*)&n[0];
    n[1] = *(float*)((char*)this + 0xe4) + ep[1];
    *(int*)(e + 0xe4) = *(int*)&n[1];
    p->on_moved();
    if (f104 & 0x100) {
        float v;
        v = *(float*)(e + 0xf0) + *(float*)(e + 0xe8);
        if (v > *(float*)((char*)this + 0xf0))
            *(float*)((char*)this + 0xf0) = v;
        v = *(float*)(e + 0xf8) + *(float*)(e + 0xe8);
        if (v < *(float*)((char*)this + 0xf8))
            *(float*)((char*)this + 0xf8) = v;
        v = *(float*)(e + 0xf4) + *(float*)(e + 0xec);
        if (v > *(float*)((char*)this + 0xf4))
            *(float*)((char*)this + 0xf4) = v;
        v = *(float*)(e + 0xfc) + *(float*)(e + 0xec);
        if (v < *(float*)((char*)this + 0xfc))
            *(float*)((char*)this + 0xfc) = v;
    }
}

// @ 0x4906b0
void ContainerBase::on_moved()
{
    MissionUi** i = kids._Myfirst;
    while (i != kids._Mylast) {
        on_child_moved(*i);
        i++;
    }
}

// @ 0x4906e0
void ContainerBase::clear_c8()
{
    MissionUi** i = kids._Myfirst;
    while (i != kids._Mylast) {
        (*i)->clear_c8();
        i++;
    }
}

// @ 0x490710
void ContainerBase::render(rgl::scene* sc, float* p3, int c, int d)
{
// the two bounds pairs arrive as ints (the slot's signature is MissionUi::render's); naming them keeps the original's register use
#define p4 ((float*)c)
#define p5 ((float*)d)
    if (f104 & 1)
        return;
    struct __declspec(align(8)) Locals {
        float z, y0, hy, ly, mx, my;
    } l;
    MissionUi::render(sc, p3, c, d);   // 0x488720
    l.mx = p4[0];
    l.y0 = p3[0];
    l.my = p4[1];
    l.hy = p5[0];
    l.ly = p5[1];
    float v = *(float*)((char*)this + 0xf8) + *(float*)((char*)this + 0xe0);
    if (l.mx < v) l.mx = v;
    v = *(float*)((char*)this + 0xf0) + *(float*)((char*)this + 0xe0);
    if (l.hy > v) l.hy = v;
    v = *(float*)((char*)this + 0xfc) + *(float*)((char*)this + 0xe4);
    if (l.my < v) l.my = v;
    v = *(float*)((char*)this + 0xf4) + *(float*)((char*)this + 0xe4);
    if (l.ly > v) l.ly = v;
    MissionUi** i = kids._Myfirst;
    while (i != kids._Mylast) {
        char* e = (char*)*i;
        if (!(*p5 < *(float*)(e + 0xf8) + *(float*)(e + 0xe0)) &&
            !(p5[1] < *(float*)(e + 0xfc) + *(float*)(e + 0xe4)) &&
            !(*(float*)(e + 0xf0) + *(float*)(e + 0xe0) < p4[0]) &&
            !(*(float*)(e + 0xf4) + *(float*)(e + 0xe4) < p4[1])) {
            l.z = p3[0];
            ((MissionUi*)e)->render(sc, &l.z, (int)&l.mx, (int)&l.hy);
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
    MissionUi** i = kids._Myfirst;
    while (i != kids._Mylast) {
        *(int*)((char*)*i + 0x100) = 0;
        i++;
    }
    i = kids._Myfirst;
    while (i != kids._Mylast) {
        (*i)->animate(p);
        i++;
    }
}

// ==== 0x490990: mark the parties of one kind dirty ===============================================================
// @ 0x490990
void GameW::mark_kind(int kind)
{
    PartyMgr* m = &pm();
    unsigned i = m->first_party_id();
    while (i < parties.n_parties) {
        if (m->at(m->lookup(i))->faction == kind)
            m->at(m->lookup(i))->f8b5 = 1;
        m->next_party_id((int*)&i);
    }
}

// ==== 0x490a30: take a party out of its leader's follower list ====================================================
// @ 0x490a30
void GameW::detach_party(unsigned id)
{
    PartyMgr* m = &pm();
    if (id >= m->n_parties)
        return;
    int i = m->lookup(id);
    if (i < 0)
        return;
    if (m->atu((unsigned)i)->type != 1)
        return;
    if (m->at(m->lookup(id))->attached_to < 0)
        return;
    bool done = false;
    unsigned* dst = (unsigned*)m->at(m->lookup(m->at(m->lookup(id))->attached_to))->attached._Myfirst;
    unsigned* cur = dst + 1;
    do {
        if ((int*)dst >= m->at(m->lookup(m->at(m->lookup(id))->attached_to))->attached._Mylast)
            break;
        if (*dst == id) {
            std::vector<int>& l = m->at(m->lookup(m->at(m->lookup(id))->attached_to))->attached;
            memmove(dst, cur, (int)(l._Mylast - (int*)cur) * 4);
            --l._Mylast;
            done = true;
        }
        dst++;
        cur++;
    } while (!done);
    m->at(m->lookup(m->at(m->lookup(id))->attached_to))->dirty = 1;
    m->at(m->lookup(id))->f8cc = 0;
    m->at(m->lookup(id))->dirty = 1;
    m->at(m->lookup(id))->attached_to = -1;
}

// ==== 0x490ce0: total a party's stacks (and its followers') into counters ========================================
// @ 0x490ce0
void GameData::count_units(unsigned id, int* a, int* b, int* c, int* d)
{
    PartyMgr* m = &parties;
    if (id >= m->n_parties)
        return;
    int i = m->lookup(id);
    if (i < 0)
        return;
    if (m->atu((unsigned)i)->type != 1)
        return;
    if (m->at(m->lookup(id))->nstacks > 0) {
        int k = 0;
        int n = 0;
        do {
            if ((*(unsigned char*)((char*)m->at(m->lookup(id)) + 0xb0 + k) & 1) != 0) {
                *c += *(int*)((char*)m->at(m->lookup(id)) + 0xa8 + k);
            } else {
                *a += *(int*)((char*)m->at(m->lookup(id)) + 0xa8 + k);
                int troop = *(int*)((char*)m->at(m->lookup(id)) + 0xac + k);
                if ((troops[troop].flags30 & 0x10) == 0) {
                    *b = *b + *(int*)((char*)m->at(m->lookup(id)) + 0xb8 + k);
                } else {
                    *d = *d + 1;
                    if (agent_low(*(int*)((char*)m->at(m->lookup(id)) + 0xac + k))) {
                        *b = *b + 1;
                    }
                }
            }
            n++;
            k += 0x20;
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
    if (id >= m->n_parties)
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
void GameW::recruit(int id, int count, Party* dest)
{
    PartyMgr* m = &pm();
    if ((unsigned)id >= m->n_parties)
        return;
    int idx = m->lookup(id);
    if (idx < 0)
        return;
    if (m->atu(idx)->type != 1)
        return;

    int n = 0;
    int members[128];
    list_members(id, members, &n);
    int a = 0, b = 0, c = 0, d = 0;
    count_units(id, &a, &b, &c, &d);
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
            if (total < 2)
                pick = 0;
            else
                pick = rand() % total;
            int base = 0;
            bool found = false;
            int j = 0;
            do {
                if (j >= n)
                    break;
                int mi = m->lookup(members[j]);
                Party* mp = m->at(mi);
                int sum = mp->sum_stat(0x134);
                base += sum;
                if (pick < base) {
                    found = true;
                    Stack* st = &mp->stacks[mp->find_worth(pick - (base - sum), 0x134)];
                    int troop = st->troop;
                    Troop* tr = &troops[troop];
                    int hp = tr->level + 10;
                    if ((tr->flags30 & 0x10) != 0)
                        hp += 10;
                    if ((float)(rand() % 0x3dc9) * (float)hp * 6.322312e-05f < 2.0f) {
                        tr = &troops[troop];
                        if ((tr->flags30 & 0x10) == 0) {
                            if ((float)(rand() % 0x3dc9) * 6.322312e-05f < g_k065
                                || (troops[troop].flags30 & 0x80) != 0) {
                                st->count--;
                                if (dest != 0)
                                    dest->add_members(troop, 1, 0, 0, 0);
                            } else {
                                st->wounded++;
                                if (dest != 0)
                                    dest->add_members(troop, 1, 0, 1, 0);
                            }
                        } else {
                            if (troops[troop].hp > g_module_settings.player_wounded_treshold)
                                troops[troop].hp = g_module_settings.player_wounded_treshold;
                            troops[troop].hp -= 15.0f;
                            if (troops[troop].hp < 0.0f)
                                troops[troop].hp = 0.0f;
                            if (dest != 0)
                                dest->add_members(troop, 1, 0, 1, 0);
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
void GameW::handout(Party* p, int id)
{
    int n = 0;
    int members[128];
    int cum[128];
    int total = 0;
    list_members(id, members, &n);
    PartyMgr* m = &pm();
    for (int i = 0; i < n; i++) {
        total += m->at(m->lookup(members[i]))->sum_stat(0x100);
        cum[i] = total;
    }
    if (total == 0)
        return;
    for (int i = 0; i < p->nstacks; i++) {
        if ((troops[p->stacks[i].troop].flags32 & 0x10) != 0)
            continue;
        for (int j = 0; j < p->stacks[i].count; j++) {
            int pick;
            if (total < 2)
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
// @ 0x4915d0
void GameW::remove_party(int id)
{
    if ((unsigned)id >= parties.n_parties)
        return;
    PartyMgr* m = &pm();
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
    if (p->attached._Myfirst != 0 && p->attached._Mylast - p->attached._Myfirst != 0) {
        for (unsigned k = 0; p->attached._Myfirst != 0 && k < (unsigned)(p->attached._Mylast - p->attached._Myfirst); k++) {
            if ((unsigned)p->attached._Myfirst[k] >= parties.n_parties)
                continue;
            int j = m->lookup(p->attached._Myfirst[k]);
            if (j < 0)
                continue;
            if (m->atu(j)->type != 1)
                continue;
            m->at(m->lookup(p->attached._Myfirst[k]))->attached_to = -1;
            m->at(m->lookup(p->attached._Myfirst[k]))->f8cc = 0;
            m->at(m->lookup(p->attached._Myfirst[k]))->dirty = 1;
        }
        if (p->attached._Myfirst != 0)
            operator delete(p->attached._Myfirst);
        p->attached._Myfirst = 0;
        p->attached._Mylast = 0;
        p->attached._Myend = 0;
    }
    if (p->owner >= 0)
        g_party_templates[p->owner].num_destroyed += 1;
    p->clear_map_flag();

    idx = m->lookup(id);
    if (idx >= 0 && idx < parties.count)
        m->at(idx)->type = 2;

    unsigned cur = m->first_party_id();
    if (cur < parties.n_parties) {
        do {
            Party* q = m->at(m->lookup(cur));
            if (q->target == id) {
                q->action = 0;
                q->target = -1;
            }
            if (q->state_target == id) {
                q->f8e4 = 0;
                q->state_target = -1;
            }
            m->next_party_id((int*)&cur);
        } while (cur < parties.n_parties);
    }
}

// ==== 0x4918d0: can a party be merged into another ================================================================
// @ 0x4918d0
bool GameW::can_transfer(unsigned id, int other, char flag)
{
    PartyMgr* mgr = &pm();
    if (id >= parties.n_parties) return false;
    int idx = mgr->lookup(id);
    if (idx < 0) return false;
    if (mgr->atu((unsigned)idx)->type != 1) return false;
    if (!mgr->is_live(other)) return false;
    int ia = mgr->lookup(id);
    Party* a = mgr->at(ia);
    int ib = mgr->lookup(other);
    Party* b = mgr->at(ib);
    if (b->flags_lo & 0x100000) {
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
    Troop* chars = troops;
    for (int k = 0; k < na; k++) {
        Stack* s = &a->stacks[k];
        bool matched = false;
        if (!(chars[s->troop].flags32 & 0x10)) {
            for (int j = 0; j < b->nstacks; j++) {
                if (b->stacks[j].troop == s->troop) {
                    if (!flag) {
                        if (((s->flags ^ b->stacks[j].flags) & 1) == 0) matched = true;
                    } else {
                        if (((s->flags ^ b->stacks[j].flags) & 1) != 0) matched = true;
                    }
                }
            }
            if (matched) continue;
        }
        if (!flag) { if (s->flags & 1) c2++; else c1++; }
        else       { if (s->flags & 1) c1++; else c2++; }
    }
    if (c1 > d1) return false;
    if (c2 > d2) return false;
    return true;
}

// ==== 0x491b20: rating of a party against an opponent ===============================================================
// @ 0x491b20
int GameW::gate_rating(unsigned p1, unsigned p2, int p3) {
    PartyMgr* mgr = &pm();
    int a = 0, b = 0, c = 0, d = 0;
    if (p2 < parties.n_parties) {
        int i = mgr->lookup(p2);
        if (i >= 0 && mgr->atu((unsigned)i)->type == 1) {
            count_units(p2, &a, &b, &c, &d);
        }
    }
    float f1 = (float)(a - b);
    float f2 = 0.0f;
    if (p1 < parties.n_parties) {
        int i = mgr->lookup(p1);
        if (i >= 0 && mgr->atu((unsigned)i)->type == 1) {
            int j = mgr->lookup(p1);
            f2 = (float)mgr->at(j)->sum_stat(0x110);
        }
    }
    float f3 = 0.0f;
    if (p3 >= 0 && mgr->is_live(p3)) {
        a = b = c = d = 0;
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
    if (p1 < parties.n_parties) {
        int i = mgr->lookup(p1);
        if (i >= 0 && mgr->atu((unsigned)i)->type == 1) {
            int j = mgr->lookup(p1);
            r += mgr->atu((unsigned)j)->skill_level(0xf) / 2;
        }
    }
    if (p2 < parties.n_parties) {
        int i = mgr->lookup(p2);
        if (i >= 0 && mgr->atu((unsigned)i)->type == 1) {
            int j = mgr->lookup(p2);  // the original looks the party up again here (2nd call 0x48ad00)
            r -= mgr->atu((unsigned)j)->skill_level(0xf) / 2;
        }
    }
    if (r < -0x14) return -0x14;
    if (r > 0x14) return 0x14;
    return r;
}

// ==== 0x491e50: put a party near a point =============================================================================
// @ 0x491e50
void GameW::set_pos(float x, float y, int id, float radius) {
    PartyMgr* m = &pm();
    random_position_around(Vec2(x, y), radius, (Vec2*)&x);
    if ((unsigned)id >= parties.n_parties) return;
    int i = m->lookup(id);
    if (i < 0) return;
    if (m->atu((unsigned)i)->type != 1) return;
    i = m->lookup(id);
    Party* p = m->at(i);
    p->pos.x = x;
    p->pos.y = y;
    i = m->lookup(id);
    m->at(i)->place_on_map();
}

// ==== 0x491f20 ======================================================================================================
// @ 0x491f20
int GameW::token_lookup(int a, int b, int c, int d) {
    if (a == f78) return tbl[b][c];
    return stack_dna(a, b, c, d);
}

// ==== 0x491f50: mission helpers (agent pool walks, ratings) =============================================================
// @ 0x491f50
int MissionW::count_agents(int idx)
{
    int total = 0;
    if (teams[idx].relation[0] > 0) total = counts[0][1];
    if (teams[idx].relation[1] > 0) total += counts[1][1];
    if (teams[idx].relation[2] > 0) total += counts[2][1];
    if (teams[idx].relation[3] > 0) total += counts[3][1];
    if (teams[idx].relation[4] > 0) total += counts[4][1];
    if (teams[idx].relation[5] > 0) total += counts[5][1];
    if (teams[idx].relation[6] > 0) total += counts[6][1];
    if (teams[idx].relation[7] > 0) total += counts[7][1];
    if (total == 0) return 0;
    int cu = f5c;
    if (cu < 0) return 0;
    AgentW* rr = rec((unsigned)cu);
    if (rr->team < 0) return 0;
    int n = teams[idx].relation[rr->team];
    int s = f6520();
    if (n < 0) s = -s;
    float v = total * (1.0f / speed_factor((float)s));
    double fl = floor(v);
    int res = (int)fl;
    if (v - fl > 0.5) res++;
    if (res < 1) res = 1;
    return res;
}

// @ 0x4920e0
bool MissionW::all_defeated(int a)
{
    if (f5c >= 0) {
        int team = rec((unsigned)f5c)->team;
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
bool MissionW::hero_down()
{
    unsigned cu = (unsigned)f5c;
    if (cu == 0xffffffff) return false;
    return rec(cu)->f14 != 1;
}

// @ 0x4921b0
unsigned MissionW::find_agent_by_troop(int id)
{
    unsigned found = 0xffffffff;
    AgentTable* pool = &agents;
    unsigned i = pool->first_used();
    unsigned n = (unsigned)pool->count;
    if (i < n) {
        std::vector<Agent*>::iterator pages = agents.pages.begin();
        do {
            if (((AgentW*)((char*)pages[i >> 4] + (i & 0xf) * 0x5928))->troop == id) found = i;
            pool->next_used(&i);
        } while (i < n);
    }
    return found;
}

// ==== 0x492230: weapon orders of a team =========================================================================
// @ 0x492230
void MissionW::set_weapon_order_1(int team, unsigned value)
{
    char* self = (char*)this;
    int base = team * 0x16c;
    if (*(int*)(self + base + 0x52c8) != 0)
        *(unsigned*)(self + base + 0x528c) = value;
    if (*(int*)(self + base + 0x5334) != 0)
        *(unsigned*)(self + base + 0x52f8) = value;
    if (*(int*)(self + base + 0x53a0) != 0)
        *(unsigned*)(self + base + 0x5364) = value;

    int n = *(int*)(self + 4);
    unsigned i = 0;
    if (n > 0) {
        do {
            if (*(int*)((char*)pages_raw()[i >> 4] + (i & 0xf) * 0x5928 + 4) != 0)
                break;
            i++;
        } while ((int)i < n);
    }
    AgentTable* pool = &agents;
    unsigned j = i;
    for (; j < (unsigned)pool->count; pool->next_used(&j)) {
        int off = (j & 0xf) * 0x5928;
        Agent** pg = pages_raw();                               // read back each iteration (a call follows)
        AgentW* r = (AgentW*)((char*)pg[j >> 4] + off);
        if (r->f14 == 1) {
            int k = r->team;
            if (k < 0 || k >= 8)
                k = 7;
            if (k == team) {
                int c = ((AgentX*)(void*)r)->agent_class();
                if (*(int*)(self + base + c * 0x6c + 0x52c8) != 0) {
                    int w = r->f56d4();
                    r->f56d4() = 0;
                    ((Agent*)(void*)r)->set_weapon_mode(w);
                }
            }
        }
    }
}

// @ 0x492350
void MissionW::set_weapon_order_2(int team, unsigned value)
{
    char* self = (char*)this;
    int base = team * 0x16c;
    if (*(int*)(self + base + 0x52c8) != 0)
        *(unsigned*)(self + base + 0x5290) = value;
    if (*(int*)(self + base + 0x5334) != 0)
        *(unsigned*)(self + base + 0x52fc) = value;
    if (*(int*)(self + base + 0x53a0) != 0)
        *(unsigned*)(self + base + 0x5368) = value;

    int n = *(int*)(self + 4);
    unsigned i = 0;
    if (n > 0) {
        do {
            if (*(int*)((char*)pages_raw()[i >> 4] + (i & 0xf) * 0x5928 + 4) != 0)
                break;
            i++;
        } while ((int)i < n);
    }
    AgentTable* pool = &agents;
    unsigned j = i;
    for (; j < (unsigned)pool->count; pool->next_used(&j)) {
        int off = (j & 0xf) * 0x5928;
        Agent** pg = pages_raw();                               // read back each iteration (a call follows)
        AgentW* r = (AgentW*)((char*)pg[j >> 4] + off);
        if (r->f14 == 1) {
            int k = r->team;
            if (k < 0 || k >= 8)
                k = 7;
            if (k == team) {
                int c = ((AgentX*)(void*)r)->agent_class();
                if (*(int*)(self + base + c * 0x6c + 0x52c8) != 0) {
                    int w = r->f56d4();
                    r->f56d4() = 0;
                    ((Agent*)(void*)r)->set_weapon_mode(w);
                }
            }
        }
    }
}

// ==== 0x492470: the battle object's rating and side selection ====================================================
// @ 0x492470
void MissionCtx::refresh_rating()
{
    PartyMgr* mgr = &((GameW*)g_game)->pm();
    int idx = mgr->lookup(ids[side]);
    int t = mgr->pages[idx >> 4][idx & 0xf].sum_stat(0x10);
    if (t <= 3)
        tier = 0;
    else if (t <= 0xa)
        tier = 1;
    else
        tier = (t > 0x32) + 2;

    int s = side;
    int r = ((GameW*)g_game)->gate_rating(((GameW*)g_game)->f78, ids[s], ids[1 - s]);
    f1c = r;
    if (r > 6)
        f1c = 6;
    if (f1c < -0x20)
        f1c = -0x20;
}

// ==== 0x492530: move all stacks of one party into another =========================================================
// @ 0x492530
void GameW::transfer_stack(int a, int b, char flag)
{
    WL_MARKER(12)
    PartyMgr* mgr = &pm();
    Party* pa = mgr->at(mgr->lookup(a));
    Party* pb = mgr->at(mgr->lookup(b));
    int* wstacks = &tbl[0][0];
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
        if (b == f78 && r >= 0) {
            int shift = s->count;
            if (shift < 0x20) {
                int* d = wstacks + r * 0x20 + shift;
                int* p = wstacks + r * 0x20;
                for (int j = 0x20 - shift; j; j--)
                    *d++ = *p++;
            } else {
                shift = 0x20;
            }
            int* out = wstacks + r * 0x20;
            for (int j = 0; j < shift; j++) {
                int v;
                if (a == f78) {
                    v = wstacks[k * 0x20 + j];
                } else {
                    v = -1;
                    if (a >= 0) {
                        Party* px = mgr->at(mgr->lookup(a));
                        v = px->stacks[k].troop;
                    }
                    v = v * 0x1bf + 0x45db + a * 0x7bb7 + j;
                }
                out[j] = v;
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
        f24 = s;
    if (ids[1 - s] >= 0) {
        int flip = 1 - s;
        side = flip;
        f10 &= ~0x10;
        f144 = 1;
        if (m == 4 && flip == 1)
            f10 |= m;
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
    TroopFile* s = *(TroopFile**)&troops;
    scene_open = 0;
    if (s != 0) {
        delete[] s;
        troops = 0;
    }

    RecPoly* p = *(RecPoly**)&levels;
    if (p != 0) {
        delete[] p;
        levels = 0;
    }

    FactionFile* t = *(FactionFile**)&factions;
    if (t != 0) {
        delete[] t;
        factions = 0;
    }

    QuestFile* q = *(QuestFile**)&quests;
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
        ((rgl::world*)_Ptr)->~world();                // 0x475d50
        ::operator delete(_Ptr);
    }
    worlds.pop_back();
}

// ==== 0x492f40: the game object's destructor =======================================================================
struct CursorObj {
    void reset();   // 0x492a20 (GameW::free_arrays)
};
#include "../ui/timer_list.h"                          // TimerList: ~TimerList 0x4317b0
struct MapDialog {
    void clear_objects();   // 0x4c6e30 (rgl/mesh2/gui_edit.h)
};
// @ 0x492f40
void GameData::world_free()
{
    char* self = (char*)this;
    if (scene_open != 0)
        free_arrays();

    {
        tstring* p = (tstring*)(self + 0x1a290);
        for (int n = 5; n != 0; --n) {
            --p;
            p->~tstring();
        }
    }
    ((tstring*)(self + 0x1a210))->~tstring();

    if (*(void**)(self + 0x1a1bc) != 0)
        ::operator delete(*(void**)(self + 0x1a1bc));
    *(void**)(self + 0x1a1bc) = 0;
    *(void**)(self + 0x1a1c0) = 0;
    *(void**)(self + 0x1a1c4) = 0;

    ((tstring*)(self + 0x1a080))->~tstring();
    ((tstring*)(self + 0x1a06c))->~tstring();

    {
        tstring* p = (tstring*)(self + 0x13eec);
        for (int n = 0x32; n != 0; --n) {
            --p;
            p->~tstring();
        }
    }

    ((tstring*)(self + 0x13b50))->~tstring();
    ((tstring*)(self + 0x13ab8))->~tstring();
    ((tstring*)(self + 0x13aa8))->~tstring();
    ((tstring*)(self + 0x13a70))->~tstring();

    if (*(void**)(self + 0x13a64) != 0)
        ::operator delete(*(void**)(self + 0x13a64));
    *(void**)(self + 0x13a64) = 0;
    *(void**)(self + 0x13a68) = 0;
    *(void**)(self + 0x13a6c) = 0;

    ((TimerList*)(self + 0x139f4))->~TimerList();                    // 0x4317b0

    if (*(void**)(self + 0x139ec) != 0)
        ((RecD*)*(void**)(self + 0x139ec))->scalar_delete(3);

    parties.clear();
    ((MapDialog*)(self + 0xcc))->clear_objects();
    ((tstring*)(self + 0xb8))->~tstring();

    if (*(void**)(self + 0x38) != 0)
        ::operator delete(*(void**)(self + 0x38));
    *(void**)(self + 0x38) = 0;
    *(void**)(self + 0x3c) = 0;
    *(void**)(self + 0x40) = 0;

    ((tstring*)(self + 0x1c))->~tstring();
    ((tstring*)(self + 8))->~tstring();
}

// ==== 0x493090: std::vector<__int64>::resize (out-of-line instance; include/i64_vec.h I64Vec::resize) ==============
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
    *(V2*)&f1b0 = V2(0.2265f, 0.029f);
    *(V2*)&f1bc = V2(0.018f, 0.018f);
    f1b8 = 0.0135f;
    *(V2*)&f1c4 = V2(0.02f, 0.01f);
}

// @ 0x4932a0
void DropDown::render(rgl::scene* sc, float* count, int c, int d)
{
    if (f180 == 0)
        f178 = 1;
    else
        f178 = 0;
    b11c[3] = 0;
    if (f178 != 0) {
        b11c[2] = 1;
        b11c[1] = 0;
    } else {
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

    float tmp[2];
    set_text(tmp, &rows[v15c]);
}

// @ 0x4933b0
void DropDown::reset_tail()
{
    rows._Tidy();                                               // 0x42d390
    f164 = 0;
    v15c = 0;
}


// UTF-8 -> code points (0x487b00). The original uses cl's static register convention, so the arguments are ordered
// to put the tstring in EDI and the count out-pointer in EDX.
static __declspec(noinline) void decode_utf8_487b00(int* _Out, const tstring* _s, int* _Buf, const int* _Maxn)
{
    *_Out = 0;
    int _n = _s->len;
    int _i = 0;
    if (_n > 0) {
        do {
            if (*_Out >= *_Maxn)
                break;
            unsigned char _c = _s->buf[_i];
            int _ch;
            if (_c >= 0xf0 && _c < 0xf5) {
                const unsigned char* _q = (const unsigned char*)_s->buf + _i;
                _ch = (((((_q[0] & 7) << 6) | (_q[1] & 0x3f)) << 6 | (_q[2] & 0x3f)) << 6) | (_q[3] & 0x3f);
                _i += 4;
            } else if (_c >= 0xe0 && _c < 0xf0) {
                const unsigned char* _q = (const unsigned char*)_s->buf + _i;
                _ch = ((((_q[0] & 0xf) << 6) | (_q[1] & 0x3f)) << 6) | (_q[2] & 0x3f);
                _i += 3;
            } else if (_c >= 0xc2 && _c < 0xe0) {
                const unsigned char* _q = (const unsigned char*)_s->buf + _i;
                _ch = ((_q[0] & 0x1f) << 6) | (_q[1] & 0x3f);
                _i += 2;
            } else {
                _i++;
                if (_c >= 0x80)
                    continue;
                _ch = (signed char)_c;
            }
            _Buf[*_Out] = _ch;
            (*_Out)++;
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
    decode_utf8_487b00(&_count, _name, _buf, &_maxn);
    float _w = (float)_count * 0.0077999993f;
    if (_w > 0.21f)
        _scale = 0.21f / _w * 0.02f;
    if (_scale < f140) {
        float _pair[2];
        _pair[0] = _scale;
        _pair[1] = _scale;
        set_size(_pair);
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

// @ 0x4934d0 (the constructor: the members' own constructors run first, then this body)
Party::Party() throw()
{
    for (int i = 0; i < 0x40; ++i) {
        stacks[i].count = 0;
        stacks[i].troop = -1;
        stacks[i].flags = 0;
        stacks[i].spawned = 0;
        stacks[i].wounded = 0;
        stacks[i].f14 = 0;
        stacks[i].f18 = 0;
    }

    clk8d0.clock = 1;
    clk8d0.start = g_ticks[1];
    clk8f8.clock = 1;
    clk8f8.start = g_ticks[1];
    clk908.clock = 0;
    clk908.start = g_ticks[0];
    clk938.clock = 1;
    clk938.start = g_ticks[1];
    clk948.clock = 1;
    clk948.start = g_ticks[1];
    clk59b0.clock = 1;
    clk59b0.start = g_ticks[1];

    note_tableau = -1;
    ignore_timer.clock = 1;
    ignore_timer.start = g_ticks[1];

    init();

    clk59b0.start = g_ticks[clk59b0.clock] - 100000;
    f5a04 = 0;
}

// @ 0x4936b0
template void std::vector<int>::push_back(const int&);

// @ 0x493700
unsigned RecPool::alloc_record()
{
    if (free_ids.size() <= 0) {
        Rec98* blk = (Rec98*)new Event[16];
        pages.push_back(blk);
        for (int i = 0; i < 16; ++i) {
            blk[i].f4 = 0;
            blk[i].f8 = next_id + i;
            free_ids.push_back(next_id + i);
        }
        next_id += 16;
    }
    unsigned idx = free_ids.back();
    free_ids.pop_back();
    Rec98* r = pages[idx >> 4] + (idx & 0xf);
    r->f4 = 1;
    r->fc = serial;
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
    p->id = n_parties;

    int h = n_parties;
    int slot = h % 0x2717;
    while (tab[slot].key != (unsigned)-1)
        slot = (slot + 0x4f) % 0x2717;
    tab[slot].key = h;
    tab[slot].value = idx;
    ++n_parties;
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
SceneBaseCtor* SceneBaseCtor::construct()
{
    char* self = (char*)this;

    {
        void* _n0 = ((ListInt*)(self + 0x2c))->_Buynode();
        *(void**)(self + 0x30) = _n0;
        *(int*)(self + 0x34) = 0;
        *(int*)(self + 0x38) = 0x400;
        *(int*)(self + 0x3c) = 0;
    }
    {
        void* _n1 = ((ListInt*)(self + 0x40))->_Buynode();
        *(void**)(self + 0x44) = _n1;
        *(int*)(self + 0x48) = 0;
        *(int*)(self + 0x4c) = 0x400;
        *(int*)(self + 0x50) = 0;
    }

    *(int*)(self + 0x58) = 0;
    *(int*)(self + 0x5c) = 0;
    *(int*)(self + 0x60) = 0;
    *(int*)(self + 0x80) = 0;
    *(int*)(self + 0x84) = 0;
    *(int*)(self + 0x88) = 0;
    *(int*)(self + 0x90) = 0;
    *(int*)(self + 0x94) = 0;
    *(int*)(self + 0x98) = 0;
    *(int*)(self + 0xa4) = 0;
    *(int*)(self + 0xa8) = 0;
    *(int*)(self + 0xac) = 0;
    *(int*)(self + 0xbc) = 0;
    *(int*)(self + 0xc0) = 0;
    *(int*)(self + 0xc4) = 0;
    *(int*)(self + 0xcc) = 0;
    *(int*)(self + 0xd0) = 0;
    *(int*)(self + 0xd4) = 0;

    ((ViewParams*)(self + 0xd8))->ViewParams::ViewParams();   // 0x46b940

    *(int*)(self + 0x330) = 0;
    *(int*)(self + 0x334) = 0;
    *(int*)(self + 0x338) = 0;
    *(int*)(self + 0x340) = 0;
    *(int*)(self + 0x9c) = 1;

    float _g[4];
    {
        float _x[2] = { 1000.0f, 1000.0f };
        float _y[2] = { -1000.0f, -1000.0f };
        ((rgl::world*)(void*)this)->init(_y, _x, 0, 0);
    }

    *(float*)(self + 0x30c) = 1.0f;
    *(float*)(self + 0x310) = 0.0f;
    *(float*)(self + 0x314) = 0.0f;
    *(float*)(self + 0x318) = _g[3];
    *(float*)(self + 0x31c) = 0.0f;
    *(float*)(self + 0x320) = 0.0f;
    *(float*)(self + 0x324) = 0.0f;
    *(float*)(self + 0x328) = _g[3];

    *(int*)(self + 0xb0) = 0;
    *(int*)(self + 0xb4) = 0;
    *(int*)(self + 0x64) = 0;
    *(float*)(self + 0x68) = 0.0f;
    *(float*)(self + 0x6c) = 0.0f;
    *(float*)(self + 0x70) = 0.0f;
    *(float*)(self + 0x74) = _g[3];
    *(int*)(self + 0x78) = 0;

    return this;
}

// ==== 0x493c40: the slot-array reserve ===========================================================================
// @ 0x493c40
void I64VecW::reserve(int* _pNewsize)
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
        ((std::vector<__int64>*)(void*)this)->resize((unsigned)_n, _z);   // the instance above (0x493090)
        for (int i = _old; i < _n; ++i) {
            E8* _p = &first()[i];
            _p->a = 0;
            _p->b = 0;
        }
    }
}

// @ 0x493d70
void SlotArray::set(const int& slot, const __int64& value)
{
    int _n = slot + 1;
    ((I64VecW*)(void*)this)->reserve(&_n);
    (*(std::vector<__int64>*)this)[slot] = value;
}

// @ 0x493db0
void ScriptState::set_var(int _idx, int _val)
{
    I64VecW* _p = (I64VecW*)&vars;
    int _n = _idx + 1;
    _p->reserve(&_n);
    (*(std::vector<__int64>*)_p)[_idx] = _val;
}

// @ 0x493df0
void* E68W::find_or_create(int* _key)
{
    ResSlot* _found = 0;
    for (unsigned i = 0; i < slots().size(); ++i) {
        if (slots()[i]->key == *_key) {
            _found = slots()[i];
            break;
        }
    }
    if (_found != 0)
        return _found;

    ResSlot* _new = (ResSlot*)((MissionView*)(void*)this)->make(_key);
    slots().push_back(_new);
    return _new;
}

// ==== 0x493e90: resource cache groups ==================================================================================
// @ 0x493e90
void E68W::purge()
{
    int _i = 0;
    int _last = (int)slots().size() - 1;
    if (_last >= 0) {
        do {
            if (slots()[_i]->refs <= 0) {
                slots()[_i]->f5c = -1;
                slots()[_i]->key = -1;
                ((void (__fastcall*)(void*, StackWord))clear_f14)(&g_resmgr, *(StackWord*)&slots()[_i]);   // 0x4175f0 (the resource manager in ecx)
                slots()[_i] = slots()[_last];
                --_last;
            } else
                ++_i;
        } while (_i <= _last);
    }
    slots().resize(_last + 1, 0);
}

// @ 0x493f30
void TableauTbl::purge_all()
{
    for (unsigned _i = 0; _i < (unsigned)recs.size(); ++_i)
        ((E68W*)&recs[_i])->purge();
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
int WidgetMgrW::create_button_overlay(const tstring* text, unsigned flags)
{
    UiA* w = new UiA;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        vt(w)->d22(&v);
    }
    {
        unsigned f = (flags & 0x181c) | 0x200;
        vt(w)->d29(&f);
    }
    {
        unsigned c = 0xff55ff50;
        vt(w)->d39(&c);
    }
    {
        vec2 v;
        v.x = 1.0f;
        v.y = 0.75f;
        vt(w)->d25(&v);
        vt(w)->d41(&v, (void*)text);
    }
    scene->d50(w);
    widgets.push_back(w);
    return (int)widgets.size() - 1;
}

// ==== 0x494260: open a screen ==========================================================================================
// @ 0x494260
void WindowMgrW::open(int _idx)
{
    g_gui.set_overlay(0, 1);
    dq.push_back(_idx);
    arr[_idx]->s2();
    bool v = arr[dq.back()]->s8();
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
    play(_a, (int)_f, (const float*)&frame.o, 1.0f, 0);
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
    *(rgl::mat3*)&_m = _rot;
    _m.o = _pos;
    if (g_cfg.disable_sound == 0) {
        frame = _m;
        FSOUND_3D_Listener_SetAttributes(&frame.o.x, 0, *(float*)((char*)this + 0x20), *(float*)((char*)this + 0x28), *(float*)((char*)this + 0x24),
                               *(float*)((char*)this + 0x30), *(float*)((char*)this + 0x38), *(float*)((char*)this + 0x34));
    }
}

// ==== 0x494410..0x494680: quest lines on the message log ================================================================
extern "C" const char s_ui_quest_completed_value[];             // 0x592650
extern "C" const char s_ui_quest_succeeded_value[];             // 0x59266c
extern "C" const char s_ui_quest_failed_value[];                // 0x592688

static inline bool qbit(unsigned v, int n) { return ((v >> n) & 1) != 0; }

// @ 0x494410
void GameW::quest_done(int _idx)
{
    WL_MARKER(12)
    if (qbit(*(unsigned*)((char*)&quests[_idx] + 0x50), 1) ||
        ((*(unsigned*)((char*)&quests[_idx] + 0x64) & 0x200) == 0 && (*(unsigned*)((char*)&quests[_idx] + 0x64) & 0x100) != 0)) {
        char* _e = (char*)&quests[_idx];
        if ((*(unsigned*)(_e + 0x64) & 0x800) == 0) {
            tstring _val(*(const tstring*)(_e + 0x40));
            char _buf[0x200];
            {
                tstring _fmt("ui_quest_completed_value");
                char* _f = _val.buf; if (_f == 0) _f = (char*)&_val.buf;
                char* _r = (char*)g_strtable.lookup(_fmt) + 4;
                if (*(char**)_r != 0)
                    _r = *(char**)_r;
                mb_snprintf(_buf, 0x200, _r, _f);
            }
            {
                tstring _txt(_buf);
                g_gui.push(_txt, 0xffd0f0c8);
            }
            g_sound_mgr.play(g_sound_mgr.slot[3], 1, (const float*)&g_sound_mgr.frame.o, 1.0f, 0);
        }
        if (qbit(*(unsigned*)(_e + 0x50), 1))
            *(int*)(_e + 0x64) = 0;
        else
            *(unsigned*)(_e + 0x64) |= 0x200;
        WL_MARKER(13)
    }
}

// @ 0x494560
void GameW::quest_succeeded(int _idx)
{
    WL_MARKER(12)
    char* _e = (char*)((char*)quests + _idx * 0x138);
    *(unsigned*)(_e + 0x64) |= 0x1400;
    tstring _val(*(const tstring*)((char*)quests + _idx * 0x138 + 0x40));
    char _buf[0x200];
    {
        tstring _fmt("ui_quest_succeeded_value");
        char* _f = _val.buf ? _val.buf : (char*)&_val.buf;
        char* _r = (char*)g_strtable.lookup(_fmt) + 4;
        if (*(char**)_r != 0)
            _r = *(char**)_r;
        mb_snprintf(_buf, 0x200, _r, _f);
    }
    {
        tstring _txt(_buf);
        g_gui.push(_txt, 0xff90ff28);
    }
    g_sound_mgr.play(g_sound_mgr.slot[4], 1, (const float*)&g_sound_mgr.frame.o, 1.0f, 0);
    WL_MARKER(13)
}

// @ 0x494680
void GameW::quest_failed(int _idx)
{
    WL_MARKER(12)
    char* _e = (char*)((char*)quests + _idx * 0x138);
    *(unsigned*)(_e + 0x64) |= 0x1800;
    tstring _val(*(const tstring*)((char*)quests + _idx * 0x138 + 0x40));
    char _buf[0x200];
    {
        tstring _fmt("ui_quest_failed_value");
        char* _f = _val.buf ? _val.buf : (char*)&_val.buf;
        char* _r = (char*)g_strtable.lookup(_fmt) + 4;
        if (*(char**)_r != 0)
            _r = *(char**)_r;
        mb_snprintf(_buf, 0x200, _r, _f);
    }
    {
        tstring _txt(_buf);
        g_gui.push(_txt, 0xffd02028);
    }
    g_sound_mgr.play(g_sound_mgr.slot[6], 1, (const float*)&g_sound_mgr.frame.o, 1.0f, 0);
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
void GameW::set_quest_concluded(int _idx)
{
    WL_MARKER(12)
    char* _e = (char*)((char*)quests + _idx * 0x138);
    *(unsigned*)(_e + 0x64) |= 0x1000;
    tstring _val(*(const tstring*)((char*)quests + _idx * 0x138 + 0x40));
    char _buf[0x200];
    {
        tstring _fmt("ui_quest_concluded_value");
        char* _f = _val.buf ? _val.buf : (char*)&_val.buf;
        char* _r = (char*)g_strtable.lookup(_fmt) + 4;
        if (*(char**)_r != 0)
            _r = *(char**)_r;
        mb_snprintf(_buf, 0x200, _r, _f);
    }
    {
        tstring _txt(_buf);
        g_gui.push(_txt, 0xffd02028);
    }
    g_sound_mgr.play(g_sound_mgr.slot[5], 1, (const float*)&g_sound_mgr.frame.o, 1.0f, 0);
    WL_MARKER(13)
}

// @ 0x4948c0
void GameW::set_quest_cancelled(int _idx)
{
    WL_MARKER(12)
    char* _e = (char*)((char*)quests + _idx * 0x138);
    *(unsigned*)(_e + 0x64) = 0;
    tstring _val(*(const tstring*)((char*)quests + _idx * 0x138 + 0x40));
    char _buf[0x200];
    {
        tstring _fmt("ui_quest_cancelled_value");
        char* _f = _val.buf ? _val.buf : (char*)&_val.buf;
        char* _r = (char*)g_strtable.lookup(_fmt) + 4;
        if (*(char**)_r != 0)
            _r = *(char**)_r;
        mb_snprintf(_buf, 0x200, _r, _f);
    }
    {
        tstring _txt(_buf);
        g_gui.push(_txt, 0xffd02028);
    }
    g_sound_mgr.play(g_sound_mgr.slot[7], 1, (const float*)&g_sound_mgr.frame.o, 1.0f, 0);
    WL_MARKER(13)
}


// ==== 0x4949d0, 0x494ad0: party objectives =================================================================================
// @ 0x4949d0
void GameW::set_objective(int party, int value)
{
    PartyMgr* m = &pm();
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
void GameW::switch_objective(int party, int mode)
{
    if (party == encountered_a())
        return;
    if (party == encountered_b())
        return;
    if (party == f78)
        return;
    if (!objectives_on())
        return;
    if (((Party*)parties.get(party))->attached_to >= 0)
        detach_party(party);
    if (mode == 0)
        set_objective(party, f78);
    else if (mode == 1)
        set_objective(party, encountered_a());
    else if (mode == 2)
        set_objective(party, encountered_b());
    ((Party*)parties.get(party))->f8cc = 1;
}

// ==== 0x494b60: share an experience amount over a party ==================================================================
// @ 0x494b60
void GameW::share_experience(int party, float amount)
{
    if (party >= parties.n_parties)
        return;
    PartyMgr* pm_ = &pm();
    int idx = pm_->lookup(party);
    if (idx < 0)
        return;
    if (pm_->pages[idx >> 4][idx & 15].type != 1)
        return;
    Party* p = pm_->at(pm_->lookup(party));
    int total = 0;
    int i = 0;
    for (; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if (s.flags & 1)
            break;
        int troop = s.troop;
        char hero = (troops[troop].flags32 & 0x10) != 0;
        if (hero)
            total = (int)((float)total + (i == 0 ? 10.0f : 3.0f));
        else
            total += s.count;
    }
    if (total <= 0)
        return;                                 // 0x494c69: the whole function ends here, no message either
    {
        float per = amount / (float)total;
        for (int j = 0; j < p->nstacks; j++) {
            int idx2 = j;
            Stack& s = p->stacks[idx2];
            if (s.flags & 1)
                break;
            int troop = s.troop;
            char hero = (troops[troop].flags32 & 0x10) != 0;
            if (hero)
                GameData::add_experience(troop, (int)(per * (idx2 == 0 ? 10.0f : 3.0f)));
            else
                p->add_xp_to_stack(idx2, (int)((float)s.count * per));
        }
    }
    double fl = floor(amount);                  // the CRT floor 0x536b10; the result stays a double (0x494d39..0x494d4a)
    int n = (int)fl;
    if (!(amount - fl <= 0.5f))
        n = (int)fl + 1;
    if (n != 0 && party == f78) {
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
void GameW::award_experience(int party, float amount, int mode)
{
    if ((unsigned)party >= parties.n_parties)
        return;
    PartyMgr& m2 = pm();
    int idx = m2.lookup(party);
    if (idx < 0)
        return;
    if (m2.atu((unsigned)idx)->type != 1)
        return;
    idx = m2.lookup(party);                      // the original looks the party up a second time
    Party* p = &((Party*)(m2.pages[idx >> 4]))[idx & 15];
    int m = mode != 0 ? mode - 1 : rand() % 2;

    int total = 0;
    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if (s.flags & 1)
            break;
        if (troops[s.troop].flags30 & 0x10)
            total = (int)((float)total + (i == 0 ? 10.0f : 3.0f));
        else
            total += s.count;
    }
    float per = amount / (float)total;

    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if (s.flags & 1)
            break;
        if (troops[s.troop].flags30 & 0x10) {
            int r = rand() % 15817;
            // no float temporary: the original keeps the product on the x87 stack (0x494fd1 .. 0x494fdb) and truncates it from there
            if (i == 0)
                GameData::add_experience(s.troop, (int)((((float)r * (1.0f / 31634.0f) + 0.75f) * per) * 10.0f));
            else
                GameData::add_experience(s.troop, (int)((((float)r * (1.0f / 31634.0f) + 0.75f) * per) * 3.0f));
        } else {
            int r = rand() % 15817;
            p->add_xp_to_stack(i, (int)((((float)r * (1.0f / 15817.0f)) * 0.5f + 0.75f) * (float)s.count * per));
        }
    }

    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if (s.flags & 1)
            break;
        if (troops[s.troop].flags30 & 0x10)
            continue;
        if (s.f14 <= 0)
            continue;
        int lead = troops[s.troop].f50;
        if (m == 1 && (unsigned)troop_f54(&troops[s.troop]) > 0)
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
            Stack& d = p->stacks[k];
            d.count += s.f14;
            int extra = (s.f14 - s.count) + s.wounded;
            if (extra < 0)
                extra = 0;
            d.wounded += extra;
            if (s.f14 == s.count) {
                int out[8];
                p->remove_row(out, i, 0);
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
                s.f18 = 0;
            } else {
                int n2 = 0;
                for (int j = 0; j < p->nstacks; j++) {
                    if (p->stacks[j].flags & 1)
                        break;
                    n2++;
                }
                if (n2 < p->max_stacks()) {
                    int x = (s.f14 - s.count) + s.wounded;
                    if (x < 0)
                        x = 0;
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
int GameW::add_random_experience(int amount)
{
    int r = rand() % 15817;
    // GameData::add_experience (0x4428c0) is void, the original returns what it left in eax: the same function through an int-returning member type
    typedef int (GameData::*AddExperienceInt)(int, int);
    return (((GameData*)this)->*(AddExperienceInt)&GameData::add_experience)(player, (int)(((float)r * (1.0f / 15817.0f) + 0.5f) * amount));
}

// ==== 0x4952f0: put a light / wind object at the trigger frame ======================================================
// @ 0x4952f0
void MissionW::add_wind_effect(float a1, float a2)
{
    GameW* g = (GameW*)g_game;
    Light* o = ((rgl::world*)*(void**)this)->spawn_wind();   // 0x478f30
    o->flicker_magnitude = a1;
    o->flicker_interval = a2;
    o->color = g->cur_color();
    o->type = 0;
    rgl::vector4 v;
    v.x = g->trig_row(2).x * g->pos_delta().z + g->trig_row(1).x * g->pos_delta().y +
          g->trig_row(0).x * g->pos_delta().x + g->trig_row(3).x;
    v.y = g->trig_row(0).y * g->pos_delta().x + g->trig_row(2).y * g->pos_delta().z +
          g->trig_row(1).y * g->pos_delta().y + g->trig_row(3).y;
    v.z = g->trig_row(0).z * g->pos_delta().x + g->trig_row(2).z * g->pos_delta().z +
          g->trig_row(1).z * g->pos_delta().y + g->trig_row(3).z;
    o->pos = v;
    o->base_color = o->color;
    o->range = (o->color.z + o->color.y + o->color.x) * 10.0f;
}

// ==== 0x495430: show / hide the carried meshes of an agent's four slots ========================================================
// cdecl; arg1 = the agent body (skeleton at +0x304), arg2 = four carried item refs, arg3 = the three per-slot mesh arrays.
// @ 0x495430
void __cdecl place_scene_agents(CarryBody* holder, CarryRef* table, CarryCtx* ctx)
{
    char placed[11] = {0};
    for (int i = 3; i >= 0; i--) {
        int id = table[i].item;
        if (id < 0)
            continue;
        CarryKind* it = (CarryKind*)&g_items[id];
        if (((it->f1d0 & 0xf0000000) | (it->f1d4 & 7)) == 0)
            continue;
        rgl::matrix M;
        int attach;
        int bone;
        ((ItemKind*)it)->get_carry_pos((pos2*)&M, &attach, &bone);
        if (placed[bone] != 0 || (it->type == 7 && (table[i].m4 & 0xffff) <= 0xa)) {
            if (ctx->c3[i] != 0)
                ctx->c3[i]->vis = 0;
            continue;
        }
        placed[bone] = 1;
        if (ctx->c3[i] != 0) {
            ctx->c3[i]->vis = 0xffff;
            continue;
        }
        if (ctx->c1[i] != 0) {
            ctx->c1[i]->vis = 0xffff;
            CarrySkel* sk = holder->skel;
            ((Skeleton*)sk)->remove_shape((shape_base*)ctx->c1[i]);
            ctx->c1[i]->frame = M;
            ((FxElem*)&sk->elems[attach])->add_group((int)ctx->c1[i]);
        }
        if (ctx->c2[i] != 0) {
            ctx->c2[i]->vis = 0xffff;
            CarrySkel* sk = holder->skel;
            ((Skeleton*)sk)->remove_shape((shape_base*)ctx->c2[i]);
            ctx->c2[i]->frame = M;
            ((FxElem*)&sk->elems[attach])->add_group((int)ctx->c2[i]);
        }
    }
}

// ==== 0x4955e0: register an emitter in a table and record its index ==============================================
// @ 0x4955e0
void EmitterCtl::add(int e)
{
    if (target == 0) return;
    float tmp[5];                                               // a zeroed float quad (stores are immediates, not a shared register)
    tmp[0] = 0.0f;
    tmp[1] = 0.0f;
    tmp[2] = 0.0f;
    tmp[3] = 0.0f;
    Emitter* em = (Emitter*)g_emit_defs.spawn(&e, (const rgl::vector4*)&tmp[1], (const int*)&tmp[0]);   // pos = the zeroed quad tmp[1..4], owner = tmp[0] (0x479d20)
    if (em != 0) {
        target->add(em);
        list.push_back(e);
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
    GlobVec* g = (GlobVec*)g_presentation;
    if (g != 0) {
        g_presentation = 0;
        if (g->first != 0)
            operator delete(g->first);
        g->first = 0;
        g->f8 = 0;
        g->fc = 0;
        g_presentation = 0;
    }
    g_gui.set_overlay(0, 1);
    if (arr[idx]->s7()) {
        while (dq.size() != 0) {
            arr[dq.back()]->s1();
            dq.pop_back();
        }
        g_gui.reset();
    }
    open(idx);
}

// ==== 0x4957d0: spread an experience reward over the main party's heroes ====================================================
// @ 0x4957d0
void GameW::spread_party_experience(int amount)
{
    WL_MARKER(12)
    GameData::add_experience(player, amount);
    PartyMgr* m = &pm();
    Party* p = m->at(m->lookup(f78));
    int live = 0;
    for (int i = 0; i < p->nstacks; i++) {
        if (p->stacks[i].flags & 1)
            break;
        live++;
    }
    int heroes = 0;
    if (live <= 1)
        return;
    for (int i = 1; i < live; i++) {
        int troop = p->stacks[i].troop;
        if (troops[troop].flags32 & 0x10)
            heroes++;
    }
    if (!heroes)
        return;
    for (int i = 1; i < live; i++) {
        int troop = p->stacks[i].troop;
        if (troops[troop].flags32 & 0x10)
            GameData::add_experience(troop, amount / heroes);
    }
    WL_MARKER(13)
}
