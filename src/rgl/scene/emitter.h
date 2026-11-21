// emitter.h: the particle emitter ("psys", 0x1bc bytes). rgl::psys in ../psys.h is the same class seen through the
// fields psys::emit / psys::update (0x413e50 / 0x414960) read; this header carries every field the scene modules
// touch. Layout evidence: psys::init 0x420b40 (defaults for most fields), world::alloc_emitter 0x427ea0 (ctor,
// noise_index), world::step 0x427f40, world::partition_items 0x41efd0, find_emitter 0x421a50.
//
// The particle vector's element is rgl::particle (0x64 bytes, ../particles/mesh_obj.h; the std::vector<particle> instances
// are mesh_obj.cpp's, 0x40f160..0x411a90; psys::init's resize is 0x41fb00).
#ifndef SCENE_EMITTER_H
#define SCENE_EMITTER_H
#include <vector>
#include "../rgl_math.h"
#include "./scene_math.h"
#include "../particles/mesh_obj.h"          // rgl::particle, the psf_* flags

#include "../../include/pod_elems.h"   // E8 / E12 / E100 / rec36 / rec136 (shared)
struct skin;                                                    // ../resource.h (global)

namespace rgl {

struct mesh;
struct scene;

// piecewise-linear key: value v0 until t0, v1 after t1, linear in between (psys::emit 0x413e50 reads the curves as these)
struct curve4 { float t0, v0, t1, v1; };

struct psys {
    mesh* cache;                   // 0x00 (zeroed by init 0x420b40)
    skin* res;                      // 0x04
    mesh* src;                      // 0x08 template mesh (null: emit camera-facing quads; psys::emit 0x413e50)
    std::vector<particle> particles;   // 0x0c (first at 0x10, last at 0x14)   0x41efd0, 0x420b40
    float noise_scale;              // 0x1c (EmitDef turbulence size)      0x420b40
    float noise_amount;             // 0x20 (EmitDef turbulence strength)
    float drag;                     // 0x24 (EmitDef damping)
    curve4 curve[5];                     // 0x28 the five (t0,v0,t1,v1) curves (0x420b40; psys::emit 0x413e50): alpha, r, g, b, size
    vec2 half;                           // 0x78 (x = half width, y = half height; 0x413e50)
    vec2 quad[4];                        // 0x80 corner uvs (0x413e50)
    float lifetime;                 // 0xa0 particle life in seconds (EmitDef::life; mesh_start.cpp stores a float)
    vector4 axis_x, axis_z;         // 0xa4 x axis, 0xb4 z axis (the particle axis of 0x414960)   0x420b40
    vector4 accel;                  // 0xc4 gravity (0, 0, -9.8)
    float accel_scale;              // 0xd4 (EmitDef gravity strength)
    vector4 tint;                   // 0xd8 colour tint: init sets x, y, z to 1 (x = brightness, 0x413e50); agent_logic stores the skybox colour; Mission::spawn_effect 0x442ca0 stores the camera-relative position of a follow-the-camera emitter here
    unsigned flags;                 // 0xe8 psf_* (1 from init; mask 0xf00 selects how the frame is built; 0x413e50)
    float height;                   // 0xec height offset of a placed emitter (Mission::post_spawn 0x47a0c0)
    int disabled;                   // 0xf0
    mat4 world, local;              // 0xf4, 0x134 (identity)
    vector4 box;                    // 0x174 emission box half-extents
    vector4 vel_base;               // 0x184 (0,0,1)
    float vel_scale;                // 0x194
    float spawn_rate;               // 0x198 (EmitDef::f24; Mission::start_setup 0x484590 / the impact particles of agent_logic scale it)
    int spawn_left;                 // 0x19c  0x41efd0 (<= 0 and no particles: the emitter is dead)
    int spawn_total;                // 0x1a0
    float spawn_accum;              // 0x1a4
    int tag;                        // 0x1a8 (id searched by find_emitter 0x421a50; init sets -1)
    float radius;                   // 0x1ac (render_scene 0x429890 and StrategicEntity::update 0x421b70 read it)
    float spin_scale;               // 0x1b0 (EmitDef rotation speed, radians)
    float spin_drag;                // 0x1b4 (EmitDef rotation damping)
    int noise_index;                // 0x1b8 (world::alloc_emitter 0x427ea0 stores the world's counter)
    psys(const int& idx) : particles() { noise_index = idx; init(); }   // inline in world::alloc_emitter
    void init();                                    // 0x420b40
    void update(const float* dt, const matrix& frame);   // 0x412150 (the simulation step, ../particles/mesh_obj.cpp)
    void spawn(particle* out);                      // 0x40ec90 (one new particle in the emission box, mesh_obj.cpp)
    void update(scene* sc);                         // 0x414960 (render-time update, ../psys.cpp)
    void emit(particle* p, matrix* m, mesh* out, const vector4* ref);   // 0x413e50 (../psys.cpp)
    void set_v184(const vector4* v);                // 0x433740 (xmlcfg/agent_pick.cpp: vel_base)
    void set_placement(const matrix* m, const vector4* a, const vector4* b, float c);   // 0x433770 (agent_pick.cpp; agent_logic.cpp has the body visible too)
    void reset_pending(int v);                      // 0x433810 (agent_pick.cpp: clears the pending flag, spawn_left = spawn_total = v)
};

} // namespace rgl
#endif
