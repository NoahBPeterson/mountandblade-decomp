// alloc_construct.h: the std::allocator<T>::construct instances of src/game/agent/agent_queries.cpp (0x436c90, 0x436cb0, 0x436cd0, 0x436cf0, 0x436d80): one class per
// element type, shared by every module that names them (agent, agent2, terrain_a, terrain_b, game_2, rgl/mesh2). Each is defined once, in
// agent_queries.cpp, and called (never inlined) from the _Uninit_copy / _Uninit_fill_n instances of the other objects.
#ifndef GAME_ALLOC_CONSTRUCT_H
#define GAME_ALLOC_CONSTRUCT_H
#include <memory>
#include "engine/core/tstring.h"
#include "engine/core/ptr_tag.h"
#include "engine/core/pod_elems.h"      // rec36 / rec136 / E12

// std::allocator<tstring>::construct is the one out-of-line instance 0x436c90 (game/agent/agent_queries.cpp defines it); every other object calls it
typedef std::allocator<tstring> alloc_tstring;
#ifndef ALLOC_TSTRING_CONSTRUCT_DEFINED_HERE
namespace std { template<> void allocator<tstring>::construct(tstring* p, const tstring& v); }
#endif
// the other element types' allocator<T>::construct instances (0x436cb0 rec36, 0x436cd0 rec136, 0x436cf0 E12) and the 4-byte one
// 0x436d80 (pointers / 4-byte classes: ICF folded them to one body; agent_queries.cpp instantiates it for the ragdoll bone pointers): out of
// line, defined in agent_queries.cpp (which instantiates them, so it does not declare them)
#ifndef ALLOC_CONSTRUCT_DEFINED_HERE
namespace std {
template<> void allocator<rec36>::construct(rec36* p, const rec36& v);
template<> void allocator<rec136>::construct(rec136* p, const rec136& v);
template<> void allocator<E12>::construct(E12* p, const E12& v);
}
#endif

#endif // GAME_ALLOC_CONSTRUCT_H
