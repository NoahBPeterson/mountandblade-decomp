// Slice s004d3720: init/destroy of a large world-record object (0x4d3880, 0x4d3720), a
// "restore defaults" routine (0x4d3a50), a hash-table node insertion (0x4d3c00) and two
// range-destructor wrappers (0x4d3e50, 0x4d3e70). Names provisional.
#include <windows.h>
#include <new>
#include "rgl.h"

// ---- engine string (16 bytes) ---------------------------------------------------------------------------
struct tstring {
    int pos;                // +0
    char* buf;              // +4
    int cap;                // +8
    int len;                // +0xc
    tstring();                                      // 0x42bbd0
    tstring(const char* s);                         // 0x42bbe0
    ~tstring();                                     // 0x42bc40
    void operator=(const tstring& o);               // 0x42bc50
    void clear();                                   // 0x42be50
};
void tstring_dtor_at(void* p);                      // 0x42bc40

struct Tail {                                       // 0x10-byte string tail record
    int a, b, c, d;
    ~Tail();                                        // 0x42d390
};

struct Rec18 {                                      // 0x18-byte record with a tstring-style dtor
    char pad[0x18];
    ~Rec18();                                       // 0x42bc40
};

// ---- callees -------------------------------------------------------------------------------------------
void  op_delete(void* p);                           // 0x535712  ??3@YAXPAX@Z
void  destroy_4727a0(void* first, void* last, void* self, void* alloc);   // 0x4727a0
void  destroy_4ca7e0(void* first, void* last, void* self, void* alloc);   // 0x4ca7e0
void  hash_alloc(int n, void* sentinel);            // 0x4cd010
void* hash_new_node(void* node, void* hint, void* key);                   // 0x4c4e50
void  node_free_hint(void* self, int one);          // 0x4cb6f0
void  destroy_4cd570(void* first, void* last, void* self, void* alloc);   // 0x4cd570
void* copy_4cd620(void* first, void* last, void* self, void* out);        // 0x4cd620
// 0x4c4e50 is a same-TU static in the original (the caller keeps ebx/ebp across it); declare it as one so our
// build keeps the same live registers. Its argument order is the original's (node, hint, key) with an unused 4th.
__declspec(noinline) void* list_int_make_node(void* node, void* hint, void* key, void* pad);   // 0x4c4e50
void  block_4c2d10(void* p);                        // 0x4c2d10 (0x130-byte block ctor)
struct Block {
    char pad[0x130];
    void init();                                    // 0x4c2d10
};
void  sub_4be6b0(void* p);                          // 0x4be6b0
void  sub_4d1360(void* p);                          // 0x4d1360
struct Sub63d0 {
    char pad[0x667c - 0x63d0];
    void init();                                    // 0x4be6b0
};
struct Sub6698 {
    char pad[0x67d8 - 0x6698];
    void init();                                    // 0x4d1360
};
struct TStr {
    void ctor();                                    // 0x42bbd0
    ~TStr();                                        // 0x42bc40
};
void  ui_open_close(int flag);                      // 0x494260
struct StrTab { const tstring* lookup(const tstring& s); };   // 0x43a1e0
struct UiObj { void open(int flag); };                         // 0x494260

struct Rec34 {
    char pad[0x10];
    void vecdel(unsigned flags);                    // 0x401870  ??_ERec34@@QAEPAXI@Z
};

struct Vec3 {                                      // allocator dword, begin/last/end
    int  alloc;
    int* first;
    int* last;
    int* end;
    void clear(void* a) {
        if (first) {
            destroy_4727a0(first, last, this, a);
            op_delete(first);
        }
        first = 0; last = 0; end = 0;
    }
};
struct VecP {                                      // trivial elements: only the buffer is freed
    int  alloc;
    void* first;
    void* last;
    void* end;
    void clear(void* a) {
        if (first) op_delete(first);
        first = 0; last = 0; end = 0;
    }
};
struct VecB {                                      // 0xb0-byte rows, destroyed by 0x4ca7e0
    int  alloc;
    int* first;
    int* last;
    int* end;
    void clear(void* a) {
        if (first) {
            destroy_4ca7e0(first, last, this, a);
            op_delete(first);
        }
        first = 0; last = 0; end = 0;
    }
};

// ---- globals -------------------------------------------------------------------------------------------
extern int g_5e2958;                                // 0x5e2958
extern int g_5e295c;                                // 0x5e295c
extern int g_5a9f18;                                // 0x5a9f18
struct GameL;
extern GameL* g_game;                               // 0x5e2930
extern char* g_world;                               // 0x5e292c
extern char* g_ui;                                  // 0xa73980

// ---- the large record (all members are nominal; accesses are by offset) --------------------------------
struct Big {
    int  f000; int f004; int f008; int f00c;
    int  f010; int f014; int f018; int f01c;
    int  f020; int f024; int f028; int f02c;
    int  f030; int f034; int f038; int f03c;
    Rec34* f040;                                    // +0x40
    Rec18* f044;                                    // +0x44
    Vec3 v48;                                       // +0x48
    Vec3 v58;                                       // +0x58
    Vec3 v68;                                       // +0x68
    VecP v78;                                       // +0x78
    char pad088[0x19c - 0x88];
    Tail f19c;
    Tail f1ac;
    VecB v1bc;                                      // +0x1bc
    char pad1cc[0x670 - 0x1cc];
    char blocks[0x40 * 0x130];                      // +0x670 .. +0x5270
    char pad5270[0x5de0 - 0x5270];
    int  f5de0;
    char pad5de4[4];
    int  f5de8;
    int  f5dec;
    char pad5df0[0x5e00 - 0x5df0];
    int  f5e00; int f5e04; int f5e08;
    char pad5e0c[0x62e8 - 0x5e0c];
    int  f62e8;
    char pad62ec[4];
    int  f62f0; int f62f4; int f62f8;
    char pad62fc[4];
    int  f6300; int f6304; int f6308;
    char pad630c[4];
    int  f6310; int f6314; int f6318;
    char pad631c[4];
    int  f6320; int f6324; int f6328;
    char pad632c[4];
    int  f6330; int f6334;
    char pad6338[0x63c4 - 0x6338];
    int  f63c4; int f63c8; int f63cc;
    Sub63d0 f63d0;
    int  f667c; int f6680; int f6684;
    char pad6688[4];
    int  f668c; int f6690; int f6694;
    Sub6698 f6698;
    int  f67d8;
    char pad67dc[4];
    int  f67e0; int f67e4;
    char pad67e8[0x6800 - 0x67e8];
    int  f6800;
    char pad6804[4];
    int  f6808; int f680c;
    char pad6810[4];
    char f6814[0x10];                               // +0x6814 (a tstring)
    Big* init();                                    // 0x4d3880
    void destroy();                                 // 0x4d3720
};

// @ 0x4d3880
Big* Big::init()
{
    Big* p = this;
    p->f00c = 0; p->f010 = 0; p->f014 = 0;
    p->f01c = 0; p->f020 = 0; p->f024 = 0;
    p->f004 = 0;
    p->f028 = 0; p->f02c = 0;
    p->f038 = 0; p->f03c = 0; p->f040 = 0;
    p->v48.alloc = 0; p->v48.first = 0; p->v48.last = 0;
    p->f030 = 0;
    p->v48.end = 0; p->v58.alloc = 0;
    for (int i = 0; i < 0x40; i++)
        ((Block*)(p->blocks + i * 0x130))->init();

    p->f5de0 = 2;
    p->f5de8 = g_5e2958;
    p->f5dec = g_5e295c;
    p->f5e00 = 0; p->f5e04 = 0; p->f5e08 = 0;

    p->f62e8 = 2;
    p->f62f0 = g_5e2958;
    p->f62f4 = g_5e295c;
    p->f62f8 = 2;
    p->f6300 = g_5e2958;
    p->f6304 = g_5e295c;
    p->f6308 = 2;
    p->f6310 = g_5e2958;
    p->f6314 = g_5e295c;
    p->f6318 = 2;
    p->f6320 = g_5e2958;
    p->f6324 = g_5e295c;
    p->f6328 = 2;
    p->f6330 = g_5e2958;
    p->f6334 = g_5e295c;
    p->f63c4 = 0; p->f63c8 = 0; p->f63cc = 0;
    p->f63d0.init();

    p->f667c = 0; p->f6680 = 0; p->f6684 = 0;
    p->f668c = 0; p->f6690 = 0; p->f6694 = 0;
    p->f6698.init();

    p->f67d8 = 2;
    p->f67e0 = g_5e2958;
    p->f67e4 = g_5e295c;
    p->f6800 = 2;
    p->f6808 = g_5e2958;
    p->f680c = g_5e295c;

    ((TStr*)p->f6814)->ctor();
    p->f000 = 0;
    *(int*)((char*)p + 0x5270) = 0;
    return this;
}

// @ 0x4d3720
void Big::destroy()
{
    if (f040)
        f040->vecdel(3);
    delete[] f044;

    v1bc.clear(this);

    f1ac.~Tail();
    f19c.~Tail();

    v78.clear(this);
    v68.clear(this);
    v58.clear(this);
    v48.clear(this);

    ((TStr*)&f028)->~TStr();
    ((TStr*)this)->~TStr();
}

// @ 0x4d3a50
struct GameL {
    char pad0[0x1a090];
    int f1a090;
    int f1a094;
    int f1a098;
    char pad1a09c[0x1a0b8 - 0x1a09c];
    int f1a0b8, f1a0bc, f1a0c0, f1a0c4;
    char pad1a0c8[0x1a1cc - 0x1a0c8];
    int f1a1cc;
};

void restore_defaults()
{
    g_game->f1a090 = 0;
    g_game->f1a098 = 0;
    g_game->f1a0b8 = -1;
    g_game->f1a0bc = -1;
    g_game->f1a0c0 = -1;
    g_game->f1a0c4 = -1;

    *(char*)(*(int*)(g_world + 0x65d0) + 0x140) = 0;
    *(char*)(*(int*)(g_world + 0x65dc) + 0x140) = 0;
    *(char*)(*(int*)(g_world + 0x65e4) + 0x140) = 0;
    if (*(int*)(g_world + 0x6738)) *(char*)(*(int*)(g_world + 0x6738) + 0x140) = 0;
    if (*(int*)(g_world + 0x6740)) *(char*)(*(int*)(g_world + 0x6740) + 0x140) = 0;

    if (g_game->f1a1cc) {
        g_game->f1a1cc = 0;
        g_5a9f18 = -1;
        {
            tstring s1("ui_no_discard_changes");
            tstring s2("ui_yes_save");
            tstring s3("ui_question_save_changes");
            const tstring* r1 = ((StrTab*)0x5e2a18)->lookup(s1);
            const tstring* r2 = ((StrTab*)0x5e2a18)->lookup(s2);
            const tstring* r3 = ((StrTab*)0x5e2a18)->lookup(s3);
            char* ui = g_ui;
            ((tstring*)(ui + 0x68))->clear();
            *(tstring*)(ui + 0x1c) = *r3;
            *(tstring*)(ui + 0x78) = *r2;
            *(tstring*)(ui + 0x88) = *r1;
            *(int*)(ui + 0x4c) = (int)&g_5a9f18;
            *(int*)(ui + 0x50) = 0;
            *(int*)(ui + 0x4) = 1;
        }
        ((UiObj*)0xa73980)->open(0);
        g_game->f1a090 = 0;
    }
}

// ---- hash table ----------------------------------------------------------------------------------------
// The container helpers below take (first, last, owner, alloc). cl packs the two trailing pointer-sized
// args: the owner arg rides in ECX (still holding `this` at the call) -- see 0x4d3e50/0x4d3e70.
void range4(void* a, void* b, void* c, void* d);    // 0x4cd570
struct HNode {
    HNode* next;                                    // +0
    HNode* prev;                                    // +4
    unsigned key;                                   // +8
};
struct HOut { HNode* node; char inserted; };
struct NodePool {                                   // lives at HashTab+4
    char pad0[4];
    HNode* sentinel;                                // +4 (HashTab+8)
    HNode* copy_node(HNode* n, HNode* hint, unsigned* key);   // 0x4c4e50
    void   reserve(int one);                        // 0x4cb6f0
};
struct HashTab {
    int      f0;
    NodePool pool;                                  // +4
    int      f0c;                                   // +0xc
    char*    f10;                                   // +0x10
    HNode**  f14;                                   // +0x14
    HNode**  f18;                                   // +0x18
    HNode**  f1c;                                   // +0x1c
    unsigned f20;                                   // +0x20
    unsigned f24;                                   // +0x24
    void insert(HOut* out, unsigned* key);          // 0x4d3c00
};

// @ 0x4d3c00
void HashTab::insert(HOut* out, unsigned* key)
{
    unsigned count = f24;
    unsigned n = f14 ? (unsigned)(((char*)f18 - (char*)f14) >> 2) : 0u;
    if (f24 <= (unsigned)(f0c >> 2)) {
        if (count < n - 1) {
            if (f20 < count)
                f20 = f20 * 2 + 1;
        } else {
            f20 = n * 2 - 3;
            hash_alloc(n * 2 - 1, pool.sentinel);
        }
        unsigned idx = count - (f20 >> 1) - 1;
        HNode* p = f14[idx];
        if (p != f14[idx + 1]) {
            HNode* q;
            do {
                if (((p->key ^ 0xdeadbeef) & f20) == idx) {
                    q = p->next;
                } else {
                    q = p->next;
                    if (q != pool.sentinel) {
                        HNode* b = f14[idx];
                        unsigned u = idx;
                        while (p == b) {
                            f14[u] = q;
                            if (u == 0)
                                break;
                            u = u - 1;
                            b = f14[u];
                        }
                        HNode* s = pool.sentinel;
                        *(HNode**)p->prev = q;
                        *(HNode**)q->prev = s;
                        *(HNode**)s->prev = p;
                        HNode* t = s->prev;
                        s->prev = q->prev;
                        q->prev = p->prev;
                        p->prev = t;
                        p = s->prev;
                        f14[f24 + 1] = s;
                    }
                    unsigned u2;
                    for (u2 = f24; idx < u2 && f14[u2] == pool.sentinel; u2 = u2 - 1)
                        f14[u2] = p;
                    if (q == pool.sentinel)
                        break;
                }
                p = q;
            } while (q != f14[idx + 1]);
        }
        f24 = f24 + 1;
    }

    unsigned k = *key;
    unsigned i = (k ^ 0xdeadbeef) & f20;
    if (f24 <= i)
        i = i + (-1 - (f20 >> 1));
    HNode* p = f14[i + 1];
    if (p != f14[i]) {
        do {
            p = p->prev;
            if ((int)p->key <= (int)k) {
                if ((int)k <= (int)p->key) {
                    out->node = p;
                    out->inserted = 0;
                    return;
                }
                p = p->next;
                break;
            }
        } while (p != f14[i]);
    }
    HNode* nn = (HNode*)list_int_make_node(p, p->prev, key, 0);
    pool.reserve(1);
    p->prev = nn;
    *(HNode**)nn->prev = nn;
    HNode* q = (HNode*)p->prev;
    if (p == f14[i]) {
        do {
            f14[i] = q;
            if (i == 0)
                break;
            i = i - 1;
        } while (p != f14[i]);
    }
    out->node = q;
    out->inserted = 1;
}

struct TreeW {
    char pad0[8];
    void* f8;                                       // +0x08
    void erase(void* p1, void* p2);                 // 0x4d3e50
    void insert(void** out, void* a, void* b);      // 0x4d3e70
};

// @ 0x4d3e50
void TreeW::erase(void* p1, void* p2)
{
    range4(p1, p2, this, p2);
}

// @ 0x4d3e70
void TreeW::insert(void** out, void* a, void* b)
{
    if (a != b) {
        void* p = copy_4cd620(b, f8, a, out);
        destroy_4ca7e0(p, f8, this, out);
        f8 = p;
    }
    *out = a;
}
