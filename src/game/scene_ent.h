// scene_ent.h: SceneEnt, the scene entity as the agent and missile code call it. ONE definition for src/game/agent4 (agent_logic.cpp)
// and src/game/terrain_b (poly_mesh.cpp: Missile::settle moves the missile's entity, 0x472650).
#ifndef GAME_SCENE_ENT_H
#define GAME_SCENE_ENT_H
#include <vector>
#include "../rgl/rgl_math.h"

// ---- the scene entity as the agent code calls it (vtable 0x54b460 family: src/game/entity.h has the base class rgl::entity,
// which cannot be extended here: the physics entities add virtual slots 24..28 and the code reads fields of the derived classes).
// Slot / field names are the ones of the calls in this module; unused slots are placeholders.
struct shape_base;
namespace rgl { struct mesh; }
struct capsule;
struct SceneEnt {
    virtual ~SceneEnt();                                // 0
    virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual float get_fbc();                            // 5 (+0x14) 0x41fd80
    virtual void set_fbc(const float* p);               // 6 (+0x18) 0x41fd90
    virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14();
    virtual bool is_movable();                          // 15 (+0x3c) 0x45d820
    virtual void s16();
    virtual void s17(); virtual void s18(); virtual void s19(); virtual void s20(); virtual void s21();
    virtual void set_frame(const rgl::matrix* m, const rgl::vector4* scale);   // 22 (+0x58) (0x44c210)
    virtual void s23(); virtual void s24(); virtual void s25();
    virtual void collide(const rgl::vector4* point, const rgl::vector4* impulse);   // 26 (+0x68) (0x45d820)
    virtual void s27();
    virtual void set_shape(shape_base* s);              // 28 (+0x70) (0x44c210)
    char pad4[0xc - 4];
    unsigned id;                                        // +0xc (0x448eb0, 0x44c210, 0x45d820)
    int type;                                           // +0x10 (0xb = agent, 0 = static; the prop code calls it kind)
    char pad14[0x18 - 0x14];
    rgl::matrix p18;                                    // +0x18 the frame the entity moves in (0x45d820: transform_point_to_local)
    rgl::matrix local;                                  // +0x58 local frame (0x44be50)
    shape_base* shape;                                  // +0x98 collision shape
    char pad9c[0xc4 - 0x9c];
    rgl::vector4 vc4;                                   // +0xc4 (0x44be50)
    char padd4[0x118 - 0xd4];
    std::vector<rgl::mesh*> meshes;                       // +0x118 (first at +0x11c) (0x44c210)
    char pad128[0x140 - 0x128];
    char f140;                                          // +0x140 (visible) (0x44be50)
    char pad141[0x15c - 0x141];
    float f15c;                                         // +0x15c (0x44c210)
    char pad160[0x170 - 0x160];
    float f170;                                         // +0x170 (0x44c210)
    char pad174[0x19c - 0x174];
    rgl::vector4 v19c;                                  // +0x19c (0x44c210)
    rgl::vector4 v1ac;                                  // +0x1ac
    char pad1bc[0x1d4 - 0x1bc];
    char f1d4;                                          // +0x1d4 (0x44c210)
    __forceinline void add_extra(rgl::mesh* m);            // 0x442110 (Poser::add_extra; defined in the .cpp)
};

#endif // GAME_SCENE_ENT_H
