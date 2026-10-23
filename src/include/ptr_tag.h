// ptr_tag.h: std::_Nonscalar_ptr_iterator_tag as the VC7.1 STL instantiations of the original pass it. An empty
// class passed by value: cl 13.10 still pushes one 4-byte stack slot for it, and an UNINITIALISED local pushes
// whatever EAX holds, so callers that mirror the original's `tag` argument declare a local of this type.
// Callees: the range helpers at 0x4727a0 (_Destroy_range), 0x46b570.. (_Uninit_fill_n), 0x468050 (_Copy_opt).
// One definition for src/game (agent2/agent_types.h, terrain_b/records.h) and src/rgl (mesh2/mesh_2.h).
#ifndef MB_PTR_TAG_H
#define MB_PTR_TAG_H

struct ptr_tag {};

#endif // MB_PTR_TAG_H
