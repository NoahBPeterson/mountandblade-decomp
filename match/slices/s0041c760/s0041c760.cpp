// Slice s0041c760: std::vector<elem14>::_Insert_n / resize (elem14 = bone influence {bone, vector<IdxWeight>}),
// then the resource manager's LOD-mesh group builder and finder (provisional names).
// Built with the default flags (/O2 /MT /Gy /TP): the _Insert_n keeps an EH frame without /EH*.
#include <vector>
#include <memory>
#include "s0041c760.h"

using namespace rgl;

// @ 0x41c760
// (catch funclets at 0x41c916 and 0x41c9cd belong to this COMDAT)
template void std::vector<elem14>::_Insert_n(std::vector<elem14>::iterator, size_t, const elem14&);

static inline int lod_digit(char c)
{
    if (c == '4') return 4;
    if (c == '3') return 3;
    if (c == '2') return 2;
    return c == '1';
}

// @ 0x41ca60
// Builds a LOD group from every mesh whose name matches `pat` (name, or name.lodN); copy != 0 shares the
// meshes, copy == 0 clones them. Missing lower-detail levels are then filled from the previous level.
lod_mesh* resmgr::make_lod_mesh(const tstring& pat, bool copy)
{
    lod_mesh* g = new lod_mesh;
    tstring unused;
    int pos = pat.len;
    int a;
    g->name = pat;
    for (std::vector<mesh*>::iterator it = meshes.begin(); it != meshes.end(); ++it) {
        int level;
        a = level = 0;
        if (!(*it)->name.match(&a, &pos, &pat))
            continue;
        if (pos != (*it)->name.len) {
            const char* p = (*it)->name.buf + pos;
            if (*p == '.') {
                if ((*it)->name.len <= pos + 4)
                    continue;
                if (p[1] != 'l' && p[1] != 'L')
                    continue;
                if (p[2] != 'o' && p[2] != 'O')
                    continue;
                if (p[3] != 'd' && p[3] != 'D')
                    continue;
                level = lod_digit(p[4]);
            } else {
                a = (*it)->name.len;
                continue;
            }
        }
        if (copy) {
            vpush(g->lists[level].v, *it);
        } else {
            mesh* mm = (*it)->clone();
            vpush(g->lists[level].v, mm);
        }
        if (level + 1 > g->count)
            g->count = level + 1;
    }
    for (int k = 1; k < g->count - 1; k++) {
        std::vector<mesh*>& prev = g->lists[k - 1].v;
        std::vector<mesh*>& cur = g->lists[k].v;
        if (cur.size() == 0 && prev.size() > 0) {
            cur.resize(prev.size());
            for (unsigned j = 0; j < prev.size(); j++) {
                if (copy) {
                    vat(cur, j) = vat(prev, j);
                } else {
                    mesh** src = &vat(prev, j);
                    mesh** dst = &vat(cur, j);
                    *dst = (*src)->clone();
                }
            }
        }
    }
    return g;
}

// @ 0x41cd10
template void std::vector<elem14>::resize(size_t, elem14);

// @ 0x41cdc0
lod_mesh* resmgr::get_lod_mesh(const tstring& name)
{
    bool found = false;
    lod_mesh* result = 0;
    std::vector<lod_mesh*>::iterator p = lods.begin();
    while (p != lods.end()) {
        if ((*p)->name.equals(name)) {
            result = *p;
            found = true;
        }
        ++p;
        if (!found)
            continue;
        return result;
    }
    if (found)
        return result;
    lod_mesh* g = make_lod_mesh(name, true);
    result = g;
    g->name = name;
    vpush(lods, result);
    return g;
}
