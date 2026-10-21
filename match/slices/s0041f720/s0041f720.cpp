// Slice s0041f720: VC7.1 <vector>/<list> instantiations (resize and a 4-byte _Insert_n) and the
// scene "entity" object (vtable 0x54b460): constructor, frame accessors and setters.
#include <vector>
#include <list>
#include <memory>
#include "entity.h"

using namespace rgl;

struct E100 { int v[25]; };    // 0x64

// STL instantiations. 0x41f880 is the catch(...) funclet + cold branches of the _Insert_n at 0x41f720 (same COMDAT).
// @ 0x41f720
template void std::vector<Obj*>::_Insert_n(std::vector<Obj*>::iterator, size_t, Obj* const&);
// @ 0x41f970
template void std::vector<mesh*>::resize(size_t, mesh*);
// @ 0x41fa00
template void std::vector<Obj*>::resize(size_t, Obj*);
// @ 0x41fa90
template void std::list<int>::resize(size_t, int);
// @ 0x41fb00
template void std::vector<E100>::resize(size_t, E100);

namespace rgl {

// @ 0x41fba0
entity::entity()
{
    f14 = 0;
    parent = 0;
    mode = 0;
    f4 = 0;
    fbc = 0;
    vc4 = vector4(0.0f, 0.0f, 0.0f);
    f10 = 0;
    f138 = 0;
    f13c = 0;
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
    world = local;
    const vector4* src;
    if (parent) {
        parent->apply(&local);
        v9c = parent->get_a();
        src = &parent->get_b();
    } else {
        v9c = local.o;
        src = &local.o;
    }
    vac = *src;
}

// @ 0x41fd80
float entity::get_fbc() { return fbc; }
// @ 0x41fd90
void entity::set_fbc(const float* p) { fbc = *p; }
// @ 0x41fda0
void entity::commit(int)
{
    world = local;
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
void entity::set_local(const mat4* m) { local = *(const smat*)m; }
// @ 0x41fe70
void entity::set_local_dirty(const mat4* m)
{
    local = *(const smat*)m;
    f141 = 1;
}
// @ 0x41fe90
void entity::set_local_scaled(const mat4* m, const vector4* k)
{
    local = *(const smat*)m;
    scale = *k;
    local.scale_rows(k);
    recalc();
}
// @ 0x41ff40
void entity::set_local_scaled_dirty(const mat4* mm, const vector4* k)
{
    local = *(const smat*)mm;
    scale = *k;
    smat* m = &local;
    m->s.x = m->s.x * k->x;
    m->s.y = m->s.y * k->x;
    m->s.z = m->s.z * k->x;
    m->f.x = k->y * m->f.x;
    m->f.y = k->y * m->f.y;
    m->f.z = k->y * m->f.z;
    m->u.x = k->z * m->u.x;
    m->u.y = m->u.y * k->z;
    m->u.z = m->u.z * k->z;
    recalc();
    f141 = 1;
}

// cl assumes the register usage of a callee defined in the same TU; the original calls 0x41fee0 as an
// opaque function, so the caller (0x41fe90) is built with -DOPAQUE_SCALE_ROWS, which drops the body.
#ifndef OPAQUE_SCALE_ROWS
// @ 0x41fee0
void smat::scale_rows(const vector4* k)
{
    s.x = k->x * s.x;
    s.y = s.y * k->x;
    s.z = s.z * k->x;
    f.x = f.x * k->y;
    f.y = k->y * f.y;
    f.z = k->y * f.z;
    u.x = k->z * u.x;
    u.y = u.y * k->z;
    u.z = u.z * k->z;
}
#endif
} // namespace rgl
