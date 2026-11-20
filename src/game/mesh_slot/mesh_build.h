// mesh_build.h: the two agent-body builders of mesh_slot, declared once for every caller (party2/camera.cpp spawn_party_scene,
// mesh_slot/mesh_game.cpp, agent5/agent_hud.cpp) and for their definitions (mesh_appear.cpp, mesh_scene.cpp).
#ifndef GAME_MESH_BUILD_H
#define GAME_MESH_BUILD_H
struct ItemRef;                          // game/item_ref.h
struct EquipSet;                         // mesh_appear.cpp: the agent's ten equipment slots as the builder reads them
namespace rgl { struct strategic_entity; }   // the body whose item meshes are built
namespace rgl { struct strategic_entity; struct lod_mesh2; struct mesh; }
// 0x482240 (cdecl, 12 arguments): the appearance lods of an entity
void build_appearance(rgl::strategic_entity* a, int hand, int troop, int kind, ItemRef* hweapon,
                      rgl::lod_mesh2** ap, int* stats, rgl::mesh** extra, int* extra_count,
                      rgl::lod_mesh2** out_body, EquipSet* eq, int hit_troop);
// 0x47cfe0 (cdecl, 7 arguments): the carried-item display meshes of an agent body
// meshes: the agent's 22 display slots (Agent::slot_mesh..): the four item meshes go to meshes[4..7], the carry meshes to meshes[12..15]
void setup_item_meshes(rgl::strategic_entity* body, int flag, int troop, rgl::lod_mesh2** meshes, int* unused, ItemRef* items, int id);
#endif // GAME_MESH_BUILD_H
