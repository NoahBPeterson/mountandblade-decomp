// Slice s004cb3a0: fifteen methods of the mesh graph at 0x5d7080's owner plus a few vector helpers.
//   0x4cb3a0  graph_obj::split_edge  - turn the picked (or only) 2-face edge into a 4-face triangle edge
//   0x4cb610  __fastcall(dtor view)  - gui reset, free the record at +0x2e4 and clear it
//   0x4cb650  same                  - gui reset, free the record at +0x1ac
//   0x4cb680  same                  - gui reset, free the record at +0x178
//   0x4cb6b0  vec3_holder::clear    - destroy [begin, end) rows and drop the buffer
//   0x4cb6f0  list_node::grow       - reserve, throwing "list<T> too long"
//   0x4cb780  graph_obj::clear_vecs - free eight vectors at +0x08 .. +0x6c
//   0x4cb840  graph_obj::clean      - clear edge flags, mark face edges, drop the stale edges
//   0x4cb940  graph_obj::remove_face - move the last face over one and rebuild
//   0x4cb9e0  free lists, the record at +0x2e74 and 64 gadget slots, then the graph
//   0x4cbb10  tvec::resize           - std::vector<tstring>::resize
//   0x4cbb90..0x4cbc80  tvec::reserve - std::vector<..., E100>::reserve for rows of 0x50/0x38/0x204/0xb0
// Offsets are read off the disassembly; the classes are provisional.
#include <windows.h>
#include <set>
#include <vector>
#include <stdexcept>
#include "rgl.h"

struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    ~tstring();                                  // 0x42bc40
    tstring() : pos(0), buf(0), cap(0xf), len(0) {}
};

struct gui_manager {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c();
    virtual void v50(); virtual void v54(); virtual void v58(); virtual void v5c();
    virtual void v60(); virtual void v64(); virtual void v68(); virtual void v6c();
    virtual void v70(); virtual void v74(); virtual void v78(); virtual void v7c();
    virtual void v80(); virtual void v84(); virtual void v88(); virtual void v8c();
    virtual void v90(); virtual void v94(); virtual void v98(); virtual void v9c();
    virtual void va0(); virtual void va4(); virtual void va8(); virtual void vac();
    virtual void vb0(); virtual void vb4(); virtual void vb8(); virtual void vbc();
    virtual void vc0(); virtual void vc4(); virtual void vc8(); virtual void vcc();
    virtual void vd0(); virtual void vd4(); virtual void vd8(); virtual void vdc();
    virtual void ve0(); virtual void ve4(); virtual void ve8(); virtual void vec();
    virtual void vf0(); virtual void vf4(); virtual void vf8(); virtual void vfc();
    virtual void v100(); virtual void v104(); virtual void v108(); virtual void v10c();
    virtual void v110(); virtual void v114(); virtual void v118(); virtual void v11c();
    virtual void v120(); virtual void v124(); virtual void v128(); virtual void v12c();
    virtual void v130(); virtual void v134(); virtual void v138(); virtual void v13c();
    virtual void v140(); virtual void v144(); virtual void v148(); virtual void v14c();
    virtual void v150(); virtual void v154(); virtual void v158(); virtual void v15c();
    virtual void v160(); virtual void v164(); virtual void v168(); virtual void v16c();
    virtual void v170(); virtual void reset();        // 0x42d1b0 at +0x174
    char pad[0x1728 - 4 - 0x174];
};
extern gui_manager g_gui;                        // 0x5d7080

// the engine's virtual "destroy the contents then free" record (0x475d50)
struct res_rec {
    virtual void free_contents() = 0;            // +0x00
    virtual void v04() = 0; virtual void v08() = 0; virtual void v0c() = 0;
    virtual void v10() = 0; virtual void v14() = 0; virtual void v18() = 0; virtual void v1c() = 0;
    virtual void v20() = 0; virtual void v24() = 0; virtual void v28() = 0; virtual void v2c() = 0;
    virtual void v30() = 0; virtual void v34() = 0; virtual void v38() = 0; virtual void v3c() = 0;
    virtual void v40() = 0; virtual void v44() = 0; virtual void v48() = 0; virtual void release() = 0;  // +0x4c
    // the direct (non-virtual) teardown entry at 0x475d50: thiscall, `this` in ecx
    void remove();                               // 0x475d50
};

// a gadget with a virtual delete-in-place slot
struct gadget {
    virtual void release(int flag) = 0;          // +0x00
};

// the lib's 4-arg _Destroy_range (0x4727a0) takes (first, last, allocator&, tag): the tag is an empty
// class passed by value, so the call materialises a local `ptr_tag` before the branch.
struct ptr_tag { };
void obj_free(void* first, void* last, void* al, ptr_tag tag); // 0x4727a0
struct node {                                    // 0x18 bytes
    float v[4];                                  // +0x00
    unsigned char used;                          // +0x10
    char pad11[0x7];
};

struct face {                                    // 0x64 bytes
    char pad00[0x28];
    int  verts[4];                               // +0x28   (vertex indices of the ring)
    int  ends[4];                                // +0x38   (edge indices of the ring)
    int  count;                                  // +0x48   (n of verts/ends)
    char pad4c[0x18];
};

struct edge {                                    // 0x34 bytes
    int a;                                       // +0x00
    int b;                                       // +0x04
    int f0;                                      // +0x08
    int f1;                                      // +0x0c
    int n;                                       // +0x10
    unsigned char flag;                          // +0x14
    char pad15[0xb];
    int used;                                    // +0x20
    char pad24[0x10];
};

struct graph_obj {
    bool split_edge(int e);                      // 0x4cb3a0
    char pad0[0x8];
    void** vec08;                                // +0x08
    void** vec08end;
    void** vec08cap;
    char pad14[0x8];
    void** vec1c;                                // +0x1c
    void** vec1cend;
    void** vec1ccap;
    char pad28[0x4];
    face*  faces;                                // +0x2c
    face*  faces_end;                            // +0x30
    face*  faces_cap;                            // +0x34
    char pad38[0x4];
    edge*  edges;                                // +0x3c
    edge*  edges_end;                            // +0x40
    edge*  edges_cap;                            // +0x44
    char pad48[0x4];
    void** vec4c;                                // +0x4c
    void** vec4cend;
    void** vec4ccap;
    char pad58[0x4];
    void** vec5c;                                // +0x5c
    void** vec5cend;
    void** vec5ccap;
    char pad68[0x4];
    void** vec6c;                                // +0x6c
    void** vec6cend;
    void** vec6ccap;
    // the lib's vector::_Tidy (inlined): free the buffer if any, then zero the triple
    void tidy(void* base) { void** q = (void**)base; if (q[0]) operator delete(q[0]); q[0] = 0; q[1] = 0; q[2] = 0; }
    void   remove_edge(int i);                   // 0x4ca9b0
    void   remove_face(int i);                   // 0x4cb940
    void   clear_vecs();                         // 0x4cb780
    void   clean();                              // 0x4cb840
    void   finish();                             // 0x470920
};

// the row helper at 0x4cb6b0 (vector of three pointers in a separate record)
void vec_destroy(void* p, int a, int b);         // 0x4727a0

// a list node that only counts elements
struct list_node {
    int* next;                                   // +0x00
    int* prev;                                   // +0x04
    int  count;                                  // +0x08
    void grow(unsigned n);                       // 0x4cb6f0
};

// @ 0x4cb3a0
bool graph_obj::split_edge(int e)
{
    int count = 0;
    int i;
    int idx = 0;
    if (e == -1) {
        for (i = 0; i < (edges ? (int)(edges_end - edges) : 0); i++) {
            if (edges[i].flag) {
                count++;
                e = i;
            }
        }
        if (count != 1)
            return false;
    }
    if (edges[e].n != 2)
        return false;
    face* f0 = &faces[edges[e].f0];
    if (f0->count != 3)
        return false;
    face* f1 = &faces[edges[e].f1];
    if (f1->count != 3)
        return false;

    int i0 = -1, i1 = -1;
    if (f0->ends[0] == e)
        i0 = 0;
    if (f1->ends[0] == e)
        i1 = 0;
    if (f0->ends[1] == e)
        i0 = 1;
    if (f1->ends[1] == e)
        i1 = 1;
    if (f0->ends[2] == e)
        i0 = 2;
    if (f1->ends[2] == e)
        i1 = 2;

    int xs[4];
    int ys[4];
    xs[0] = f0->verts[(i0 + 1) % 3];
    ys[0] = f0->ends[(i0 + 1) % 3];
    xs[1] = f1->verts[(i1 + 1) % 3];
    ys[1] = f1->ends[(i1 + 1) % 3];
    xs[2] = f0->verts[(i0 + 2) % 3];
    ys[2] = f0->ends[(i0 + 2) % 3];
    xs[3] = f1->verts[(i1 + 2) % 3];
    ys[3] = f1->ends[(i1 + 2) % 3];

    f0->verts[0] = xs[0];
    f0->verts[1] = xs[1];
    f0->verts[2] = xs[2];
    f0->verts[3] = xs[3];
    f0->ends[0] = ys[0];
    f0->ends[1] = ys[1];
    f0->ends[2] = ys[2];
    f0->ends[3] = ys[3];
    f0->count = 4;
    remove_edge(e);
    finish();
    return true;
}

// @ 0x4cb610
struct rec_2e4 {
    void release();                              // 0x4cb610
    char pad0[0x2e4];
    res_rec* p;                                  // +0x2e4
};

void rec_2e4::release()
{
    g_gui.reset();
    res_rec* r = p;
    if (r) {
        r->remove();
        operator delete(r);
        p = 0;
    }
}

// @ 0x4cb650
struct rec_1ac {
    void release();                              // 0x4cb650
    char pad0[0x1ac];
    res_rec* p;                                  // +0x1ac
};

void rec_1ac::release()
{
    g_gui.reset();
    res_rec* r = p;
    if (r) {
        r->remove();
        operator delete(r);
    }
}

// @ 0x4cb680
struct rec_178 {
    void release();                              // 0x4cb680
    char pad0[0x178];
    res_rec* p;                                  // +0x178
};

void rec_178::release()
{
    void* self = this;
    g_gui.reset();
    res_rec* r = ((rec_178*)self)->p;
    if (r) {
        r->remove();
        operator delete(r);
    }
}

// @ 0x4cb6b0
struct vec3_holder {
    void clear();                                // 0x4cb6b0
    char pad0[4];
    void* begin;                                 // +0x04
    void* end;                                   // +0x08
    void* cap;                                   // +0x0c
};

void delete_obj(void* p);                        // 0x535712

void vec3_holder::clear()
{
    ptr_tag tag;
    if (begin) {
        obj_free(begin, end, (void*)pad0, tag);
        delete_obj(begin);
    }
    begin = 0;
    end = 0;
    cap = 0;
}

// @ 0x4cb6f0
void list_node::grow(unsigned n)
{
    if (0x7ffffffu - count < n)
        throw std::length_error("list<T> too long");
    count += n;
}

// @ 0x4cb780
void graph_obj::clear_vecs()
{
    tidy(&vec6c);
    tidy(&vec5c);
    tidy(&vec4c);
    tidy(&edges);
    tidy(&faces);
    tidy(&vec1c);
    tidy(&vec08);
}

// @ 0x4cb840
void graph_obj::clean()
{
    for (;;) {
        int i;
        for (i = 0; i < (edges ? (int)(edges_end - edges) : 0); i++)
            edges[i].used = 0;
        for (i = 0; i < (faces ? (int)(faces_end - faces) : 0); i++) {
            int j;
            for (j = 0; j < faces[i].count; j++)
                edges[faces[i].ends[j]].used = 1;
        }
        i = 0;
        for (; i < (edges ? (int)(edges_end - edges) : 0); i++) {
            if (!edges[i].used)
                break;
        }
        if (i >= (edges ? (int)(edges_end - edges) : 0))
            return;
        remove_edge(i);
    }
}

// @ 0x4cb940
void graph_obj::remove_face(int i)
{
    int last = (faces ? (int)(faces_end - faces) : 0) - 1;
    if (i != last)
        faces[i] = faces[last];
    if ((faces ? (int)(faces_end - faces) : 0) != 0)
        faces_end--;
    clean();
}

// the big record that owns the graph plus a few gadget slots
struct big_obj {
    void release();                              // 0x4cb9e0
    char pad0[0x8];
    void** vec08;                                // +0x08
    void** vec08end;
    void** vec08cap;
    char pad14[0x10];
    tstring ts24;                                // +0x24
    char pad34[0x10];
    void** vec44;                                // +0x44
    void** vec44end;
    void** vec44cap;
    char pad50[0x2400];
    tstring names[0x80];                         // +0x2450 .. +0x2c50
    char pad2c50[0x104];
    gadget* slots[64];                           // +0x2d54 .. +0x2e54 (8 rows of 8)
    char pad2e54[0x20];
    res_rec* p2e74;                              // +0x2e74
    char pad2e78[0x8];
    void** vec2e80;                              // +0x2e80
    void** vec2e80end;
    void** vec2e80cap;
    char pad2e8c[0x4];
    void** vec2e90;                              // +0x2e90
    void** vec2e90end;
    void** vec2e90cap;
    char pad2e9c[0x4];
    void** vec2ea0;                              // +0x2ea0
    void** vec2ea0end;
    void** vec2ea0cap;
    char pad2eac[0x4];
    void** vec2eb0;                              // +0x2eb0
    void** vec2eb0end;
    void** vec2eb0cap;
    // the lib's vector::_Tidy (inlined): free the buffer if any, then zero the triple
    void tidy(void* base) { void** q = (void**)base; if (q[0]) operator delete(q[0]); q[0] = 0; q[1] = 0; q[2] = 0; }
};

// @ 0x4cb9e0
void big_obj::release()
{
    res_rec* r = p2e74;
    if (r) {
        r->remove();
        operator delete(r);
    }
    gadget** s = slots;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (*s) {
                (*s)->release(1);
                *s = 0;
            }
            s++;
        }
    }
    tidy(&vec2eb0);
    tidy(&vec2ea0);
    tidy(&vec2e90);
    tidy(&vec2e80);
    for (int i = 0x7f; i >= 0; i--)
        names[i].~tstring();
    tidy(&vec44);
    ts24.~tstring();
}

// @ 0x4cbb10
struct tvec {
    char pad0[4];
    tstring* begin;                              // +0x04
    tstring* end;                                // +0x08
    tstring* cap;                                // +0x0c
    unsigned size() const { return begin ? (unsigned)(end - begin) : 0; }
    void resize(unsigned n, tstring val);        // 0x4cbb10
    bool reserve_50(unsigned n);                 // 0x4cbb90
    bool reserve_38(unsigned n);                 // 0x4cbbe0
    bool reserve_204(unsigned n);                // 0x4cbc30
    bool reserve_b0(unsigned n);                 // 0x4cbc80
    // the lib's _Insert_n (0x43e310) takes (pos, n, const tstring&) with `this` in ecx
    void insert_n(tstring* pos, unsigned n, const tstring& src);            // 0x43e310
    // thiscall helper (this in ecx) taking (n, first, last)
    void erase_range(unsigned* n, tstring* first, tstring* last);           // 0x4c8840
    // the lib's _Xlen (0x43e2a0): thiscall, no arguments, never returns
    __declspec(noreturn) void _Xlen() const;                                 // 0x43e2a0
};

// @ 0x4cbb10
void tvec::resize(unsigned n, tstring val)
{
    if (size() < n)
        insert_n(end, n - size(), val);
    else if (n < size())
        erase_range(&n, begin + n, end);
}

// @ 0x4cbb90
bool tvec::reserve_50(unsigned n)
{
    begin = 0;
    end = 0;
    cap = 0;
    if (n == 0)
        return false;
    if (n > 0x3333333) {
        _Xlen();
    }
    tstring* p = (tstring*)operator new(n * 0x50);
    cap = p + n * 5;
    begin = p;
    end = p;
    return true;
}

// @ 0x4cbbe0
bool tvec::reserve_38(unsigned n)
{
    begin = 0;
    end = 0;
    cap = 0;
    if (n == 0)
        return false;
    if (n > 0x4924924)
        _Xlen();
    tstring* p = (tstring*)operator new(n * 0x38);
    cap = (tstring*)((char*)p + n * 0x38);
    begin = p;
    end = p;
    return true;
}

// @ 0x4cbc30
bool tvec::reserve_204(unsigned n)
{
    begin = 0;
    end = 0;
    cap = 0;
    if (n == 0)
        return false;
    if (n > 0x7f01fc)
        _Xlen();
    tstring* p = (tstring*)operator new(n * 0x204);
    cap = (tstring*)((char*)p + n * 0x204);
    begin = p;
    end = p;
    return true;
}

// @ 0x4cbc80
bool tvec::reserve_b0(unsigned n)
{
    begin = 0;
    end = 0;
    cap = 0;
    if (n == 0)
        return false;
    if (n > 0x1745d17)
        _Xlen();
    tstring* p = (tstring*)operator new(n * 0xb0);
    cap = (tstring*)((char*)p + n * 0xb0);
    begin = p;
    end = p;
    return true;
}
