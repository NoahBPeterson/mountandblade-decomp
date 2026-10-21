// Slice s0041ac10: std::vector / std::list instantiations over the resource manager's types, the skeleton
// model/animation records, and the resource manager's constructor (provisional names).
#include <algorithm>
#include <new>
#include "s0041ac10.h"

// ---- VC7.1 <vector>/<list> instantiations (the originals are linked out of line) ---------------
// @ 0x41ac10
// std::vector<res_obj*> copy constructor
template std::vector<res_obj*>::vector(const std::vector<res_obj*>&);
// @ 0x41acc9
// (not a function of its own: the catch(...) { clear(); throw; } funclet of the copy constructor above, emitted by
// cl right after it; the 203-byte COMDAT section is byte-identical to 0x41ac10..0x41acda)
// @ 0x41ace0
template void std::list<res_obj*>::push_back(res_obj* const&);
// @ 0x41ad20
template void std::vector<res_obj*>::resize(size_t, res_obj*);
// @ 0x41adb0
template void std::vector<vec_array>::resize(size_t, vec_array);
// @ 0x41ae70
template void std::vector<IdxWeight>::resize(size_t, IdxWeight);
// @ 0x41afc0
template BoneInfluence* std::copy_backward(BoneInfluence*, BoneInfluence*, BoneInfluence*);

// ---- skeleton model -------------------------------------------------------------------------
frame_set::frame_set() {}
int frame_set::type() { return 7; }
// @ 0x41aee0
// vector deleting destructor ??_Eframe_set (the dtor body is inlined into both paths). NOT byte-exact: the original
// expands `delete[] bones` in the array path with __vec_dtor(bones, 0xdc, n, &Bone::~Bone) and keeps the call to
// Bone's vector deleting destructor in the scalar path; cl 13.10 here calls it in both (see nonmatching.txt).
frame_set::~frame_set()
{
    if (nbones != 0 && bones != 0)
        delete[] bones;
}

// ---- resource manager (global object at 0x5d6fa0) ----------------------------------------------
// @ 0x41b000
resmgr::resmgr()
{
    c4 = 1;
    c8 = 1;
    cc = 1;
    f0 = 0;
    fce = 0;
}

// @ 0x41b0c0
Bone::Bone()
{
    f0 = 0;
    fa0 = 0; fa4 = 0; fa8 = 0;
    vb_c = rgl::vector4(0.0f, 1.0f, 0.0f);
    va_c = rgl::vector4(0.0f, 0.0f, 0.0f);
    f9c = 2.0f;
}

#define BIT(i) ((unsigned)g_bit[i])
// @ 0x41b180
stream_base* read_frame_set(stream_base* s, frame_set* m)
{
    s->r_tstr(&m->name);
    s->r_int(&m->nbones);
    m->bones = new Bone[m->nbones];
    int i;
    for (i = 0; i < m->nbones; i++) {
        Bone* b = &m->bones[i];
        s->r_int(&b->parent);
        s->r_tstr(&b->name);
        s->r_uint(&b->flags);
        mat4* lm = &b->local;
        rgl::vector4* o = &lm->o;
        read_matrix(s, lm)->r_float(&o->x).r_float(&o->y).r_float(&o->z);
    }
    for (i = 0; i < m->nbones; i++) {
        if (m->bones[i].parent < 0)
            m->bones[i].world = m->bones[i].local;
        else
            m->bones[i].world = m->bones[m->bones[i].parent].world * m->bones[i].local;
        m->table.data[i] = m->bones[i].world.o;
        m->table.data[i].w = 0.0f;
    }
    m->table.count = m->nbones;
    if (m->name.equals("skel_horse")) {
        m->f28 = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(8) | BIT(9) | BIT(10) | BIT(11) | BIT(12) | BIT(13) | BIT(14) | BIT(15) | BIT(16) | BIT(17) | BIT(18) | BIT(19) | BIT(20) | BIT(21) | BIT(22) | BIT(23) | BIT(24) | BIT(25) | BIT(26) | BIT(27);
        m->f2c = m->f28 | BIT(4) | BIT(5) | BIT(6) | BIT(7);
        m->f30 = m->f2c;
    } else {
        m->f28 = BIT(4) | BIT(5) | BIT(6) | BIT(0) | BIT(1) | BIT(2) | BIT(3);
        m->f2c = m->f28 | BIT(7) | BIT(8) | BIT(9) | BIT(15) | BIT(16) | BIT(17) | BIT(18) | BIT(19);
        m->f30 = m->f2c | BIT(10) | BIT(11) | BIT(12) | BIT(13);
    }
    return s;
}

// @ 0x41b4e0
// (vec_array::vec_array() zeroes the tail record, the key vector zeroes itself)
skel_anim::skel_anim()
{
}
int skel_anim::type() { return 8; }
// @ 0x41b520
// skel_anim::~skel_anim() is the implicit destructor: members (tail record arrays, key vector) then the root
// resource destructor. Its vector deleting destructor is 0x41bed0 (slice s0041b5a0).
// vtable anchors: an array new makes cl emit the vector deleting destructors (??_E) as slot 0
skel_anim* make_skel_anim_array(int n) { return new skel_anim[n]; }
frame_set* make_frame_set_array(int n) { return new frame_set[n]; }
