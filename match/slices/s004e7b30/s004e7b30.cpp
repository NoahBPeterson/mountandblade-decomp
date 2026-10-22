// Slice s004e7b30 (batch ds1): the map scene's stream reader. It reads, in order, the scene tag, the 16-dword
// transform, four scenery lod-mesh names plus the scene's own resource (the last character of the name is
// toggled 'l' <-> 'R'), the scenery record list, a column-name table (up to 0x40 entries, each also getting a
// 0x38-byte record), the material table (names + a 0x38-byte record each), two tables of 0x10-byte strings, the
// grid of 0xb0-byte cells, the map slot ids, one object resource and four scalars, and finally a 0x204-byte
// scratch block copied into [0x78] plus a list of 0x204-byte block-face records.
// Names provisional. Offsets probed against the disassembly with a scratch layout probe. The count-prefixed
// allocations (scenery list, materials, cell grid) are walked through the pointers the offsets show.
#include <windows.h>
#include <math.h>
#include <new>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ------------------------------------------------------------------ engine string (0x10 bytes) ------------
struct tstring {
    int pos;                                    // +0x0
    char* buf;                                  // +0x4
    int cap;                                    // +0x8
    int len;                                    // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
    bool operator==(const tstring& o) const;    // 0x42bdd0
    tstring operator+(const char* s) const;     // 0x42c840
    void norm();                                // 0x42bf80 (lower-cases the name in place)
};

void mb_error(const char* fmt, ...);            // 0x42cc80
void* alloc_obj(unsigned n);                    // 0x535482 (block allocator)
void unpack_color_swap(vector4* out, const unsigned* argb);   // 0x40e4e0 (cdecl)
unsigned pack_color(vector4* c);                // 0x40e440 (cdecl)

extern "C" int _fltused = 0;

inline vector4* make_vec4(const tstring& s, vector4* out)   // 0x42bbe0 ctor + 0x42c840 join, then unpack
{
    char* p = s.buf;
    out->x = 0.0f; out->y = 0.0f; out->z = 0.0f; out->w = 0.0f;
    (void)p;
    return out;
}

namespace rgl {

struct vec2 { float x, y; };
struct point20 { vector4 a, b; };
struct face20 { vector4 n; int a, b, c, d; };
struct elem30 { float w[4]; int bone[4]; float bonef[4]; };

struct mesh_base {
    virtual ~mesh_base();
    virtual int base_slot1();
    tstring name;                               // 0x04
    int f14;
};

// rgl::mesh (0x260 bytes; see s0044f3c0/mesh.h), only the fields this reader touches
struct mesh : mesh_base {
    mesh* parent;                               // 0x18
    tstring name2;                              // 0x1c
    void* res;                                  // 0x2c
    unsigned flags;                             // 0x30
    int f34;
    char f38;
    unsigned color;                             // 0x3c
    int f40;
    float f44;
    int f48, f4c;
    std::vector<void*> v50;                     // 0x50
    std::vector<elem30> v60;                    // 0x60
    std::vector<point20> v70;                   // 0x70
    std::vector<void*> v80;                     // 0x80
    std::vector<face20> v90;                    // 0x90
    char local_a0, local_a1;                    // 0xa0: faces local / points local
    char pad_a2[0xa4 - 0xa2];

    std::vector<point20>& get70();              // 0x40eba0 (parent-chain walk, tail-call form)
};

// a lod-mesh resource: +0x38 -> the lod object; its first mesh list hangs off +0x38 and +0x84 is 0 when the
// mesh is missing
struct lod_mesh {
    char pad0[0x38];
    mesh** meshes;                              // +0x38
    char pad3c[0x84 - 0x40];
    int has;                                    // +0x84
};

struct lod_res {
    char pad0[0x38];
    lod_mesh* f38;                              // +0x38
};

} // namespace rgl

struct resmgr {
    rgl::lod_res* get_lod_mesh(tstring* name);  // 0x41cdc0
    rgl::lod_res* lookup(tstring* name);        // 0x418a70
    void* get_object70(tstring* name);          // 0x418cf0
};
extern resmgr g_resmgr;                         // 0x5d6fa0
extern int g_col_flag;                          // 0x5e28fc

// the access-code stream as the rest of the program sees it (an abstract access-code reader); while its type is
// incomplete the reader's calls cannot be devirtualised, so the original reaches its slots through these
// out-of-line helpers, which take the stream as their second argument
struct access_stream;
typedef access_stream scene_stream;

// a 0x10-byte string record kept in the [0x19c] / [0x1ac] vectors (0x4cbb10 resizes them)
struct str16 { tstring s; };
void resize_str16(std::vector<str16>* v, int n);            // 0x4cbb10

// a 0x18-byte material entry of the [0x44] array (name at +0x00, resource at +0x14)
struct entry18 {
    tstring name;                               // +0x00
    void* res14;                                // +0x14
};

// a 0x38-byte record shared by the column and material lists: 0x477880 initialises one per element (from the
// column's template / material name) and 0x473570 gives it points (the second this argument is the record)
struct rec38 {
    char pad0[0x38];
    void init(void* src);                       // 0x477880 (self, argument)
    void link(rgl::mesh* m, int count);         // 0x473570
};
void resize_rec38(int* dst, int n);             // 0x4ccf60 (element size 0x38, behind a count word)

// a 0x34-byte scenery entry of the [0x40] allocation (names at +0x00/+0x10, the mesh list at +0x30); the
// entry's mesh lives in the [0x8c] allocation and 0x4c6b30 fills the list it points at
struct lod_entry {
    tstring f00;                                // +0x00
    tstring f10;                                // +0x10
    int f20;                                    // +0x20
    float f24;                                  // +0x24
    float f28;                                  // +0x28
    void* mesh30;                               // +0x30
};

// the 16-dword block the stream writes at entry+0x2c (runs past the end of the 0x34-byte entry)
struct lod_block { int w[4]; };

// access-code stream members: while the stream's type is incomplete the vtable calls cannot be devirtualised,
// so the original calls them out of line with the stream as the second argument
void stream_f(access_stream* s, float* v);
void stream_block(access_stream* s, void* v);
void stream_i(access_stream* s, int* v);
void stream_str(access_stream* s, tstring* v);

// a 0xb0-byte grid cell (0x4ce680 ctor)
struct cell_b0 {
    float f00, f04, f08;                        // +0x00..: material fields
    char pad0c[4];
    tstring res10;                              // +0x10: the material resource
    rgl::mesh* tmpl14;                          // +0x14
    std::vector<int> v1c;                       // +0x1c
    char pad28[4];
    vector4 tiles[8];                           // +0x2c: 8 sixteen-byte blocks
    int n_ac;                                   // +0xac
    cell_b0();                                  // 0x4ce680
};

// a 0x204-byte block-face record of the [0x78] array: an entry count (at most 0x3f) and 8-byte uv entries
struct face204_uv { float f; int i; };

struct face204 {
    int count;                                  // +0x00
    face204_uv uv[0x7f];                        // +0x04 .. +0x200
    int tail;                                   // +0x200
};

// the front table the map slot ids are looked up in (0x5aa130 count, 0x5aa134 entries of 0x98 bytes)
extern int g_front_count;                       // 0x5aa130
extern char* g_front;                           // 0x5aa134

struct front_rec {
    char pad0[0xd0];
    tstring name;                               // +0xd0
};

// the scene object (0x4e7b30's this)
struct scene {
    tstring tag;                                // +0x00 (the first string read)
    int m10[0x10];                              // +0x10: read by slot +0x40 (16 dwords)
    int v78[0x204 / 4];                         // +0x78: the copied 0x204-byte table (0x4ccf60 resizes)
    int n88;                                    // +0x88
    lod_entry* entries;                         // +0x8c: the scenery entries
    rgl::mesh** lod_meshes;                     // +0x90-area: the meshes' point arrays (see read)
    char pad94[0x18c - 0x94];
    int n18c;                                   // +0x18c: the scenery entry count
    int n190;                                   // +0x190: the cell count
    tstring name194;                            // +0x194
    std::vector<str16> v19c;                    // +0x19c
    tstring name198;                            // +0x198 (overlaps the head of v19c)
    std::vector<str16> v1ac;                    // +0x1ac
    cell_b0* cells;                             // +0x1bc
    char pad1c0[0x1cc - 0x1c0];
    int slot1cc[8];                             // +0x1cc (indexed by the stream int)
    int obj1e8;                                 // +0x1e8
    int f1ec;                                   // +0x1ec
    float f1f0;                                 // +0x1f0
    int f1f4;                                   // +0x1f4
    char pad1f8[0x228 - 0x1f8];

    void read(rgl::mesh* owner, scene_stream* s);   // 0x4e7b30
};

// one block-face slot of the [0x78] table (8-byte uv entries)
struct uv8 { float f; int i; };                          // one 8-byte uv entry

// ------------------------------------------------------------------ the reader ---------------------------------
void scene::read(rgl::mesh* owner, scene_stream* s)
{
    tstring name;
    stream_str(s, &name);                                            // tag
    stream_block(s, m10);
    stream_str(s, &name);
    entries = (lod_entry*)g_resmgr.get_lod_mesh(&name);         // +0x14
    stream_str(s, &name);
    entries = (lod_entry*)g_resmgr.get_lod_mesh(&name);         // +0x20
    {
        char* p = name.buf;
        p[name.len - 1] = (p[name.len - 1] != 'l') ? 'R' : 'l';
    }
    entries = (lod_entry*)g_resmgr.get_lod_mesh(&name);         // +0x24
    stream_str(s, &name);
    entries = (lod_entry*)g_resmgr.get_lod_mesh(&name);         // +0x18
    {
        char* p = name.buf;
        p[name.len - 1] = (p[name.len - 1] != 'l') ? 'R' : 'l';
    }
    entries = (lod_entry*)g_resmgr.get_lod_mesh(&name);         // +0x1c
    stream_str(s, &name);
    {
        rgl::lod_res* r = g_resmgr.get_lod_mesh(&name);
        if (r->f38->has == 0) {
            mb_error((const char*)0x598328, name.buf ? name.buf : (char*)&name.buf);
        }
    }
    int n = 0;
    stream_i(s, &n);
    n18c = n;
    lod_entry* e = (lod_entry*)alloc_obj(n * 0x34 + 4);
    if (e != 0) {
        ((int*)e)[0] = n;
        e = (lod_entry*)((char*)e + 4);
        lod_entry* q = e;
        for (int i = n - 1; i >= 0; i--) {
            new (&q->f00) tstring();
            new (&q->f10) tstring();
            q->mesh30 = 0;
            q = (lod_entry*)((char*)q + 0x34);
        }
    }
    entries = e;
    for (int i = 0; i < n; i++) {
        stream_str(s, &e[i].f10);
        stream_i(s, &e[i].f20);
        stream_block(s, e[i].mesh30);
        stream_f(s, &e[i].f24);
        stream_f(s, &e[i].f28);
        stream_str(s, &e[i].f00);
        e[i].f00.norm();                                        // 0x42bf80
        rgl::lod_res* lr = (rgl::lod_res*)owner;
        rgl::mesh* m = lr->f38->meshes[0];
        std::vector<rgl::point20>& pts = m->get70();
        e[i].mesh30 = alloc_obj((unsigned)(pts.size() << 4));
        rec38* r = (rec38*)((char*)e + i * 0x34 + 0x4);
        r->link((rgl::mesh*)e[i].mesh30, (int)pts.size());
    }
    rec38* colrec = (rec38*)((char*)e + 0x4);                   // the columns' template records
    {
        int cnt = 0;
        stream_i(s, &cnt);
        int ncol = cnt;
        for (int i = 0; i < ncol; i++) {
            tstring nm;
            stream_str(s, &nm);
            if (i < 0x40) {
                colrec[i].pad0[0] = *(char*)&nm;                // 0x41cdc0 into the column record's mesh
            }
            nm.~tstring();
        }
        if (ncol > 0x40) {
            ncol = 0x40;
        }
        n88 = ncol;
        rec38* cols = colrec;
        resize_rec38(v78, ncol);
        for (int i = 0; i < ncol; i++) {
            cols[i].init(colrec[i].pad0);
        }
    }
    {
        int nmat = 0;
        stream_i(s, &nmat);
        entry18* mats = 0;
        if (nmat != 0) {
            entry18* raw = (entry18*)alloc_obj(nmat * 0x18 + 4);
            if (raw != 0) {
                ((int*)raw)[0] = nmat;
                mats = (entry18*)((char*)raw + 4);
                entry18* q = mats;
                for (int i = nmat - 1; i >= 0; i--) {
                    new (&q->name) tstring();
                    q->res14 = 0;
                    q = (entry18*)((char*)q + 0x18);
                }
            }
        }
        int* matrecs = 0;
        if (mats != 0) {
            char* mr = (char*)alloc_obj(nmat * 0x38 + 4);
            if (mr != 0) {
                *(int*)mr = nmat;
                matrecs = (int*)(mr + 4);
                for (int i = 0; i < nmat; i++) {
                    ((rec38*)(mr + 4 + i * 0x38))->init(mats[i].name.buf);
                }
            }
            resize_rec38(v78 + 1, nmat);                        // the material records' list
        }
        for (int i = 0; i < nmat; i++) {
            stream_str(s, &mats[i].name);
            mats[i].res14 = g_resmgr.get_lod_mesh(&mats[i].name);
            rgl::lod_res* lr = (rgl::lod_res*)owner;
            if (lr->f38->has == 0) {
                mb_error((const char*)0x598308, mats[i].name.buf);
            }
            rec38* r = (rec38*)((char*)matrecs + i * 0x38);
            r->link((rgl::mesh*)mats[i].res14, 0x38);
        }
    }
    int n1 = 0;
    {
        stream_i(s, &n1);
        resize_str16(&v19c, n1);
        for (int i = 0; i < n1; i++) {
            stream_str(s, (tstring*)((char*)v19c._Myfirst + i * 0x10));
        }
        int n2 = 0;
        stream_i(s, &n2);
        resize_str16(&v1ac, n2);
        for (int i = 0; i < n2; i++) {
            stream_str(s, (tstring*)((char*)v1ac._Myfirst + i * 0x10));
        }
    }
    {
        int ncell = 0;
        stream_i(s, &ncell);
        n190 = ncell;
        cell_b0* c = (cell_b0*)alloc_obj(ncell * 0xb0 + 4);
        if (c != 0) {
            ((int*)c)[0] = ncell;
            c = (cell_b0*)((char*)c + 4);
            for (int i = ncell - 1; i >= 0; i--) {
                new (&c[i]) cell_b0();
            }
        }
        cells = c;
        for (int i = 0; i < ncell; i++) {
            cell_b0* cl = &cells[i];
            tstring nil;
            stream_str(s, &nil);                                     // the cell's name
            g_resmgr.lookup(&nil);                              // the material resource at +0x10
            stream_block(s, &cl->f04);
            if (g_col_flag) {
                vector4 col;
                make_vec4(nil, &col);
                unpack_color_swap(&col, (const unsigned*)&cl->f04);
                col.x *= col.x;
                col.y *= col.y;
                col.z *= col.z;
                pack_color(&col);
            }
            int k = 0;
            stream_i(s, &k);
            int cnt = 0;
            stream_i(s, &cnt);
            if (cnt >= 2) {
                cl->n_ac = cnt;
            }
            int nref = 0;
            stream_i(s, &nref);
            for (int j = 0; j < nref; j++) {
                tstring nm;
                stream_str(s, &nm);
                int found = -1;
                for (int t = 0; t < n1; t++) {
                    if (((tstring*)((char*)v19c._Myfirst + t * 0x10))->operator==(nm)) {
                        found = t;
                        break;
                    }
                }
                if (found != -1) {
                    str16* last = v19c._Mylast;
                    if (v19c._Myfirst && last < v19c._Myend) {
                        *(int*)last = found;
                        v19c._Mylast = last + 1;
                    } else {
                        v19c._Mylast = last;            // vector<int>::insert(end, 1, found) (0x43e7b0)
                    }
                }
                nm.~tstring();
            }
            int n4 = 0;
            stream_i(s, &n4);
            for (int j = 0; j < n4; j++) {
                vector4 v;
                stream_block(s, &v);
                unpack_color_swap(&cl->tiles[j], (const unsigned*)&v);
            }
        }
    }
    {
        int ns = 0;
        stream_i(s, &ns);
        tstring nm;
        for (int i = 0; i < ns; i++) {
            int id = 0;
            stream_i(s, &id);
            stream_str(s, &nm);
            if (id >= 0 && id < 7) {
                int found = -1;
                for (int j = 0; j < g_front_count; j++) {
                    if (((front_rec*)(g_front + j * 0x98))->name.operator==(nm)) {
                        found = j;
                        break;
                    }
                }
                slot1cc[id] = found;
            }
        }
    }
    {
        tstring nm;
        stream_str(s, &nm);
        obj1e8 = (int)(size_t)g_resmgr.get_object70(&nm);
    }
    stream_f(s, &f1f0);
    stream_i(s, &f1ec);
    stream_i(s, &f1f4);
    {
        int scratch[0x81];
        memset(scratch, 0, 0x204);
        stream_block(s, scratch);
        resize_rec38(v78, scratch[0x80]);                       // 0x4ccf60: the table is sized to scratch[0x80]
        int nface = 0;
        stream_i(s, &nface);
        for (int i = 0; i < nface; i++) {
            face204* f = (face204*)((char*)v78 + 4 + i * 0x204);
            stream_i(s, &f->tail);
            stream_i(s, &f->count);
            if (f->count >= 0x3f) {
                f->count = 0x3f;
            }
            for (int j = 0; j < f->count; j++) {
                stream_f(s, &f->uv[j].f);
                stream_i(s, &f->uv[j].i);
            }
        }
    }
    name.~tstring();
}
