// ragdoll_models.cpp: the ragdoll bone loader (TinyXML) and the two model record constructors (0x4d3330..0x4d3720).
// Names provisional. The ragdoll bone is not rgl's skeleton Bone (src/rgl/core/resource_io.h): it is called
// RagBone here.
#include <windows.h>
#include <d3d9.h>
#include "../../rgl/rgl_math.h"
#include "../../tinyxml/tinyxml.h"
#include "../tstring.h"

using rgl::vector4;

// ---- the engine's growable array: allocator slot, then begin / last / end (begin at +4); the ctor clears the pointers ---------
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
#include "./vec44.h"                               // vec44: push_back 0x4cefe0 (mission_items.cpp)
struct Bodyvec : vec<RagBody> {
    // the element-appending helper is vec44::push_back (a RagBody is 0x2c bytes); the vec<RagBody> layout is vec44's
    __forceinline void push_back(const RagBody& b) { ((vec44*)(void*)this)->push_back((char*)&b); }
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

// ---- the script interpreter's state object (g_script 0xa70440, include/script_state.h): constructor and reset ----------------------
#include "../../include/script_state.h"

// the library's vector::_Tidy of a pointer triple: free the buffer if any, then zero first / last / end
static __forceinline void tidy3(void** q)
{
    if (q[0])
        operator delete(q[0]);
    q[0] = 0;
    q[1] = 0;
    q[2] = 0;
}

// @ 0x4c80c0
__declspec(noinline) void ScriptState::reset()
{
    screenshot_count = 0;
    next_track = -1;
    next_track_options = 0;
    cue_track = -1;
    param1_lo = -100000;
    param1_hi = -1;
    param2_lo = -100000;
    param2_hi = -1;
    trigger_result.store0 = 0;
    trigger_result.store1 = 0;
    fixed_point_multiplier = 100;
    tidy3((void**)&vars._Myfirst);
}

// @ 0x4d3580
ScriptState::ScriptState()
{
    new_game_done = 0;
    reset();
    level = 0;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++) {
            map_saved[i * 8 + j] = 0;
            map_cell[i * 8 + j] = 0;
        }
}

// ---- the skin record (src/include/skin.h) -----------------------------------------------------------------------
#include "../arr_set.h"
#include "../../include/vec_elems.h"
#include "../../include/skin.h"

// @ 0x4d3640
Skin::Skin()
{
    body = 0; hand_l = 0; hand_r = 0; calf_l = 0; calf_r = 0;
    n_face_keys = 0; face_keys = 0; beard_meshes = 0;
    skeleton = 0; blood1 = 0; blood2 = 0;
    scale = 1.0f;
    for (int i = 0; i < 7; i++)
        voices.v[i] = -1;
}
