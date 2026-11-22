// mesh_build.h: the two agent-body builders of mesh_slot, declared once for every caller (party2/camera.cpp spawn_party_scene,
// game/mission/agent_spawn_and_tick.cpp, game/ui/hud/agent_hud.cpp) and for their definitions (agent_appearance.cpp, scene_entities.cpp).
#ifndef GAME_MESH_BUILD_H
#define GAME_MESH_BUILD_H
struct ItemRef;                          // game/data/item_ref.h
struct EquipSet;                         // agent_appearance.cpp: the agent's ten equipment slots as the builder reads them
namespace rgl { struct strategic_entity; }   // the body whose item meshes are built
namespace rgl { struct strategic_entity; struct lod_mesh; struct mesh; }
// 0x482240 (cdecl, 12 arguments): the appearance lods of an entity
void build_appearance(rgl::strategic_entity* a, int hand, int troop, int kind, ItemRef* hweapon,
                      rgl::lod_mesh** ap, int* stats, rgl::mesh** extra, int* extra_count,
                      rgl::lod_mesh** out_body, EquipSet* eq, int hit_troop);
// 0x47cfe0 (cdecl, 7 arguments): the carried-item display meshes of an agent body
// meshes: the agent's 22 display slots (Agent::slot_mesh..): the four item meshes go to meshes[4..7], the carry meshes to meshes[12..15]
void setup_item_meshes(rgl::strategic_entity* body, int flag, int troop, rgl::lod_mesh** meshes, int* unused, ItemRef* items, int id);
#endif // GAME_MESH_BUILD_H
