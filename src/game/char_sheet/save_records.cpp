#include "./save_records.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4e9850    void GameRecord::write(OutStream* out)   (thiscall, ret 4)
void GameRecord::write(OutStream* out)
{
    char* p = (char*)this;
    f4f4.write8(out);
    for (int i = 0; i < 4; i++)
        out->w_raw(p + 0x58 + i * 4);
    for (int i = 0; i < 7; i++)
        out->w_int((int*)(p + 0x80 + i * 4));
    for (int i = 0; i < 6; i++)
        out->w_count((int*)(p + 0x68 + i * 4));
    out->w_int5((int*)(p + 0x30) + 0).w_raw(p + 0x3b8);
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
        out->w_int5((int*)r5);
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
//   `this` is the outer object: the record list (RecList) is at this+0xf8, the id looked up is this+0x78,
//   the party-record array is the pointer stored at this+0x13a00 and the 0x20-int block table is this+0x1405c.
void OutRecList::write_missions(OutStream* out)
{
    RecList* l = (RecList*)((char*)this + 0xf8);
    l->flush_ids();
    l->write_table(out);
    int idx = l->index_of(f78);
    Rec5ac8* r = &l->pages[idx >> 4][idx & 0xf];
    int i = 0;
    if (r->f8a8 > 0) {
        int* pk = this->pk;
        char* e = (char*)r + 0xc0;
        do {
            if (!(records[*(int*)(e - 0x14)].f30 & 0x10)) {
                out->w_int((int*)e);
                out->w_raw(e - 4);
                for (int k = 0; k < 0x20; k++)
                    out->w_raw(pk + k);
            }
            i++; e += 0x20; pk += 0x20;
        } while (i < r->f8a8);
    }
}

// @ 0x4e9c50    void RecList::write_table(OutStream* out)   (thiscall, ret 4)
void RecList::write_table(OutStream* out)
{
    char* self = (char*)this;
    out->w_raw((int*)(self + 0x138b8));
    out->w_raw((int*)(self + 0x138dc));
    int i = 0;
    int* pi = &i;
    while (i < count) {
        Rec5ac8* r = rec_at(this, i);
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

// @ 0x4e9d10    void Rec5ac8::write(OutStream* out)   (thiscall, ret 4)   the big record walk
void Rec5ac8::write(OutStream* out)
{
    char* p = (char*)this;
    out->w_struct(&f20);
    out->w_struct((char*)this + 0x0c);   // +0xc of the record body (the +0x20 struct is above it)
    out->w_int5((int*)(p + 0xa0));
    out->w_raw(p + 0x90);
    out->w_raw(p + 0x8c);
    out->w_raw(p + 0x70);
    out->w_raw(p + 0x74);
    out->w_raw(p + 0x8e4);
    out->w_raw(p + 0x918);
    out->w_raw(p + 0x8e8);
    out->w_raw(p + 0x91c);
    Quad& a0 = *(Quad*)(p + 0x8ec);
    int* pa = &a0[0];
    OutStream* sA = &out->w_int(pa);
    sA->w_int(&pa[1]);
    Quad& b0 = *(Quad*)(p + 0x924);
    int* pb = &b0[0];
    OutStream* sB = &out->w_int(pb);
    sB->w_int(&pb[1]);
    int* p3 = (int*)(p + 0x30);
    out->w_int(p3).w_int(p3 + 1).w_int(p3 + 2);
    int* pq = (int*)((char*)this + 0x8a8);
    out->w_raw(pq);
    int j = 0;
    if (*pq > 0) {
        char* b = (char*)this + 0xa8;
        do {
            out->w_raw(b + 4);
            out->w_raw(b);
            out->w_raw(b + 0x10);
            out->w_raw(b + 8);
            j++;
            b += 0x20;
        } while (j < *pq);
    }
    // the tail (offsets per 0x4e94b0 / 0x4e9620): +0x50, +0x1c (int4), +0x54 (struct), the three ints at
    // +0x7c, +0x8f4, +0x958, the raw pair +0x6c/+0x78, +0x5ab8 (int1), +0x94/+0x8c8/+0x8cc (int4), then
    // the vectors at +0x8bc and +0x59f8, the struct-of-arrays block at +0x5a08 and the 8-byte vector +0x59d8.
    out->w_int((int*)((char*)this + 0x50));
    out->w_int4((int*)((char*)this + 0x1c));
    out->w_struct((char*)this + 0x54);
    OutStream* s3 = &out->w_int((int*)((char*)this + 0x7c));
    s3 = &s3->w_int((int*)((char*)this + 0x80));
    s3->w_int((int*)((char*)this + 0x84));
    out->w_int((int*)((char*)this + 0x8f4));
    out->w_int((int*)((char*)this + 0x958));
    out->w_raw((char*)this + 0x6c);
    out->w_int((int*)((char*)this + 0x78));
    out->w_int1((int*)((char*)this + 0x5ab8));
    out->w_raw((char*)this + 0x94);
    out->w_raw((char*)this + 0x8c8);
    out->w_int4((int*)((char*)this + 0x8cc));
    write_vec4((List4*)((char*)this + 0x8bc), out);
    write_vec4((List4*)((char*)this + 0x59f8), out);
    write_soa(out, (char*)this + 0x5a08);
    f59d8.write8(out);
    return;
}

// @ 0x4e9fe0    void RecList::flush_ids()   (thiscall, ret 0)   drop the records whose type is 2
void RecList::flush_ids()
{
    unsigned i = 0;
    if (count > 0) {
        do {
            Rec5ac8* r = rec_at(this, i);
            if (r->type == 2)
                id_remove(r->id);
            i++;
        } while ((int)i < count);
    }
}
// @ 0x4ea030    void RecList::id_remove(int id)   (thiscall, ret 4)   the 0x2717 open-addressed set
//   The receiver (ecx) is the set itself -- RecList + 0x138bc. The record behind the slot is type=0'd and the
//   slot index is passed on to int_remove (0x440b30) with the set pointer.
void RecList::id_remove(int id)
{
    int* set = (int*)this;
    int  probe = 0;
    int  slot;
    if (id < 0x2717) {
        slot = id;
    } else {
        slot = id % 0x2717;
        while (set[slot * 2] != id) {
            if (probe >= 0x2717)
                break;
            slot = (slot + 0x4f) % 0x2717;
            probe++;
        }
    }
    if (set[slot * 2] == id) {
        int idx = set[slot * 2 + 1];
        set[slot * 2] = -1;
        set[slot * 2 + 1] = -1;
        rec_at(this, idx)->type = 0;
        ((IntVec*)((char*)this + 0x138bc))->push_back_out(idx);
    }
}
