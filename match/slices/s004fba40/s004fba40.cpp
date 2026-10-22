// Slice s004fba40: builds the display mesh of an item reference (0x4fba40) and darkens
// its vertex colours in place (0x4fbef0). Names provisional; layouts from the disassembly.
#include <windows.h>
#include <new>
#include <vector>

// 16-byte rgl string (global class tstring; ctor 0x42bbe0, dtor 0x42bc40).
struct tstring {
    int pos;            // +0x00
    char* buf;          // +0x04
    int cap;            // +0x08
    int len;            // +0x0c
    ~tstring();         // 0x42bc40
};

// 64-byte rgl transform (rows s, f, u, o).
struct vector4 { float x, y, z, w; };
struct mat4 { vector4 s, f, u, o; };

mat4* __cdecl identity_pos(mat4* m);                   // 0x40cd00
mat4* __cdecl identity3(mat4* m);                      // 0x40cd60
mat4* __cdecl matrix_rotate_fu(float* angle);          // 0x40e570
mat4* __cdecl matrix_rotate_su(float* angle);          // 0x40e670
mat4* __cdecl matrix_rotate_sf(float* angle);          // 0x40e770
void __cdecl unpack_color(float* out, const unsigned* c);   // 0x40e4e0
unsigned __cdecl pack_color(const float* c);                // 0x40e440
void __cdecl mb_warning(const char* fmt, ...);              // 0x42cb80

// One vertex of mesh::v50 (0x60 bytes); only the fields this slice touches.
struct vertex60 {
    char pad0[0x38];
    float f38;          // +0x38 (shade value)
    char pad3c[0x54 - 0x3c];
    unsigned color;     // +0x54 packed ARGB
    char pad58[0x60 - 0x58];
};

struct mesh {
    char pad0[0x50];
    std::vector<vertex60> v50;          // +0x50 (first +0x54, last +0x58)
    char pad60[0x138 - 0x60];
    char f138;                          // +0x138 (dirty flag)

    mesh();                             // 0x412a90
    mesh* clone();                      // 0x41c6d0
    void append_skinned(mesh* other, const mat4* m, int a, int b, char c);  // 0x44eb00
    void translate(const vector4* v);   // 0x44f240
    void scale(float s);                // 0x44f0c0
    void transform(const mat4* m);      // 0x495ba0
    void calc_bounds(vector4* min, vector4* max);   // 0x438d80
    void ensure_local();                // 0x412ee0
    void darken_vertices();             // 0x4fbef0
};

// A std::vector<mesh*> as this CRT lays it out: 16 bytes, first at +4.
struct mesh_vec {
    int pad;
    mesh** first;
    mesh** last;
    mesh** end;
};

struct resmgr {
    mesh_vec* get_objects(mesh_vec* out, const tstring* name);  // 0x41b960
};
extern resmgr g_resmgr;                     // 0x5d6fa0

struct ItemRef {
    int item;                               // +0x00
    unsigned m4;                            // +0x04 (byte +7 selects the mesh variant)
    tstring mesh_name() const;              // 0x434b20
};

struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                    // +0x1c8 (low byte = rotation class)
    char pad1cc[0x20c - 0x1cc];
    int f20c;                               // +0x20c (size percent)
    char pad210[0x238 - 0x210];
    mesh* get_mesh(int a, int b, int c);    // 0x433cb0
};

extern ItemKind* g_items;                   // 0x5e29d0
extern int g_item_count;                    // 0x5e29d4

// @ 0x4fba40 -------------------------------------------------------------------------------
mesh* __cdecl make_item_mesh(const ItemRef* ref, const float* scale)
{
    mesh* result;
    mat4 m;
    if (ref->item >= 0 && ref->item < g_item_count &&
        (result = g_items[ref->item].get_mesh(0, 0x10000000, (int)*(signed char*)((const char*)ref + 7))) != 0) {
        result = result->clone();
    } else {
        mesh_vec objects;
        {
            tstring name = ref->mesh_name();
            g_resmgr.get_objects(&objects, &name);
        }
        if (objects.first == 0 || objects.last - objects.first == 0) {
            {
                tstring name = ref->mesh_name();
                const char* s = name.buf ? name.buf : (const char*)&name.buf;
                mb_warning("Unable to find mesh %s", s);
            }
            result = new mesh;
            if (objects.first)
                ::operator delete(objects.first);
            return result;
        }
        result = objects.first[0]->clone();
        for (unsigned i = 1; i < (unsigned)(objects.last - objects.first); i++) {
            identity_pos(&m);
            result->append_skinned(objects.first[i], &m, -1, -1, 0);
        }
        if (objects.first)
            ::operator delete(objects.first);
        objects.first = 0;
        objects.last = 0;
        objects.end = 0;
    }

    unsigned type = g_items[ref->item].type_flags & 0xff;
    identity3(&m);
    float angle;
    if (type == 0xd) {
        angle = -1.5707964f;
        matrix_rotate_fu(&angle);
        angle = 3.1415927f;
        goto sf;
    } else if (type == 1) {
        angle = -1.5707964f;
        matrix_rotate_fu(&angle);
        angle = 1.5707964f;
        goto sf;
    } else if (type == 2 || type == 3 || type == 4) {
        angle = -0.7853982f;
        goto sf;
    } else if (type == 0xe) {
        angle = -1.5707964f;
        matrix_rotate_fu(&angle);
        angle = 2.670354f;
        matrix_rotate_sf(&angle);
        angle = -0.31415927f;
        matrix_rotate_fu(&angle);
        goto done;
    } else if (type == 7) {
        angle = 1.5707964f;
        goto sf;
    } else if (type == 0xc) {
        angle = -0.47123894f;
        matrix_rotate_su(&angle);
        angle = 0.62831855f;
        matrix_rotate_fu(&angle);
        goto done;
    } else if (type == 9) {
        angle = 1.5707964f;
        matrix_rotate_su(&angle);
        angle = -2.3561945f;
        matrix_rotate_fu(&angle);
        goto done;
    } else if (type == 0x11) {
        goto done;
    } else if (type == 8) {
        angle = 2.3561945f;
        goto sf;
    } else if (type == 0xa || type == 5 || type == 6) {
        angle = 0.7853982f;
        goto sf;
    } else if (type == 0xb) {
        angle = -1.2566371f;
        matrix_rotate_fu(&angle);
        angle = 0.7853982f;
        goto sf;
    } else if (type == 0x14) {
        angle = -0.62831855f;
        matrix_rotate_fu(&angle);
        angle = -0.47123894f;
        goto sf;
    } else {
        goto done;
    }
sf:
    matrix_rotate_sf(&angle);
done:
    result->transform(&m);

    vector4 bmin, bmax;
    result->calc_bounds(&bmin, &bmax);

    float sumz = bmax.z + bmin.z;
    float cx = (bmin.x + bmax.x) * 0.5f;
    float cy = (bmin.y + bmax.y) * 0.5f;

    unsigned t2 = g_items[ref->item].type_flags & 0xff;
    float size = 1.0f;
    if (t2 == 2 || t2 == 3 || t2 == 4) {
        size = (float)g_items[ref->item].f20c * 0.00625f + 0.3f;
        if (size < 0.5f)
            size = 0.5f;
        else if (size > 1.0f)
            size = 1.0f;
    }

    float extent = bmax.y - bmin.y;
    if (extent < bmax.x - bmin.x)
        extent = bmax.x - bmin.x;

    vector4 tr;
    tr.x = -cx;
    tr.y = -cy;
    tr.z = -(sumz * 0.5f);
    tr.w = 0.0f;
    result->translate(&tr);
    result->scale(((*scale / extent) * size) * 0.95f);
    result->darken_vertices();
    return result;
}

// @ 0x4fbef0 -------------------------------------------------------------------------------
void mesh::darken_vertices()
{
    ensure_local();
    for (unsigned i = 0; i < v50.size(); i++) {
        vertex60& v = v50[i];
        float f = (v.f38 + 0.25f) * 0.8f;
        float rgb[3];
        unsigned* color = &v.color;
        unpack_color(rgb, color);
        rgb[0] *= f;
        rgb[1] *= f;
        rgb[2] *= f;
        *color = pack_color(rgb);
    }
    f138 = 1;
}
