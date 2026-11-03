// entity_base.cpp (0x41fba0..0x41fee0): rgl::entity's constructor and the small virtual accessors and setters of the base
// class: the end of the object that holds the effect element, the scene's teardown and the pools (src/game/entity.cpp).
// The class is scene_entity.h's: the same rgl::entity the derived entity classes (mesh, collision, strategic, static, skeleton,
// dynamic) are built on, so one vtable (0x54b460) and one set of virtual names. (entity.cpp used to declare its own view of
// the class with other names for the same slots: commit / get_fbc / awake() void / load(), which left the derived classes'
// overrides unbound.)
// Boundary: start 0x41fba0 (entity.cpp's functions before it are the effect element, the pools and the vector instances);
// end 0x41fee0 (smat::scale_rows, entity_recalc.cpp, is called forward by set_local_scaled).
#include <vector>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "./scene_math.h"
#include "./frustum.h"
#include "./scene.h"
#include "./emitter.h"
#include "./scene_entity.h"
#include "./world.h"
#include "./fx.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

using namespace rgl;

namespace rgl {

// @ 0x41fba0
entity::entity()
{
    refs = 0;
    shape = 0;
    mode = 0;
    owner = 0;
    fbc = 0;
    vc4 = vector4(0.0f, 0.0f, 0.0f);
    f10 = 0;
    p138 = 0;
    p13c = 0;
    f141 = 0;
    f142 = 0;
    scale = vector4(1.0f, 1.0f, 1.0f);
    f140 = 1;
    fc0 = 0.01f;
    fc = -1;
    f144 = 1.0f;
}

// @ 0x41fcb0
string entity::type_name() { return string("entity"); }

// @ 0x41fcd0
void entity::sync()
{
    prev = local;
    const vector4* src;
    if (shape) {
        shape->set_frame(&local);
        v9c = shape->get_a();
        src = &shape->get_b();
    } else {
        v9c = local.o;
        src = &local.o;
    }
    vac = *src;
}

// @ 0x41fd80
float entity::get_radius() { return fbc; }
// @ 0x41fd90
void entity::set_fbc(const float* p) { fbc = *p; }
// @ 0x41fda0
void entity::update(float)
{
    prev = local;
    f142 = f141;
    f141 = 0;
}
// @ 0x41fdd0
void entity::set_f144(const float* p) { f144 = *p; }
// @ 0x41fde0
const vector4* entity::local_origin() { return &local.o; }
// @ 0x41fdf0
const vector4* entity::get_vac() { return &vac; }
// @ 0x41fe00
const vector4* entity::get_v9c() { return &v9c; }
// @ 0x41fe10
int entity::is_dirty()
{
    if (f141 || f142)
        return 1;
    return 0;
}
// @ 0x41fe30
void entity::set_mode(unsigned m) { mode = (mode & 0xfffffffc) | m; }
// @ 0x41fe50
void entity::set_local(const matrix* m) { local = *(const smat*)m; }
// @ 0x41fe70
void entity::set_local_dirty(const mat4* m)
{
    local = *(const smat*)m;
    f141 = 1;
}
// @ 0x41fe90
void entity::set_local_scaled(const matrix* m, const vector4* k)
{
    local = *(const smat*)m;
    scale = *k;
    local.scale_rows(k);
    recalc();
}

} // namespace rgl
