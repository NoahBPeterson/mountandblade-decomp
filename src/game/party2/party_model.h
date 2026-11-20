// party_model.h: what the map-party code (party2/party_2.h) needs around the party's model. Party::model is the
// rgl::strategic_entity world::create_entity(9) builds (rgl/scene/scene_entity.h); its fade 0x4b1800 is
// strategic_entity::set_fade (party2/party_2.cpp).
#ifndef GAME_PARTY2_PARTY_MODEL_H
#define GAME_PARTY2_PARTY_MODEL_H
#include "../../rgl/rgl_math.h"
#include "../../include/ptr_vec.h"
#include "../../rgl/mesh.h"
#include "../../rgl/scene/scene_entity.h"   // rgl::strategic_entity: Party::model
namespace rgl { struct psys; }
using rgl::vector4;

struct Poser {
    void add_extra(rgl::mesh* val);   // 0x442110 (agent3/agent_fx_sound.cpp, ret 4)
};
struct Rot3 {                                           // the three rows of a frame (side, forward, up)
    vector4 s, f, u;
    void rotate_about_u(const float* angle) { ((rgl::matrix*)this)->rotate_about_u(angle); }   // 0x40e770 (ret 4: rows s and f about u)
};
inline Rot3* identity3(Rot3* out) { return (Rot3*)rgl::identity3((rgl::mat3*)out); }
#endif // GAME_PARTY2_PARTY_MODEL_H
