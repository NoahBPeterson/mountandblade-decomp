#include "./save_records.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../out_stream.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4e9850    void Troop::write_save(OutStream* out)  (the slice called the troop record GameRecord)   (thiscall, ret 4)
void Troop::write_save(OutStream* out)
{
    char* p = (char*)this;
    saved.write8(out);
    for (int i = 0; i < 4; i++)
        out->w_raw(p + 0x58 + i * 4);
    for (int i = 0; i < 7; i++)
        out->w_int((int*)(p + 0x80 + i * 4));
    for (int i = 0; i < 6; i++)
        out->w_count((int*)(p + 0x68 + i * 4));
    out->w_int8((int*)(p + 0x30) + 0).w_raw(p + 0x3b8);
    out->w_raw(p + 0x3b4).w_raw(p + 0x9c);
    out->w_int((int*)(p + 0xa0));
    out->w_raw(p + 0x4c);
    out->w_raw(p + 0xa4).w_raw(p + 0xa8).w_raw(p + 0xac);
    out->w_raw(p + 0xb0);
    ((cPairs96*)(p + 0xb4))->write(out);
    char* q = p + 0x4a4;
    for (int i = 0; i < 10; i++, q += 8) {
        out->w_raw(q);
        out->w_raw(q + 4);
    }
    char* r5 = p + 0x3c0;
    for (int i = 0; i < 4; i++, r5 += 8)
        out->w_int8((int*)r5);
    write_soa(out, p + 0x400);
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

// @ 0x4e99d0    void Table::write_table(OutStream* out)   (thiscall, ret 4)   the sparse page table
void Table::write_table(OutStream* out)
{
    int last = -1;
    int n = count;
    for (int i = 0; i < n; i++) {
        Entry* e = pages[(unsigned)i >> 4] + (i & 0xf);
        if (e->f04)
            last = i;
    }
    int w = last + 1;
    out->w_raw(&w);
    out->w_raw((int*)((char*)this + 0x24));
    int k = 0;
    while (k <= last) {
        Entry* e = pages[(unsigned)k >> 4] + (k & 0xf);
        if (e->f04) {
            int one = 1;
            out->w_raw(&one);
            out->w_raw(&e->f0c);
            e->write(out);
        } else {
            int zero = 0;
            out->w_raw(&zero);
        }
        k++;
    }
}

// @ 0x4e9ab0    void Entry::write(OutStream* out)   (thiscall, ret 4)
//   three chained pairs at +0x24, +0x2c and +0x34 (each: slot 0xc, then slot 0xc on its result at +4).
void Entry::write(OutStream* out)
{
    out->w_struct(&f14);
    out->w_raw(&f10);
    int* p;
    int* q;
    p = &fa3;
    OutStream* s = &out->w_int(p);
    q = p + 1;
    s->w_int(q);
    p = &fa5;
    s = &out->w_int(p);
    q = p + 1;
    s->w_int(q);
    p = &fa7;
    s = &out->w_int(p);
    q = p + 1;
    s->w_int(q);
    out->w_raw(&f54);
    out->w_raw(&f58);
    out->w_int1(&f68);
    out->w_int(&f70);
}

// @ 0x4e9b50    void OutRecList::write_missions(OutStream* out)   (thiscall, ret 4)
//   `this` is the outer object: the party manager (PartyMgr) is at this+0xf8, the id looked up is this+0x78,
//   the party-record array is the pointer stored at this+0x13a00 and the 0x20-int block table is this+0x1405c.
void OutRecList::write_missions(OutStream* out)
{
    PartyMgr* l = (PartyMgr*)((char*)this + 0xf8);
    l->flush_ids();
    l->write_table(out);
    int idx = l->lookup(f78);
    Party* r = &l->pages[idx >> 4][idx & 0xf];
    int i = 0;
    if (r->nstacks > 0) {
        int* pk = this->pk;
        Stack* e = r->stacks;
        do {
            if (!(records[e->troop].flags30 & 0x10)) {
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
    out->w_raw(&n_parties);
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
    out->w_raw(&f90);
    out->w_raw(&owner);
    out->w_raw(&f70);
    out->w_raw(&f74);
    out->w_raw(&state);
    out->w_raw(&action);
    out->w_raw(&state_target);
    out->w_raw(&target);
    int* pa = &state_f8ec;
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
    OutStream* s3 = &out->w_int((int*)&f7c);
    s3 = &s3->w_int((int*)&zoom);
    s3->w_int((int*)&f84);
    out->w_int((int*)&f8f4);
    out->w_int((int*)&f958);
    out->w_raw(&f6c);
    out->w_int((int*)&f78);
    out->w_int1((int*)&ignore_timer.start);
    out->w_raw(&f94);
    out->w_raw(&f8c8);
    out->w_int4((int*)&f8cc);
    char* soa = (char*)note_times;
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
