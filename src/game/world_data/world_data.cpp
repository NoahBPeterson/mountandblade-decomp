//-----------------------------------------------------------------------------
// File: world_data.cpp  (0x490030..0x4958f0)
//
// Desc: the party / quest / experience members of the game object, the container-widget helpers, the scene lists, the
//       message-log quest lines and the neighbouring small objects of translation units 83 (world_2) and 84
//       (game_data_2) of the TU map. Names provisional. Functions in ascending address order. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./world_data.h"
#include "../../include/crt.h"      // canonical CRT declarations (listed directly: the resolution audit reads only the source's own includes)
#include "../game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)
#include "../rec98.h"             // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../../include/game_recs.h"
#include "../../include/msg_log.h"
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
#include "../../include/rec1c8.h"
#include "../../include/i64_vec.h"
#include "../../include/e68.h"
#include "../../include/mesh_parts.h"
#include "../../rgl/mesh.h"
#include "./tableau.h"
#include "./widget_mgr_w.h"
#include "../eff_obj.h"      // (listed directly: the resolution audit reads only the source's own includes)


// ==== 0x490030: a random position around a point ===========================================================
// @ 0x490030
void GameW::random_position_around(Vec2 pos, float r, Vec2* out)
{
    Vec2 np;
    float dx, dy;
    int gi;
    *out = pos;
    char ok = 0;
    int iter = 0;
    FaceId* f = find_at(&pos.x);
    if (f == 0)
        return;
    int first = f->m4;
    if (first < 0)
        return;
    while (!ok) {
        if (iter >= 5000)
            return;
        iter++;
        dx = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        dy = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        if (sqrt(dx * dx + dy * dy) <= r) {
            np = Vec2(pos.x + dx, pos.y + dy);
            *out = np;
            FaceId* g = find_at(&out->x);
            if (g != 0) {
                gi = g->m4;
                ok = g_level->obj->shares(&first, &gi);
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
        FaceId* f = find_at(&out->x);
        if (f == 0)
            continue;
        int kind = *(int*)(g_level->obj->kinds + f->m4 * 0x88 + 0x24);
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
        PartyMgrW* m = &pm();
        int i = m->index_of(id);
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
// @ 0x490340
void PartyMgrW::clear()
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
    if (pages._Myfirst != 0)
        operator delete(pages._Myfirst);
    pages._Myfirst = 0;
    pages._Mylast = 0;
    pages._Myend = 0;
    void** base = (void**)((char*)this + 0x138c0);
    if (*base != 0)
        operator delete(*base);
    base[0] = 0;
    base[1] = 0;
    base[2] = 0;
}

// @ 0x4903e0
int PartyMgrW::is_live(unsigned id)
{
    if (id < limit()) {
        int i = index_of(id);
        if (i >= 0)
            return atu((unsigned)i)->type == 1;
    }
    return 0;
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
    return ammo_skill((char*)g_items + id * 0x238);
}

// ==== 0x490520: the container widget (children vector at +0x17c) =================================================
// @ 0x490520
void ContainerW::dtor_common()
{
    *(void**)this = (void*)0x5924a0;
    release();
    if (ents_first() != 0)
        operator delete(ents_first());
    ents_first() = 0;
    ents_last() = 0;
    ents_end() = 0;
    dtor_base();
}

// @ 0x490570
void ContainerW::place(int* a)
{
    WD_FLD(int, 0xe8) = a[0];
    WD_FLD(int, 0xec) = a[1];
    if (p154 != 0) {
        ((EntVt*)p154)->v23(this);
        ((EntVt*)this)->v24();
        return;
    }
    WD_FLD(int, 0xe0) = a[0];
    WD_FLD(int, 0xe4) = a[1];
    ((EntVt*)this)->v24();
}

// @ 0x4905c0
void ContainerW::track_bounds(void* p)
{
    char* e = (char*)p;
    float n[2];
    volatile float* ep = (volatile float*)(e + 0xe8);
    n[0] = ep[0] + *(float*)((char*)this + 0xe0);
    *(int*)(e + 0xe0) = *(int*)&n[0];
    n[1] = ep[1] + *(float*)((char*)this + 0xe4);
    *(int*)(e + 0xe4) = *(int*)&n[1];
    ((EntVt*)e)->v24();
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
void ContainerW::activate_all()
{
    void** i = ents_first();
    while (i != ents_last()) {
        ((EntVt*)this)->v23(*i);
        i++;
    }
}

// @ 0x4906e0
void ContainerW::reset_all()
{
    void** i = ents_first();
    while (i != ents_last()) {
        ((EntVt*)*i)->v16();
        i++;
    }
}

// @ 0x490710
void ContainerW::bounds_of(int* a, float* p3, float* p4, float* p5)
{
    if (f104 & 1)
        return;
    struct __declspec(align(8)) Locals {
        float z, y0, hy, ly, mx, my;
    } l;
    render(a, p3, p4, p5);
    l.mx = p4[0];
    l.y0 = p3[0];
    l.my = p4[1];
    l.ly = p5[1];
    l.hy = p5[0];
    float v = *(float*)((char*)this + 0xf8) + *(float*)((char*)this + 0xe0);
    if (l.mx < v) l.mx = v;
    v = *(float*)((char*)this + 0xf0) + *(float*)((char*)this + 0xe0);
    if (l.hy > v) l.hy = v;
    v = *(float*)((char*)this + 0xfc) + *(float*)((char*)this + 0xe4);
    if (l.my < v) l.my = v;
    v = *(float*)((char*)this + 0xf4) + *(float*)((char*)this + 0xe4);
    if (l.ly > v) l.ly = v;
    void** i = ents_first();
    while (i != ents_last()) {
        char* e = (char*)*i;
        if (!(*p5 < *(float*)(e + 0xf8) + *(float*)(e + 0xe0)) &&
            !(p5[1] < *(float*)(e + 0xfc) + *(float*)(e + 0xe4)) &&
            !(*(float*)(e + 0xf0) + *(float*)(e + 0xe0) < p4[0]) &&
            !(*(float*)(e + 0xf4) + *(float*)(e + 0xe4) < p4[1])) {
            l.z = p3[0];
            ((EntVt*)e)->v10(a, &l.z, &l.mx, &l.hy);
            if (l.z > l.y0) l.y0 = l.z;
        }
        i++;
    }
    p3[0] = l.y0;
}

// @ 0x4908c0
void ContainerW::update_all(int p)
{
    if (f104 & 1)
        return;
    void** i = ents_first();
    while (i != ents_last()) {
        *(int*)((char*)*i + 0x100) = 0;
        i++;
    }
    i = ents_first();
    while (i != ents_last()) {
        ((EntVt*)*i)->v02(p);
        i++;
    }
}

// @ 0x490930
ContainerW* ContainerW::destroy(unsigned flag)
{
    *(void**)this = (void*)0x5924a0;
    release();
    if (ents_first() != 0)
        operator delete(ents_first());
    ents_first() = 0;
    ents_last() = 0;
    ents_end() = 0;
    dtor_base();
    if (flag & 1)
        operator delete(this);
    return this;
}

// ==== 0x490990: mark the parties of one kind dirty ===============================================================
// @ 0x490990
void GameW::mark_kind(int kind)
{
    PartyMgrW* m = &pm();
    unsigned i = m->first_party_id();
    while (i < parties.n_parties) {
        if (m->at(m->index_of(i))->kind70() == kind)
            m->at(m->index_of(i))->flag8b5() = 1;
        m->next_party_id(&i);
    }
}

// ==== 0x490a30: take a party out of its leader's follower list ====================================================
// @ 0x490a30
void GameW::detach_party(unsigned id)
{
    PartyMgrW* m = &pm();
    if (id >= m->limit())
        return;
    int i = m->index_of(id);
    if (i < 0)
        return;
    if (m->atu((unsigned)i)->type != 1)
        return;
    if (m->at(m->index_of(id))->leader() < 0)
        return;
    bool done = false;
    unsigned* dst = (unsigned*)m->at(m->index_of(m->at(m->index_of(id))->leader()))->fol_first();
    unsigned* cur = dst + 1;
    do {
        if ((int*)dst >= m->at(m->index_of(m->at(m->index_of(id))->leader()))->fol_last())
            break;
        if (*dst == id) {
            std::vector<int>& l = m->at(m->index_of(m->at(m->index_of(id))->leader()))->list();
            memmove(dst, cur, (int)(l._Mylast - (int*)cur) * 4);
            --l._Mylast;
            done = true;
        }
        dst++;
        cur++;
    } while (!done);
    m->at(m->index_of(m->at(m->index_of(id))->leader()))->dirty = 1;
    m->at(m->index_of(id))->f8cc() = 0;
    m->at(m->index_of(id))->dirty = 1;
    m->at(m->index_of(id))->leader() = -1;
}

// ==== 0x490ce0: total a party's stacks (and its followers') into counters ========================================
// @ 0x490ce0
void GameW::count_units(unsigned id, int* a, int* b, int* c, int* d)
{
    PartyMgrW* m = &pm();
    if (id >= m->limit())
        return;
    int i = m->index_of(id);
    if (i < 0)
        return;
    if (m->atu((unsigned)i)->type != 1)
        return;
    if (m->at(m->index_of(id))->nstacks > 0) {
        int k = 0;
        int n = 0;
        do {
            if ((*(unsigned char*)((char*)m->at(m->index_of(id)) + 0xb0 + k) & 1) != 0) {
                *c += *(int*)((char*)m->at(m->index_of(id)) + 0xa8 + k);
            } else {
                *a += *(int*)((char*)m->at(m->index_of(id)) + 0xa8 + k);
                int troop = *(int*)((char*)m->at(m->index_of(id)) + 0xac + k);
                if ((troops[troop].flags30 & 0x10) == 0) {
                    *b = *b + *(int*)((char*)m->at(m->index_of(id)) + 0xb8 + k);
                } else {
                    *d = *d + 1;
                    if (agent_low(*(int*)((char*)m->at(m->index_of(id)) + 0xac + k))) {
                        *b = *b + 1;
                    }
                }
            }
            n++;
            k += 0x20;
        } while (n < m->at(m->index_of(id))->nstacks);
    }
    for (unsigned j = 0; j < m->at(m->index_of(id))->list().size(); j++)
        count_units(m->at(m->index_of(id))->list()[j], a, b, c, d);
}

// ==== 0x490fa0: a party and, recursively, its followers ===========================================================
// @ 0x490fa0
void GameW::list_members(unsigned id, int* out, int* n)
{
    PartyMgrW* m = &pm();
    if (id >= m->limit())
        return;
    int i = m->index_of(id);
    if (i < 0)
        return;
    if (m->atu((unsigned)i)->type != 1)
        return;
    if (*n >= 0x80)
        return;
    out[*n] = (int)id;
    (*n)++;
    for (unsigned j = 0; j < m->at(m->index_of(id))->list().size(); j++)
        list_members(m->at(m->index_of(id))->list()[j], out, n);
}

// ==== 0x4910a0: casualties of a battle: turn some of a party's troops into prisoners / dead =====================
// @ 0x4910a0
void GameW::recruit(int id, int count, PartyW* dest)
{
    PartyMgrW* m = &pm();
    if ((unsigned)id >= m->limit())
        return;
    int idx = m->index_of(id);
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

    idx = m->index_of(id);
    PartyW* p = m->at(idx);
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
                int mi = m->index_of(members[j]);
                PartyW* mp = m->at(mi);
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
                            if (troops[troop].hp > g_player_wounded_treshold)
                                troops[troop].hp = g_player_wounded_treshold;
                            troops[troop].hp -= g_k15;
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
void GameW::handout(PartyW* p, int id)
{
    int n = 0;
    int members[128];
    int cum[128];
    int total = 0;
    list_members(id, members, &n);
    PartyMgrW* m = &pm();
    for (int i = 0; i < n; i++) {
        total += m->at(m->index_of(members[i]))->sum_stat(0x100);
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
            m->at(m->index_of(members[k]))->add_members(p->stacks[i].troop, 1, p->stacks[i].flags, 0, 0);
        }
    }
}

// ==== 0x4915d0: remove_party a party =================================================================================
// @ 0x4915d0
void GameW::remove_party(int id)
{
    if ((unsigned)id >= parties.n_parties)
        return;
    PartyMgrW* m = &pm();
    int idx = m->index_of(id);
    if (idx < 0)
        return;
    if (m->atu(idx)->type != 1)
        return;

    idx = m->index_of(id);
    PartyW* p = m->at(idx);
    if (p->leader() >= 0) {
        if (m->is_live(p->leader()))
            detach_party(id);
        p->leader() = -1;
    }
    if (p->fol_first() != 0 && p->fol_last() - p->fol_first() != 0) {
        for (unsigned k = 0; p->fol_first() != 0 && k < (unsigned)((p->fol_last() - p->fol_first()) >> 2); k++) {
            if ((unsigned)p->fol_first()[k] >= parties.n_parties)
                continue;
            int j = m->index_of(p->fol_first()[k]);
            if (j < 0)
                continue;
            if (m->atu(j)->type != 1)
                continue;
            m->at(m->index_of(p->fol_first()[k]))->leader() = -1;
            m->at(m->index_of(p->fol_first()[k]))->f8cc() = 0;
            m->at(m->index_of(p->fol_first()[k]))->f8b4() = 1;
        }
        if (p->fol_first() != 0)
            operator delete(p->fol_first());
        p->fol_first() = 0;
        p->fol_last() = 0;
        p->fol_end() = 0;
    }
    if (p->owner >= 0)
        *(int*)(g_party_templates + p->owner * 0xb0 + 0x38) += 1;
    p->clear_map_flag();

    idx = m->index_of(id);
    if (idx >= 0 && idx < parties.count)
        m->at(idx)->type = 2;

    unsigned cur = m->first_party_id();
    if (cur < parties.n_parties) {
        do {
            PartyW* q = m->at(m->index_of(cur));
            if (q->f91c() == id) {
                q->f918() = 0;
                q->f91c() = -1;
            }
            if (q->f8e8() == id) {
                q->f8e4() = 0;
                q->f8e8() = -1;
            }
            m->next_party_id(&cur);
        } while (cur < parties.n_parties);
    }
}

// ==== 0x4918d0: can a party be merged into another ================================================================
// @ 0x4918d0
bool GameW::can_transfer(unsigned id, int other, char flag)
{
    PartyMgrW* mgr = &pm();
    if (id >= parties.n_parties) return false;
    int idx = mgr->index_of(id);
    if (idx < 0) return false;
    if (mgr->atu((unsigned)idx)->type != 1) return false;
    if (!mgr->is_live(other)) return false;
    int ia = mgr->index_of(id);
    PartyW* a = mgr->at(ia);
    int ib = mgr->index_of(other);
    PartyW* b = mgr->at(ib);
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
    PartyMgrW* mgr = &pm();
    int a = 0, b = 0, c = 0, d = 0;
    if (p2 < parties.n_parties) {
        int i = mgr->index_of(p2);
        if (i >= 0 && mgr->atu((unsigned)i)->type == 1) {
            count_units(p2, &a, &b, &c, &d);
        }
    }
    float f1 = (float)(a - b);
    float f2 = 0.0f;
    if (p1 < parties.n_parties) {
        int i = mgr->index_of(p1);
        if (i >= 0 && mgr->atu((unsigned)i)->type == 1) {
            int j = mgr->index_of(p1);
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
    if (prod - fl > 0.5f) r++;
    if (r < 1) r = 1;
    if (p1 < parties.n_parties) {
        int i = mgr->index_of(p1);
        if (i >= 0 && mgr->atu((unsigned)i)->type == 1) {
            int j = mgr->index_of(p1);
            r += mgr->atu((unsigned)j)->skill_level(0xf) / 2;
        }
    }
    if (p2 < parties.n_parties) {
        int i = mgr->index_of(p2);
        if (i >= 0 && mgr->atu((unsigned)i)->type == 1) {
            r -= mgr->atu((unsigned)i)->skill_level(0xf) / 2;
        }
    }
    if (r < -0x14) return -0x14;
    if (r > 0x14) return 0x14;
    return r;
}

// ==== 0x491e50: put a party near a point =============================================================================
// @ 0x491e50
void GameW::set_pos(float x, float y, int id, float radius) {
    PartyMgrW* m = &pm();
    random_position_around(Vec2(x, y), radius, (Vec2*)&x);
    if ((unsigned)id >= parties.n_parties) return;
    int i = m->index_of(id);
    if (i < 0) return;
    if (m->atu((unsigned)i)->type != 1) return;
    i = m->index_of(id);
    PartyW* p = m->at(i);
    p->x30() = x;
    p->y34() = y;
    i = m->index_of(id);
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
    if (f5c < 0) return false;
    int* p = teams[rec((unsigned)f5c)->team].relation;
    int* h = &counts[0][1];
    for (int k = 0; k < 8; k++, p++, h += 6) {
        if (*p == -1 && *h > 0) return false;
    }
    return (float)g_ticks[2] * 1e-05f - changed_at >= (float)a;
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
    unsigned n = (unsigned)agents.count;
    for (unsigned i = agents.first_used(); i < n; agents.next_used(&i)) {
        if (rec(i)->troop == id) found = i;
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
                int c = r->agent_class();
                if (*(int*)(self + base + c * 0x6c + 0x52c8) != 0) {
                    int w = r->f56d4();
                    r->f56d4() = 0;
                    r->set_weapon_mode(w);
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
                int c = r->agent_class();
                if (*(int*)(self + base + c * 0x6c + 0x52c8) != 0) {
                    int w = r->f56d4();
                    r->f56d4() = 0;
                    r->set_weapon_mode(w);
                }
            }
        }
    }
}

// ==== 0x492470: the battle object's rating and side selection ====================================================
// @ 0x492470
void BattleW::refresh_rating()
{
    GameW* g = (GameW*)g_game;
    PartyMgrW* mgr = &g->pm();
    PartyW* pp = mgr->at(mgr->index_of(ids[sel]));
    int t = pp->sum_stat(0x10);
    if (t <= 3)
        tier = 0;
    else if (t <= 0xa)
        tier = 1;
    else
        tier = (t > 0x32) + 2;

    int s = sel;
    int r = g->gate_rating(g->f78, ids[s], ids[1 - s]);
    rating = r;
    if (r > 6)
        rating = 6;
    if (rating < -0x20)
        rating = -0x20;
}

// ==== 0x492530: move all stacks of one party into another =========================================================
// @ 0x492530
void GameW::transfer_stack(int a, int b, char flag)
{
    PartyMgrW* mgr = &pm();
    PartyW* pa = mgr->at(mgr->index_of(a));
    PartyW* pb = mgr->at(mgr->index_of(b));
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
                        PartyW* px = mgr->at(mgr->index_of(a));
                        v = px->stacks[k].troop;
                    }
                    v = v * 0x1bf + 0x45db + a * 0x7bb7 + j;
                }
                out[j] = v;
            }
        }
    }
    remove_party(a);
}

// @ 0x492750
void BattleW::select_side(int s)
{
    int m = mode;
    if (m != 4)
        side = s;
    if (ids[1 - s] >= 0) {
        int fl = flags & ~0x10;
        int flip = 1 - s;
        sel = flip;
        flags = fl;
        f144 = 1;
        if (m == 4 && flip == 1)
            flags = fl | m;
        refresh_rating();
    }
}

// ==== 0x4927d0: std::vector<int>::_Insert_n (with its cold tail 0x492930) ======================================
// The original instance is the pointer-element one (every vector<T*> shares it, /OPT:ICF): the scene list below uses it.
// @ 0x4927d0
template void std::vector<SceneObject*>::_Insert_n(std::vector<SceneObject*>::iterator, size_t, SceneObject* const&);

// ==== 0x492a20: free the game object's record arrays =============================================================
// @ 0x492a20
void GameW::free_arrays()
{
    RecS* s = *(RecS**)&troops;
    scene_open() = 0;
    if (s != 0) {
        delete[] s;
        troops = 0;
    }

    RecPoly* p = *(RecPoly**)&levels;
    if (p != 0) {
        delete[] p;
        levels = 0;
    }

    RecT* t = *(RecT**)&factions;
    if (t != 0) {
        delete[] t;
        factions = 0;
    }

    RecQ* q = *(RecQ**)&quests;
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
struct Obj475d50 { void dtor(); };                              // 0x475d50
// @ 0x492ee0
void PtrStack14::pop_back_delete()
{
    unsigned _n = size();
    void* _Ptr = _Myfirst[_n - 1];
    if (_Ptr != 0) {
        ((Obj475d50*)_Ptr)->dtor();
        ::operator delete(_Ptr);
    }
    if (_Myfirst != 0 && size() != 0)
        --_Mylast;
}

// ==== 0x492f40: the game object's destructor =======================================================================
struct CursorObj { void reset(); };                             // 0x492a20 (GameW::free_arrays)
struct Obj4317b0 { void reset(); };                             // 0x4317b0
struct Obj4c6e30 { void release(); };                           // 0x4c6e30
// @ 0x492f40
void GameW::world_free()
{
    char* self = (char*)this;
    if (scene_open() != 0)
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

    ((Obj4317b0*)(self + 0x139f4))->reset();

    if (*(void**)(self + 0x139ec) != 0)
        ((RecD*)*(void**)(self + 0x139ec))->scalar_delete(3);

    pm().clear();
    ((Obj4c6e30*)(self + 0xcc))->release();
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
void* ContainerW::construct()
{
    ((PanelBase*)this)->init();
    *(void**)this = (void*)vtbl_5924a0;
    ents_first() = 0;
    ents_last() = 0;
    ents_end() = 0;
    return this;
}

// @ 0x4931d0
DropDownW* DropDownW::construct()
{
    char* self = (char*)this;
    ((PanelBase*)this)->init();
    *(int*)(self + 0x178) = 0;
    *(char*)(self + 0x121) = 0;
    *(void**)self = (void*)vtbl_592570;
    *(int*)(self + 0x190) = 0;
    *(int*)(self + 0x194) = 0;
    *(int*)(self + 0x198) = 0;
    *(char*)(self + 0x180) = 0;
    *(int*)(self + 0x188) = 0;
    *(int*)(self + 0x164) = 0;
    *(int*)(self + 0x160) = 0;
    {
        FPair _p0 = { 0.2265f, 0.029f };
        *(FPair*)(self + 0x1b0) = _p0;
        FPair _p1 = { 0.018f, 0.018f };
        *(FPair*)(self + 0x1bc) = _p1;
        *(float*)(self + 0x1b8) = 0.0135f;
        FPair _p2 = { 0.02f, 0.01f };
        *(FPair*)(self + 0x1c4) = _p2;
    }
    return this;
}

// @ 0x4932a0
void ListPanelW::set_mode()
{
    char* self = (char*)this;
    if (*(char*)(self + 0x180) == 0)
        *(int*)(self + 0x178) = 1;
    else
        *(int*)(self + 0x178) = 0;
    *(char*)(self + 0x11f) = 0;
    if (*(int*)(self + 0x178) != 0) {
        *(char*)(self + 0x11e) = 1;
        *(char*)(self + 0x11d) = 0;
    } else {
        *(char*)(self + 0x11e) = 0;
        *(char*)(self + 0x11d) = 1;
    }
    render();
}

// @ 0x493300
int panel_count_493300()
{
    return 5;
}

// @ 0x493310
void ListPanelW::set_index(int _v)
{
    char* self = (char*)this;
    Vec16W* _v16 = (Vec16W*)(self + 0x18c);
    int _lo = *(int*)(self + 0x160);
    *(int*)(self + 0x15c) = _v;
    if (_v < _lo)
        *(int*)(self + 0x15c) = _lo;
    else if (_v > *(int*)(self + 0x164))
        *(int*)(self + 0x15c) = *(int*)(self + 0x164);

    int _cur = *(int*)(self + 0x15c);
    if (_cur < _lo)
        *(int*)(self + 0x15c) = _lo;
    else if ((unsigned)_cur > _v16->size())
        *(int*)(self + 0x15c) = _v16->size();

    int _tmp[2];
    ((PanelVt*)this)->v41(_tmp, (void*)((char*)_v16->_Myfirst + *(int*)(self + 0x15c) * 16));
}

// @ 0x4933b0
void ListPanelW::reset_tail()
{
    char* self = (char*)this;
    ((TailDtor*)(self + 0x18c))->dtor();
    *(int*)(self + 0x164) = 0;
    *(int*)(self + 0x15c) = 0;
}

// @ 0x4933d0
void* ListPanelW::destroy(unsigned _flags)
{
    char* self = (char*)this;
    ((TailDtor*)(self + 0x18c))->dtor();
    ((ContainerW*)self)->dtor_base();
    if (_flags & 1)
        ::operator delete(self);
    return self;
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
void ListPanelW::append(const tstring* _name)
{
    char* self = (char*)this;
    Vec16W* _v16 = (Vec16W*)(self + 0x18c);
    int _count;
    int _maxn = 0x800;
    int _buf[0x800];
    decode_utf8_487b00(&_count, _name, _buf, &_maxn);
    float _scale = (float)_count * 0.0077999993f;
    if (_scale > 0.21f)
        _scale = 0.21f / _scale * 0.02f;
    else
        _scale = 0.02f;
    if (_scale < *(float*)(self + 0x140)) {
        float _pair[2];
        _pair[0] = _scale;
        _pair[1] = _scale;
        ((PanelVt*)this)->v27(_pair);
    }
    _v16->push_back(*_name);
    ++*(int*)(self + 0x164);
    if (_v16->size() == 1)
        ((PanelVt*)this)->v11((void*)0);
}

// ==== 0x4934d0: the party record constructor and its page / hash management ========================================
void __stdcall vector_constructor_iterator(void* first, unsigned size, unsigned count, void* fn);   // 0x401800
void elem_ctor_48c210(void* self);                              // 0x48c210
struct ListNode { void* _Buynode(); };                          // 0x46c0c0 (thiscall)

// @ 0x4936a0
void PartyW::thunk_4936a0()
{
    init();
}

// @ 0x4934d0
PartyW* PartyW::construct()
{
    char* self = (char*)this;

    *(void**)(self) = (void*)vtbl_592644;
    new ((tstring*)(self + 0xc)) tstring();
    new ((tstring*)(self + 0x20)) tstring();
    new ((tstring*)(self + 0x54)) tstring();

    {
        char* p = self + 0xa8;
        for (int i = 0; i < 0x40; ++i) {
            *(int*)(p + 0x00) = 0;
            *(int*)(p + 0x04) = -1;
            *(int*)(p + 0x08) = 0;
            *(int*)(p + 0x0c) = 0;
            *(int*)(p + 0x10) = 0;
            *(int*)(p + 0x14) = 0;
            *(int*)(p + 0x18) = 0;
            p += 0x20;
        }
    }

    *(int*)(self + 0x8bc) = 0;
    *(int*)(self + 0x8c0) = 0;
    *(int*)(self + 0x8c4) = 0;
    *(int*)(self + 0x8d0) = 1;
    *(__int64*)(self + 0x8d8) = g_ticks[1];
    *(int*)(self + 0x8f8) = 1;
    *(__int64*)(self + 0x900) = g_ticks[1];
    *(int*)(self + 0x908) = 0;
    *(__int64*)(self + 0x910) = g_ticks[0];
    *(int*)(self + 0x938) = 1;
    *(__int64*)(self + 0x940) = g_ticks[1];
    *(int*)(self + 0x948) = 1;
    *(__int64*)(self + 0x950) = g_ticks[1];
    *(int*)(self + 0x59b0) = 1;
    *(__int64*)(self + 0x59b8) = g_ticks[1];
    *(int*)(self + 0x59dc) = 0;
    *(int*)(self + 0x59e0) = 0;
    *(int*)(self + 0x59e4) = 0;
    *(int*)(self + 0x59f8) = 0;
    *(int*)(self + 0x59fc) = 0;
    *(int*)(self + 0x5a00) = 0;

    for (int i = 0; i < 8; ++i)
        new ((tstring*)(self + 0x5a28 + i * 0x10)) tstring();

    *(int*)(self + 0x5aa8) = -1;
    *(int*)(self + 0x5ab0) = 1;
    *(__int64*)(self + 0x5ab8) = g_ticks[1];

    init();

    *(__int64*)(self + 0x59b8) = g_ticks[*(int*)(self + 0x59b0)] - 100000;
    *(char*)(self + 0x5a04) = 0;
    return this;
}

// @ 0x4936b0
template void std::vector<int>::push_back(const int&);

// @ 0x493700
void RecPool::alloc_record()
{
    if (free_ids.size() <= 0) {
        Rec98* blk = new Rec98[16];
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
}

// @ 0x493830
PartyMgrW* PartyMgrW::grow()
{
    Party* blk = (Party*)new PartyNew[16];
    pages.push_back(blk);

    for (int i = 0; i < 0x10; ++i) {
        blk[i].type = 0;
        free_ids().push_back(count + i);
    }
    count += 0x10;
    return this;
}

// @ 0x493960
int PartyMgrW::add_slot()
{
    MapEntry* tab = (MapEntry*)this;                                                  // the id -> index hash table at the start

    if (free_ids().size() <= 0)
        grow();

    unsigned idx = free_ids().back();
    free_ids().pop_back();

    Party* p = pages[idx >> 4] + (idx & 0xf);
    p->type = 1;
    p->id = limit();

    int h = limit();
    int slot = h % 0x2717;
    while (tab[slot].key != (unsigned)-1)
        slot = (slot + 0x4f) % 0x2717;
    tab[slot].key = h;
    tab[slot].value = idx;
    ++limit();
    return p->id;
}

// @ 0x493a30
void ContainerW::add_child(MissionUi* _obj)
{
    children().push_back(_obj);
    _obj->p154 = this;
    ((EntVt*)this)->v23(_obj);
}

// @ 0x493ab0
SceneBaseCtor* SceneBaseCtor::construct()
{
    char* self = (char*)this;

    {
        void* _n0 = ((ListNode*)(self + 0x2c))->_Buynode();
        *(void**)(self + 0x30) = _n0;
        *(int*)(self + 0x34) = 0;
        *(int*)(self + 0x38) = 0x400;
        *(int*)(self + 0x3c) = 0;
    }
    {
        void* _n1 = ((ListNode*)(self + 0x40))->_Buynode();
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

    ((ViewParamsCtor*)(self + 0xd8))->ctor();

    *(int*)(self + 0x330) = 0;
    *(int*)(self + 0x334) = 0;
    *(int*)(self + 0x338) = 0;
    *(int*)(self + 0x340) = 0;
    *(int*)(self + 0x9c) = 1;

    float _g[4];
    {
        float _x[2] = { 1000.0f, 1000.0f };
        float _y[2] = { -1000.0f, -1000.0f };
        init(_y, _x, 0, 0);
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
        I64Vec::resize((unsigned)_n, _z);
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
    ((I64VecOp*)this)->reserve(&_n);
    (*(std::vector<__int64>*)this)[slot] = value;
}

// @ 0x493db0
void SlotHolder40::set40(int _idx, int _val)
{
    I64VecOp* _p = (I64VecOp*)((char*)this + 0x40);
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

    ResSlot* _new = (ResSlot*)make(_key);
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
                g_resmgr_rel.release(slots()[_i]);
                slots()[_i] = slots()[_last];
                --_last;
            } else
                ++_i;
        } while (_i <= _last);
    }
    slots().resize(_last + 1, 0);
}

// @ 0x493f30
void Vec68W::purge_all()
{
    for (unsigned _i = 0; _i < (unsigned)(begin == 0 ? 0 : (end - begin)); ++_i)
        ((E68W*)(begin + _i))->purge();
}

// ==== 0x493f80, 0x494020: tableau light nodes ============================================================================
// @ 0x493f80
void TableauTbl::add_point_light(const rgl::matrix* p, const rgl::vector4& c)
{
    char* other = (char*)p;
    int* _n = (int*)((NodeAlloc*)worlds[worlds.size() - 1])->node_478f30();
    const int* _d = (const int*)&c;
    memcpy(_n + 1, _d, 0x10);
    _n[0] = 0;
    memcpy(_n + 9, other + 0x30, 0x10);
    memcpy(_n + 5, _n + 1, 0x10);
    float* _f = (float*)_n;
    _f[0x15] = (_f[3] + _f[2] + _f[1]) * 10.0f;
}

// @ 0x494020
void TableauTbl::add_sun_light(const rgl::matrix* p, const rgl::vector4& c)
{
    char* other = (char*)p;
    int _g[4];
    int* _n = (int*)((NodeAlloc*)worlds[worlds.size() - 1])->node_478f30();
    const int* _d = (const int*)&c;
    memcpy(_n + 1, _d, 0x10);
    _n[0] = 2;
    memcpy(_n + 0x11, other + 0x10, 0x10);
    float* _f = (float*)_n;
    _f[9]  = *(float*)(other + 0x30) - _f[0x11] * 1000.0f;
    _f[10] = *(float*)(other + 0x34) - _f[0x12] * 1000.0f;
    _f[0xb] = *(float*)(other + 0x38) - _f[0x13] * 1000.0f;
    _n[0xc] = _g[3];
    memcpy(_n + 5, _n + 1, 0x10);
}

// ==== 0x494110: add a button widget to the manager ===================================================================
// @ 0x494110
int WidgetMgrW::create_button_overlay(const tstring* text, unsigned flags)
{
    UiAInline* w = new UiAInline;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        vt(w)->d22(&v);
    }
    flags = (flags & 0x181c) | 0x200;
    vt(w)->d29(&flags);
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
    char* self = (char*)this;
    g_gui.set_o568(0, 1);
    dq.push_back(_idx);
    arr[_idx]->s2();

    unsigned _u = *(unsigned*)(self + 0x88) + *(unsigned*)(self + 0x84) - 1;
    unsigned _q = _u >> 2;
    unsigned _r = _u - (_q << 2);
    if (*(unsigned*)(self + 0x80) <= _q)
        _q -= *(unsigned*)(self + 0x80);
    int* _row = *(int**)(*(int*)(self + 0x7c) + _q * 4);
    int _elem = *(int*)((char*)_row + _r * 4);
    int _bl = (unsigned char)arr[_elem]->s8();
    if (g_render.dev != 0) {
        PanelFn _fn = (PanelFn)(*(void***)g_render.dev)[12];
        _fn((void*)g_render.dev, (unsigned char)_bl);
    }
    g_gui.f560 = (char)_bl;
}

// ==== 0x4942f0, 0x494310: the sound player ================================================================================
// @ 0x4942f0
void SoundW::play_ui(int _a, unsigned _b)
{
    unsigned _f = _b;
    _f |= 1;
    play(_a, (int)_f, (const float*)&listener, 1.0f, 0);
}

// @ 0x494310
void SoundW::stop_all()
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
    RotRows _rot = identity3();
    rgl::matrix _m;
    *(RotRows*)&_m = _rot;
    _m.o = _pos;
    if (g_cfg.disable_sound == 0) {
        listener_frame() = _m;
        FSOUND_3D_Listener_SetAttributes(&listener, 0, *(int*)((char*)this + 0x20), *(int*)((char*)this + 0x28), *(int*)((char*)this + 0x24),
                               *(int*)((char*)this + 0x30), *(int*)((char*)this + 0x38), *(int*)((char*)this + 0x34));
    }
}

// ==== 0x494410..0x494680: quest lines on the message log ================================================================
extern "C" const char s_ui_quest_completed_value[];             // 0x592650
extern "C" const char s_ui_quest_succeeded_value[];             // 0x59266c
extern "C" const char s_ui_quest_failed_value[];                // 0x592688

// @ 0x494410
void GameW::quest_done(int _idx)
{
    char* _e = (char*)((char*)quests + _idx * 0x138);
    if ((*(unsigned*)(_e + 0x50) >> 1 & 1) != 0 ||
        ((*(unsigned*)(_e + 0x64) & 0x200) == 0 && (*(unsigned*)(_e + 0x64) & 0x100) != 0)) {
        if ((*(unsigned*)(_e + 0x64) & 0x800) == 0) {
            tstring _val(*(const tstring*)(_e + 0x40));
            char _buf[0x200];
            {
                tstring _fmt(s_ui_quest_completed_value);
                char* _f = _val.buf ? _val.buf : (char*)&_val;
                char* _r = (char*)g_strtable.lookup(&_fmt) + 4;
                if (*(char**)_r != 0)
                    _r = *(char**)_r;
                mb_snprintf(_buf, 0x200, _r, _f);
            }
            {
                tstring _txt(_buf);
                g_msglog.push(_txt, 0xffd0f0c8);
            }
            g_sound_mgr.play(g_quest_sounds[0], 1, (const float*)&g_sound_mgr.listener, 1.0f, 0);
        }
        if ((*(unsigned*)(_e + 0x50) >> 1 & 1) == 0)
            *(unsigned*)(_e + 0x64) |= 0x200;
        else
            *(int*)(_e + 0x64) = 0;
    }
}

// @ 0x494560
void GameW::quest_succeeded(int _idx)
{
    char* _e = (char*)((char*)quests + _idx * 0x138);
    *(unsigned*)(_e + 0x64) |= 0x1400;
    tstring _val(*(const tstring*)((char*)quests + _idx * 0x138 + 0x40));
    char _buf[0x200];
    {
        tstring _fmt(s_ui_quest_succeeded_value);
        char* _f = _val.buf ? _val.buf : (char*)&_val;
        char* _r = (char*)g_strtable.lookup(&_fmt) + 4;
        if (*(char**)_r != 0)
            _r = *(char**)_r;
        mb_snprintf(_buf, 0x200, _r, _f);
    }
    {
        tstring _txt(_buf);
        g_msglog.push(_txt, 0xff90ff28);
    }
    g_sound_mgr.play(g_quest_sounds[1], 1, (const float*)&g_sound_mgr.listener, 1.0f, 0);
}

// @ 0x494680
void GameW::quest_failed(int _idx)
{
    char* _e = (char*)((char*)quests + _idx * 0x138);
    *(unsigned*)(_e + 0x64) |= 0x1800;
    tstring _val(*(const tstring*)((char*)quests + _idx * 0x138 + 0x40));
    char _buf[0x200];
    {
        tstring _fmt(s_ui_quest_failed_value);
        char* _f = _val.buf ? _val.buf : (char*)&_val;
        char* _r = (char*)g_strtable.lookup(&_fmt) + 4;
        if (*(char**)_r != 0)
            _r = *(char**)_r;
        mb_snprintf(_buf, 0x200, _r, _f);
    }
    {
        tstring _txt(_buf);
        g_msglog.push(_txt, 0xffd02028);
    }
    g_sound_mgr.play(g_quest_sounds[3], 1, (const float*)&g_sound_mgr.listener, 1.0f, 0);
}

// ==== 0x4947a0, 0x4948c0: quest concluded / cancelled ====================================================================
// the ring-buffer append inlined by this TU
static __forceinline void log_add(const tstring& s)
{
    if (g_msglog.enabled && g_msglog.ring.count < 32) {
        int idx = (g_msglog.ring.head + g_msglog.ring.count) % 32;
        g_msglog.ring.names[idx] = s;
        g_msglog.ring.vals[idx] = 0xfff000;
        g_msglog.ring.count++;
    }
}

// @ 0x4947a0
void GameW::set_quest_concluded(int quest, int value)
{
    if (quest < 0 || value < 0) return;
    char buf[512];
    QuestRec* q = &quests[quest];
    q->flags |= 0x1000;
    {
        tstring id(q->desc);
        tstring key("ui_quest_concluded_value");
        mb_snprintf(buf, 0x200, g_strtable.lookup(&key)->c_str(), id.c_str());
    }
    {
        tstring msg(buf);
        g_msglog.push(msg, 0xffd02028);
    }
    g_sound_mgr.play(g_quest_sounds[2], 1, (const float*)&g_sound_mgr.listener, 1.0f, 0);
}

// @ 0x4948c0
void GameW::set_quest_cancelled(int quest)
{
    if (quest < 0) return;
    char buf[512];
    QuestRec* q = &quests[quest];
    q->flags = 0;
    {
        tstring id(q->desc);
        tstring key("ui_quest_cancelled_value");
        mb_snprintf(buf, 0x200, g_strtable.lookup(&key)->c_str(), id.c_str());
    }
    {
        tstring msg(buf);
        g_msglog.push(msg, 0xffd02028);
    }
    g_sound_mgr.play(g_quest_sounds[4], 1, (const float*)&g_sound_mgr.listener, 1.0f, 0);
}


// ==== 0x4949d0, 0x494ad0: party objectives =================================================================================
// @ 0x4949d0
void GameW::set_objective(int party, int value)
{
    PartyMgrW* m = &pm();
    PartyW* pp = m->at(m->index_of(party));
    if (pp->leader() >= 0)
        detach_party(party);
    PartyW* pp2 = m->at(m->index_of(party));
    pp2->leader() = value;
    PartyW* pv = m->at(m->index_of(value));
    pv->list().push_back(party);
    m->at(m->index_of(value))->f8b4() = 1;
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
    if (((PartyW*)parties.get(party))->leader() >= 0)
        detach_party(party);
    if (mode == 0)
        set_objective(party, f78);
    else if (mode == 1)
        set_objective(party, encountered_a());
    else if (mode == 2)
        set_objective(party, encountered_b());
    ((PartyW*)parties.get(party))->f8cc() = 1;
}

// ==== 0x494b60: share an experience amount over a party ==================================================================
// @ 0x494b60
void GameW::share_experience(int party, float amount)
{
    if (party >= parties.n_parties)
        return;
    PartyMgrW* pm_ = &pm();
    int idx = pm_->index_of(party);
    if (idx < 0)
        return;
    if (pm_->pages[idx >> 4][idx & 15].type != 1)
        return;
    PartyW* p = pm_->at(pm_->index_of(party));
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
    if (total > 0) {
        float per = amount / (float)total;
        for (int j = 0; j < p->nstacks; j++) {
            int idx2 = j;
            Stack& s = p->stacks[idx2];
            if (s.flags & 1)
                break;
            int troop = s.troop;
            char hero = (troops[troop].flags32 & 0x10) != 0;
            if (hero)
                add_experience(troop, (int)(per * (idx2 == 0 ? 10.0f : 3.0f)));
            else
                p->add_xp_to_stack(idx2, (int)((float)s.count * per));
        }
    }
    float fl = floorf(amount);
    int n = (int)fl;
    if (!(amount - fl <= 0.5f))
        n = (int)fl + 1;
    if (n != 0 && party == f78) {
        char buf[512];
        {
            tstring key("ui_shared_number_experience_within_party");
            sprintf(buf, g_strtable.lookup(&key)->c_str(), n);
        }
        {
            tstring msg(buf);
            log_add(msg);
        }
    }
}

// ==== 0x494e40: award experience to the troop stacks of a party and fold the share-in counters ===========================
extern int max_stacks();                                   // 0x4b1700 (== 0x20)
// @ 0x494e40
void GameW::award_experience(int party, float amount, int mode)
{
    if ((unsigned)party >= parties.n_parties)
        return;
    PartyMgrW& m2 = pm();
    int idx = m2.index_of(party);
    if (idx < 0)
        return;
    if (m2.atu((unsigned)idx)->type != 1)
        return;
    idx = m2.index_of(party);                      // the original looks the party up a second time
    PartyW* p = &((PartyW*)(m2.pages[idx >> 4]))[idx & 15];
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
            float v = ((float)r * (1.0f / 31634.0f) + 0.75f) * per;
            if (i == 0)
                add_experience(s.troop, (int)(v * 10.0f));
            else
                add_experience(s.troop, (int)(v * 3.0f));
        } else {
            int r = rand() % 15817;
            float v = (((float)r * (1.0f / 15817.0f)) * 0.5f + 0.75f) * (float)s.count * per;
            p->add_xp_to_stack(i, (int)v);
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
                if (n2 < max_stacks()) {
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
    return add_experience(player, (int)(((float)r * (1.0f / 15817.0f) + 0.5f) * amount));
}

// ==== 0x4952f0: put a light / wind object at the trigger frame ======================================================
// @ 0x4952f0
void MissionW::add_wind_effect(int a1, int a2)
{
    GameW* g = (GameW*)g_game;
    EffObj* o = ((EffMgr*)*(void**)this)->create();
    o->f70 = a1;
    o->f74 = a2;
    o->pos = g->cur_color();
    o->f0 = 0;
    rgl::vector4 v;
    v.x = g->trig_row(2).x * g->pos_delta().z + g->trig_row(1).x * g->pos_delta().y +
          g->trig_row(0).x * g->pos_delta().x + g->trig_row(3).x;
    v.y = g->trig_row(0).y * g->pos_delta().x + g->trig_row(2).y * g->pos_delta().z +
          g->trig_row(1).y * g->pos_delta().y + g->trig_row(3).y;
    v.z = g->trig_row(0).z * g->pos_delta().x + g->trig_row(2).z * g->pos_delta().z +
          g->trig_row(1).z * g->pos_delta().y + g->trig_row(3).z;
    o->f24 = v;
    o->f14 = o->pos;
    o->f54 = (o->pos.z + o->pos.y + o->pos.x) * 10.0f;
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
        it->get_carry_pos(&M, &attach, &bone);
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
            sk->remove_shape(ctx->c1[i]);
            ctx->c1[i]->frame = M;
            sk->elems[attach].add_group(ctx->c1[i]);
        }
        if (ctx->c2[i] != 0) {
            ctx->c2[i]->vis = 0xffff;
            CarrySkel* sk = holder->skel;
            sk->remove_shape(ctx->c2[i]);
            ctx->c2[i]->frame = M;
            sk->elems[attach].add_group(ctx->c2[i]);
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
SceneObject* SceneList::push()
{
    SceneObject* o = new SceneObject;
    v.push_back(o);
    return v[v.size() - 1];
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
    g_gui.set_o568(0, 1);
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
    add_experience(player, amount);
    PartyMgrW* m = &pm();
    PartyW* p = m->at(m->index_of(f78));
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
            add_experience(troop, amount / heroes);
    }
}
