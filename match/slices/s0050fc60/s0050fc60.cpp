// Slice s0050fc60: SkeletonBody::load(const tstring&) -- load a skeleton body file (TinyXML document),
// take the <Skeletons> element and for every sibling element append a fresh bone-list record to this
// vector and read the record from the element.
#include <string.h>
#include "tinyxml.h"

// ------------------------------------------------------------------ tstring -------------------------------
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                      // 0x42bbd0
    ~tstring();                                     // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// ------------------------------------------------------------------ externals -----------------------------
void mb_error(const char* msg);                     // 0x42cc80 (cdecl)

// the error-path helper: TiXmlNode's destructor body, called by the inlined ~TiXmlDocument
// (TiXmlNode::~TiXmlNode is declared out of line in tinyxml.h, so the doc's implicit dtor emits a call).

// ------------------------------------------------------------------ bone list record ----------------------
// The element type stored inside a record's list; only ever passed through to destroy_items.
struct bone {
    char b[1];
};
struct junk_tag { int pad; junk_tag() {} };
void destroy_items(bone* first, bone* last, void* self, junk_tag tag);       // 0x4d10f0

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
struct row20 {
    tstring name;                                   // +0x00
    bone_vec list;                                  // +0x10
    void load(const TiXmlElement* e);               // 0x4e4690
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
    void insert_n(row20* where, unsigned n, const row20& val);       // 0x50d260
    void add(const row20& val);                                     // 0x50fbf0
    row20* back() { return &first[size() - 1]; }
    void load(const tstring& filename);
};

// @ 0x50fc60
void SkeletonBody::load(const tstring& filename)
{
    TiXmlDocument doc(filename.c_str());
    if (!doc.LoadFile(0)) {
        mb_error("Unable To Load Skeleton Body File");
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
