// Slice s0041ee20, part 3: erase_entity compiled apart from the std::remove instantiation (0x41f080): in the
// original the call is to an opaque function, while a visible std::remove body lets cl keep end() in a register
// across it.
#include <vector>

namespace std {
template<class _FI, class _Ty> _FI remove(_FI _First, _FI _Last, const _Ty& _Val);     // 0x41f080 (defined in s0041ee20.cpp)
}

struct entity;
struct world_b {
    char pad0[0x7c];
    std::vector<entity*> ents;   // +0x7c (first at +0x80)
    void erase_entity(entity* e);           // 0x41f450
};

// @ 0x41f450
void world_b::erase_entity(entity* e)
{
    std::vector<entity*>::iterator it = std::remove(ents.begin(), ents.end(), e);
    ents.erase(it, ents.end());
}
