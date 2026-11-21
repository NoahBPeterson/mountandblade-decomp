// ragdoll_models.cpp: the ragdoll bone loader (TinyXML) and the two model record constructors (0x4d3330..0x4d3720).
// The ragdoll bone is rgl/core/bone.h's RagdollBone (the element of RagModel::bones), not the skeleton's Bone.
#include <windows.h>
#include <d3d9.h>
#include "../../rgl/rgl_math.h"
#include "../../tinyxml/tinyxml.h"
#include "../tstring.h"
#include "../../rgl/core/bone.h"                   // RagdollBone, CapDef

using rgl::vector4;

// std::vector<CapDef>::push_back stays out of line (mission_items.cpp)
namespace std { template<> void vector<CapDef>::push_back(const CapDef& v); }   // 0x4cefe0

// @ 0x4d3330
void RagdollBone::load(TiXmlElement* e)
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
        if (e->QueryDoubleAttribute("rotate_side", &d) == 0) rotate.x = (float)d;
        if (e->QueryDoubleAttribute("rotate_forward", &d) == 0) rotate.y = (float)d;
        if (e->QueryDoubleAttribute("rotate_up", &d) == 0) rotate.z = (float)d;
    }

    if (socket_dir.length() < 0.01f) {
        vector4 v(0.0f, 1.0f, 0.0f);
        socket_dir = v;
    }
    socket_dir.normalize();

    TiXmlHandle hh = TiXmlHandle(e).FirstChild("Bodies").FirstChild();
    TiXmlElement* elem = hh.Element();
    while (elem) {
        bodies.push_back(CapDef());                   // ctor 0x4bc170 / load 0x4bc1f0 (config.cpp)
        bodies[bodies.size() - 1u].load(elem);
        elem = elem->NextSiblingElement();
    }
}

// ---- the script interpreter's state object (g_script 0xa70440, include/script_state.h): constructor and reset ----------------------
#include "../../include/script_state.h"

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
    vars.clear();
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
