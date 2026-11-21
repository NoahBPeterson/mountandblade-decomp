// party_2_copy.cpp: 0x4b2160, std::allocator<TrackRec>::construct of the party_2 object.
// It is a thiscall member in the original (ret 8, the ecx allocator unused); it lives in its own translation unit so that the
// callers in party_2.cpp keep treating it as an opaque thiscall (cl would otherwise give them the callee's register knowledge).
#define PARTY2_CONSTRUCT_DEFINED_HERE
#include "./party_2.h"

// @ 0x4b2160
template void std::allocator<TrackRec>::construct(TrackRec*, const TrackRec&);
