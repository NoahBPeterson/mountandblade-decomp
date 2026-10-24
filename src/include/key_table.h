// key_table.h: one key-frame table: the values (vector4 or quaternion records), the key array (ints) and the key count.
// ONE definition for src/rgl/dynamic/fx_anim.cpp (the table object an AnimRec points to; find 0x423a50 is defined there)
// and src/game/agent/agent.cpp (KeyTable::sample 0x436ae0 reads quaternion records; the vector4 flavour is KeyFrames
// in agent.h). The value pointer is untyped: qv() / v4() give the two views.
#ifndef MB_KEY_TABLE_H
#define MB_KEY_TABLE_H
#include "./quaternion.h"
namespace rgl { struct vector4; }

struct KeyTable {
    void* vals; int* keys; int count;
    __declspec(noinline) void find(int* cur, float* frac, float* key);                      // 0x423a50
    rgl::vector4* v4() const { return (rgl::vector4*)vals; }
    quaternion* qv() const { return (quaternion*)vals; }
    quaternion sample(int key);                                                             // 0x436ae0 (hidden return pointer)
};

#endif // MB_KEY_TABLE_H
