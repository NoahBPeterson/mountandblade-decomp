// Slice s004d1410 (provisional names). Two passes of the mission-template / battle-scene builder.
// The object owns three std-style containers (3-pointer + capacity layout, as in s004d0c20):
//   this+0x1c  vector<node_t>            stride 0x18 (pos xyz at +0/+4/+8, flag byte at +0x10)
//   this+0x28  vector<edge_t>            stride 0x20
//   this+0x2c  vector<region_t>          stride 0x64
//   this+0x38  vector<edge_t>            stride 0x20 (second edge set)
//   this+0x3c  vector<face_t>            stride 0x34 (2 node indices at +0/+4)
// Helpers used by both functions (other slices): 0x470920 reset flags, 0x477e10 push_back<edge_t>,
// 0x477d20 push_back<region_t>, 0x4c5f60 graph search, 0x4c8000 Block::normal, 0x4c5670 init.
#ifndef S004D1410_H
#define S004D1410_H

#include "rgl.h"

typedef unsigned char uchar;
typedef rgl::vector4 vec3;      // the engine's 3-component math vector is rgl::vector4 (w unused)

// A node of the search graph: position and the "reached" flag.
struct node_t {
    float x;            // +0
    float y;            // +4
    float z;            // +8
    uchar pad0c[4];     // +0xc
    uchar flag;         // +0x10
    uchar pad11[7];
};

// 0x60-byte edge record (two node indices, cost and the 0x50-byte payload the containers store).
struct edge_t {
    int from;           // +0
    int to;             // +4
    float cost;         // +8
    int pad0c;          // +0xc
    uchar pad10[0x50];
};

// 0x34-byte face record: a pair of node indices.
struct face_t {
    int a;              // +0
    int b;              // +4
    int pad08;          // +8
    int pad0c;          // +0xc
    int f10;            // +0x10 used slots
    int f14;            // +0x14
    int f18;            // +0x18
    int f1c;            // +0x1c
    int f20;            // +0x20
    uchar pad24[0x10];
};

// 0x64-byte region record: indices at +0x28.., count at +0x48, edge indices at +0x38.
struct region_t {
    int node;           // +0x00 index of the region's own node
    int f04;            // +0x04
    int f08;            // +0x08
    int f0c;            // +0x0c
    int f10;            // +0x10
    int f14;            // +0x14
    int f18;            // +0x18
    int f1c;            // +0x1c
    int f20;            // +0x20
    int f24;            // +0x24
    int f28;            // +0x28
    int f2c;            // +0x2c
    int f30;            // +0x30
    int f34;            // +0x34
    int f38;            // +0x38
    int f3c;            // +0x3c
    int f40;            // +0x40
    int f44;            // +0x44
    int f48;            // +0x48 count
    int nodes[6];       // +0x4c .. index of slots used
};

struct scene_builder {
    uchar pad00[0x1c];              // +0
    node_t* nodes_first;            // +0x1c
    node_t* nodes_last;             // +0x20
    node_t* nodes_end;              // +0x24
    int e28;                        // +0x28  vector<edge_t> (3 pointers + spare)
    int e2c;                        // +0x2c
    int e30;                        // +0x30
    int e34;                        // +0x34
    edge_t* edges2_first;           // +0x38 vector<edge_t>: edge list built by this file
    edge_t* edges2_last;            // +0x3c
    edge_t* edges2_end;             // +0x40
    int pick;                       // +0x44
    int f48;                        // +0x48
    int f4c;                        // +0x4c
    uchar pad50[0x2c];              // +0x50 .. +0x7b
    int f7c;                        // +0x7c
    bool build_first_pass();        // 0x4d1410
    void build_second_pass();       // 0x4d1880
};

static inline int nodes_size(const scene_builder* s) { return (int)(s->nodes_last - s->nodes_first); }
static inline int edges2_size(const scene_builder* s) { return (int)(s->edges2_last - s->edges2_first); }

#endif
