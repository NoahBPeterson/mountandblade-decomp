// fxdef_init.cpp (0x428820..0x428b10): the effect definition's constructor and FxDef::init (element array from the
// loaded skeleton resource: parents, frames, children, envelope arrays). Migrated from slice s00428240 (0x428820,
// 0x4288c0).
// Boundary: start 0x428820 (skeleton_entity::load 0x4286e0 calls the constructor and init forward: high); end
// 0x428b10 (init calls quat_from_matrix 0x428b10, EnvRec::alloc 0x428c60 and the FxElem constructor 0x428d10
// forward: high; init is the last function before them).
#include <vector>
#include <new>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "./scene_math.h"
#include "./frustum.h"
#include "./scene.h"
#include "./emitter.h"
#include "./scene_entity.h"
#include "./world.h"
#include "./fx.h"
#include "../resource.h"                 // frame_set: the skeleton FxDef::init reads
#include "../core/resource_io.h"         // Bone
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"

// @ 0x428820
FxDef::FxDef(int a) : psys(), groups(), clock(a)
{
    root_pos = vector4(0.0f, 0.0f, 0.0f);
    nenv = 0;
    env = 0;
    count = 0;
    f4c = 0;
    elems = 0;
    key_idx = 0;
    pose_valid = 0;
    frozen = 0;
    pose_src = 0;
    track_root_vel = 0;
    keep_weight = 1;
    pose_dt = 100.0f;
    root_vel = vector4(0.0f, 0.0f, 0.0f);
}

// @ 0x4288c0
void FxDef::init(frame_set* r)
{
    res = r;
    int n = r->nbones;
    count = n;
    elems = new FxElem[n];
    for (int i = 0; i < count; i++) {
        elems[i].parent = r->bones[i].parent;
        if (elems[i].parent >= 0)
            elems[elems[i].parent].nchild++;
        elems[i].m44 = r->bones[i].local;
        elems[i].src = &r->bones[i];
        if (elems[i].parent < 0)
            elems[i].m4 = elems[i].m44;
        else
            elems[i].m4 = elems[elems[i].parent].m4 * elems[i].m44;
        elems[i].m84 = elems[i].m4;
    }
    for (int i = 0; i < count; i++) {
        if (elems[i].nchild != 0)
            elems[i].children = new int[elems[i].nchild];
        else
            elems[i].children = 0;
        elems[i].nchild = 0;
        quat_from_matrix(&elems[i].q, elems[i].m44);
    }
    for (int i = 0; i < count; i++) {
        int p = elems[i].parent;
        if (p >= 0) {
            elems[p].children[elems[p].nchild] = i;
            elems[p].nchild++;
        }
    }
    for (int i = 0; i < nenv; i++)
        env[i].alloc(count);
}
