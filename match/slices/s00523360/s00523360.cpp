// Slice s00523360 (batch ds1): a 4-bit alpha/colour nibble setter, the terrain-type -> material table,
// a link-chain builder over the 0x88/0x24 record arrays, and a small array-scan helper.
// Layouts below are read off the disassembly of this range; names are provisional.
#include <windows.h>
#include "rgl.h"
#include "../s0042b790/tstring.h"

// ---- resource manager (already used by s0049b630 / s004f7f00 / s004fd1d0) --------------------------------
struct material;
struct resource_manager {
    material* get_material(const tstring& name);        // 0x418a70 (thiscall, ret 4)
};
extern resource_manager g_resmgr;                       // 0x5d6fa0

// ---- 0x523360 -----------------------------------------------------------------------------------------
// The object at +0x2c carries a render flag word at +0x1c: bits 24..27 are a 4-bit colour/alpha nibble.
struct FlagNode {
    char     pad[0x1c];
    unsigned f1c;                                       // +0x1c
};
struct NibbleObj {
    char      pad[0x2c];
    FlagNode* f2c;                                      // +0x2c
    void set_nibble(int v);                             // 0x523360 (thiscall, ret 4)
};

// @ 0x523360    void NibbleObj::set_nibble(int v)   (thiscall, ret 4)
void NibbleObj::set_nibble(int v)
{
    if (this->f2c) {
        this->f2c->f1c &= 0xf0ffffff;
        if (v < 0)
            v += 0x10;
        this->f2c->f1c |= (unsigned)v << 24;
    }
}

// ---- 0x523390 -----------------------------------------------------------------------------------------
// @ 0x523390    material* terrain_material(int type)   (ret 4; `this` unused -> __stdcall)
material* __stdcall terrain_material(int type)
{
    if (type == 0)
        return g_resmgr.get_material(tstring("map_ocean"));
    if (type == 1)
        return g_resmgr.get_material(tstring("map_mountain"));
    if (type == 3)
        return g_resmgr.get_material(tstring("map_plain"));
    if (type == 2)
        return g_resmgr.get_material(tstring("map_steppe"));
    if (type == 4)
        return g_resmgr.get_material(tstring("map_snow"));
    if (type == 5)
        return g_resmgr.get_material(tstring("map_desert"));
    if (type == 8)
        return g_resmgr.get_material(tstring("map_river"));
    if (type == 9)
        return g_resmgr.get_material(tstring("map_mountain_forest"));
    if (type == 7)
        return g_resmgr.get_material(tstring("map_bridge"));
    if (type == 0x15)
        return g_resmgr.get_material(tstring("map_shore"));
    if (type == 0x16)
        return g_resmgr.get_material(tstring("map_foam"));
    if (type == 0x17)
        return g_resmgr.get_material(tstring("map_reverse_foam"));
    return g_resmgr.get_material(tstring("default"));
}

// ---- 0x5234e0 -----------------------------------------------------------------------------------------
// Record arrays: +0x14 holds records of 0x88 bytes (count first, then a neighbour-id list at +0x14);
// +0x24 holds nodes of 0x24 bytes (0,4,8,c,10 are the fields this code reads).
struct Rec88 {
    int  count;                                         // +0x00
    char pad4[0x14 - 4];
    int  nb[1];                                         // +0x14 neighbour ids
};
struct Node24 {
    int f00;                                            // +0x00
    int f04;                                            // +0x04
    int f08;                                            // +0x08
    int f0c;                                            // +0x0c
    int f10;                                            // +0x10
    char pad14[0x24 - 0x14];
};
struct ChainGraph {
    char   pad0[0x14];
    Rec88* f14;                                         // +0x14
    char   pad18[0x24 - 0x18];
    Node24* f24;                                        // +0x24

    void build_chain(int target, int start, int* out, int* count);   // 0x5234e0 (ret 0x10)
};

// @ 0x5234e0    void ChainGraph::build_chain(int target, int start, int* out, int* count)   (ret 0x10)
// Follows the 0x24-node chain from `start`, picking (at each step) the last neighbour that links to
// `target`, is not the node we came from, and appends the far end of that link once.
void ChainGraph::build_chain(int target, int start, int* out, int* count)
{
    int cur = start;
    int prev = -1;
    int next;
    *count = 0;
    do {
        Rec88* rec = (Rec88*)((char*)this->f14 + cur * 0x88);
        int n = rec->count;
        int found = -1;
        if (n <= 0)
            return;
        int* nb = (int*)((char*)rec + 0x14);
        do {
            int v = *nb;
            Node24* node = (Node24*)((char*)this->f24 + v * 0x24);
            if (node->f00 > 1 && node->f04 != prev && node->f08 != prev
                && (node->f0c == target || node->f10 == target))
                found = v;
            nb++;
        } while (--n);
        if (found < 0)
            return;
        Node24* node = (Node24*)((char*)this->f24 + found * 0x24);
        next = (node->f04 == cur) ? node->f08 : node->f04;
        int c = *count;
        bool dup = false;
        if (node->f0c == target) {
            for (int i = 0; i < c; i++)
                if (out[i] == node->f10)
                    dup = true;
            if (!dup) {
                out[c] = node->f10;
                (*count)++;
            }
        } else {
            for (int i = 0; i < *count; i++)
                if (out[i] == node->f0c)
                    dup = true;
            if (!dup) {
                out[c] = node->f0c;
                (*count)++;
            }
        }
        prev = cur;
        cur = next;
    } while (next != start);
}

// ---- 0x523630 -----------------------------------------------------------------------------------------
struct SceneArrays {
    char  pad0[4];
    char* a_first;                                      // +0x04 (elements of 0x24)
    char* a_last;                                       // +0x08
    char  padc[0x14 - 0xc];
    char* b_first;                                      // +0x14 (elements of 0x88)
    char* b_last;                                       // +0x18
    int a_size() const { return a_first ? (int)((a_last - a_first) / 0x24) : 0; }
    int b_size() const { return b_first ? (int)((b_last - b_first) / 0x88) : 0; }
};
struct Scene {
    char         pad0[0xb0];
    SceneArrays* arr;                                   // +0xb0
};
extern Scene* g_scene;                                  // 0xa732b4

// @ 0x523630    void __stdcall scan_arrays(void* p, int unused)   (ret 8)
void __stdcall scan_arrays(void* p, int unused)
{
    for (unsigned i = 0; i < (unsigned)g_scene->arr->a_size(); i++)
        *(int*)((char*)p + 0xc3500) = (int)0xcb189680;
    volatile char buf[100000];
    for (unsigned i = 0; i < (unsigned)g_scene->arr->b_size(); i++)
        buf[i] = 0;
    for (unsigned i = 0; i < (unsigned)g_scene->arr->b_size(); i++)
        ;
}
