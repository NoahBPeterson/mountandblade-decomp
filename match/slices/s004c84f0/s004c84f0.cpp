// Slice s004c84f0: the scene-layout/tableau group of the mission module -- the tableau code table (0x5a9c98)
// mapping (0x4c84f0), the layout record destructor (0x4c85e0), the layout -> scene-graph builder (0x4c8630),
// the loader thread body (0x4c8900) with its wait/critical-section globals (0x5e28d8/0x5e28dc/0x5e2a0c),
// the layout array deleting destructor (0x4c8980), container destructors (0x4c8a40/0x4c8ab0/0x4c8ba0/0x4c8bf0)
// and a scene-node zeroing helper (0x4c8c20). Also two uninitialised-fill helpers (0x4c8810, 0x4c88e0) that
// wrap the range fillers 0x4c5010/0x4c5080.
// Every layout below was recovered from offsets alone; names are provisional. Flags: default /O2 /MT /Gy /TP.
#include <windows.h>
#include <stddef.h>
#include <new>
#include "rgl.h"

using rgl::vector4;

// ---- engine strings (0x42bc40 dtor, 0x42bd10 equals a C string) -----------------------------------------
struct tstring {
    int pos;            // +0
    char* buf;          // +4  (null while empty)
    int cap;            // +8
    int len;            // +0xc
    ~tstring() throw();                                      // 0x42bc40
    char equals(const char* s) const throw();                // 0x42bd10
};

// Text log stream (global 0x5a9f60); slot 10 (w_cstr) is used here.
struct stream_base {
    virtual void reset();
    virtual void rewind();
    virtual stream_base& w_stream(void* o);
    virtual stream_base& w_float(void* v);
    virtual stream_base& w_bool(void* v);
    virtual stream_base& w_u64(void* v);
    virtual stream_base& w_i64(void* v);
    virtual stream_base& w_uint(void* v);
    virtual stream_base& w_int(int* v);
    virtual stream_base& w_byte(void* v);
    virtual stream_base& w_cstr(const char* s);              // 10
    stream_base& operator<<(const char* s) { return w_cstr(s); }
};
extern stream_base g_log;                                    // 0x5a9f60

// ---- the tableau-code names (0x5a9c98; three const char* in a row) ------------------------------------
extern const char* g_conv_names[3];                          // 0x5a9c98
extern const char g_empty[];                                 // 0x54b118 (an empty string literal)

// ---- the layout record (conv_rec) and its pieces -------------------------------------------------------
// +0x1c is a std::vector-like triple of pointers into an array of 0x68-byte entries.
struct ConvItem16 {                                          // a 0x68-byte entry (tstring first, data at +0x10)
    tstring name;                                            // +0
    char pad10[0x94];
    int a4;                                                  // +0xa4
    int a8;                                                  // +0xa8
    int ac;                                                  // +0xac
};
struct ConvOwner {
    char pad0[4];
    ConvItem16* begin;                                       // +4
    ConvItem16* end;                                         // +8
    int count() { return begin ? (int)((char*)end - (char*)begin) / 0x68 : 0; }
};
int conv_count(ConvOwner* o) { return o->count(); }

// 0x2c-byte vertex-editor record of the +0x1c vector (layout from s004c5080).
struct grid_slot_rec;
struct edit_vert;
struct conv_rec {
    char pad0[0x1c];
    edit_vert* begin;                                        // +0x1c  (vector<edit_vert>-style triple)
    edit_vert* end;                                          // +0x20
    char pad24[0xc3b0dc - 0x24];
};

// 0x24 bytes: a scene-graph vertex.
struct scene_vert {
    vector4 pos;                                             // +0
    int f10;                                                 // +0x10
};

// a variable of the map, as a grid of 4-dword vertices
struct map_var {
    vector4 min;                                             // +0
    vector4 max;                                             // +0x10
    int n;                                                   // +0x20
    int pad;                                                 // +0x24
};

// 0x94-byte records: one cell of the tableau grid (0x25 dwords): six ints, then a 0x911c-byte row.
struct grid_slot {
    int f0;                                                  // +0   (code stored in the scene cell)
    vector4 v;                                               // +4   (map vertex; read at slot-0x14 in the row below)
    int f14;                                                 // +0x14 (scene vertex index, < 0 = none)
    int f18;                                                 // +0x18
    // one 0x911c-byte row follows at +0x18
};
struct grid_row {                                            // 0x911c bytes per row
    char cell[0x9110];
    map_var var;                                             // +0x9110
};

// 0x88-byte record of the +0xb0 sub-object; +0x84 points at the vertex array for its n (+0) indices (+4).
struct var_rec {
    int n;                                                   // +0
    int idx[32];                                             // +4
    char pad84[0x84];
    scene_vert* verts;                                       // +0x84
};

struct matrix3 {                                             // 3x4 = 0x30 bytes
    float m[3][4];
};
matrix3* __cdecl identity3(matrix3* out);                    // 0x40cd60

// the scene graph global (at +0xc3b0dc of the layout record)
struct scene_sub {
    char pad0[4];
    scene_vert* verts;                                       // +4
    char pad8[0xc];
    var_rec* first;                                          // +0x14
    var_rec* last;                                           // +0x18
};
struct world_root {
    char pad0[0x64];
    int** codes;                                             // +0x64 (int* codes[n]; each points at a 16-byte cell)
    char pad68[0x48];
    scene_sub* sub;                                          // +0xb0
};

struct item38;                                               // a 0x38-byte record (element of the 0x4c5080 sequences)

struct PhysSet {
    char pad[4];
    void update(float dt);                                   // 0x472dc0
};
extern PhysSet g_phys;                                       // 0xa181f8

extern char g_loading;                                       // 0x5a9c38 (loader thread runs while set)
extern HANDLE g_load_event;                                  // 0x5e28d8
extern CRITICAL_SECTION g_load_cs;                           // 0x5e28dc
extern float g_load_dt;                                      // 0x5e2a0c (pending physics step)
extern void* g_conv_scratch;                                 // 0x5e299c

// the +0x2c-byte record of the +0x1c vector
struct edit_vert {
    vector4 pos;                                             // +0
    vector4 nrm;                                             // +0x10
    char used;                                               // +0x20
    char pad21[3];
    int flag;                                                // +0x24
    char pad28[4];
};


extern void uninit_fill_n(edit_vert* dst, unsigned n, const edit_vert* src, conv_rec* self, const edit_vert* src2);  // 0x4c5010
extern void conv_remove_child(conv_rec* self, int a, int b);     // 0x4c57d0
extern void conv_remove_child2(conv_rec* self, int a, int b);    // 0x4c58c0
extern void conv_pop_slot(conv_rec* self, int a);                // 0x4c59c0
extern void conv_reset_groups(conv_rec* self);                   // 0x4c8330
extern tstring* assign_range(tstring* first, tstring* last, tstring* dst, tstring* dst2);  // 0x4331e0 (self, first, last)
extern void conv_copy_range(conv_rec* self, void* src_first, void* src_last, void* dst);    // 0x4c5080
extern item38* copy_range38(item38* dst, const item38* first, const item38* last, conv_rec* self, const item38* last2);  // 0x4c5080
extern void conv_free_one(conv_rec* self);                       // 0x4c85e0

// @ 0x4c84f0
void conv_map_codes(conv_rec* self)
{
    char* p = (char*)self;
    const char** names = g_conv_names;
    int* out = (int*)(p + 0x24);
    do {
        ConvItem16* begin = *(ConvItem16**)(p + 4);
        ConvItem16* end = *(ConvItem16**)(p + 8);
        const char* nm = *names;
        int found = -1;
        unsigned i = 0;
        do {
            if (begin == 0 || i >= (unsigned)((int)((char*)end - (char*)begin) / 0x68)) {
                g_log << "WARNING: UNABLE TO MAP GAME TABLEAU CODE: " << nm << g_empty;
                break;
            }
            if (((ConvItem16*)((char*)begin + i * 0x68))->name.equals(nm))
                found = i;
            i++;
        } while (found == -1);
        *out = found;
        out++;
        names++;
    } while (names < g_conv_names + 3);
}

// @ 0x4c85e0
void conv_free_one(conv_rec* self)
{
    ConvItem16* p = (ConvItem16*)self;
    if (p->a4 != 0)
        operator delete((void*)p->a4);
    p->a4 = 0;
    p->a8 = 0;
    p->ac = 0;
    ((tstring*)((char*)p + 0x10))->~tstring();
    ((tstring*)p)->~tstring();
}

// @ 0x4c8630
void conv_build_scene(conv_rec* self)
{
    char* c = (char*)self;
    int gy = 0;
    if (*(int*)(c + 0x27c) >= 0) {
        grid_slot* slot = (grid_slot*)(c + 0x29c);
        do {
            int x = 0;
            if (*(int*)(c + 0x278) >= 0) {
                int y16 = gy * 16;
                do {
                    int** codes = (*(world_root**)(c + 0xc3b0dc))->codes;
                    int* cell = codes[x];
                    *(int*)(y16 + (int)cell) = slot->f0;
                    if (slot->f14 >= 0) {
                        scene_vert* verts = (*(world_root**)(c + 0xc3b0dc))->sub->verts;
                        scene_vert* v = &verts[slot->f14 * 9];
                        v->pos = *(vector4*)((char*)slot + 4);
                    }
                    x++;
                    slot = (grid_slot*)((char*)slot + 0x911c);
                } while (x <= *(int*)(c + 0x278));
            }
            gy++;
            slot = (grid_slot*)((char*)slot + 0x94);
        } while (gy <= *(int*)(c + 0x27c));
    }
    scene_sub* sub = (*(world_root**)(c + 0xc3b0dc))->sub;
    for (unsigned k = 0;
         sub->first != 0 && k < (unsigned)((int)((char*)sub->last - (char*)sub->first) / 0x88);
         k++) {
        var_rec* it = (var_rec*)((char*)sub->first + k * 0x88);
        var_rec* r = *(var_rec**)&it->verts;
        if (r != 0) {
            int j = 0;
            if (r->n > 0) {
                scene_vert* dst = r->verts;
                do {
                    int t = it->idx[j];
                    dst->pos = sub->verts[t].pos;
                    j++;
                    dst++;
                } while (j < r->n);
            }
            matrix3 loc;
            loc.m[0][0] = 0;
            loc.m[0][1] = 0;
            loc.m[0][2] = 0;
            matrix3 m = *identity3(&loc);
            ((void (__fastcall*)(void*, int, matrix3*))*(void**)r)(r, 0, &m);
        }
    }
}

// @ 0x4c8810
edit_vert* fill_verts(edit_vert* dst, int n, const edit_vert* src)
{
    uninit_fill_n(dst, (unsigned)n, src, 0, src);
    return dst + n;
}

// @ 0x4c8840
void conv_assign_back(conv_rec* self, conv_rec** out, conv_rec* first, conv_rec* last)
{
    if (first != last) {
        tstring* end = assign_range((tstring*)first, (tstring*)last, *(tstring**)((char*)self + 8), *(tstring**)((char*)self + 8));
        tstring* it = *(tstring**)((char*)self + 8);
        while (end != it) {
            ((tstring*)it)->~tstring();
            it = (tstring*)((char*)it + 0x10);
        }
        *(tstring**)((char*)self + 8) = end;
    }
    *out = first;
}

// the returned iterator of the pointer vector below (one pointer, returned by value)
struct ptr_iter {
    int* p;                                                  // +0
};

// the pointer vector the erase helper belongs to (first at +0, end at +8)
struct ptr_vec {
    void* first;                                             // +0
    void* last;                                              // +4
    void* end;                                               // +8
    ptr_iter erase_range(int* first, int* last);             // 0x4c88a0
};

// @ 0x4c88a0
ptr_iter ptr_vec::erase_range(int* first, int* last)
{
    if (first != last) {
        int* end = (int*)this->end;
        int* d = first;
        while (last != end) {
            *d = *last;
            last++;
            d++;
        }
        this->end = d;
    }
    ptr_iter r;
    r.p = first;
    return r;
}

// the 0x38-byte record vector the copy helper belongs to
struct item38_vec {
    char pad0[8];
    void copy_range(item38* dst, const item38* first, const item38* last);   // 0x4c88e0
};

// @ 0x4c88e0
void item38_vec::copy_range(item38* dst, const item38* first, const item38* last)
{
    copy_range38(dst, first, last, (conv_rec*)this, last);
}

// @ 0x4c8900
void __stdcall loader_thread_body()
{
    while (g_loading) {
        WaitForSingleObject(g_load_event, INFINITE);
        EnterCriticalSection(&g_load_cs);
        if (g_load_dt > 0.0f) {
            g_phys.update(g_load_dt);
            g_load_dt = 0.0f;
        }
        LeaveCriticalSection(&g_load_cs);
    }
}

// @ 0x4c8980
void* conv_item_array_dtor(conv_rec* self, unsigned flag)
{
    void* p = self;
    if (!(flag & 2)) {
        conv_free_one(self);
        if (flag & 1)
            operator delete(p);
        return p;
    }
    int n = *(int*)((char*)p - 4);
    for (int i = n - 1; i >= 0; i--) {
        ConvItem16* e = (ConvItem16*)((char*)p + i * 0xb0);
        if (e->a4 != 0)
            operator delete((void*)e->a4);
        e->a4 = 0;
        e->a8 = 0;
        e->ac = 0;
        ((tstring*)((char*)e + 0x10))->~tstring();
        ((tstring*)e)->~tstring();
    }
    if (flag & 1)
        operator delete((char*)p - 4);
    return (char*)p - 4;
}

// @ 0x4c8a40
void conv_group_dtor(conv_rec* self)
{
    char* p = (char*)self;
    conv_reset_groups(self);
    int i = 16;
    char* q = p + 0x1c8;
    do {
        q -= 0x18;
        ((tstring*)q)->~tstring();
    } while (--i != 0);
    if (*(void**)(p + 0x34) != 0)
        operator delete(*(void**)(p + 0x34));
    *(void**)(p + 0x34) = 0;
    *(void**)(p + 0x38) = 0;
    *(void**)(p + 0x3c) = 0;
    ((tstring*)(p + 0x20))->~tstring();
    ((tstring*)(p + 0x10))->~tstring();
    ((tstring*)p)->~tstring();
}

// @ 0x4c8ab0
void __stdcall conv_sub_dtor(void** p)
{
    if (p[8] != 0)
        operator delete(p[8]);
    p[8] = 0;
    p[9] = 0;
    p[10] = 0;
    ((tstring*)p)->~tstring();
}

// @ 0x4c8af0
void conv_remove_last(conv_rec* self, int idx)
{
    char* p = (char*)self;
    int n = 0;
    if (*(int*)(p + 0x1c) != 0)
        n = (*(int*)(p + 0x20) - *(int*)(p + 0x1c)) / 0x18;
    int last = n - 1;
    if (idx != last) {
        int* src = (int*)(*(int*)(p + 0x1c) + last * 0x18);
        int* dst = (int*)(*(int*)(p + 0x1c) + idx * 0x18);
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst[4] = src[4];
        dst[5] = src[5];
        conv_remove_child(self, last, idx);
        conv_remove_child2(self, last, idx);
    }
    if (*(int*)(p + 0x1c) != 0) {
        if ((*(int*)(p + 0x20) - *(int*)(p + 0x1c)) / 0x18 != 0)
            *(int*)(p + 0x20) -= 0x18;
    }
    conv_pop_slot(self, last);
}

// @ 0x4c8ba0
void conv_list_dtor(conv_rec* self)
{
    char* p = (char*)self;
    conv_reset_groups(self);
    void** v = (void**)(p + 0x58);
    if (v[0] != 0)
        operator delete(v[0]);
    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
    if (*(void**)(p + 0x38) != 0)
        operator delete[](*(void**)(p + 0x38));
    ((tstring*)(p + 0x44))->~tstring();
    ((tstring*)p)->~tstring();
}

// @ 0x4c8bf0
void conv_clear(void** self)
{
    g_conv_scratch = 0;
    void** v = &self[1];
    if (v[0] != 0)
        operator delete(v[0]);
    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
}

// @ 0x4c8c20
void* __fastcall conv_zero_node(void* self)
{
    unsigned* s = (unsigned*)self;
    *(unsigned*)((char*)s + 0x7c) = 0;
    *(unsigned*)((char*)s + 0x80) = 0;
    *(unsigned*)((char*)s + 0x84) = 0;
    *(unsigned*)((char*)s + 0x88) = 0;
    for (int i = 0; i < 0x1e; i++)
        s[i] = 0;
    return self;
}
