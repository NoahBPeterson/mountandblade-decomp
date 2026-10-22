// Slice s004c5a20: the vertex editor's selection recalculation and its string/name accessors.
// The editor (gui_manager's vertex mode page) keeps a 0x18-byte vertex vector at +0x18, a 0x34-byte edge
// vector at +0x38 and a 0x64-byte poly vector at +0x28 with int caps at +0x48 and the "used" bytes at
// +0x10 (vertex), +0x14 (edge) and +0x60 (poly). +0x7c is the "current element" index, +0x78 the mode.
#include <windows.h>

#define MB_NOTHROW throw()

struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
    void append(const tstring& o);              // 0x42c1e0
    tstring operator+(const tstring& o) const;  // 0x42c840
    char equals(const tstring& o) const;        // 0x42bdd0
};

struct StrTable {
    tstring* find(const tstring* key) MB_NOTHROW;   // 0x4c6010 (null when absent)
};
extern StrTable g_strtable;                             // 0x5e2a18

// Shared string constants (UTF-16 / ASCII literals in .data).
extern const char g_door[];                             // 0x595c9c "_door"
extern const char g_prefix[];                           // 0x595ca4 the "+" prefix of the help strings

struct vertex {              // 0x18
    char d0[0x10];
    char used;               // +0x10
    char pad11[3];
    int flag;                // +0x14
};

// The rgl container: begin/end/cap pointers; csize() is the (guarded) unsigned element count.
template <class T> struct GV {
    T* first;                // +0
    T* last;                 // +4
    T* end;                  // +8
    unsigned csize() const { return first ? (unsigned)(last - first) : 0; }
};
// The editor's own vector template reads its begin pointer one page earlier than the array it holds.
template <class T> struct GVO {
    char pad[4];             // +0
    T* first;                // +4
    T* last;                 // +8
    T* end;                  // +0xc
    T* begin() const { return first; }
    unsigned csize() const { return begin() ? (unsigned)(last - begin()) : 0; }
};
struct edge {                // 0x34
    int v1;                  // +0
    int v2;                  // +4
    int poly;                // +8  owning poly index
    char d0c[0x14 - 0x0c];
    char used;               // +0x14
    char pad15[0x20 - 0x15];
    int flag;                // +0x20
};
struct poly {                // 0x64
    char d0[0x28];
    int nrm[4];              // +0x28 vertex indices
    int col[4];              // +0x38 edge indices
    int count;               // +0x48
    char d4c[0x60 - 0x4c];
    char used;               // +0x60
};

struct gui_manager {
    char d0[0x18];
    GV<vertex> verts;        // +0x18 vertices
    char d24[4];
    GV<poly> polys;          // +0x28 faces
    GV<edge> edges;          // +0x38 edges    char d44[0x34];
    int mode;                // +0x78
    int cur;                 // +0x7c
    char d80[0x16e8 - 0x80];
    float fx;                // +0x16e8 view x
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
    void update_vertex_mode();                   // 0x4c5a20
    void update_edge_mode();                     // 0x4c5b60
    void update_poly_mode();                     // 0x4c5c80
    void refresh_selection();                    // 0x4c5dc0
    void cycle_vertex(int* out);                 // 0x4c5f60
    tstring* find_string(const tstring* key);    // 0x4c6010
    tstring* edge_string();                      // 0x4c6080
    tstring* poly_string();                      // 0x4c6100
    tstring* vertex_string();                    // 0x4c6190
    tstring find_vertex_string();                // 0x4c61f0
    tstring* edge_string2();                     // 0x4c62b0
};

// @ 0x4c5a20
void gui_manager::update_vertex_mode()
{
    cur = -1;
    int n = verts.csize();
    for (int i = 0; i < n; i++)
        verts.first[i].used = 0;
    if (mode == 1) {
        int ne = edges.first ? (int)(edges.last - edges.first) : 0;
        for (int i = 0; i < ne; i++) {
            if (edges.first[i].used) {
                verts.first[edges.first[i].v1].used = 1;
                verts.first[edges.first[i].v2].used = 1;
                cur = edges.first[i].v2;
            }
        }
    } else {
        int np = polys.first ? (int)(polys.last - polys.first) : 0;
        for (int i = 0; i < np; i++) {
            poly* p = &polys.first[i];
            if (p->used) {
                for (int k = 0; k < p->count; k++) {
                    verts.first[p->nrm[k]].used = 1;
                    cur = p->nrm[k];
                }
            }
        }
    }
}

// @ 0x4c5b60
void gui_manager::update_edge_mode()
{
    cur = -1;
    int ne = edges.csize();
    for (int i = 0; i < ne; i++)
        edges.first[i].used = 0;
    if (mode == 0) {
        for (int i = 0; i < ne; i++) {
            if (verts.first[edges.first[i].v1].used || verts.first[edges.first[i].v2].used)
                edges.first[i].used = 1;
        }
    } else {
        int np = polys.first ? (int)(polys.last - polys.first) : 0;
        for (int i = 0; i < np; i++) {
            poly* p = &polys.first[i];
            if (p->used) {
                for (int k = 0; k < p->count; k++)
                    edges.first[p->col[k]].used = 1;
            }
        }
    }
}

// @ 0x4c5c80
void gui_manager::update_poly_mode()
{
    cur = -1;
    int np = polys.csize();
    for (int i = 0; i < np; i++)
        polys.first[i].used = 0;
    if (mode == 0) {
        for (int i = 0; i < np; i++) {
            poly* p = &polys.first[i];
            for (int k = 0; k < p->count; k++)
                if (verts.first[p->nrm[k]].used)
                    p->used = 1;
        }
    } else {
        for (int i = 0; i < np; i++) {
            poly* p = &polys.first[i];
            for (int k = 0; k < p->count; k++)
                if (edges.first[p->col[k]].used)
                    p->used = 1;
        }
    }
}

// @ 0x4c5dc0
void gui_manager::refresh_selection()
{
    if (mode == 0) {
        calc_vertex_mode(mode);
        int ne = edges.first ? (int)(edges.last - edges.first) : 0;
        for (int i = 0; i < ne; i++) {
            if (verts.first[edges.first[i].v1].flag) {
                verts.first[edges.first[i].v2].used = 1;
                cur = edges.first[i].v2;
            }
            if (verts.first[edges.first[i].v2].flag) {
                verts.first[edges.first[i].v1].used = 1;
                cur = edges.first[i].v1;
            }
        }
    } else if (mode == 1) {
        calc_vertex_mode(mode);
        int ne = edges.first ? (int)(edges.last - edges.first) : 0;
        for (int i = 0; i < ne; i++) {
            if (verts.first[edges.first[i].v1].flag || verts.first[edges.first[i].v2].flag)
                edges.first[i].used = 1;
        }
    } else {
        calc_edge_mode(mode);
        int np = polys.first ? (int)(polys.last - polys.first) : 0;
        for (int i = 0; i < np; i++) {
            poly* p = &polys.first[i];
            for (int k = 0; k < p->count; k++)
                if (edges.first[p->col[k]].flag)
                    p->used = 1;
        }
    }
}

// @ 0x4c5f60
// Step to the next vertex of the face that owns the edge *out; *out ends up with that next vertex.
void gui_manager::cycle_vertex(int* out)
{
    int e = *out;
    poly* p = &polys.first[edges.first[e].poly];
    int idx = -1;
    for (int k = 0; k < p->count; k++) {
        if (p->nrm[k] == e) {
            idx = k;
            break;
        }
    }
    *out = p->nrm[idx];
    *out = p->nrm[(idx + 1) % p->count];
}

// @ 0x4c6010
tstring* gui_manager::find_string(const tstring* key)
{
    unsigned h = ((unsigned(*)(const tstring*))0x433bb0)(key) << 4;
    char* base = (char*)this + h;
    int n = *(void**)(base + 4) ? (int)(*(char**)(base + 8) - *(char**)(base + 4)) / 0x10 : 0;
    for (int i = 0; i < n; i++)
        if (((tstring*)(*(char**)(base + 4) + i * 0x10))->equals(*key))
            return (tstring*)(*(char**)(base + 0xdf34) + i * 0x10);
    return 0;
}

// @ 0x4c6080 (byte-identical to 0x4c6100 / 0x4c62b0 in the original)
tstring* gui_manager::edge_string()
{
    tstring* r = g_strtable.find(&(*(tstring*)((char*)this + 0x20) + tstring(g_door)));
    if (r)
        return r;
    tstring s = *(tstring*)((char*)this + 0x10);
    s.append(tstring(g_door));
    return r;
}

// @ 0x4c6100
tstring* gui_manager::poly_string()
{
    tstring* r = g_strtable.find(&(*(tstring*)((char*)this + 0x20) + tstring(g_door)));
    if (r)
        return r;
    tstring s = *(tstring*)((char*)this + 0x10);
    s.append(tstring(g_door));
    return r;
}

// @ 0x4c6190
tstring* gui_manager::vertex_string()
{
    tstring prefix(g_prefix);
    tstring* r = g_strtable.find(&(*(tstring*)((char*)this + 0x20) + prefix));
    return r ? r : (tstring*)this;
}

// @ 0x4c61f0
tstring gui_manager::find_vertex_string()
{
    tstring prefix(g_prefix);
    tstring* r = g_strtable.find(&(*(tstring*)((char*)this + 0x20) + prefix));
    if (r)
        return *r;
    tstring s = *(tstring*)((char*)this + 0x10);
    s.append(tstring(g_door));
    return s;
}

// @ 0x4c62b0
tstring* gui_manager::edge_string2()
{
    tstring* r = g_strtable.find(&(*(tstring*)((char*)this + 0x20) + tstring(g_door)));
    if (r)
        return r;
    tstring s = *(tstring*)((char*)this + 0x10);
    s.append(tstring(g_door));
    return r;
}
