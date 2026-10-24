// prop_ent.h: the scene entity of a placed prop (a Rec1c8::ent; rgl::entity of src/rgl/scene/scene_entity.h seen through the parts
// the prop code uses): the vtable (slot 23 = set_local_scaled_dirty 0x41ff40 takes the new frame), and the two pointer vectors at
// +0x108 (lods) and +0x118 (meshes). ONE definition for src/game: terrain_a (Prop::update 0x467c50 drives the frame) and
// mission_ui2 (trigger_item 0x48cec0, trigger_mesh_obj 0x48cf40 read the first element of each vector) saw one half each.
// No includes (rgl::matrix is only forward-declared).
#ifndef GAME_PROP_ENT_H
#define GAME_PROP_ENT_H

namespace rgl { struct matrix; }
struct PropEnt {
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22();
    virtual void set_frame(const rgl::matrix* m, const void* extra);     // slot 23 (+0x5c)
    struct Vec {                                // a std::vector's four words (allocator slot, first, last, end)
        int pad0;                               // +0x00
        int* first;                             // +0x04
        int* last;                              // +0x08
        int* cap;                               // +0x0c
    };
    char pad4[0x108 - 4];
    Vec lods;                                   // +0x108 (0x48cec0)
    Vec meshes;                                 // +0x118 (0x48cf40)
};

#endif // GAME_PROP_ENT_H
