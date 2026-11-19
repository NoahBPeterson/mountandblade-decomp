// rag_model.h: RagModel, the 0x20-byte ragdoll model record (a name and the vector of bones; load 0x4e4690).
// The element of every std::vector<RagModel> instance in the image: _Copy_backward_opt 0x4e42b0, allocator::construct
// 0x4e42f0 (tstring copy 0x42c110, then the vector<E50> copy constructor 0x4da1f0), fill 0x4e5f80 (vector<E50>::operator=
// 0x4dc1c0), _Uninit_copy 0x4ebe10, _Ucopy 0x4fe840, _Insert_n 0x50d260 (Font's skeleton list and the loading screen's
// Entries are this vector). ONE definition (moved out of cls_map/progress_bar.h so that small TUs need not pull in cls_map.h).
#ifndef MB_RAG_MODEL_H
#define MB_RAG_MODEL_H
#include <vector>
#include "../game/tstring.h"
#include "./vec_elems.h"                        // E50: the bone record
#include "./layout32.h"

class TiXmlElement;

struct RagModel {
    tstring name;                               // +0x00
    std::vector<E50> bones;                     // +0x10
    RagModel() {}
    // memberwise, spelled out: the implicit one would call tstring's copy constructor (tstring::operator= returns void)
    RagModel& operator=(const RagModel& o) { name = o.name; bones = o.bones; return *this; }
    void load(TiXmlElement* e);                 // 0x4e4690 (cls_map/progress_bar.cpp)
};
MB_LAYOUT32_ASSERT(rag_model_size_check, sizeof(RagModel) == 0x20);   // the stride of every vector<RagModel> COMDAT (a wrong one crashed the skeleton loader)

// The skeleton table g_skeletons (0xa180e8): the vector of registered ragdoll models, filled from skeleton_bodies.xml.
struct SkeletonBody : std::vector<RagModel> {
    void load(const tstring& path);             // 0x50fc60 (inventory_panel/skeleton_load.cpp; ret 4)
};

#endif // MB_RAG_MODEL_H
