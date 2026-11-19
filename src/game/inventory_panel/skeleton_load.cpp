// Slice s0050fc60: SkeletonBody::load(const tstring&) -- load a skeleton body file (TinyXML document),
// take the <Skeletons> element and for every sibling element append a fresh bone-list record to this
// vector and read the record from the element.
#include <string.h>
#include "../../tinyxml/tinyxml.h"

// ------------------------------------------------------------------ tstring -------------------------------
#include "../tstring.h"

// ------------------------------------------------------------------ externals -----------------------------
void mb_error(const char* fmt, ...);                 // 0x42cc80 (cdecl, game/strat/gui_manager.h)

// the error-path helper: TiXmlNode's destructor body, called by the inlined ~TiXmlDocument
// (TiXmlNode::~TiXmlNode is declared out of line in tinyxml.h, so the doc's implicit dtor emits a call).

// ------------------------------------------------------------------ the record --------------------------
// RagModel (src/include/rag_model.h): a name and a vector of bones; load 0x4e4690 (cls_map/progress_bar.cpp)
#include <vector>
#include "../../include/rag_model.h"
namespace std {
template<> void _Destroy_range(E50*, E50*, allocator<E50>&, _Nonscalar_ptr_iterator_tag);   // 0x4d10f0 (module_load: the bone list's destroy)
template<> void vector<RagModel>::push_back(const RagModel&);                                // 0x50fbf0 (inventory_panel/loading_screen.cpp)
}

// ------------------------------------------------------------------ this -----------------------------------
// @ 0x50fc60
void SkeletonBody::load(const tstring& filename)
{
    TiXmlDocument doc(filename.c_str());
    if (!doc.LoadFile()) {
        mb_error("Unable To Load Skeleton Body File!");
        return;
    }

    TiXmlHandle hdoc(&doc);
    TiXmlHandle hskel(0);
    TiXmlElement* root = hdoc.FirstChildElement().Element();
    if (!root)
        return;
    hskel = TiXmlHandle(root);

    TiXmlElement* e = hskel.FirstChild("Skeletons").FirstChild().Element();
    for (; e; e = e->NextSiblingElement()) {
        push_back(RagModel());
        (*this)[size() - 1].load(e);
    }
}
