// Slice s0052fe40: the mission view's per-frame camera update and the mesh-name
// chooser used by the level/object browser.
//   0x52fe40  View::apply: integrate the look axes into the eye position, build the
//             view matrix (identity + fu/su/sf rotations + a z rotation for the
//             2x2 heading), and scale the per-frame velocity vector.  Seven flag
//             out-parameters tell the caller which parts were touched.
//   0x5302f0  build_mesh_name: choose the mesh list for the selected object
//             (item / map / map point / passage / entry arrow) and look it up in
//             the resource manager.
// All names are provisional; every offset is read off this disassembly.
#include <windows.h>
#include <vector>
#include <math.h>
#pragma intrinsic(sqrt, fabs, sin, cos, tan, fsin, fcos)
#include "rgl.h"

extern "C" int _fltused = 0;

// ---- engine string (layout from s0042b790 / s004333c0) ------------------------------------------------
struct tstring {
    int pos;                                            // +0
    char* buf;                                          // +4
    int cap;                                            // +8
    int len;                                            // +0xc
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    ~tstring();                                         // 0x42bc40
    void operator=(const tstring& o);                   // 0x42bc50
    void assign(const char* s);                         // 0x42bcb0
    char equals(const tstring& o) const;                // 0x42bdd0
};

// ---- global input axes / camera flags (0xa17xxx block; names provisional) -------------------------------
extern int   g_a17148;                                  // 0xa17148
extern int   g_a17348;                                  // 0xa17348
extern int   g_a1715c;                                  // 0xa1715c
extern int   g_a17124;                                  // 0xa17124
extern int   g_a171b4;                                  // 0xa171b4
extern int   g_a173b4;                                  // 0xa173b4
extern int   g_a17120;                                  // 0xa17120
extern int   g_a17520;                                  // 0xa17520
extern int   g_a17188;                                  // 0xa17188
extern int   g_a17128;                                  // 0xa17128
extern int   g_a17184;                                  // 0xa17184
extern int   g_a1712c;                                  // 0xa1712c
extern int   g_a17194;                                  // 0xa17194
extern int   g_a17594;                                  // 0xa17594
extern float g_axis_x;                                  // 0xa178d4
extern float g_axis_y;                                  // 0xa178d8

extern float  g_k005;                                   // 0x590c60 (0.005f)
extern double g_d001;                                   // 0x59c480 (0.01)
extern float  g_one;                                    // 0x549cdc (1.0f)
extern float  g_k005b;                                  // 0x54b4d0 (0.05f)

// ---- game tables ---------------------------------------------------------------------------------------
namespace rgl {
struct mesh;
}

struct Item;
struct ItemRef {
    int item;                                           // +0
    unsigned m4;                                        // +4
    tstring mesh_name();                                // 0x434b20 (returns tstring by value)
};

struct Item {
    char pad0[0x20];
    tstring name;                                       // +0x20
    char pad30[0x1c8 - 0x30];
    int type_flags;                                     // +0x1c8 (low byte: item type)
    char pad1cc[0x238 - 0x1cc];
    tstring mesh_name_for(int bit);                     // 0x433bf0 (returns tstring by value)
    rgl::mesh* get_mesh(int a, unsigned mask, int b);   // 0x433cb0
};
extern Item* g_items;                                   // 0x5e29d0
extern int  g_item_count;                               // 0x5e29d4
int item_index(const tstring* name);                    // 0x434ad0

struct MapName { char pad[0x48]; };
struct MapEntry {
    tstring name;                                       // +0x00
    char pad10[8];
    int count;                                          // +0x18
    MapName* names;                                     // +0x1c
};
extern MapEntry* g_map;                                 // 0x5e28f8
extern int g_map_count;                                 // 0x5e29e8

struct MapPoint {
    char pad0[0x10];
    unsigned flags;                                     // +0x10 (bit 14)
    char pad14[0x40 - 0x14];
    std::vector<rgl::mesh*> meshes;                     // +0x40
    char pad50[0x78 - 0x50];
};
extern MapPoint* g_map_points;                          // 0x5e29dc
extern int g_map_points_count;                          // 0x5e29e0
int map_point_index(const tstring* name);               // 0x4352a0
extern int g_no_cam;                                    // 0x5a9fb0

// ---- resource manager (0x5d6fa0) -----------------------------------------------------------------------
struct resmgr {
    std::vector<rgl::mesh*> get(const tstring* name);   // 0x41b960
};
extern resmgr g_resmgr;                                 // 0x5d6fa0

// ---- the object the mesh chooser reads (offsets from this disassembly) --------------------------------
struct MeshOwner {
    char pad0[0x188];
    int f188;                                           // +0x188 kind
    int f18c;                                           // +0x18c item / map point id
    tstring name;                                       // +0x190
    int f1a0;                                           // +0x1a0 variant / index
};

// ---- matrix / rotation helpers -------------------------------------------------------------------------
struct Matrix12 {
    float v[12];
    void rotate_fu(const float* angle);                 // 0x40e570
    void rotate_su(const float* angle);                 // 0x40e670
    void rotate_sf(const float* angle);                 // 0x40e770
};
Matrix12* identity3(Matrix12* out);                     // 0x40cd60

struct Mat2 {
    float m[4];
    void rotate(float angle);                           // 0x4f3b40 (thiscall, ret 4)
};
struct vector2 {
    float x, y;
    vector2() {}
    vector2(float a, float b) : x(a), y(b) {}
};

// ---- the view object (`this`; +0x268 yaw, layout from s005284a0) --------------------------------------
struct ViewCtrl {
    char pad0[0x268];
    float yaw;                                          // +0x268

    void apply(char* f1, float* pos, char* f2, float* out, char* f3, float* vec, char* f4);
};

// rotate one width-2 row of the view matrix (the original inlines this three times)
static void rotate_pair(float* p, float c, float s)
{
    float x = p[0], y = p[1];
    p[0] = c * x - s * y;
    p[1] = c * y + s * x;
}

// @ 0x52fe40 ---------------------------------------------------------------------------------------------
void ViewCtrl::apply(char* f1, float* pos, char* f2, float* out, char* f3, float* vec, char* f4)
{
    float k = 0.6f;
    if (g_a17148 != 0 || g_a17348 != 0)
        k = 0.6f * 0.1;
    // two statements: the original rounds (y+x)*k to 24 bit before multiplying by 0.005
    float angle = (g_axis_x + g_axis_y) * k;
    angle = angle * g_k005;

    *f1 = 0;
    *f2 = 0;
    *f3 = 0;

    vector2 m[2];

    if (g_a1715c != 0) {
        vector2 a = vector2(1.0f, 0.0f);
        vector2 b = vector2(0.0f, 1.0f);
        m[0] = a;
        m[1] = b;
        ((Mat2*)&m[0])->rotate(yaw);

        float dx = g_axis_x * k * g_d001;
        pos[0] += m[0].x * dx;
        pos[1] += m[0].y * dx;
        float dy = g_axis_y * k * g_d001;
        pos[0] -= m[1].x * dy;
        pos[1] -= m[1].y * dy;
        *f1 = 1;
        *f4 = 1;
    }

    if (g_a17124 != 0) {
        if (g_a171b4 == 0 && g_a173b4 == 0) {
            // (k*0.01) first, then *axis_y, to match the original's 24-bit rounding order
            double dz = k * g_d001;
            pos[2] = pos[2] - dz * g_axis_y;
        } else
            pos[2] = 0.0f;
        *f1 = 1;
        *f4 = 1;
    }

    if (g_a17120 != 0 && g_a17520 == 0) {
        Matrix12 tmp;
        Matrix12* id = identity3(&tmp);
        *(Matrix12*)out = *id;
        *f2 = 1;
    }

    if (g_a17188 != 0 && g_a171b4 == 0 && g_a173b4 == 0 && g_a17194 == 0) {
        ((Matrix12*)out)->rotate_fu(&angle);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_a17128 != 0 && g_a171b4 == 0 && g_a173b4 == 0 && g_a17194 == 0) {
        ((Matrix12*)out)->rotate_su(&angle);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_a17184 != 0 && g_a171b4 == 0 && g_a173b4 == 0 && g_a17194 == 0) {
        ((Matrix12*)out)->rotate_sf(&angle);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_a1712c != 0) {
        float c = cos(angle);
        float s = sin(angle);
        rotate_pair(out + 0, c, s);
        rotate_pair(out + 4, c, s);
        rotate_pair(out + 8, c, s);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0 &&
        g_a17188 == 0 && g_a17128 == 0 && g_a17184 == 0) {
        float s = g_one - angle * g_k005b;
        vec[0] = vec[0] * s;
        vec[1] = vec[1] * s;
        vec[2] = vec[2] * s;
        *f3 = 1;
        *f4 = 1;
    }
    if (g_a17188 != 0 && g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        float s = g_one - angle * g_k005b;
        vec[0] = vec[0] * s;
        *f3 = 1;
        *f4 = 1;
    }
    if (g_a17128 != 0 && g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        float s = g_one - angle * g_k005b;
        vec[1] = vec[1] * s;
        *f3 = 1;
        *f4 = 1;
    }
    if (g_a17184 != 0 && g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        float s = g_one - angle * g_k005b;
        vec[2] = vec[2] * s;
        *f3 = 1;
        *f4 = 1;
    }
    if (g_a17194 != 0 && g_a17594 == 0 && (g_a171b4 != 0 || g_a173b4 != 0)) {
        vec[0] = 1.0f;
        vec[1] = 1.0f;
        vec[2] = 1.0f;
        vec[3] = m[1].y;
        *f3 = 1;
        *f4 = 1;
    }
}

// @ 0x5302f0 ---------------------------------------------------------------------------------------------
void __stdcall build_mesh_name(MeshOwner* o, std::vector<rgl::mesh*>* out)
{
    out->resize(0, (rgl::mesh*)0);

    tstring name;
    int kind = o->f188;

    if (kind == 2) {
        int idx = item_index(&o->name);
        name = g_items[idx].mesh_name_for(0);
    } else if (kind == 6) {
        int idx = item_index(&o->name);
        int var = o->f1a0;
        Item* it = &g_items[idx];
        int t = it->type_flags & 0xff;
        ItemRef ref;
        ref.item = idx;
        ref.m4 = var;
        rgl::mesh* m;
        if (t == 2 || t == 3 || t == 4 || t == 6 || t == 5 || t == 0x12) {
            if (idx >= 0 && idx < g_item_count)
                m = it->get_mesh(0, 0x30000000, var >> 24);
            else
                m = 0;
            if (m != 0) {
                out->push_back(m);
                goto done;
            }
        }
        {
            name = ref.mesh_name();
        }
    } else if (kind == 4) {
        int i;
        for (i = 0; i < g_map_count; i++) {
            if (g_map[i].name.equals(o->name) != 0)
                break;
        }
        if (i == g_map_count)
            i = 0;
        o->f1a0 = o->f1a0 % g_map[i].count;
        name = *(tstring*)((char*)g_map[i].names + o->f1a0 * 0x48);
    } else if (kind == 7) {
        name = g_items[o->f18c].mesh_name_for(o->f1a0);
    } else if (kind == 0) {
        int pi = map_point_index(&o->name);
        if (pi < 0)
            pi = o->f18c;
        if (pi >= 0 && pi < g_map_points_count) {
            MapPoint* p = &g_map_points[pi];
            if (g_no_cam != 0 || (p->flags & 0x4000) == 0)
                *out = p->meshes;
        }
    } else if (kind == 1) {
        if (g_no_cam != 0)
            name.assign("entry_arrow");
        else
            name.assign("");
    } else if (kind == 5) {
        if (g_no_cam != 0)
            name.assign("_mesh_passage");
        else
            name.assign("");
    }

done:
    if (name.len > 0) {
        std::vector<rgl::mesh*> tmp = g_resmgr.get(&name);
        *out = tmp;
    }
}
