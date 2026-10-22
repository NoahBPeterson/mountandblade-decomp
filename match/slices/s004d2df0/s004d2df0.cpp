// Slice s004d2df0: a small collision-mesh "add a quad" helper (0x4d2df0), a ragdoll-body / bone definition
// loader from XML (0x4d3330) and two large record constructors (0x4d3580, 0x4d3640). Names provisional.
#include <windows.h>
#include <d3d9.h>
#include "rgl.h"
#include "tinyxml.h"

using rgl::vector4;

// ---- engine string (16 bytes, layout from s004bb3f0 / s00463790) ------------------------------------------
struct tstring {
    int pos;                // +0
    char* buf;              // +4
    int cap;                // +8
    int len;                // +0xc
    tstring();                                      // 0x42bbd0
    void assign(const char* s);                     // 0x42bcb0
};

// ---- the engine's growable arrays: empty allocator slot then begin/last/end (begin at +4) ----------------
template<class T> struct vec {
    char al;                                        // +0
    T* b;                                           // +4
    T* e;                                           // +8
    T* cap;                                         // +0xc
    vec() : b(0), e(0), cap(0) {}
    unsigned size() const { return b ? (unsigned)(e - b) : 0u; }
    T& operator[](unsigned i) { return b[i]; }
};

// ---- 0x18-byte vertex: position, a zeroed flag byte, a vertex id packed into 0x18 bytes ---------------------
struct E24 {
    float x, y, z, w;                               // +0 (w left untouched by the ctor)
    char f10;                                       // +0x10
    char pad11[3];
    int id;                                         // +0x14
    E24() {}
    E24(float x_, float y_, float z_) : x(x_), y(y_), z(z_) { f10 = 0; }
};

// ---- 0x34-byte edge: two vertex ids, two face refs (-10000), count, flag, three crease values ---------------
struct E52 {
    int v0, v1;                                     // +0
    int face0, face1;                               // +8 (-10000 = none)
    int nfaces;                                     // +0x10
    char flag;                                      // +0x14
    char pad15[3];
    float crease0, crease1, crease2;                // +0x18
    int rest[4];                                    // +0x24
    E52() { face0 = -10000; face1 = -10000; }
};

// ---- 0x64-byte face: ids at +0x28/+0x38, n at +0x48, flags at +0x5c/+0x60 --------------------------------
struct E100 {
    int f0;
    int pad4[9];
    int vid[4];                                     // +0x28
    int eid[4];                                     // +0x38
    int n;                                          // +0x48
    int pad4c[4];
    int f5c;                                        // +0x5c
    char f60;                                       // +0x60
    char pad61[3];
    E100() { f0 = 0; f5c = 0; f60 = 0; }
};

// out-of-line instantiations of std::vector<T>::push_back for the three mesh element types
struct E24vec : vec<E24> {
    void push_back(const E24&);                                 // 0x477c10
};
struct E52vec : vec<E52> {
    void push_back(const E52&);                                 // 0x477e10
};
struct E100vec : vec<E100> {
    void push_back(const E100&);                                // 0x477d20
};

// the collision mesh: three vecs at +0x18 (vertices), +0x28 (faces), +0x38 (edges)
struct QuadMesh {
    char pad0[0x18];
    E24vec verts;                                   // 0x18
    E100vec faces;                                  // 0x28
    E52vec edges;                                   // 0x38
    void add_quad(const vector4* p);                // 0x4d2df0
    void build_adjacency();                         // 0x470920
};

// ---- ragdoll body record (0x2c bytes) ----------------------------------------------------------------------
struct Body {
    vector4 v;                                      // +0
    char pad10[0x1c];
    Body();                                         // 0x4bc170
    void load(TiXmlElement* e);                     // 0x4bc1f0
};
struct Bodyvec : vec<Body> {
    void push_back(const Body&);                    // 0x4cefe0
};

struct Bone {
    tstring name;                                   // +0
    float bone_mass;                                // +0x10
    float cone_min;                                 // +0x14
    float cone_max;                                 // +0x18
    float twist_limit;                              // +0x1c
    float rotate_side;                              // +0x20
    float rotate_forward;                           // +0x24
    float rotate_up;                                // +0x28
    char pad2c[4];
    vector4 socket_dir;                             // +0x30
    Bodyvec bodies;                                 // +0x40
    void load(TiXmlElement* e);                     // 0x4d3330
};

// @ 0x4d2df0
void QuadMesh::add_quad(const vector4* p)
{
    E24 a(p->x - 3.0f, p->y - 3.0f, p->z);
    E24 b(p->x + 3.0f, p->y - 3.0f, p->z);
    E24 c(p->x + 3.0f, p->y + 3.0f, p->z);
    E24 d(p->x - 3.0f, p->y + 3.0f, p->z);
    E52 e0, e1, e2, e3;
    E100 face;

    e0.nfaces = 0; e0.flag = 0; e0.crease0 = 1.0f; e0.crease1 = 1.0f;
    e1.nfaces = 0; e1.flag = 0; e1.crease0 = 1.0f; e1.crease1 = 1.0f;
    e2.nfaces = 0; e2.flag = 0; e2.crease0 = 1.0f; e2.crease1 = 1.0f;
    e3.nfaces = 0; e3.flag = 0; e3.crease0 = 1.0f; e3.crease1 = 1.0f;

    e0.v0 = verts.size(); e0.v1 = verts.size() + 1;
    e1.v0 = verts.size() + 1; e1.v1 = verts.size() + 2;
    e2.v0 = verts.size() + 2; e2.v1 = verts.size() + 3;
    e3.v0 = verts.size() + 3; e3.v1 = verts.size();

    face.vid[0] = verts.size();
    face.vid[1] = verts.size() + 1;
    face.vid[2] = verts.size() + 2;
    face.vid[3] = verts.size() + 3;
    face.eid[0] = edges.size();
    face.eid[1] = edges.size() + 1;
    face.eid[2] = edges.size() + 2;
    face.eid[3] = edges.size() + 3;
    face.n = 4;

    verts.push_back(a);
    verts.push_back(b);
    verts.push_back(c);
    verts.push_back(d);
    edges.push_back(e0);
    edges.push_back(e1);
    edges.push_back(e2);
    edges.push_back(e3);
    faces.push_back(face);
    build_adjacency();
}

// @ 0x4d3330
void Bone::load(TiXmlElement* e)
{
    const char* s = e->Attribute("name");
    if (s)
        name.assign(s);
    {
        double d;
        if (e->QueryDoubleAttribute("bone_mass", &d) == 0) bone_mass = (float)d;
        if (e->QueryDoubleAttribute("bone_cone_min", &d) == 0) cone_min = (float)d;
        if (e->QueryDoubleAttribute("bone_cone_max", &d) == 0) cone_max = (float)d;
        if (e->QueryDoubleAttribute("bone_twist_limit", &d) == 0) twist_limit = (float)d;
        if (e->QueryDoubleAttribute("socket_dir_x", &d) == 0) socket_dir.x = (float)d;
        if (e->QueryDoubleAttribute("socket_dir_y", &d) == 0) socket_dir.y = (float)d;
        if (e->QueryDoubleAttribute("socket_dir_z", &d) == 0) socket_dir.z = (float)d;
        if (e->QueryDoubleAttribute("rotate_side", &d) == 0) rotate_side = (float)d;
        if (e->QueryDoubleAttribute("rotate_forward", &d) == 0) rotate_forward = (float)d;
        if (e->QueryDoubleAttribute("rotate_up", &d) == 0) rotate_up = (float)d;
    }

    if (socket_dir.length() < 0.01f) {
        vector4 v(0.0f, 1.0f, 0.0f);
        socket_dir = v;
    }
    socket_dir.normalize();

    TiXmlHandle hh = TiXmlHandle(e).FirstChild("Bodies").FirstChild();
    TiXmlElement* elem = (hh.node && hh.node->ToElement()) ? hh.node->ToElement() : 0;
    while (elem) {
        bodies.push_back(Body());
        bodies[bodies.size() - 1].load(elem);
        elem = elem->NextSiblingElement();
    }
}

// ---- 0x2ebc-byte model record (provisional layout from its ctor) -------------------------------------------
struct model_a {
    char pad0[0x24];
    tstring name;                                   // 0x24
    char pad34[0x40 - 0x34];
    Bodyvec v40;                                    // 0x40 (begin +0x44)
    char pad50[0x2450 - 0x50];
    tstring names[128];                             // 0x2450
    char f2c50;                                     // 0x2c50
    char pad2c51[3];
    int m1[64];                                     // 0x2c54
    int m2[64];                                     // 0x2d54
    char pad2e54[0x2e74 - 0x2e54];
    int f2e74;                                      // 0x2e74
    char pad2e78[4];
    vec<int> v2e7c;                                 // 0x2e7c
    vec<int> v2e8c;                                 // 0x2e8c
    vec<int> v2e9c;                                 // 0x2e9c
    vec<int> v2eac;                                 // 0x2eac
    void init_base();                               // 0x4c80c0
    model_a();                                      // 0x4d3580
};

// @ 0x4d3580
model_a::model_a()
{
    f2c50 = 0;
    init_base();
    f2e74 = 0;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++) {
            m2[i * 8 + j] = 0;
            m1[i * 8 + j] = 0;
        }
}

// ---- 0x1f8+ record (provisional) --------------------------------------------------------------------------
struct model_b {
    tstring a;                                      // +0
    char pad10[4];
    int f14, f18, f1c, f20, f24;                    // +0x14
    tstring b;                                      // +0x28
    char pad38[4];
    int f3c, f40, f44;                              // +0x3c
    vec<int> v48;                                   // +0x48
    vec<int> v58;                                   // +0x58
    vec<int> v68;                                   // +0x68
    vec<int> v78;                                   // +0x78
    char pad88[0x19c - 0x88];
    vec<int> v19c;                                  // +0x19c
    vec<int> v1ac;                                  // +0x1ac
    vec<int> v1bc;                                  // +0x1bc
    int neg[7];                                     // +0x1cc
    int f1e8;                                       // +0x1e8
    float f1ec;                                     // +0x1ec
    int f1f0, f1f4;                                 // +0x1f0
    model_b();                                      // 0x4d3640
};

// @ 0x4d3640
model_b::model_b()
{
    f14 = 0; f18 = 0; f1c = 0; f20 = 0; f24 = 0;
    f3c = 0; f40 = 0; f44 = 0;
    f1e8 = 0; f1f0 = 0; f1f4 = 0;
    f1ec = 1.0f;
    for (int i = 0; i < 7; i++)
        neg[i] = -1;
}
