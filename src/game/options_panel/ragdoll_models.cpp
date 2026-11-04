// ragdoll_models.cpp: the ragdoll bone loader (TinyXML) and the two model record constructors (0x4d3330..0x4d3720).
// Names provisional. The ragdoll bone is not rgl's skeleton Bone (src/rgl/core/resource_io.h): it is called
// RagBone here.
#include <windows.h>
#include <d3d9.h>
#include "../../rgl/rgl_math.h"
#include "../../tinyxml/tinyxml.h"
#include "../tstring.h"

using rgl::vector4;

// ---- the engine's growable arrays: vec<T> (../model_b.h) -------------------------------------------------
#include "../model_b.h"

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

extern "C" void _ReadWriteBarrier(void);   // compiler barrier (emits no code): pins the original's store order (docs/codegen.md 10.9)

// @ 0x4c80c0
__declspec(noinline) void ScriptState::reset()
{
    screenshot_count = 0;
    _ReadWriteBarrier();
    next_track = -1;
    _ReadWriteBarrier();
    next_track_options = 0;
    _ReadWriteBarrier();
    cue_track = -1;
    _ReadWriteBarrier();
    param1_lo = -100000;
    _ReadWriteBarrier();
    param1_hi = -1;
    _ReadWriteBarrier();
    param2_lo = -100000;
    _ReadWriteBarrier();
    param2_hi = -1;
    _ReadWriteBarrier();
    trigger_result.store0 = 0;
    _ReadWriteBarrier();
    trigger_result.store1 = 0;
    _ReadWriteBarrier();
    fixed_point_multiplier = 100;
    _ReadWriteBarrier();
    if (vars._Myfirst)
        operator delete(vars._Myfirst);
    vars._Myfirst = 0;
    vars._Mylast = 0;
    vars._Myend = 0;
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

// ---- 0x1f8+ record (provisional) --------------------------------------------------------------------------
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
