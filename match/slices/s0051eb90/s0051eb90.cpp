// Slice s0051eb90 (batch ds1): the scene half of the save loader.
//
//   Scene::read (0x51eb90) is the counterpart of s004e8f10's save-side scene writer: it clears the
//   scene, grows the party page table until it holds the saved record count, then reads each record
//   through the 0x5ac8-byte reader, inserting live ids into the open-addressed id hash that occupies
//   the first 0x2717*8 bytes of the object (0x2717 is prime, so % 0x2717 is the bucket walk; the probe
//   step is 0x4f).  Dead slots are recorded in the party table's free index vector.
//   Scene::clear (0x51eda0) resets that hash and table, Scene::get (0x51ee50) turns an id into its
//   record, Rec5ac8::read (0x51ee80) reads one record.
//
//   The stream is the same reader object used by s0051e2b0: slot 0x38 reads one int and returns the
//   stream (chained reads reuse EAX as the next `this`), 0x3c/0x44 read 8-byte ints, 0x48 a count,
//   0x4c raw bytes and 0x54 a nested structure.
#pragma warning(disable:4530)
#include <new>
#include "rgl.h"

// ---- the binary reader (mirror of s0051e2b0's LoadStream) -----------------------------------------
struct LoadStream {
    virtual void s00() = 0;                              // +0x00
    virtual void s01() = 0;                              // +0x04
    virtual void s02() = 0;                              // +0x08
    virtual void s03() = 0;                              // +0x0c
    virtual void s04() = 0;                              // +0x10
    virtual void s05() = 0;                              // +0x14
    virtual void s06() = 0;                              // +0x18
    virtual void s07() = 0;                              // +0x1c
    virtual void s08() = 0;                              // +0x20
    virtual void s09() = 0;                              // +0x24
    virtual void s10() = 0;                              // +0x28
    virtual void s11() = 0;                              // +0x2c
    virtual void s12() = 0;                              // +0x30
    virtual void s13() = 0;                              // +0x34
    virtual LoadStream* r_int(int* v) = 0;               // +0x38  one int
    virtual void r_q1(void* v) = 0;                      // +0x3c  8-byte int
    virtual void r_q2(void* v) = 0;                      // +0x40  8-byte int
    virtual LoadStream* r_q3(void* v) = 0;               // +0x44  8-byte int
    virtual void r_cnt(void* v) = 0;                     // +0x48  an 8-byte pair/count
    virtual LoadStream* r_raw(void* v) = 0;              // +0x4c  raw bytes
    virtual void s14() = 0;                              // +0x50
    virtual void r_obj(void* v) = 0;                     // +0x54  nested structure
};

// ---- engine services that live in other slices ---------------------------------------------------
void __cdecl read_soa(LoadStream* s, char* base);        // 0x51e1f0  (s0051d920)
void __cdecl free(void* p);                              // 0x535712

struct RglStr {                                          // rgl string: data at +4, length at +0xc
    int   pad0;                                          // +0x0
    char* data;                                          // +0x4
    int   pad8;                                          // +0x8
    int   len;                                           // +0xc
    int   fix();                                         // 0x42bf80  '_' -> ' ', returns length
};

// ---- containers (leading pad dword, then _Myfirst/_Mylast/_Myend) --------------------------------
struct IntVec {                                          // vector<int>
    int  pad0;                                           // +0x0
    int* _Myfirst;                                       // +0x4
    int* _Mylast;                                        // +0x8
    int* _Myend;                                         // +0xc
    void resize(unsigned n, int v);                      // 0x43f310
    void insert_n(int* where, unsigned count, const int* val);          // 0x43e7b0
    void push_back(int v)
    {
        unsigned size;
        if (_Myfirst == 0)
            size = 0;
        else
            size = (unsigned)(_Mylast - _Myfirst);
        if (_Myfirst && size < (unsigned)(_Myend - _Myfirst)) {
            int* last = _Mylast;
            *last = v;
            _Mylast = last + 1;
        } else {
            insert_n(_Mylast, 1, &v);
        }
    }
};

struct List8 {                                           // 8-byte element vector (party slots at +0x59d8)
    int   pad0;                                          // +0x0
    void* _Myfirst;                                      // +0x4
    void* _Mylast;                                       // +0x8
    void* _Myend;                                        // +0xc
    void  resize_e(unsigned n, int a, int b);            // 0x493090
};

struct PtrVec {                                          // vector<char*>: the party page array
    int    pad0;                                         // +0x0
    char** _Myfirst;                                     // +0x4
    char** _Mylast;                                      // +0x8
    char** _Myend;                                       // +0xc
};

struct Rec5ac8 {                                         // 0x5ac8-byte scene party record
    char data[0x5ac8];
    __declspec(noinline) void read(LoadStream* s, int flag);   // 0x51ee80
};

struct HashEntry {                                       // Scene id hash slot
    int key;                                             // +0x0  party id, -1 when empty
    int value;                                           // +0x4  record index
};

struct PartyTable {                                      // Scene's party pool (s0049e780 layout)
    int      count;                                      // +0x00
    IntVec   free_ids;                                   // +0x04
    PtrVec   pages;                                      // +0x14
};

struct Scene {                                           // the scene embedded in Game at +0xf8
    HashEntry hash[0x2717];                              // +0x0000  id -> record index
    PartyTable parties;                                  // +0x138b8
    int        f138dc;                                   // +0x138dc  last id read
    int  lookup(int id);                                 // 0x48ad00
    void add_page();                                     // 0x493830
    __declspec(noinline) void clear();                   // 0x51eda0
    __declspec(noinline) void read(LoadStream* s, int flag);   // 0x51eb90
    __declspec(noinline) Rec5ac8* get(int id);           // 0x51ee50
};

// -------------------------------------------------------------------------------------------------
// @ 0x51eb90    void Scene::read(LoadStream* s, int flag)                     (ret 8)
void Scene::read(LoadStream* s, int flag)
{
    this->clear();
    int n;
    s->r_raw(&n);
    int last;
    s->r_raw(&last);
    while (this->parties.count < n) {
        this->add_page();
        if (this->parties.free_ids._Myfirst)
            free(this->parties.free_ids._Myfirst);
        this->parties.free_ids._Myfirst = 0;
        this->parties.free_ids._Mylast = 0;
        this->parties.free_ids._Myend = 0;
    }
    for (int i = 0; i < n; i++) {
        char* rec = (char*)this->parties.pages._Myfirst[(unsigned)i >> 4] + (i & 0xf) * 0x5ac8;
        int live;
        s->r_raw(&live);
        if (live != 0) {
            int index;
            s->r_raw(&index);
            int id;
            s->r_raw(&id);
            *(int*)(rec + 4) = 1;
            int key = id % 0x2717;
            if (this->hash[key].key != -1) {
                do {
                    key = (key + 0x4f) % 0x2717;
                } while (this->hash[key].key != -1);
            }
            this->hash[key].value = index;
            this->hash[key].key = id;
            *(int*)(rec + 8) = id;
            ((Rec5ac8*)rec)->read(s, flag);
        } else {
            *(int*)(rec + 4) = 0;
        }
    }
    IntVec* ids = &this->parties.free_ids;
    if (ids->_Myfirst)
        free(ids->_Myfirst);
    ids->_Myfirst = 0;
    ids->_Mylast = 0;
    ids->_Myend = 0;
    for (int i = 0; i < this->parties.count; i++) {
        char* rec = (char*)this->parties.pages._Myfirst[(unsigned)i >> 4] + (i & 0xf) * 0x5ac8;
        if (*(int*)(rec + 4) != 1)
            ids->push_back(i);
    }
    this->f138dc = last;
}

// @ 0x51eda0    void Scene::clear()                                           (ret 0)
void Scene::clear()
{
    for (int i = 0; i < 0x2717; i++) {
        this->hash[i].key = -1;
        this->hash[i].value = -1;
    }
    this->parties.free_ids.resize(0, 0);
    for (int i = 0; i < this->parties.count; i++) {
        *(int*)((char*)this->get(i) + 4) = 0;
        this->parties.free_ids.push_back(i);
    }
    this->f138dc = 0;
}

// @ 0x51ee50    Rec5ac8* Scene::get(int id)                                   (ret 4)
Rec5ac8* Scene::get(int id)
{
    int k = this->lookup(id);
    return (Rec5ac8*)((k & 0xf) * 0x5ac8 + (int)this->parties.pages._Myfirst[k >> 4]);
}

// @ 0x51ee80    void Rec5ac8::read(LoadStream* s, int flag)                   (ret 8)
void Rec5ac8::read(LoadStream* s, int flag)
{
    s->r_obj(data + 0x20);
    s->r_obj(data + 0xc);
    ((RglStr*)(data + 0xc))->fix();
    s->r_q3(data + 0xa0);
    s->r_raw(data + 0x90);
    s->r_raw(data + 0x8c);
    s->r_raw(data + 0x70);
    s->r_raw(data + 0x74);
    s->r_raw(data + 0x8e4);
    s->r_raw(data + 0x918);
    s->r_raw(data + 0x8e8);
    s->r_raw(data + 0x91c);
    int* p = (int*)(data + 0x8ec);
    LoadStream* rs = s->r_int(p);
    rs->r_int(++p);
    p = (int*)(data + 0x924);
    rs = s->r_int(p);
    rs->r_int(++p);
    p = (int*)(data + 0x30);
    rs = s->r_int(p);
    rs = rs->r_int(p + 1);
    rs->r_int(p + 2);
    s->r_raw(data + 0x8a8);
    char* e = data + 0xa8;
    for (int i = 0; i < *(int*)(data + 0x8a8); i++, e += 0x20) {
        s->r_raw(e + 4);
        s->r_raw(e);
        s->r_raw(e + 0x10);
        s->r_raw(e + 8);
    }
    s->r_int((int*)(data + 0x50));
    if (flag > 0) {
        s->r_q1(data + 0x1c);
        s->r_obj(data + 0x54);
        p = (int*)(data + 0x7c);
        rs = s->r_int(p);
        rs = rs->r_int((int*)(data + 0x80));
        rs->r_int((int*)(data + 0x84));
        s->r_int((int*)(data + 0x8f4));
        s->r_int((int*)(data + 0x958));
        s->r_raw(data + 0x6c);
        s->r_int((int*)(data + 0x78));
        s->r_cnt(data + 0x5ab8);
        s->r_raw(data + 0x94);
        s->r_raw(data + 0x8c8);
        s->r_q1(data + 0x8cc);
        int n1;
        s->r_raw(&n1);
        ((IntVec*)(data + 0x8b8))->resize(n1, 0);
        for (int i = 0; i < n1; i++)
            s->r_raw(*(int**)(data + 0x8bc) + i);
        int n2;
        s->r_raw(&n2);
        ((IntVec*)(data + 0x59f4))->resize(n2, 0);
        for (int i = 0; i < n2; i++)
            s->r_raw(*(int**)(data + 0x59f8) + i);
        data[0x5a04] = 1;
        read_soa(s, data + 0x5a08);
        int n3;
        s->r_raw(&n3);
        List8* slots = (List8*)(data + 0x59d8);
        slots->resize_e(n3, 0, 0);
        for (int i = 0; i < n3; i++)
            s->r_cnt(*(char**)((char*)slots + 4) + i * 8);
    } else {
        *(float*)(data + 0x78) = (float)*(unsigned char*)(data + 0xa6) * *(float*)0x59bfec;
    }
}
