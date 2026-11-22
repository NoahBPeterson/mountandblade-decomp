// appearance_lods.h: AppearanceLods, the agent's extra lods and body lod (+0x194 of the agent record), what build_appearance
// (game/mission/agent_appearance.cpp) fills besides the item meshes. ONE definition for agent_obj.h (Agent) and game/terrain/agent_rec.h
// (MAgent, whose reset 0x472f90 clears extra_count and body).
#ifndef GAME_APPEARANCE_LODS_H
#define GAME_APPEARANCE_LODS_H
namespace rgl { struct lod_mesh; struct mesh; }
struct AppearanceLods {                                 // +0x194 of the agent
    rgl::mesh* extra[4];                                // +0 the extra particle meshes (build_appearance adds them to the skeleton's psys list)
    int extra_count;                                    // +0x10
    rgl::lod_mesh* body;                               // +0x14 the body lod (out_body)
};
#endif // GAME_APPEARANCE_LODS_H
