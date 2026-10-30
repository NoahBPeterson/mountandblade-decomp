// Slice s0050fc60: SkeletonBody::load(const tstring&) -- load a skeleton body file (TinyXML document),
// take the <Skeletons> element and for every sibling element append a fresh bone-list record to this
// vector and read the record from the element.
#include <string.h>
#include "skeleton_tinyxml.h"

// ------------------------------------------------------------------ tstring -------------------------------
#include "../tstring.h"

// ------------------------------------------------------------------ externals -----------------------------
void mb_error(const char* fmt, ...);                 // 0x42cc80 (cdecl, game/strat/gui_manager.h)

// the error-path helper: TiXmlNode's destructor body, called by the inlined ~TiXmlDocument
// (TiXmlNode::~TiXmlNode is declared out of line in tinyxml.h, so the doc's implicit dtor emits a call).

// ------------------------------------------------------------------ bone list record ----------------------
// The element type stored inside a record's list; only ever passed through to destroy_items.
struct bone {
    char b[1];
};
#include "../junk_tag.h"
// 0x4d10f0 (game/module_load.cpp: item_modifier_array_dtor) is cdecl with two parameters; the original pushes the allocator and the scratch
// slot as well (the caller pops), so the call goes to the definition's own declaration through the original's argument list
struct item_modifier;
void __cdecl item_modifier_array_dtor(item_modifier* first, item_modifier* last);   // 0x4d10f0
static __forceinline void destroy_items(bone* first, bone* last, void* self, junk_tag tag)
{ ((void (__cdecl*)(bone*, bone*, void*, junk_tag))item_modifier_array_dtor)(first, last, self, tag); }

// engine vector: allocator dword then first/last/end (we only need the destructor, 0x50cc60's model).
struct bone_vec {
    void* alloc;                                    // +0
    bone* first;                                    // +4
    bone* last;                                     // +8
    bone* end;                                      // +0xc
    bone_vec() : first(0), last(0), end(0) {}
    bone_vec(const bone_vec& o);                    // 0x4da1f0
    ~bone_vec()
    {
        if (first != 0) {
            destroy_items(first, last, this, junk_tag());
            ::operator delete(first);
        }
        first = 0;
        last = 0;
        end = 0;
    }
};

// 0x20-byte record: a name and a list of bones
struct RagModel {
    void load(TiXmlElement* e);   // 0x4e4690 (game/cls_map/progress_bar.cpp)
};
struct row20 {
    tstring name;                                   // +0x00
    bone_vec list;                                  // +0x10
    __forceinline void load(TiXmlElement* e) { ((RagModel*)this)->load(e); }   // 0x4e4690: the row is the RagModel record
};

struct TrackEntry;
struct Entries {
    void add(const TrackEntry& val);   // 0x50fbf0 (inventory_panel/loading_screen.cpp)
};

// ------------------------------------------------------------------ this -----------------------------------
// The skeleton body: a vector of 0x20-byte records (see s0050f440's Entries::add 0x50fbf0).
struct SkeletonBody {
    void* alloc;                                    // +0
    row20* first;                                   // +4
    row20* last;                                    // +8
    row20* end;                                     // +0xc
    int size() const { return first == 0 ? 0 : (int)(last - first); }
    int capacity() const { return first == 0 ? 0 : (int)(end - first); }
    __forceinline void add(const row20& val) { ((Entries*)this)->add(*(const TrackEntry*)&val); }   // 0x50fbf0 (loading_screen.cpp's Entries::add: the same vector of 0x20-byte records)
    row20* back() { return &first[size() - 1]; }
    void load(const tstring& filename);
};

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
    TiXmlElement* root = hdoc.FirstChildElement().ToElement();
    if (!root)
        return;
    hskel = TiXmlHandle(root);

    TiXmlElement* e = hskel.FirstChild("Skeletons").FirstChild().ToElement();
    for (; e; e = e->NextSiblingElement()) {
        add(row20());
        back()->load(e);
    }
}
