// Slice s004d4b80: mission-screen button handler + world teardown.
//   0x4d5290  Hud::handle_buttons  forward the frame to the item source, then test every toolbar button
//             (used-this-frame flag 0x10000) and open the matching main-menu option.
//   0x4d51d0  world_destroy        sub-object destructors of the world object, then its six vectors.
//   0x4d4b80  Mesh::on_selection_changed  collect the (up to four, expect three) selected vertices,
//             when exactly four are selected find the faces touching them, compute their average normal,
//             classify each of the four against it (three cross products), and add the single triangle the
//             resulting edge test picks; deletes a face otherwise.
// Offsets are read off the disassembly; names provisional. 0x495700 is a main-menu option choice on the
// object at 0xa73980.
#include <windows.h>
#include <vector>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

// ---- the mesh object (0x4d1d80 / 0x470920 family) --------------------------------------------------------
// Layout verified byte-exact in the neighbour slice s004d1d80: vertices first at +0x1c (0x18 bytes),
// faces first at +0x2c (0x64 bytes), edges first at +0x3c (0x34 bytes).
struct Vtx {                                              // 0x18: position, a flag, the vertex id
    float x, y, z;                                        // 0x00
    float f0c;
    int   f10;                                            // 0x10
    int   id;                                             // 0x14 (non-zero when the vertex is selected)
};
struct FaceM {                                            // 0x64
    int f0;                                               // 0x00
    int f4;                                               // 0x04
    vector4 centroid;                                     // 0x08
    vector4 normal;                                       // 0x18
    int vid[4];                                           // 0x28
    int eid[4];                                           // 0x38
    int n;                                                // 0x48 number of used vid slots
    int nbr[4];                                           // 0x4c
    int f5c;                                              // 0x5c
    char f60;                                             // 0x60
    char pad61[3];
};
struct EdgeM {                                            // 0x34
    int v0, v1;                                           // 0x00
    int face[2];                                          // 0x08
    int nfaces;                                           // 0x10
    char flag;                                            // 0x14
    char pad15[3];
    float crease[2];                                      // 0x18
    int uses;                                             // 0x20
    vector4 mid;                                          // 0x24
};

struct Mesh {
    char pad0[0x18];
    std::vector<Vtx>  verts;                              // +0x18 (first at 0x1c)
    std::vector<FaceM> faces;                             // +0x28 (first at 0x2c)
    std::vector<EdgeM> edges;                             // +0x38 (first at 0x3c)
    bool add_triangle(int* tri);                          // 0x4d1d80 (another slice)
    void delete_face(int i);                              // 0x4cb940
    void rebuild();                                       // 0x470920
    bool on_selection_changed();                          // 0x4d4b80
};
// callees are members of Mesh, defined in other TUs (no body here so cl emits a real call)

extern const float g_zero;                                // 0x549ca8 0.0f
extern const float g_one;                                 // 0x549cdc 1.0f

// rgl::vector4 with the (1e-10) normalize callers inline (0x407d60).
struct Vec4 : vector4 {
    Vec4() {}
    Vec4(float a, float b, float c) : vector4(a, b, c) {}
    float normalize_inl()
    {
        float len = length();
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            x *= inv; y *= inv; z *= inv;
        } else {
            x = 0.0f; y = 1.0f; z = 0.0f;
        }
        return len;
    }
};

// ---- menu ------------------------------------------------------------------------------------------------
struct MainMenu {
    char pad0[0x18];
    int  sel;                                       // +0x18
    void choose(int option);                        // 0x495700 (thiscall, ret 4)
};
extern MainMenu g_main_menu;                        // 0xa73980

// menu widgets the options write back into: +0x4 = option index, +0x18 = a second index, +0x8c = value
extern int* g_menu_w1;                              // 0xa7398c
extern int* g_menu_w4;                              // 0xa739c8
extern int* g_menu_w5;                              // 0xa739d4
extern int* g_menu_w6;                              // 0xa739f0
extern char g_5e2a08;                               // 0x5e2a08

// ---- widgets ---------------------------------------------------------------------------------------------
struct Widget {
    virtual void m00(); virtual void m01(); virtual void m02(); virtual void m03();
    virtual void m04(); virtual void m05(); virtual void m06(); virtual void m07();
    virtual void m08(); virtual void m09(); virtual void m0a();
    virtual void activate(int v);                   // +0x2c
    virtual void m0c();
    virtual int  get_value();                       // +0x30
    virtual void m0e(); virtual void m0f();
    virtual float get_float();                      // +0x38
    virtual char used(int flag);                    // +0x44
};

// the item source at Hud+0x30: its pick (0x4d5410) lives in slice s004d5410
struct ItemSource {
    void pick(int* arg);                            // 0x4d5410
};

struct Hud {
    char    pad00[0x4];
    int     f04;                                    // +0x4
    char    pad08[0x30 - 8];
    char    src30[0x4];                             // +0x30 item source (this-argument of 0x4d5410)
    char    pad34[0x60 - 0x34];
    Widget* f60;                                    // +0x60
    Widget* f64;                                    // +0x64
    Widget* f68;                                    // +0x68
    Widget* f6c;                                    // +0x6c (may be null)
    Widget* f70;                                    // +0x70
    Widget* f74;                                    // +0x74
    Widget* f78;                                    // +0x78
    char    pad7c[0x88 - 0x7c];
    int     f88;                                    // +0x88

    void handle_buttons(int arg);                   // 0x4d5290
};

// @ 0x4d4b80
bool Mesh::on_selection_changed()
{
    int idx[4];
    int count = 0;
    for (unsigned i = 0; i < verts.size(); i++) {
        if (verts[i].id != 0) {
            if (count < 4)
                idx[count] = (int)i;
            count++;
        }
    }
    if (count == 3)
        return add_triangle(idx);
    if (count != 4)
        return false;

    std::vector<int> found;
    for (int i = 0; i < (int)faces.size(); i++) {
        int n = faces[i].n;
        int cnt = 0;
        const int* p = faces[i].vid;
        for (int j = 0; j < n; j++) {
            int v = *p++;
            if (v == idx[0]) cnt++;
            if (v == idx[1]) cnt++;
            if (v == idx[2]) cnt++;
            if (v == idx[3]) cnt++;
        }
        if (cnt > 0)
            found.push_back(i);
        if (cnt > 2)
            return false;
    }

    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
    for (unsigned i = 0; i < found.size(); i++) {
        FaceM& f = faces[found[i]];
        nx = nx + f.normal.x;
        ny = ny + f.normal.y;
        nz = nz + f.normal.z;
    }
    Vec4 nrm(nx / (float)found.size(), ny / (float)found.size(), nz / (float)found.size());
    nrm.normalize_inl();

    const Vtx* v = &verts[idx[0]];
    Vec4 a(v[idx[1]].x - v->x, v[idx[1]].y - v->y, v[idx[1]].z - v->z);
    Vec4 b(v[idx[2]].x - v->x, v[idx[2]].y - v->y, v[idx[2]].z - v->z);
    Vec4 c(v[idx[3]].x - v->x, v[idx[3]].y - v->y, v[idx[3]].z - v->z);

    int cls[3];
    cls[0] = cls[1] = cls[2] = 0;
    if ((a.z * b.y - a.y * b.z) * nrm.x +
        ((a.x * b.z - b.x * a.z) * nrm.y +
         (b.x * a.y - a.x * b.y) * nrm.z) > 0.0f)
        cls[0] = 1;
    else
        cls[1] = 1;
    if ((b.z * c.y - b.y * c.z) * nrm.x +
        ((c.x * b.z - b.x * c.z) * nrm.y +
         (b.x * c.y - c.x * b.y) * nrm.z) > 0.0f)
        cls[1] = cls[1] + 1;
    else
        cls[2] = 1;
    if ((b.y * a.z - b.z * a.y) * nrm.x +
        ((b.z * c.x - b.x * c.z) * nrm.y +
         (b.x * c.y - c.x * b.y) * nrm.z) > 0.0f)
        cls[2] = cls[2] + 1;
    else
        cls[0] = cls[0] + 1;

    if (cls[0] == 1 && cls[1] == 1 && cls[2] == 1)
        return false;

    char ok = 1;
    int nfaces = (int)faces.size();
    for (int i = 0; i < 3; i++) {
        if (cls[i] != 1)
            continue;
        for (int j = 0; j < 3; j++) {
            if (cls[j] == 1)
                continue;
            if (ok) {
                int tri[3];
                tri[0] = idx[0];
                tri[1] = idx[i + 1];
                tri[2] = idx[j + 1];
                ok = add_triangle(tri);
                if (!ok) {
                    if (nfaces < (int)faces.size()) {
                        delete_face((int)faces.size() - 1);
                        rebuild();
                    }
                    return false;
                }
            }
        }
    }
    if (ok) {
        int sel;
        if (cls[0] == 1)
            sel = idx[1];
        else if (cls[1] == 1)
            sel = idx[2];
        else
            sel = idx[3];
        for (unsigned i = 0; i < edges.size(); i++) {
            EdgeM& e = edges[i];
            if ((e.v0 == idx[0] && e.v1 == sel) || (e.v1 == idx[0] && e.v0 == sel))
                break;
        }
    }
    return true;
}

// @ 0x4d5290
void Hud::handle_buttons(int arg)
{
    ((ItemSource*)((char*)(void*)this + 0x30))->pick(&arg);
    if (f60->used(0x10000)) {
        g_menu_w1[6] = 4;
        g_menu_w1[1] = 3;
        g_menu_w1[0x8c / 4] = 0;
        g_main_menu.choose(3);
        return;
    }
    if (f64->used(0x10000)) {
        g_menu_w1[1] = 0;
        g_menu_w1[0x8c / 4] = f88;
        g_main_menu.choose(3);
        return;
    }
    if (f68->used(0x10000)) {
        g_menu_w5[6] = 4;
        g_main_menu.choose(0x15);
        return;
    }
    if (f6c != 0 && f6c->used(0x10000)) {
        g_menu_w1[6] = 4;
        g_menu_w1[1] = 2;
        g_menu_w1[0x8c / 4] = 0;
        g_main_menu.choose(3);
        return;
    }
    if (f70->used(0x10000)) {
        g_menu_w4[6] = 4;
        g_main_menu.choose(0x12);
        return;
    }
    if (f74->used(0x10000)) {
        g_menu_w6[6] = 4;
        g_main_menu.choose(0x1c);
        return;
    }
    if (f78->used(0x10000))
        g_5e2a08 = 1;
}

// ---- the world object (0x5e292c) -------------------------------------------------------------------------
extern "C" void __cdecl free_ptr(void* p);          // 0x535712 (operator delete / free)
void world_base_dtor(void* p);                      // 0x479160
void dtor_6814(void* p);                            // 0x42bc40 (16-byte string at +0x6814)
void dtor_6698(void* p);                            // 0x4cb780
void dtor_6688(void* p);                            // 0x431a60
void dtor_63d0(void* p);                            // 0x4be6f0
void dtor_0030(void* p);                            // 0x4c6e30
void dtor_0004(void* p);                            // 0x4c6e30

struct World {
    // the sub-objects the destructor tears down, in address order
    struct Vec { void* begin; void* end; void* cap; };  // an STL vector triple

    void destroy();                                 // 0x4d51d0 (tail-call shape, ret 0)
};

// @ 0x4d51d0
void World::destroy()
{
    char* p = (char*)(void*)this;
    world_base_dtor(p);
    dtor_6814(p + 0x6814);
    dtor_6698(p + 0x6698);
    dtor_6688(p + 0x6688);
    if (*(void**)(p + 0x667c))
        free_ptr(*(void**)(p + 0x667c));
    *(void**)(p + 0x667c) = 0;
    *(void**)(p + 0x6680) = 0;
    *(void**)(p + 0x6684) = 0;
    dtor_63d0(p + 0x63d0);
    if (*(void**)(p + 0x63c4))
        free_ptr(*(void**)(p + 0x63c4));
    *(void**)(p + 0x63c4) = 0;
    *(void**)(p + 0x63c8) = 0;
    *(void**)(p + 0x63cc) = 0;
    if (*(void**)(p + 0x5e00))
        free_ptr(*(void**)(p + 0x5e00));
    dtor_0030(p + 0x30);
    *(void**)(p + 0x5e00) = 0;
    *(void**)(p + 0x5e04) = 0;
    *(void**)(p + 0x5e08) = 0;
    dtor_0004(p + 0x4);
}
