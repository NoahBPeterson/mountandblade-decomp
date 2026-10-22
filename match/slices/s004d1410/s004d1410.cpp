// Slice s004d1410: two passes of the mission-template / battle-scene builder (provisional names).
// NOTE: reconstruction only, not byte-exact (see nonmatching.txt). Both originals build their result
// records in wide stack temporaries (the 0x4d1410 edge record lives at [esp+0x20..0x6f] and is filled in
// order a, b, c, n_first, n_last, kind, 0, edges, 1, 1, 1; the 0x4d1880 position record lives at
// [esp+0x20..0x5f]) before pushing a 0x50/0x40-byte copy into a vector that lives at this+0x38.
// The vectors are plain 3-pointer std::vector<> objects, so growth happens in the inlined
// "_Buy / _Insert_n" path (capacity == 0 -> fallback insert; capacity > 0 -> append, then _Move_backward).
#include <windows.h>
#include <d3d9.h>
#include "rgl.h"
#include "s004d1410.h"

// ---- helpers owned by other slices --------------------------------------------------------------------
void reset_reach_flags(scene_builder* s);                          // 0x470920
void push_edge(edge_t* vec, const edge_t* what);                   // 0x477e10
void push_region(region_t* vec, const region_t* what);             // 0x477d20
void graph_search(scene_builder* s, int* out, int node, int face); // 0x4c5f60
const float* block_normal(scene_builder* s, int block, float* out); // 0x4c8000
void init_scene(scene_builder* s, int n);                          // 0x4c5670

// @ 0x4d1410
bool scene_builder::build_first_pass()
{
    // exactly two nodes must be reached, otherwise nothing happens
    int reached = 0;
    for (node_t* it = nodes_first; it != nodes_last; ++it)
        if (it->flag)
            reached++;
    if (reached != 2)
        return false;

    bool built = false;
    for (edge_t* pe = edges2_first; pe != edges2_last; ++pe) {
        region_t* reg = (region_t*)pe;
        const int count = reg->f48;
        if (count <= 0)
            continue;

        // the region must contain exactly two reached nodes
        int hits = 0;
        for (int i = 0; i < count; i++)
            if (nodes_first[reg->nodes[i]].flag)
                hits++;
        if (hits != 2)
            continue;

        // ... and no face may join two reached nodes
        bool skip = false;
        for (int i = 0; i < count; i++) {
            const face_t* f = &((const face_t*)edges2_first)[reg->nodes[i]];
            if (nodes_first[f->a].flag && nodes_first[f->b].flag)
                skip = true;
        }
        if (skip)
            continue;

        reg->f48 = 3;

        // push_edge(end(), record): the stack copy of the record is the insert source (a, b, c, then n)
        edge_t src;
        src.from = reg->f2c;                            // +0x00
        src.to = 0;                                     // +0x04, set with the region update below
        src.cost = (float)(edges2_last - edges2_first); // +0x08
        src.pad0c = 0;                                  // +0x0c
        src.pad10[0] = 0;                               // +0x10
        src.pad10[1] = 1;                               // +0x11
        src.pad10[2] = 1;                               // +0x12
        src.pad10[3] = 1;                               // +0x13
        src.pad10[4] = (uchar)reg->node;                // +0x14
        src.pad10[5] = 0;                               // +0x15
        src.pad10[6] = (uchar)reg->f34;                 // +0x16
        src.pad10[7] = (uchar)reg->f40;                 // +0x17
        src.pad10[8] = (uchar)reg->f40;                 // +0x18 (read at 0x4d1684, before the update)
        src.pad10[9] = (uchar)reg->f38;                 // +0x19
        src.pad10[10] = 1;                              // +0x1a
        src.pad10[11] = 1;                              // +0x1b
        src.pad10[12] = 1;                              // +0x1c
        src.pad10[13] = 1;                              // +0x1d
        src.pad10[14] = 1;                              // +0x1e
        src.pad10[15] = 1;                              // +0x1f
        if (!nodes_first[reg->node].flag) {
            reg->f2c = reg->f30;
            src.to = edges2_first ? (int)(edges2_last - edges2_first) : 0;
            reg->f34 = reg->f40;
            reg->f38 = reg->f3c;
            reg->f3c = reg->f44;
        } else {
            src.to = reg->f2c;
            reg->f2c = reg->f30;
            reg->f34 = reg->f40;
            reg->f38 = reg->f44;
            reg->f3c = edges2_first ? (int)(edges2_last - edges2_first) : 0;
        }

        // the direction of the region: (B - A) x (C - A), normalised; (0,1,0) when degenerate
        const node_t* pa = &nodes_first[src.to];
        const node_t* pb = &nodes_first[reg->f2c];
        const node_t* pc = &nodes_first[reg->node];
        vec3 ab(pb->x - pa->x, pb->y - pa->y, pb->z - pa->z);
        vec3 ac(pc->x - pa->x, pc->y - pa->y, pc->z - pa->z);
        vec3 n = cross(ab, ac);
        float len = (float)sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (len > 1e-10f)
            n = n * (1.0f / len);
        else
            n = vec3(0.0f, 1.0f, 0.0f);
        (void)n;

        push_edge(edges2_end, &src);
        reset_reach_flags(this);
        built = true;
    }
    return built;
}

// @ 0x4d1880
void scene_builder::build_second_pass()
{
    init_scene(this, 1);
    f7c = -1;

    // clear the "used" count of every face, then fill the per-node slot tables
    for (face_t* f = (face_t*)edges2_first; f != (face_t*)edges2_last; ++f)
        f->f14 = 0;

    for (face_t* f = (face_t*)edges2_first; f != (face_t*)edges2_last; ++f) {
        if (f->f20)
            continue;
        // The original also walks the region table here and pushes one 0x40-byte position record per
        // surviving face (push_edge into this+0x38); that loop was not recovered in detail.
    }
}
