// Slice s00411150: particle-system support code (provisional names).  0x411150-0x411a20 are VC7.1 <vector>
// instantiations (explicit instantiations reproduce them byte for byte); 0x411920 is Psys::add_rec2.
#include "s00411150.h"

// ---- std::vector<IdxWeight> (8-byte elements) ----
// @ 0x411150
template std::vector<IdxWeight>& std::vector<IdxWeight>::operator=(const std::vector<IdxWeight>&);
// ---- std::vector<Vert96> ----
// @ 0x4112a0
template std::vector<Vert96>& std::vector<Vert96>::operator=(const std::vector<Vert96>&);
// @ 0x411420
template void std::vector<Vert96>::push_back(const Vert96&);
// ---- std::vector<SkinVert> (0x30) ----
// @ 0x4114b0
template std::vector<SkinVert>& std::vector<SkinVert>::operator=(const std::vector<SkinVert>&);
// ---- std::vector<Rec32> ----
// @ 0x411630
template std::vector<Rec32>& std::vector<Rec32>::operator=(const std::vector<Rec32>&);

// @ 0x411780
VecPair::VecPair() {}

// @ 0x4117a0
template void std::vector<SkinVert>::resize(size_t, SkinVert);
// @ 0x411840
template std::vector<Particle>::iterator std::vector<Particle>::insert(std::vector<Particle>::iterator, const Particle&);
// @ 0x4118b0
template BoneInfluence* std::copy(BoneInfluence*, BoneInfluence*, BoneInfluence*);
// @ 0x4118f0
template void std::allocator<BoneInfluence>::destroy(BoneInfluence*);

// @ 0x411920
int Psys::add_rec2(int x, int y, int z)
{
    if (!own_recs) {
        recs2 = parent->recs2;
        own_recs = true;
    }
    int n = (int)recs2.size();
    recs2.resize(n + 1);
    recs2[n].x = x;
    recs2[n].y = y;
    recs2[n].z = z;
    return n;
}

// @ 0x4119d0
template void std::vector<Obj*>::push_back(Obj* const&);
// @ 0x411a20
template void std::vector<IdxWeight>::push_back(const IdxWeight&);
