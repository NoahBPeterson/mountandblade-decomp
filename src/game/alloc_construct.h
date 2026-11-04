// alloc_construct.h: the std::allocator<T>::construct instances of src/game/agent/agent.cpp (0x436c90, 0x436cb0, 0x436cd0, 0x436cf0, 0x436d80): one class per
// element type, shared by every module that names them (agent, agent2, terrain_a, terrain_b, game_2, rgl/mesh2). Each is defined once, in
// agent.cpp, and called (never inlined) from the _Uninit_copy / _Uninit_fill_n instances of the other objects.
#ifndef GAME_ALLOC_CONSTRUCT_H
#define GAME_ALLOC_CONSTRUCT_H
#include "./tstring.h"
#include "../include/ptr_tag.h"
#include "../include/pod_elems.h"      // rec36 / rec136 / E12

struct alloc_tstring { __declspec(noinline) void construct(tstring* p, const tstring& v); };    // 0x436c90
struct alloc_rec36 { __declspec(noinline) void construct(rec36* p, const rec36& v); };          // 0x436cb0
struct alloc_rec136 { __declspec(noinline) void construct(rec136* p, const rec136& v); };       // 0x436cd0
struct alloc_E12 { __declspec(noinline) void construct(E12* p, const E12& v); };                // 0x436cf0
struct alloc_int { __declspec(noinline) void construct(int* p, const int& v); };                // 0x436d80
E12* uninit_copy_E12(const E12* first, const E12* last, E12* dest, alloc_E12& al, ptr_tag tag);              // 0x436d10

#endif // GAME_ALLOC_CONSTRUCT_H
