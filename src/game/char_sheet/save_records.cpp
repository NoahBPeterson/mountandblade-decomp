#include "./save_records.h"
#include "../../include/game_data.h"   // GameData::write_missions 0x4e9b50
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../out_stream.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4e9850    void Troop::write_save(OutStream* out)  (the slice called the troop record GameRecord)   (thiscall, ret 4)
void Troop::write_save(OutStream* out)
{
    saved.write8(out);
    for (int i = 0; i < 4; i++)
        out->w_raw(&attributes[i]);
    for (int i = 0; i < 7; i++)
        out->w_int(&istat[i]);
    for (int i = 0; i < 6; i++)
        out->w_count((int*)&nib[i]);
    out->w_int8((int*)&flags30).w_raw(&site);
    out->w_raw(&gold).w_raw(&exp);
    out->w_int(&fa0);
    out->w_raw(&faction);
    out->w_raw(&skill_points).w_raw(&attr_points).w_raw(&prof_points);
    out->w_raw(&level);
    ((cPairs96*)inv)->write(out);
    ItemRef* q = equip;
    for (int i = 0; i < 10; i++, q++) {
        out->w_raw(&q->item);
        out->w_raw(&q->m4);
    }
    unsigned __int64* r5 = face_keys;
    for (int i = 0; i < 4; i++, r5++)
        out->w_int8((int*)r5);
    write_soa(out, (SoaBlock*)note_times);
}

// @ 0x4e99a0    void cPairs96::write(OutStream* out)   (thiscall, ret 4)   the 0x60 pairs at this
void cPairs96::write(OutStream* out)
{
    char* r = raw;
    for (int i = 0x60; i; i--, r += 8) {
        out->w_raw(r);
        out->w_raw(r + 4);
    }
}

// @ 0x4e99d0    template<> void Pool<Event>::write_table(OutStream* out)   (thiscall, ret 4)   the sparse page table
template<> void Pool<Event>::write_table(OutStream* out)
{
    int last = -1;
    int n = count;
    for (int i = 0; i < n; i++) {
        Event* e = pages[(unsigned)i >> 4] + (i & 0xf);
        if (e->in_use)
            last = i;
    }
    int w = last + 1;
    out->w_raw(&w);
    out->w_raw(&serial);
    int k = 0;
    while (k <= last) {
        Event* e = pages[(unsigned)k >> 4] + (k & 0xf);
        if (e->in_use) {
            int one = 1;
            out->w_raw(&one);
            out->w_raw(&e->serial);
            e->write(out);
        } else {
            int zero = 0;
            out->w_raw(&zero);
        }
        k++;
    }
}

// @ 0x4e9ab0    void Event::write(OutStream* out)   (thiscall, ret 4)
//   three chained pairs at +0x24, +0x2c and +0x34 (each: slot 0xc, then slot 0xc on its result at +4).
void Event::write(OutStream* out)
{
    out->w_struct(&name);
    out->w_raw(&type);
    int* p;
    int* q;
    p = (int*)&pos;
    OutStream* s = &out->w_int(p);
    q = p + 1;
    s->w_int(q);
    p = (int*)&f2c;
    s = &out->w_int(p);
    q = p + 1;
    s->w_int(q);
    p = (int*)&f34;
    s = &out->w_int(p);
    q = p + 1;
    s->w_int(q);
    out->w_raw(&party_a);
    out->w_raw(&party_b);
    out->w_int1((int*)&born.start);
    out->w_int(&f70);
}

// @ 0x4e9b50    void GameData::write_missions(OutStream* out)   (thiscall, ret 4)
//   the stacks of the current party that are not heroes: their counts, wounded and the 0x20-int face seed block of each
void GameData::write_missions(OutStream* out)
{
    PartyMgr* l = &parties;
    l->flush_ids();
    l->write_table(out);
    int idx = l->lookup(cur_party);
    Party* r = &l->pages[idx >> 4][idx & 0xf];
    int i = 0;
    if (r->nstacks > 0) {
        int* pk = stack_seeds[0];
        Stack* e = r->stacks;
        do {
            if (!(troops[e->troop].flags30 & 0x10)) {
                out->w_int(&e->f18);
                out->w_raw(&e->f14);
                for (int k = 0; k < 0x20; k++)
                    out->w_raw(pk + k);
            }
            i++; e++; pk += 0x20;
        } while (i < r->nstacks);
    }
}

// @ 0x4e9c50    void PartyMgr::write_table(OutStream* out)   (thiscall, ret 4)
void PartyMgr::write_table(OutStream* out)
{
    out->w_raw(&count);
    out->w_raw(&serial);
    int i = 0;
    int* pi = &i;
    while (i < count) {
        Party* r = &pages[(unsigned)i >> 4][i & 0xf];
        if (r->type == 1) {
            int one = 1;
            out->w_raw(&one);
            out->w_raw(pi);
            out->w_raw(&r->id);
            r->write(out);
        } else {
            int zero = 0;
            out->w_raw(&zero);
        }
        i++;
    }
}

// @ 0x4e9d10    void Party::write(OutStream* out)   (thiscall, ret 4)   the big record walk
void Party::write(OutStream* out)
{
    out->w_struct(&key);
    out->w_struct(&name);
    out->w_int8((int*)&flags);
    out->w_raw(&menu);
    out->w_raw(&owner);
    out->w_raw(&faction);
    out->w_raw(&personality);
    out->w_raw(&state);
    out->w_raw(&action);
    out->w_raw(&state_target);
    out->w_raw(&target);
    int* pa = (int*)&ai_target;                   // the vector2 written as two ints
    OutStream* sA = &out->w_int(pa);
    sA->w_int(&pa[1]);
    int* pb = &f924;
    OutStream* sB = &out->w_int(pb);
    sB->w_int(&pb[1]);
    int* p3 = (int*)&pos.x;
    out->w_int(p3).w_int(p3 + 1).w_int(p3 + 2);
    int* pq = &nstacks;
    out->w_raw(pq);
    int j = 0;
    if (*pq > 0) {
        Stack* b = stacks;
        do {
            out->w_raw(&b->troop);
            out->w_raw(&b->count);
            out->w_raw(&b->wounded);
            out->w_raw(&b->flags);
            j++;
            b++;
        } while (j < *pq);
    }
    // the tail (offsets per 0x4e94b0 / 0x4e9620): +0x50, +0x1c (int4), +0x54 (struct), the three ints at
    // +0x7c, +0x8f4, +0x958, the raw pair +0x6c/+0x78, +0x5ab8 (int1), +0x94/+0x8c8/+0x8cc (int4), then
    // the vectors at +0x8bc and +0x59f8, the struct-of-arrays block at +0x5a08 and the 8-byte vector +0x59d8.
    out->w_int((int*)&angle);
    out->w_int4((int*)&flag);
    out->w_struct(&extra_text);
    OutStream* s3 = &out->w_int((int*)&morale);
    s3 = &s3->w_int((int*)&zoom);
    s3->w_int((int*)&f84);
    out->w_int((int*)&ai_patrol_radius);
    out->w_int((int*)&ai_initiative);
    out->w_raw(&in_sight);
    out->w_int((int*)&bandit_attraction);
    out->w_int1((int*)&ignore_timer.start);
    out->w_raw(&banner_icon);
    out->w_raw(&attached_to);
    out->w_int4((int*)&engaged);
    SoaBlock* soa = (SoaBlock*)note_times;
    write_vec4(attached, out);
    write_vec4(deferred, out);
    write_soa(out, soa);
    ((List8*)&slots)->write8(out);
    return;
}

// @ 0x4e9fe0    void PartyMgr::flush_ids()   (thiscall, ret 0)   drop the records whose type is 2
void PartyMgr::flush_ids()
{
    unsigned i = 0;
    if (count > 0) {
        do {
            Party* r = &pages[(unsigned)i >> 4][i & 0xf];
            if (r->type == 2)
                id_remove(r->id);
            i++;
        } while ((int)i < count);
    }
}
// std::vector<int>::push_back (0x440b30, the instance agent4/agent_logic.cpp owns), with <vector>'s body, visible here and kept out of
// line: cs_common.h only declares it, and without a body cl must assume the callee keeps &idx, which adds dependences to the spill
// of idx and schedules it one slot early. inline = a COMDAT, as the template's own instance would be.
template<> inline __declspec(noinline) void std::vector<int, std::allocator<int> >::push_back(const int& v)
{
    if (size() < capacity())
        _Mylast = _Ufill(_Mylast, 1, v);
    else
        insert(end(), v);
}
// @ 0x4ea030    void PartyMgr::id_remove(int id)   (thiscall, ret 4)   the 0x2717 open-addressed id hash
//   The record behind the slot is type=0'd and the slot index is pushed onto the free list (push_back 0x440b30).
void PartyMgr::id_remove(int id)
{
    int  probe = 0;
    int  slot;
    if (id < 0x2717) {
        slot = id;
    } else {
        slot = id % 0x2717;
        while (tab[slot].key != id) {
            if (probe >= 0x2717)
                break;
            slot = (slot + 0x4f) % 0x2717;
            probe++;
        }
    }
    if (tab[slot].key == id) {
        int idx = tab[slot].value;
        tab[slot].key = -1;
        tab[slot].value = -1;
        pages[(unsigned)idx >> 4][idx & 0xf].type = 0;
        free_ids.push_back(idx);   // 0x440b30
    }
}
