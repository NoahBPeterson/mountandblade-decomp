#include "game/save/save_records.h"
#include "game/data/game_data.h"   // GameData::write_missions 0x4e9b50
#include "engine/core/tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "engine/io/stream.h"

// @ 0x4e9850    void Troop::write_save(stream_base* out)  (the slice called the troop record GameRecord)   (thiscall, ret 4)
void Troop::write_save(stream_base* out)
{
    slots.write(out);
    for (int i = 0; i < 4; i++)
        out->w_int(&attributes[i]);
    for (int i = 0; i < 7; i++)
        out->w_float(&stat[i]);
    for (int i = 0; i < 6; i++)
        out->w_uint(&nib[i]);
    out->w_u64(&flags30).w_int(&site);
    out->w_int(&gold).w_int(&exp);
    out->w_float(&hit_points);
    out->w_int(&faction);
    out->w_int(&skill_points).w_int(&attr_points).w_int(&prof_points);
    out->w_int(&level);
    inv[0].write_run(out);
    ItemRef* q = equip;
    for (int i = 0; i < 10; i++, q++) {
        out->w_int(&q->item);
        out->w_int(&q->word);
    }
    unsigned __int64* r5 = face_keys.word;
    for (int i = 0; i < 4; i++, r5++)
        out->w_u64(r5);
    write_soa(out, &notes);
}

// @ 0x4e99a0    void ItemRef::write_run(stream_base* out)   (thiscall, ret 4)   the 0x60 refs from this
__declspec(noinline) void ItemRef::write_run(stream_base* out)
{
    ItemRef* r = this;
    for (int i = 0x60; i; i--, r++) {
        out->w_int(&r->item);
        out->w_int(&r->word);
    }
}

// @ 0x4e99d0    template<> void Pool<Event>::write_table(stream_base* out)   (thiscall, ret 4)   the sparse page table
template<> void Pool<Event>::write_table(stream_base* out)
{
    int last = -1;
    int n = count;
    for (int i = 0; i < n; i++) {
        Event* e = pages[(unsigned)i >> 4] + (i & 0xf);
        if (e->in_use)
            last = i;
    }
    int w = last + 1;
    out->w_int(&w);
    out->w_uint(&serial);
    int k = 0;
    while (k <= last) {
        Event* e = pages[(unsigned)k >> 4] + (k & 0xf);
        if (e->in_use) {
            int one = 1;
            out->w_int(&one);
            out->w_int(&e->serial);
            e->write(out);
        } else {
            int zero = 0;
            out->w_int(&zero);
        }
        k++;
    }
}

// @ 0x4e9ab0    void Event::write(stream_base* out)   (thiscall, ret 4)
//   three chained pairs at +0x24, +0x2c and +0x34 (each: slot 0xc, then slot 0xc on its result at +4).
void Event::write(stream_base* out)
{
    out->w_tstr(&name);
    out->w_int(&type);
    float* p;
    float* q;
    p = &pos.x;
    stream_base* s = &out->w_float(p);
    q = p + 1;
    s->w_float(q);
    p = &pos2.x;
    s = &out->w_float(p);
    q = p + 1;
    s->w_float(q);
    p = &f34;
    s = &out->w_float(p);
    q = p + 1;
    s->w_float(q);
    out->w_int(&party_a);
    out->w_int(&party_b);
    out->w_i64(&born.start);
    out->w_float(&next_simulation_time);
}

// @ 0x4e9b50    void GameData::write_missions(stream_base* out)   (thiscall, ret 4)
//   the stacks of the current party that are not heroes: their counts, wounded and the 0x20-int face seed block of each
void GameData::write_missions(stream_base* out)
{
    PartyMgr* l = &parties;
    l->flush_ids();
    l->write_table(out);
    int idx = l->lookup(cur_party);
    Party* r = &l->pages[idx >> 4][idx & 0xf];
    int i = 0;
    if (r->nstacks > 0) {
        SeedRow* pk = stack_seeds;
        Stack* e = r->stacks;
        do {
            if (!(troops[e->troop].flags30 & 0x10)) {
                out->w_float(&e->xp);
                out->w_int(&e->f14);
                for (int k = 0; k < 0x20; k++)
                    out->w_int(&(*pk)[k]);
            }
            i++; e++; pk++;
        } while (i < r->nstacks);
    }
}

// @ 0x4e9c50    void PartyMgr::write_table(stream_base* out)   (thiscall, ret 4)
void PartyMgr::write_table(stream_base* out)
{
    out->w_int(&count);
    out->w_uint(&serial);
    int i = 0;
    int* pi = &i;
    while (i < count) {
        Party* r = &pages[(unsigned)i >> 4][i & 0xf];
        if (r->type == 1) {
            int one = 1;
            out->w_int(&one);
            out->w_int(pi);
            out->w_int(&r->id);
            r->write(out);
        } else {
            int zero = 0;
            out->w_int(&zero);
        }
        i++;
    }
}

// @ 0x4e9d10    void Party::write(stream_base* out)   (thiscall, ret 4)   the big record walk
void Party::write(stream_base* out)
{
    out->w_tstr(&key);
    out->w_tstr(&name);
    out->w_u64(&flags);
    out->w_int(&menu);
    out->w_int(&owner);
    out->w_int(&faction);
    out->w_int(&personality);
    out->w_int(&state);
    out->w_int(&action);
    out->w_int(&state_target);
    out->w_int(&target);
    float* pa = &ai_target.x;                     // the vector2 written as two floats
    stream_base* sA = &out->w_float(pa);
    sA->w_float(&pa[1]);
    float* pb = &order_pos.x;
    stream_base* sB = &out->w_float(pb);
    sB->w_float(&pb[1]);
    float* p3 = &pos.x;
    out->w_float(p3).w_float(p3 + 1).w_float(p3 + 2);
    int* pq = &nstacks;
    out->w_int(pq);
    int j = 0;
    if (*pq > 0) {
        Stack* b = stacks;
        do {
            out->w_int(&b->troop);
            out->w_int(&b->count);
            out->w_int(&b->wounded);
            out->w_int(&b->flags);
            j++;
            b++;
        } while (j < *pq);
    }
    // the tail (offsets per 0x4e94b0 / 0x4e9620): +0x50, +0x1c (int4), +0x54 (struct), the three ints at
    // +0x7c, +0x8f4, +0x958, the raw pair +0x6c/+0x78, +0x5ab8 (int1), +0x94/+0x8c8/+0x8cc (int4), then
    // the vectors at +0x8bc and +0x59f8, the struct-of-arrays block at +0x5a08 and the 8-byte vector +0x59d8.
    out->w_float(&angle);
    out->w_bool(&flag);
    out->w_tstr(&extra_text);
    stream_base* s3 = &out->w_float(&morale);
    s3 = &s3->w_float(&zoom);
    s3->w_float(&f84);
    out->w_float(&ai_patrol_radius);
    out->w_float(&ai_initiative);
    out->w_int(&in_sight);
    out->w_float(&bandit_attraction);
    out->w_i64(&ignore_timer.start);
    out->w_int(&banner_icon);
    out->w_int(&attached_to);
    out->w_bool(&engaged);
    SoaBlock* soa = &notes;
    write_vec4(attached, out);
    write_vec4(deferred, out);
    write_soa(out, soa);
    slots.write(out);
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
