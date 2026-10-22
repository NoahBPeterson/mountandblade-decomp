// s004c5080: mesh editing primitives (provisional names).
// The vertex/edge/poly tables at +0x1c..+0x40 are a hand-written vector<T> with the MSVC 6/7.1 member order
// and the inline fresh test `x != 0 && end - x != 0` that the original inlines everywhere (std::vector with
// /O2 folds the tests away, so it cannot reproduce these functions):
//   +0x1c/+0x20/+0x24 vector<vertex> (0x18 bytes: +0x10 char used, +0x14 int flag)
//   +0x2c/+0x30/+0x34 vector<poly>   (0x64 bytes: vertex-index arrays at +0x28 and +0x38, +0x48 int count,
//                                     +0x60 char used)
//   +0x3c/+0x40/+0x44 vector<edge>   (0x34 bytes: +0 v1, +4 v2, +0x14 char used, +0x20 int flag)
#ifndef MB_CVEC_H
#define MB_CVEC_H
// proposal for match/include/rgl.h: hand-written vector with the MSVC 6/7.1 member order (_First, _Last,
// _End) and the `x != 0 && end - x != 0` test the original inlines at every use.
template <class T>
struct cvector {             // hand-written vector: 4 bytes of leading pad then _First/_Last/_End
    char pad[4];
    T* first;
    T* last;
    T* end;
    unsigned size() const { return first ? (unsigned)(last - first) : 0; }
};
#endif

#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>

struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                 // 0x42bbd0
    tstring(const char* s);                    // 0x42bbe0
    ~tstring();                                // 0x42bc40
    tstring& operator=(const tstring& o);      // 0x42bc50
};

void free_mem(void* p);                        // 0x535712 operator delete (??3@YAXPAX@Z)
struct item38 { char d[0x38]; };               // 0x38-byte record (tstring at +0x24)
void copy_item38(item38* dst, const item38* src);  // 0x46b6c0 (plain args: [esp+4] dst, [esp+8] src)

// The vertex-editor object: a tstring at +0x18, a cleared vector at +0x38, then two 0x4c-byte items
// (tstring at item+0x18, vector at item+0x38) at +0x88 and +0xd4.
struct editor {
    char d0[0x18];
    tstring name;                               // +0x18
    char d28[0x10];
    void** v38;                                 // +0x38 vector<int*> (alloc)
    void** v3c;                                 // +0x3c first
    void** v40;                                 // +0x40 last
    char d44[0x44];
    char items[2 * 0x4c];                       // +0x88, +0xd4

    void clear_vec38();                          // 0x4c5110
    void free_items();                           // 0x4c5150
};

// @ 0x4c5080
// copy-assign a range of 0x38-byte records (the original's auto-object range ctor; its EH pad is the
// next function). Arguments: dst, first, last; the 4th is the record ctor's context.
item38* copy_range38(item38* dst, const item38* first, const item38* last, editor* self)
{
    for (; first != last; first++) {
        copy_item38(dst, first);
        dst++;
    }
    return dst;
}

// SEH cleanup pad of 0x4c5080: destroys the target records it had built and rethrows (the compiler-generated
// __catch$ …,0 of the original's auto-object range constructor; not expressible as C++ in cl 13.10 source,
// so it is left as an empty function).
// @ 0x4c50ea
void copy_range38_catch(void)
{
}

// @ 0x4c5110
void editor::clear_vec38()
{
    if (v38)
        free_mem((void*)v38);
    v38 = 0;
    v3c = 0;
    v40 = 0;
    name.~tstring();
}

// @ 0x4c5150
void editor::free_items()
{
    char* p = (char*)this + 0xd0;
    int i = 2;
    do {
        p -= 0x4c;
        void** v = (void**)p;
        if (v[0])
            free_mem(v[0]);
        v[0] = 0;
        v[1] = 0;
        v[2] = 0;
        ((tstring*)(p - 0x20))->~tstring();
    } while (--i);
}

// ---- the UI root object (layout after match/slices/s0042c9b0/gui.h; the view state at +0x16e8 is added,
// and f1708 is declared as a char: 0x4c5330 writes a byte there) ----
struct gui_obj {
    virtual void d0();                           // slot 0 (deleting dtor)
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    virtual void s6();
    virtual void s7();
    virtual void s8();
    virtual void s9();
    virtual void s10();
    virtual void s11();
    virtual void s12();
    virtual void s13();
    virtual void s14();
    virtual void s15();
    virtual void s16();
    virtual void s17();
    virtual void s18();
    virtual void s19();
    virtual void s20();
    virtual void s21();
    virtual void s22();
    virtual void s23();
    virtual void s24();
    virtual void s25();
    virtual void s26();
    virtual void s27();
    virtual void s28();
    virtual void s29();
    virtual void s30();
    virtual void s31();
    virtual void s32();
    virtual void s33();
    virtual void s34();
    virtual char on_event(unsigned mask);        // slot 35 (0x8c)
};

struct vertex {              // 0x18
    char d0[0x10];
    char d10[4];
    char used;               // +0x14
    char pad15[3];
    int flag;                // +0x18
    char d1c[4];
    int used2;               // +0x20
    char on_event(unsigned mask);                // vtable slot 35
};
struct edge {                // 0x34
    int v1;                  // +0
    int v2;                  // +4
    char d8[0x14 - 8];
    char used;               // +0x14
    char pad15[3];
    int flag;                // +0x18
    char d1c[4];
    int used2;               // +0x20
    char pad24[0x34 - 0x24];
};
struct poly {                // 0x64
    char d0[0x28];
    int nrm[4];              // +0x28 (first 4 vertex indices)
    int col[4];              // +0x38
    int count;               // +0x48
    char d4c[0x4c];
    char used;               // +0x60 (the int array from +0x38 runs past count)
};

struct gui_manager {
    char d0[0x18];
    cvector<vertex> verts;   // +0x18/+0x1c/+0x20
    cvector<poly> polys;     // +0x28/+0x2c/+0x30
    cvector<edge> edges;     // +0x38/+0x3c/+0x40
    char d48[0x7c - 0x48];
    int f7c;                 // +0x7c
    char d80[0x568 - 0x80];
    gui_obj* o568;           // +0x568
    cvector<int> kids;       // +0x56c/+0x570/+0x574 a child list (only element 0 is used)
    char d580[0x16e8 - 0x588];
    float fx;                // +0x16e8 view x (clamped to [0.05, 0.95])
    float fy;                // +0x16ec view y
    float fs;                // +0x16f0 view y scale
    float f16f4;             // +0x16f4
    char d16f8[0x1708 - 0x16f8];
    char f1708;              // +0x1708
    char f1709;              // +0x1709
    char d170a[2];
    int f170c;               // +0x170c

    void set_view(const float* v);               // 0x4c5190
    void wheel_step(int mask);                   // 0x4c52f0
    void clear_selection();                      // 0x4c5330
    void select_mode(int mode, char on);         // 0x4c53f0
    int calc_vertex_mode(int mode);              // 0x4c54f0
    int calc_edge_mode(int mode);                // 0x4c5670
    void swap_face_vertices(int a, int b);       // 0x4c57d0
    void swap_poly_normals(int a, int b);        // 0x4c5840
    void swap_poly_colors(int a, int b);         // 0x4c58c0
    void remove_vertex_from_polys(int v);        // 0x4c5940
    void remove_vertex_from_edges(int v);        // 0x4c59c0
};

void mesh_bounds(float* xp, float* yp);          // 0x438d80 (x lo, x hi into xp; y lo, y hi into yp)
void remove_poly_group(int* self, unsigned i);   // 0x4cb940
void remove_poly_at(int* self, unsigned i);      // 0x4ca9b0

extern const float g_f95;                        // 0x54b890 = 0.95
extern const float g_f05;                        // 0x54b4d0 = 0.05
extern const float g_f011;                       // 0x595c98 = 0.011
extern const float g_f989;                       // 0x595c94 = 0.989
extern const float g_f739;                       // 0x595c90 = 0.739
extern const float g_f728;                       // 0x595c8c = 0.728

// @ 0x4c5190
void gui_manager::set_view(const float* v)
{
    float bx[2];
    float by[2];
    fx = v[0];
    fy = v[1];
    fs = 1.0f;
    if (fx > g_f95)
        fx = 0.95f;
    if (fx < g_f05)
        fx = 0.05f;
    if (!o568)
        return;
    mesh_bounds(bx, by);
    if (bx[0] + fx < g_f011) {
        fx = g_f011 - bx[0];
    } else if (bx[1] + fx > g_f989) {
        fx = g_f989 - bx[1];
    }
    if (by[0] + fy < g_f011) {
        fy = g_f011 - by[0];
    } else if (by[1] + fy > g_f739) {
        fy = g_f739 - by[1];
    }
    if (by[1] - by[0] > g_f728) {
        float s = g_f728 / (by[1] - by[0]);
        fs = s;
        fy = g_f011 - s * by[0];
    }
}

// Wheel step: forward the mask to the active child (the pointer at +4), else to the last item of the object's
// first vector; both are tail calls.
// @ 0x4c52f0
void gui_manager::wheel_step(int mask)
{
    gui_obj* a = *(gui_obj**)((char*)this + 0x10);
    if (a) {
        a->on_event(mask);
    } else {
        gui_obj** first = (gui_obj**)((char*)this + 4);
        gui_obj** last = (gui_obj**)((char*)this + 8);
        if (first && last - first)
            first[last - first - 1]->on_event(mask);
    }
}

// @ 0x4c5330
void gui_manager::clear_selection()
{
    gui_obj** first = (gui_obj**)((char*)this + 4);
    gui_obj** last = (gui_obj**)((char*)this + 8);
    gui_obj* a = *(gui_obj**)((char*)this + 0x10);
    f1708 = 1;
    if (a) {
        a->on_event(0x20000);
    } else if (first && last != first) {
        first[last - first - 1]->on_event(0x20000);
    }
    if (first && last - first) {
        gui_obj* e = first[last - first - 1];
        if (e)
            e->d0();
        first[last - first - 1] = 0;
    }
    f1709 = 0;
    if (first && last - first)
        last -= 1;
}

// @ 0x4c53f0
void gui_manager::select_mode(int mode, char on)
{
    int i, n;
    if (on)
        f7c = verts.first ? (int)(verts.last - verts.first) - 1 : -1;
    else
        f7c = -1;
    if (mode == 0) {
        n = verts.first ? (int)(verts.last - verts.first) : 0;
        for (i = 0; (unsigned)i < (unsigned)n; i++)
            verts.first[i].used = on;
    } else if (mode == 1) {
        n = edges.first ? (int)(edges.last - edges.first) : 0;
        for (i = 0; (unsigned)i < (unsigned)n; i++)
            edges.first[i].used = on;
    } else if (mode == 2) {
        n = polys.first ? (int)(polys.last - polys.first) : 0;
        for (i = 0; (unsigned)i < (unsigned)n; i++)
            polys.first[i].used = on;
    }
}

// @ 0x4c54f0
int gui_manager::calc_vertex_mode(int mode)
{
    for (unsigned i = 0; verts.first && i < (unsigned)verts.size(); i++)
        verts.first[i].flag = 0;
    if (mode == 0) {
        for (unsigned i = 0; verts.first && i < (unsigned)verts.size(); i++)
            if (verts.first[i].used)
                verts.first[i].flag = 1;
    } else if (mode == 1) {
        for (unsigned i = 0; edges.first && i < (unsigned)edges.size(); i++)
            if (edges.first[i].used) {
                verts.first[edges.first[i].v1].flag = 1;
                verts.first[edges.first[i].v2].flag = 1;
            }
    } else {
        for (unsigned i = 0; polys.first && i < (unsigned)polys.size(); i++)
            if (polys.first[i].used)
                for (int k = 0; k < polys.first[i].count; k++)
                    verts.first[polys.first[i].nrm[k]].flag = 1;
    }
    return 0;
}

// @ 0x4c5670
int gui_manager::calc_edge_mode(int mode)
{
    for (unsigned i = 0; edges.first && i < (unsigned)edges.size(); i++)
        edges.first[i].flag = 0;
    if (mode == 0) {
        for (unsigned i = 0; edges.first && i < (unsigned)edges.size(); i++)
            if (verts.first[edges.first[i].v1].used || verts.first[edges.first[i].v2].used)
                edges.first[i].flag = 1;
    } else if (mode == 1) {
        for (unsigned i = 0; edges.first && i < (unsigned)edges.size(); i++)
            if (edges.first[i].used)
                edges.first[i].flag = 1;
    } else {
        for (unsigned i = 0; polys.first && i < (unsigned)polys.size(); i++)
            if (polys.first[i].used)
                for (int k = 0; k < polys.first[i].count; k++)
                    edges.first[polys.first[i].col[k]].flag = 1;
    }
    return 0;
}

// @ 0x4c57d0
void gui_manager::swap_face_vertices(int a, int b)
{
    for (unsigned i = 0; edges.first && i < edges.size(); i++) {
        edge* e = edges.first + i;
        if (e->v1 == a)
            e->v1 = b;
        else if (e->v1 == b)
            e->v1 = a;
        if (e->v2 == a)
            e->v2 = b;
        else if (e->v2 == b)
            e->v2 = a;
    }
}

// @ 0x4c5840
void gui_manager::swap_poly_normals(int a, int b)
{
    for (unsigned i = 0; polys.first && i < (unsigned)polys.size(); i++) {
        poly* p = polys.first + i;
        for (int j = 0; j < p->count; j++) {
            if (p->nrm[j] == a)
                p->nrm[j] = b;
            else if (p->nrm[j] == b)
                p->nrm[j] = a;
        }
    }
}

// @ 0x4c58c0
void gui_manager::swap_poly_colors(int a, int b)
{
    for (unsigned i = 0; polys.first && i < (unsigned)polys.size(); i++) {
        poly* p = polys.first + i;
        for (int j = 0; j < p->count; j++) {
            if (p->col[j] == a)
                p->col[j] = b;
            else if (p->col[j] == b)
                p->col[j] = a;
        }
    }
}

// @ 0x4c5940
void gui_manager::remove_vertex_from_polys(int v)
{
    for (unsigned i = 0; polys.first && i < (unsigned)polys.size(); i++) {
        poly* p = polys.first + i;
        for (int k = 0; k < p->count; k++) {
            if (p->nrm[k] == v) {
                remove_poly_group((int*)this, i);
                i--;
                break;
            }
        }
    }
}

// @ 0x4c59c0
void gui_manager::remove_vertex_from_edges(int v)
{
    for (unsigned i = 0; edges.first && i < (unsigned)edges.size(); i++) {
        edge* e = edges.first + i;
        if (e->v1 == v || e->v2 == v) {
            remove_poly_at((int*)this, i);
            i--;
        }
    }
}

