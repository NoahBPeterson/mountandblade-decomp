// ragdoll_models.cpp: the ragdoll bone loader (TinyXML) and the two model record constructors (0x4d3330..0x4d3720).
// Names provisional. The ragdoll bone is not rgl's skeleton Bone (src/rgl/core/resource_io.h): it is called
// RagBone here.
#include <windows.h>
#include <d3d9.h>
#include "../../rgl/rgl_math.h"
#include "../../tinyxml/tinyxml.h"
#include "../tstring.h"

using rgl::vector4;

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

// ---- ragdoll body record (0x2c bytes) ----------------------------------------------------------------------
#include "../rag_body.h"                           // RagBody: ctor 0x4bc170, load 0x4bc1f0 (config.cpp)
struct Bodyvec : vec<RagBody> {
    void push_back(const RagBody&);                 // 0x4cefe0
};

struct RagBone {
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

// @ 0x4d3330
void RagBone::load(TiXmlElement* e)
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
    TiXmlElement* elem = hh.Element();
    while (elem) {
        bodies.push_back(RagBody());
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
