// party_model.h: what the map-party code (game/party/party_map.h) needs around the party's model. Party::model is the
// rgl::strategic_entity world::create_entity(9) builds (engine/scene/scene_entity.h); its fade 0x4b1800 is
// strategic_entity::set_fade (game/party/party_map.cpp).
#ifndef GAME_PARTY2_PARTY_MODEL_H
#define GAME_PARTY2_PARTY_MODEL_H
#include "engine/math/rgl_math.h"
#include "engine/render/mesh.h"
#include "engine/scene/scene_entity.h"   // rgl::strategic_entity: Party::model
namespace rgl { struct psys; }
using rgl::vector4;

struct Poser {
    void add_extra(rgl::mesh* val);   // 0x442110 (agent3/agent_fx_sound.cpp, ret 4)
};
#endif // GAME_PARTY2_PARTY_MODEL_H
