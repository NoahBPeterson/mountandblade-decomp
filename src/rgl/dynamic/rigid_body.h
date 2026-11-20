// rigid_body.h: the rigid body entity classes, rgl::rigid_base and rgl::dynamic_entity (vtable 0x54b668, 0x1d8 bytes;
// constructor 0x425370 and the members in dynamic_entity.cpp). ONE definition for src/rgl/dynamic (dynamic_entity.h, which
// adds the contact helpers of that object) and src/game (agent4/agent_logic.cpp: spawn_prop 0x44c210 builds a kind-7 prop's
// body). Nothing here changes how an includer's own std::vector instances compile (dynamic_entity.h's explicit
// specializations would).
#ifndef RGL_DYNAMIC_RIGID_BODY_H
#define RGL_DYNAMIC_RIGID_BODY_H
#include "../scene/scene_entity.h"

namespace rgl {

// ---- the rigid body ----------------------------------------------------------------------------------------------------
// Fields read or written by the function cited. Its constructor is inline (forced) in dynamic_entity::dynamic_entity
// (0x425370), which is why the vptr store of that function sits between the contact list and the mass.
struct rigid_base : collision_entity {
    float mass;                 // 0x170  ctor 0x425370 (5.0), apply_impulse 0x425550
    float inertia;              // 0x174  (2.0), resolve 0x4259a0
    float dt;                   // 0x178  apply_forces 0x4257e0
    vector4 force;              // 0x17c
    vector4 torque;             // 0x18c
    vector4 vel;                // 0x19c
    vector4 angvel;             // 0x1ac
    float lin_damp;             // 0x1bc  (0.1)
    float ang_damp;             // 0x1c0  (0.8)
    vector4 shift;              // 0x1c4  pending position delta (collide_shape 0x427900, integrate 0x4260a0)
    bool grounded;              // 0x1d4
    bool asleep;                // 0x1d5

    __forceinline rigid_base()
    {
        vel = vector4(0.0f, 0.0f, 0.0f);
        angvel = vector4(0.0f, 0.0f, 0.0f);
        force = vector4(0.0f, 0.0f, 0.0f);
        torque = vector4(0.0f, 0.0f, 0.0f);
        shift = vector4(0.0f, 0.0f, 0.0f);
        asleep = false;
        restitution = 0.9f;
    }
};

// vtable 0x54b668, 0x1d8 bytes
struct dynamic_entity : rigid_base {
    dynamic_entity();                                                      // 0x425370
    virtual ~dynamic_entity();
    virtual int type_id() { return 7; }                                    // 1: 0x49df30 (ICF-shared with Camera::type)
    virtual string type_name();                                           // 2: 0x425490
    virtual void update(float dt);                                         // 7: 0x425730
    virtual bool awake();                                                  // 10: 0x4254b0
    virtual vector4* point_velocity(vector4* out, const vector4* p);       // 25: 0x4254c0 v + w x (p - origin)
    virtual void apply_impulse(const vector4* p, const vector4* j);        // 26: 0x425550
    virtual float impulse_response(const vector4* p, const vector4* n);    // 27: 0x425670
    virtual void apply_forces(float dt);                                   // 28: 0x4257e0
    virtual void collide();                                                // 29: 0x4263d0
    virtual void integrate(float dt);                                      // 30: 0x4260a0
    virtual void resolve();                                                // 31: 0x4259a0
    virtual void add_torque(const vector4* t);                             // 32: 0x4258e0
    virtual void add_torque_at(const vector4* p, const vector4* f);        // 33: 0x425920
    virtual bool on_entity(entity* e);                                     // 34: 0x427000 (the other entity as a rigid body)
    virtual bool on_shape(shape_base* s);                                  // 35: 0x427900 (the shape as the entity's collision shape)
};

} // namespace rgl

#endif // RGL_DYNAMIC_RIGID_BODY_H
