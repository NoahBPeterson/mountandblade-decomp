// Shared declarations of slice s0044f3c0 (provisional names; evidence in comments).
#ifndef S0044F3C0_H
#define S0044F3C0_H
#include "mesh.h"
using rgl::vector4;
using rgl::matrix;
using rgl::mesh;
using rgl::mat4;

// rgl string with a read cursor (16 bytes), layout from s00429890/tstring.h
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
    tstring(const tstring& o);                  // 0x42c110
    void operator=(const tstring& o);           // 0x42bc50
};
struct resmgr { mesh* get_object(const tstring& name); };   // 0x419540 (global object at 0x5d6fa0)
extern resmgr g_resmgr;                                     // 0x5d6fa0

// item kind record (0x238 bytes, array pointer at 0x5e29d0, count at 0x5e29d4), see s00434ad0
struct ItemMesh { tstring name; unsigned __int64 flags; };      // 0x18
struct ItemKind {
    char pad0[0x20];
    tstring name;                       // +0x20
    char pad30[0x44 - 0x30];
    int mesh_count;                     // +0x44
    ItemMesh meshes[1];                 // +0x48
    char pad60[0x1c8 - 0x60];
    unsigned type_flags;                // +0x1c8 (low byte = itp_type_*)
    char pad1cc[4];
    unsigned __int64 flags;             // +0x1d0
    char pad1d8[0x20c - 0x1d8];
    int f20c;                           // +0x20c (percent of a unit: * 0.01)
    char pad210[0x238 - 0x210];
    tstring mesh_name_for(int bit);     // 0x433bf0 (s004333c0)
};
extern ItemKind* g_items;               // 0x5e29d0
extern int g_item_count;                // 0x5e29d4

// reference to an item with modifier: item index, amount (low byte of m4) and modifier (byte 7), see s00434ad0
struct ItemRef {
    int item;
    unsigned m4;
    tstring mesh_name() const           // 0x434b20 (inline here)
    {
        if (item >= 0 && item < g_item_count)
            return g_items[item].mesh_name_for((int)m4 >> 24);
        return tstring("Error: Invalid item");
    }
};

float frand();                                                  // 0x40e400 (rand() % 15817) / 15817

// the per-unit object: only the two equipped-item references (slots a, b at +0x80, +0x88) matter here
struct Agent {
    char pad0[0x80];
    ItemRef slots_a;                    // +0x80
    ItemRef slots_b;                    // +0x88
    ItemRef ranged_ref();               // 0x4351f0
    mesh* make_ranged_mesh();           // 0x44f6a0
};
#endif
