// pod_elems.h: plain element types of the std::vector<T> instantiations that several objects share (the linker kept one
// COMDAT copy of each helper, so every object that reached it registered the same names). ONE definition each for
// src/game and src/rgl. Only the size matters (stride of the helpers cited next to each type); the contents are unknown.
#ifndef MB_POD_ELEMS_H
#define MB_POD_ELEMS_H

struct E8 { int a, b; };                  // 0x08: stable_sort'ed vector in terrain_a (0x46b7d0..0x46c4a0), resource_manager (0x4180d0 group)
struct E12 { int v[3]; };                 // 0x0c: vector<E12> (agent_combat 0x43edf0 / 0x43f410 / 0x440b80; agent_stats construct 0x436cf0, uninit_fill 0x438960)
struct E100 { int v[25]; };               // 0x64: the particle record, std::vector<E100> (entity 0x41fb00 / 0x410dc0, psys 0x420b40)
struct rec36 { int a[9]; };               // 0x24: fill 0x467b40, copy_backward 0x467b70, uninit_copy 0x46b6e0 (terrain_a); alloc_rec36::construct in agent.cpp
struct rec136 { int a[34]; };             // 0x88: uninit_copy 0x46b750 (terrain_a); alloc_rec136::construct in agent.cpp

struct E32 { int v[8]; };                 // 0x20: deque<E32> (agent_combat), vec<E32> (char_sheet/e96_vec.h: 0x4f4860 reserve_nr, 0x4f4960 _Uninit_copy)

#endif // MB_POD_ELEMS_H
