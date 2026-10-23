//-----------------------------------------------------------------------------
// File: entity.cpp  (0x41f1d0..0x41fee0)
//
// Desc: the scene entity (rgl::entity: constructor and the small virtual accessors / setters), the effect
//       element's (FxElem) destructor and psys-list helpers, the scene teardown, the chunked free-list pool, and
//       the VC7.1 <list>/<vector> instantiations the linker kept from this object. Engine (rgl) code, placed under
//       src/game for now. Names provisional. Functions in ascending address order (cl's bottom-up emission).
//       Flags: /O2 /MT /Gy /TP (no /EH: the STL try/catch blocks compile as in the original without it, and with
//       /EHa or /EHsc the entity constructor gets an EH frame the original lacks).
//-----------------------------------------------------------------------------
#include <vector>
#include <list>
#include <memory>
#include <string.h>
#include <new>
#include "rgl.h"
#include "./entity.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../include/mat4.h"
#include "../include/mesh_prims.h"
#include "../include/poly.h"
#include "../include/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "./world.h"
#include "../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)

using rgl::vector4;

// std::remove is called, not compiled here: in the original, erase_entity (0x41f450) reaches the instance in the
// previous object (0x41f080) as an opaque function. With the <algorithm> body visible cl keeps end() in a register
// across the call, which the original does not.
namespace std {
template<class _FI, class _Ty> _FI remove(_FI _First, _FI _Last, const _Ty& _Val);     // 0x41f080
}

// @ 0x41f1d0
FxElem::~FxElem()
{
    if (has_arr)
        operator delete(arr);
}

// @ 0x41f250
void FxElem::set_data(void* p)
{
    if (psys.size() != 0) {
        rgl::mesh* s = psys[0];
        if (s->data != 0)
            operator delete(s->data);
        s->data = p;
    } else {
        operator delete(p);
    }
}

// @ 0x41f2a0
int FxElem::find_kid(int id)
{
    int r = -1;
    if (psys.size() != 0) {
        for (unsigned i = 0; i < psys[0]->kids.size(); i++) {
            if (psys[0]->kids[i]->id == id)
                r = i;
        }
    }
    return r;
}

// ---- std::list<int> (VC7.1 <list>), instantiated by list<int>::resize (0x41fa90): erase (0x41f310), _Insert
// (0x41f640), _Insert_n (0x41f680) and its catch(...) funclet (0x41f6e6). The linker folded them (ICF) with every list of
// 4-byte elements. Template rows are labelled `// 0x<va> <name>` (no `@`): a marker binds the next definition.

// 0x41f310 list::erase
template std::list<int>::iterator std::list<int>::erase(std::list<int>::iterator);

// ---- chunked free-list pool ----
template<class T> void pool<T>::link_chunks()
{
    typedef typename std::list<T*>::iterator It;
    for (It it = chunks.begin(); it != chunks.end(); ++it) {
        for (int i = 0; i < per_chunk - 1; i++) {
            (*it)[i].next = &(*it)[i + 1];
        }
        It nx = it;
        ++nx;
        if (nx != chunks.end())
            (*it)[per_chunk - 1].next = *nx;
        else
            (*it)[per_chunk - 1].next = 0;
    }
    if (chunks.begin() != chunks.end())
        free_head = *chunks.begin();
}
// 0x41f350 pool::link_chunks (cell12)
template void pool<cell12>::link_chunks();
// 0x41f3d0 pool::link_chunks (cell16)
template void pool<cell16>::link_chunks();

// @ 0x41f450
void world::erase_entity(rgl::entity* e)
{
    std::vector<rgl::entity*>::iterator it = std::remove(ents.begin(), ents.end(), e);
    ents.erase(it, ents.end());
}

// 0x41f4b0 vec::clear (vec<mface>)
template void vec<mface>::clear();

// @ 0x41f4e0
rgl::scene::~scene()
{
    if (handle != 0) {
        rgl::g_resmgr.release(handle);
        handle = 0;
    }
    scene* c = child;
    if (c != 0)
        delete c;
    for (unsigned i = 0; i < cache.size(); i++) {
        Poly* o = cache[i];
        if (o != 0)
            delete o;
    }
}

// @ 0x41f5d0
four_vecs::~four_vecs() {}

// 0x41f640 list::_Insert
template void std::list<int>::_Insert(std::list<int>::iterator, const int&);
// 0x41f680 list::_Insert_n
template void std::list<int>::_Insert_n(std::list<int>::iterator, std::list<int>::size_type, const int&);

// ---- std::vector instantiations. 0x41f880 is the catch(...) funclet + cold branches of the _Insert_n at
// 0x41f720 (same COMDAT).
// 0x41f720 vector::_Insert_n (vector<Obj*>)
template void std::vector<rgl::Obj*>::_Insert_n(std::vector<rgl::Obj*>::iterator, size_t, rgl::Obj* const&);
// 0x41f970 vector::resize (vector<mesh*>)
template void std::vector<rgl::mesh*>::resize(size_t, rgl::mesh*);
// 0x41fa00 vector::resize (vector<Obj*>)
template void std::vector<rgl::Obj*>::resize(size_t, rgl::Obj*);
// 0x41fa90 list::resize
template void std::list<int>::resize(size_t, int);
// 0x41fb00 vector::resize (vector<E100>)
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
tstring entity::type_name() { return tstring("entity"); }

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

} // namespace rgl
