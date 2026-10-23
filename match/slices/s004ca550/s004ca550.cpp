// Slice s004ca550: eight methods from one TU. The offsets overlap between the functions, so the code is
// grouped into the provisional classes it appears to belong to:
//   0x4ca550  screen_text::refresh  - test the two device records for fullscreen, set a flag through a
//                                     gadget and copy the window's name into the text; then re-apply
//   0x4ca600  screen_cfg::apply     - pop the request queue of the GUI (0x5d7080), forward the new state
//   0x4ca6c0  dev_list::select_windowed - remember the last fullscreen device entry at +0x50
//   0x4ca720  str_table::release_strings - release two 0xdf3-entry tables of string vectors
//   0x4ca7e0  destroy_range (cdecl) - destroy the 0xb0-byte rows [first, last)
//   0x4ca830  big_holder::release   - free the big buffers, destroy five 0x98-byte state records
//   0x4ca8c0  graph_obj::rebuild    - recompute the nodes' "used" labels from the edges
//   0x4ca9b0  graph_obj::remove_edge - copy the last edge over one, shrink and rebuild
// Names and layouts are provisional; every offset is read off the disassembly.
#include <windows.h>
#include <d3d9.h>
#include <limits.h>
#include <deque>
#include <vector>
#include "rgl.h"

// the engine's 16-byte string (see s0049b630/tstring.h)
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    ~tstring();                                  // 0x42bc40
    void operator=(const tstring& o);            // 0x42bc50
    void assign(const char* s);                  // 0x42bcb0
};

// the render-device record's vtable: only the slots used here are modelled
struct dev_rec {
    virtual void s00() = 0;
    virtual void s04() = 0;
    virtual void s08() = 0;
    virtual void s0c() = 0;
    virtual void s10() = 0;
    virtual void s14() = 0;
    virtual void s18() = 0;
    virtual void s1c() = 0;
    virtual void s20() = 0;
    virtual void s24() = 0;
    virtual void s28() = 0;
    virtual void s2c() = 0;
    virtual void s30() = 0;
    virtual void s34() = 0;
    virtual void s38() = 0;
    virtual void s3c() = 0;
    virtual void s40() = 0;
    virtual bool is_fullscreen(int which) = 0;   // +0x44
    virtual void s48() = 0;
};

// slots 0x04 (no-arg restore) and 0x20 (the state index stored in g_5d75e0)
// (a standalone vtable: deriving from dev_rec would put these after dev_rec's 19 slots)
struct dev_item {
    virtual void s00() = 0;
    virtual void restore() = 0;                  // +0x04
    virtual void s08() = 0;
    virtual void s0c() = 0;
    virtual void s10() = 0;
    virtual void s14() = 0;
    virtual void s18() = 0;
    virtual void s1c() = 0;
    virtual unsigned char index() = 0;           // +0x20
};

struct gui_manager {
    char pad[0x1728];
    void set_state(int a, int b);                // 0x42cdb0
};
extern gui_manager g_gui;                        // 0x5d7080

extern IDirect3DDevice9* g_dev_renderer;        // 0xa17f28 (slot 0x30 = ShowCursor)
extern int g_windowed_setting;                   // 0xa17144
extern int g_windowed_other;                     // 0xa17544
extern unsigned char g_5d75e0;                   // 0x5d75e0

struct node {                                    // 0x18 bytes
    char pad[0x14];
    int used;                                    // +0x14
};

struct edge {                                    // 0x34 bytes
    int a;                                       // +0x00
    int b;                                       // +0x04
    char pad[0x2c];
};

void* operator new(unsigned size);               // 0x53545e
void  operator delete(void* p);                  // 0x535712
void  operator delete[](void* p);                // 0x53570d
void  free_mem_sized(void* p, unsigned n);       // 0x53570d (operator delete[], ret 8)

// a virtual destroy-in-place helper (0x4c5110 "dtor" / 0x4c5150 "vector dtor"): declared with its real
// signature so the callers keep the original's argument evaluation order.
struct holder_rec {
    char pad[0x4c];
    char data[0x98 - 0x4c];
};
void holder_release(void* p, int n);             // 0x4c5150
void holder_release1(void* p);                   // 0x4c5150
// the CRT __eh_vector_destructor_iterator (0x401830, ret 0x10): destroys `n` elements of `size` bytes
// from p onwards, backwards, invoking the out-of-line destructor `d` (thiscall, element in ecx).
void __stdcall vec_dtor_iter(void* p, unsigned size, int n, void (*d)(void*));  // 0x401830
void item_dtor(void* p);                         // 0x4c5110 (same as ~item)

// a 0x1c-byte index deque (inside the object at +0x78)
struct paged {
    int  pad0;                                   // +0x00
    int** map;                                   // +0x04
    unsigned mapsize;                            // +0x08
    unsigned base;                               // +0x0c
    unsigned count;                              // +0x10
    unsigned state;                              // +0x14
    unsigned pad18;                              // +0x18
    int& operator[](unsigned i) {
        unsigned n = base + i;
        unsigned page = n >> 2;
        unsigned slot = n - page * 4;
        if (mapsize <= page)
            page -= mapsize;
        return map[page][slot];
    }
    int& back() { return (*this)[count - 1]; }
    void pop_back() {
        if (count != 0 && --count == 0)
            base = 0;
    }
};

struct screen_cfg {
    __declspec(noinline) void apply();           // 0x4ca600
    dev_item* items[1];                          // +0x00: array of records, indexed by the queue
    char pad4[0x74];
    std::deque<int> req;                         // +0x78 (VC7.1 deque: allocator, map +4, mapsize +8, off +0xc, size +0x10)
};
extern screen_cfg g_screen;                      // 0xa73980

// a gadget with the current-mode flag at +0x0c and the name text at +0x24
struct gadget {
    char pad0[0xc];
    int* flag;                                   // +0x0c
    char pad10[0x14];
    tstring* text;                               // +0x24
};

// the screen-text widget
struct screen_text {
    void refresh(int handle);                    // 0x4ca550
    char pad0[0x38];
    void* win;                                   // +0x38: the window record; a tstring at +0x128
    dev_rec* dev1;                               // +0x3c
    dev_rec* dev2;                               // +0x40
    char pad44[0x8];
    gadget* g;                                   // +0x4c
    tstring* text;                               // +0x50
};

// a list of device records: +0x38 .. +0x50 (the last slot holds the selected index)
struct dev_list {
    void select_windowed(int a);                 // 0x4ca6c0
    char pad0[0x38];
    dev_rec* dev[7];                             // +0x38 .. +0x50
    int nreg;                                    // +0x54
};

// a table row: a vector<tstring>
struct str_row {
    tstring* begin;                              // +0x00
    tstring* end;                                // +0x04
    tstring* cap;                                // +0x08
};

struct str_table {
    void release_strings();                      // 0x4ca720
    char pad0[0xdf34];
    str_row rows_b[0xdf3];                       // +0xdf34 .. +0x1be64
    str_row rows_a[0xdf3];                       // +0x1be64 ..
};

// a 0xb0-byte record: a tstring at +0 and a heap pointer at +0x20
struct big_rec {
    tstring name;                                // +0x00
    char pad10[0x10];
    void* p20;                                   // +0x20
    void* p24;                                   // +0x24
    void* p28;                                   // +0x28
    char pad2c[0x84];
};

void destroy_range(big_rec* first, big_rec* last);   // 0x4ca7e0

// a 0x4c-byte record: a tstring at +0x18 and a (alloc, first, last) vector triple at +0x38. Its
// out-of-line destructor is 0x4c5110 (see match/slices/s004c5080).
struct item {
    char d0[0x18];
    tstring name;                                // +0x18
    char d28[0x10];
    void* v0;                                    // +0x38
    void* v1;                                    // +0x3c
    void* v2;                                    // +0x40
    char d44[8];                                 // +0x44 .. +0x4c
    ~item();                                     // 0x4c5110
};

// two items: destroying a row destroys its `items[2]` array through __eh_vector_destructor_iterator
// (0x401830) with the out-of-line ~item.
struct row {
    item items[2];                               // +0x00, +0x4c
};

// the last pair is destroyed with an out-of-line call instead (0x4c5150).
struct rec {
    item a;                                      // +0x00
    item b;                                      // +0x4c
    void clear();                                // 0x4c5150
};

struct mesh_sub {                                // rgl::mesh (0x260 bytes), dtor 0x4127f0
    ~mesh_sub();                                 // 0x4127f0
    char d[0x260];
};
struct scene_sub {                               // rgl::scene, dtor 0x41f4e0
    ~scene_sub();                                // 0x41f4e0
    char d[0x100];
};

struct big_holder {
    void release();                              // 0x4ca830
    char pad0[0x14];
    row rows[5];                                 // +0x14 .. +0x30c (5 * 0x98)
    rec tail;                                    // +0x30c .. +0x3a4
    char pad3a4[4];
    void* v3a8;                                  // +0x3a8 (freed if non-null, then cleared)
    void* v3ac;                                  // +0x3ac
    void* v3b0;                                  // +0x3b0
    char pad3b4[0x4183d4 - 0x3b4];
    void* px_d4;                                 // +0x4183d4 (delete[])
    void* px_d8;                                 // +0x4183d8 (delete[])
    int   pad_dc;                                // +0x4183dc
    mesh_sub m;                                  // +0x4183e0
    scene_sub s;                                 // +0x418640
};

// the node/edge graph at +0x1c/+0x3c
struct graph_obj {
    void rebuild();                              // 0x4ca8c0
    void remove_edge(int i);                     // 0x4ca9b0
    void remove_node(int i);                     // 0x4c8af0
    void edge_changed(int last, int i);          // 0x4c5840
    void edge_removed(int last);                 // 0x4c5940
    char pad0[0x18];
    std::vector<node> nodes;                     // +0x18 (VC7.1 vector: allocator word, first +0x1c, last +0x20)
    char pad28[0x10];
    std::vector<edge> edges;                     // +0x38 (first +0x3c, last +0x40)
};

// @ 0x4ca550
void screen_text::refresh(int handle)
{
    if (dev1->is_fullscreen(0x10000))
        goto off;
    if (dev2) {
        if (dev2->is_fullscreen(0x10000))
            goto on;
        return;
    } else {
        if (!g_windowed_setting || g_windowed_other)
            return;
    }
on:
    if (g->flag)
        *g->flag = 1;
    if (text && g->text)
        g->text->assign("");
    g_screen.apply();
    return;
off:
    if (g->flag)
        *g->flag = 0;
    if (text && g->text)
        *text = *(tstring*)((char*)win + 0x128);
    g_screen.apply();
}

// @ 0x4ca600
void screen_cfg::apply()
{
    g_gui.set_state(0, 1);
    items[req.back()]->restore();
    req.pop_back();
    if (!req.empty()) {
        unsigned char v = items[req.back()]->index();
        if (g_dev_renderer)
            g_dev_renderer->ShowCursor(v);
        g_5d75e0 = v;
    }
}

// @ 0x4ca6c0
void dev_list::select_windowed(int a)
{
    int found = -1;
    for (int i = 0; i < nreg; i++) {
        if (dev[i]) {
            if (dev[i]->is_fullscreen(0x10000))
                found = i;
        }
    }
    if (found != -1) {
        dev[6] = (dev_rec*)found;
        found = (int)&g_screen;
        g_screen.apply();
    }
}

// @ 0x4ca720
void str_table::release_strings()
{
    str_row* p = &rows_a[0xdf3];
    int n = 0xdf3;
    do {
        p--;
        if (p->begin) {
            for (tstring* s = p->begin; s != p->end; s++)
                s->~tstring();
            operator delete(p->begin);
        }
        p->begin = 0;
        p->end = 0;
        p->cap = 0;
        n--;
    } while (n != 0);

    p = &rows_b[0xdf3];
    n = 0xdf3;
    do {
        p--;
        if (p->begin) {
            for (tstring* s = p->begin; s != p->end; s++)
                s->~tstring();
            operator delete(p->begin);
        }
        p->begin = 0;
        p->end = 0;
        p->cap = 0;
        n--;
    } while (n != 0);
}

// @ 0x4ca7e0
void destroy_range(big_rec* first, big_rec* last)
{
    if (last == first)
        return;
    do {
        if (first->p20)
            operator delete(first->p20);
        first->p20 = 0;
        first->p24 = 0;
        first->p28 = 0;
        first->name.~tstring();
        first++;
    } while (first != last);
}

// @ 0x4ca830
void big_holder::release()
{
    operator delete[](px_d4);
    s.~scene_sub();
    m.~mesh_sub();
    operator delete[](px_d8);
    if (v3a8)
        operator delete(v3a8);
    v3a8 = 0;
    v3ac = 0;
    v3b0 = 0;
    tail.clear();
    for (int i = 4; i >= 0; i--)
        vec_dtor_iter(&rows[i], 0x4c, 2, &item_dtor);
}

// @ 0x4ca8c0
void graph_obj::rebuild()
{
    for (;;) {
        unsigned k;
        for (k = 0; k < nodes.size(); k++)
            nodes[k].used = 0;
        for (k = 0; k < edges.size(); k++) {
            nodes[edges[k].a].used = 1;
            nodes[edges[k].b].used = 1;
        }
        for (k = 0;; k++) {
            if (!(k < nodes.size()))
                return;
            if (nodes[k].used == 0)
                break;
        }
        remove_node(k);
    }
}

// @ 0x4ca9b0
void graph_obj::remove_edge(int i)
{
    int last = (int)edges.size() - 1;
    if (i != last) {
        edges[i] = edges[last];
        edge_changed(last, i);
    }
    edges.pop_back();
    rebuild();
    edge_removed(last);
}
