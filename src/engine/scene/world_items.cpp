// world_items.cpp (0x41ef30..0x41f1d0): the view frustum constructor, the world's emitter partition and entity
// removal, and the two STL instantiations they use (std::vector<psys*>::_Ucopy, std::remove over the entity
// vector). Migrated from slice s0041ee20 (its functions from 0x41ef30; 0x41ee20 release_links and 0x41eeb0
// list::_Buynode belong to the previous object).
// Boundary: start 0x41ef30 is the TU map's; end 0x41f1d0 is where src/engine/scene/fx_element_pool.cpp begins (FxElem::~FxElem).
#include <list>
#include <vector>
#include <algorithm>
#include <string.h>
#include <new>
#include "engine/core/rgl_string.h"
#include "engine/math/rgl_math.h"
#include "engine/math/scene_math.h"
#include "engine/scene/frustum_rgl_scene.h"
#include "engine/scene/scene.h"
#include "engine/scene/emitter.h"
#include "engine/scene/scene_entity.h"
#include "engine/scene/world.h"
#include "engine/fx/fx_rgl_scene.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/scene/frustum.h"
#include "engine/fx/keyframes.h"
#include "engine/math/mat4_include.h"
#include "engine/render/mesh_prims.h"
#include "engine/math/vec.h"

using namespace rgl;

// std::vector<psys*>::_Ucopy (memmove) -- 0x41ef30. The element type of the original instance is not recoverable.
// @ 0x41ef30
template psys** std::vector<psys*>::_Ucopy<psys**>(psys**, psys**, psys**);

namespace rgl {

// @ 0x41ef60
frustum::frustum()
{
    matrix t;
    m = *identity_pos(&t);
    perspective = 1;
    left_handed = 0;
    right = 1.0f;
    left = -1.0f;
    bottom = -1.0f;
    top = 1.0f;
    nearz = 1.0f;
    farz = 1000.0f;
    build_planes();
}

// An emitter is dead when it has nothing left to spawn and no live particles; the dead ones are swapped to the end
// and `live` counts the others.
// @ 0x41efd0
void world::partition_items()
{
    live = emitters.size();
    int i = 0;
    while (i < live) {
        if (emitters[i]->spawn_left <= 0 && emitters[i]->particles.size() == 0) {
            psys* tmp = emitters[live - 1];
            emitters[live - 1] = emitters[i];
            emitters[i] = tmp;
            live--;
        } else {
            i++;
        }
    }
}

} // namespace rgl

// @ 0x41f080
template std::vector<entity*>::iterator std::remove(std::vector<entity*>::iterator, std::vector<entity*>::iterator, entity* const&);

namespace rgl {

static __forceinline bool remove_from(world* w, std::vector<entity*>& v, int id, bool rel)
{
    bool found = false;
    std::vector<entity*>::iterator it = v.begin();
    do {
        if (it >= v.end()) break;
        if ((*it)->id == id) {
            if (rel) w->release_links(*it);
            delete *it;
            v.erase(it);
            found = true;
        }
        ++it;
    } while (!found);
    return found;
}

// @ 0x41f0d0
void world::remove_entity(int id)
{
    if (!remove_from(this, ents, id, true)) {
        if (!remove_from(this, ents2, id, false))
            mb_error("unable to remove entity %d \n", id);
    }
}

} // namespace rgl
