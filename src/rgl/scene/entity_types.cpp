// entity_types.cpp (0x427d80..0x4285e0): the skeleton and static entity classes, the strategic entity's destructor,
// the world's emitter pool and per-frame step, the entity factory and entity::load. Migrated from slices
// s00427900 (0x427d80..0x428220), s00428240 (0x428240, 0x428440, 0x4284e0).
// Boundary: start 0x427d80 is the TU map's (class change collision_entity | skeleton_entity, low; the previous
// functions are the rigid-body base's and belong to the physics objects); end 0x4285e0 (entity::load calls
// shape_resource::clone_shape 0x4285e0 forward: high).
#include <vector>
#include <new>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../stream.h"
#include "../resource.h"
#include "./scene_math.h"
#include "./frustum.h"
#include "./scene.h"
#include "./emitter.h"
#include "./scene_entity.h"
#include "./world.h"
#include "./fx.h"
#include "./shape_clone.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)

using namespace rgl;

stream_base& r_matrix(stream_base& s, matrix* m);               // 0x415f30 (cdecl)

namespace rgl {

// @ 0x427d80
skeleton_entity::skeleton_entity() : fx(0) {}
// @ 0x427dc0
int skeleton_entity::type_id() { return 0xb; }
// @ 0x427dd0
string skeleton_entity::type_name() { return string("skeleton_entity"); }
// @ 0x427df0
void skeleton_entity::update(float dt)
{
    if (fx)
        fx->update(dt, &local);
}
// @ 0x427e10
skeleton_entity::~skeleton_entity()
{
    delete fx;
}

// the emitter pool: reuse the next idle emitter or allocate one
// @ 0x427ea0
psys* world::alloc_emitter()
{
    if ((unsigned)live < emitters.size()) {
        psys* e = emitters[live];
        live++;
        return e;
    }
    psys* e = new psys(noise_counter);
    emitters.push_back(e);
    ++live;
    return e;
}

// @ 0x427f40
void world::step(float dt)
{
    for (std::vector<entity*>::iterator it = ents2.begin(); it < ents2.end(); ++it) {
        (*it)->sync();
        ents.push_back(*it);
    }
    ents2.resize(0, 0);
    for (std::vector<entity*>::iterator it = ents.begin(); it < ents.end(); ++it)
        (*it)->update(dt);
    for (std::vector<entity*>::iterator it = ents.begin(); it < ents.end(); ++it) {
        if ((*it)->awake()) {
            release_links(*it);
            insert_obj(*it);
        }
    }
    for (psys** it = emitters._Myfirst; it < emitters._Mylast; ++it)
        (*it)->update(&dt, (*it)->mf4);
    partition_items();
    for (int* it = removed.first; it < removed.last; ++it)
        remove_entity(*it);
    removed.clear();
}

// @ 0x4280c0
string static_entity::type_name() { return string("static_entity"); }
// @ 0x4280e0
bool static_entity::awake() { return false; }
// 0x4280f0 is the compiler-generated scalar deleting destructor of static_entity (sets the vptr, calls ~entity
// 0x4213f0, deletes)

// @ 0x428120
strategic_entity::~strategic_entity()
{
    if (fx != 0) {
        delete fx;
        fx = 0;
    }
    if (f28c != 0) {
        delete f28c;
        f28c = 0;
    }
    if (f290 != 0) {
        delete f290;
        f290 = 0;
    }
    if (p288 != 0) {
        resmgr().release(p288);
        p288 = 0;
    }
    for (unsigned i = 0; i < emitters.size(); i++) {
        psys* e = *(emitters.begin() + i);
        delete e;
    }
}
// 0x428220 is the compiler-generated scalar deleting destructor of strategic_entity

// The factory: compares the requested id with the type id of a temporary of each entity class.
// @ 0x428240
entity* world::instantiate(int id)
{
    if (id == entity().type_id()) return new entity;
    if (id == mesh_entity().type_id()) return new mesh_entity;
    if (id == static_entity().type_id()) return new static_entity;
    if (id == dynamic_entity().type_id()) return new dynamic_entity;
    if (id == skeleton_entity().type_id()) return new skeleton_entity;
    if (id == strategic_entity().type_id()) return new strategic_entity;
    return 0;
}

// @ 0x428440
entity* world::create_entity(int id)
{
    entity* e = instantiate(id);
    if (!e) mb_error("Unable to create entity of type id %d.", id);
    e->id = next_id++;
    vpush(ents2, e);
    e->owner = this;
    return e;
}

// @ 0x4284e0
void entity::load(stream_base* s)
{
    string name;
    s->r_tstr(&name);
    if (!(name == "none"))
        shape = (coll_shape*)((shape_resource*)g_resmgr.get_object60(name))->clone_shape();
    r_matrix(*s, &local);
    int n;
    s->r_int(&n);
    meshes.resize(n, 0);
    for (int i = 0; i < n; i++) {
        string nm;
        s->r_tstr(&nm);
        if (!(nm == "none"))
        {
            mesh*& slot = *(meshes.begin() + i);
            slot = resmgr().get_object(&nm)->clone();
        }
    }
}

} // namespace rgl
