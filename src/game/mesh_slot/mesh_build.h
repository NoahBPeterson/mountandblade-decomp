// mesh_build.h: the two agent-body builders of mesh_slot, declared once for every caller (party2/camera.cpp spawn_party_scene,
// mesh_slot/mesh_game.cpp, agent5/agent_hud.cpp) and for their definitions (mesh_appear.cpp, mesh_scene.cpp).
#ifndef GAME_MESH_BUILD_H
#define GAME_MESH_BUILD_H
struct ItemRef;                          // game/item_ref.h
struct EquipSet;                         // mesh_appear.cpp: the agent's ten equipment slots as the builder reads them
struct SkelBodyV;                        // mesh_scene.cpp: the body whose item meshes are built
namespace rgl { struct strategic_entity; struct lod_mesh2; }
// 0x482240 (cdecl, 12 arguments): the appearance lods of an entity
void build_appearance(rgl::strategic_entity* a, int hand, int troop, int kind, ItemRef* hweapon,
                      rgl::lod_mesh2** ap, int* stats, rgl::lod_mesh2** extra, int* extra_count,
                      rgl::lod_mesh2** out_body, EquipSet* eq, int hit_troop);
// 0x47cfe0 (cdecl, 7 arguments): the carried-item display meshes of an agent body
void setup_item_meshes(SkelBodyV* body, int flag, int troop, char* outbase, int unused, ItemRef* items, int id);
#endif // GAME_MESH_BUILD_H
