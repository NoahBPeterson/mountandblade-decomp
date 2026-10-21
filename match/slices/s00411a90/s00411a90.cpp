// Slice s00411a90: particle-system skin/bone code (provisional names).
// 0x411a90-0x411b80 are VC7.1 <vector>/<memory> instantiations; 0x411c10/0x411d20/0x412040/0x412100 are real code.
#include "s00411a90.h"
#include <math.h>

// @ 0x411a90
template void std::vector<Particle>::push_back(const Particle&);
// @ 0x411b20
template void std::_Destroy_range(BoneInfluence*, BoneInfluence*, std::allocator<BoneInfluence>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x411b60
template void std::allocator<BoneInfluence>::construct(BoneInfluence*, const BoneInfluence&);
// @ 0x411b80
// (needs /EHsc: try/catch destroys the already-constructed prefix; the catch funclet is 0x411bea)
template BoneInfluence* std::_Uninit_copy(BoneInfluence*, BoneInfluence*, BoneInfluence*, std::allocator<BoneInfluence>&, std::_Nonscalar_ptr_iterator_tag);

// @ 0x411c10
void VecPair::copy_from(const VecPair& o)
{
    a.resize(o.a.size());
    for (unsigned i = 0; i < o.a.size(); ++i)
        a[i] = o.a[i];
    b.resize(o.b.size());
    for (unsigned j = 0; j < o.b.size(); ++j)
        b[j] = o.b[j];
}

// Builds the per-vertex skinning table: up to 4 (bone, weight) influences for each vertex slot, then the
// 0x30-byte SkinVert records.
// @ 0x411d20
void Psys::build_skin()
{
    float tw[0x10000][4];
    int tb[0x10000][4];
    SkinVert tmp;
    unsigned i;

    for (i = 0; i < recs.size(); ++i) {
        tw[i][0] = 0; tw[i][3] = 0; tw[i][2] = 0; tw[i][1] = 0;
        tb[i][3] = 0; tb[i][2] = 0; tb[i][1] = 0; tb[i][0] = 0;
    }
    for (unsigned bi = 0; bi < bones.size(); ++bi) {
        BoneInfluence* b = &bones[bi];
        for (unsigned k = 0; k < b->verts.size(); ++k) {
            int vi = b->verts[k].index;
            unsigned s = 0;
            do {
                if (!(tw[vi][s] > 1e-5f)) {
                    if (s < 4) {
                        tb[vi][s] = b->bone;
                        tw[vi][s] = b->verts[k].weight;
                    }
                    break;
                }
                ++s;
            } while (s < 4);
        }
    }
    skin.resize(verts.size(), tmp);
    for (i = 0; i < verts.size(); ++i) {
        float sum = 0;
        for (int k = 0; k < 4; ++k) {
            skin[i].w[k] = tw[verts[i].slot][k];
            skin[i].bone[k] = tb[verts[i].slot][k];
            sum += skin[i].w[k];
        }
        if (sum < 1.0f)
            skin[i].w[0] += 1.0f - sum;
    }
    for (i = 0; i < verts.size(); ++i) {
        for (int k = 0; k < 4; ++k)
            skin[i].bonef[k] = (float)skin[i].bone[k];
    }
}

// @ 0x412040
void Scene::register_obj(Obj* e)
{
    if (!e->registered && e->active) {
        if (e->flags & 0x1000) {
            objs.push_back(e);
            e->registered = true;
            return;
        }
        objs.resize(objs.size() + 1, 0);
        int i = (int)objs.size() - 1;
        while (i > 0) {
            objs[i] = objs[i - 1];
            --i;
        }
        objs[0] = e;
        e->registered = true;
    }
}

// @ 0x412100
void Scene::add_obj_param(Obj* e, int v)
{
    if (!e->registered && e->active)
        register_obj(e);
    IdxWeight w;
    w.index = v;
    w.weight = 0;
    e->params.push_back(w);
}
