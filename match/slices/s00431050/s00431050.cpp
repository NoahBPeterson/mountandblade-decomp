// Slice s00431050: TinyXML 2.5.x TiXmlDeclaration::Parse, then the compiler-generated deleting/vector
// destructors of engine record classes that follow the TinyXML code in the binary (0x431400..0x4318c0).
#include <new>      // declares operator delete[]: without it cl 13.10 frees arrays with plain operator delete
#include "tinyxml.h"

// Not part of this slice: defined first so cl sees its register use (it keeps ECX), as in the original TU.
// @ 0x42f840
void TiXmlParsingData::Stamp(const char* now, TiXmlEncoding encoding)
{
    if (tabsize < 1) return;
    int row = cursor.row;
    int col = cursor.col;
    const char* p = stamp;
    while (p < now) {
        const unsigned char* pU = (const unsigned char*)p;
        switch (*pU) {
        case 0:
            return;
        case '\r':
            ++row;
            col = 0;
            ++p;
            if (*p == '\n') {
                ++p;
            }
            break;
        case '\n':
            ++row;
            col = 0;
            ++p;
            if (*p == '\r') {
                ++p;
            }
            break;
        case '\t':
            ++p;
            col = (col / tabsize + 1) * tabsize;
            break;
        case TIXML_UTF_LEAD_0:
            if (encoding == TIXML_ENCODING_UTF8) {
                if (*(p + 1) && *(p + 2)) {
                    if (*(pU + 1) == TIXML_UTF_LEAD_1 && *(pU + 2) == TIXML_UTF_LEAD_2)
                        p += 3;
                    else if (*(pU + 1) == 0xbfU && *(pU + 2) == 0xbeU)
                        p += 3;
                    else if (*(pU + 1) == 0xbfU && *(pU + 2) == 0xbfU)
                        p += 3;
                    else {
                        p += 3;
                        ++col;
                    }
                }
            } else {
                ++p;
                ++col;
            }
            break;
        default:
            if (encoding == TIXML_ENCODING_UTF8) {
                int step = TiXmlBase::utf8ByteTable[*((unsigned char*)p)];
                if (step == 0) step = 1;
                p += step;
                ++col;
            } else {
                ++p;
                ++col;
            }
            break;
        }
    }
    cursor.row = row;
    cursor.col = col;
    stamp = p;
}


// @ 0x431050
const char* TiXmlDeclaration::Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding _encoding)
{
    p = SkipWhiteSpace(p, _encoding);
    TiXmlDocument* document = GetDocument();
    if (!p || !*p || !StringEqual(p, "<?xml", true, _encoding)) {
        if (document) document->SetError(TIXML_ERROR_PARSING_DECLARATION, 0, 0, _encoding);
        return 0;
    }
    if (data) {
        data->Stamp(p, _encoding);
        location = data->Cursor();
    }
    p += 5;

    version = "";
    encoding = "";
    standalone = "";

    while (p && *p) {
        if (*p == '>') {
            ++p;
            return p;
        }
        p = SkipWhiteSpace(p, _encoding);
        if (StringEqual(p, "version", true, _encoding)) {
            TiXmlAttribute attrib;
            p = attrib.Parse(p, data, _encoding);
            version = attrib.Value();
        } else if (StringEqual(p, "encoding", true, _encoding)) {
            TiXmlAttribute attrib;
            p = attrib.Parse(p, data, _encoding);
            encoding = attrib.Value();
        } else if (StringEqual(p, "standalone", true, _encoding)) {
            TiXmlAttribute attrib;
            p = attrib.Parse(p, data, _encoding);
            standalone = attrib.Value();
        } else {
            while (p && *p && *p != '>' && !IsWhiteSpace(*p))
                ++p;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------------------
// 0x431400..0x4318c0: compiler-generated deleting destructors (scalar ??_G in the vtable, ??_E vector
// deleting destructors for `delete[]`) and destructor bodies of engine record classes. Class names and
// field names are provisional; only the layouts (sizes, offsets, destruction order) are verified.
// ---------------------------------------------------------------------------------------------------------

// The engine's 16-byte string with a read cursor (see s00429890/tstring.h); only its destructor matters here.
struct tstring {
    int pos;
    char* buf;          // +4, freed by the destructor
    int cap;
    int len;
    ~tstring();         // 0x42bc40
};

// A heap array pointer followed by two words and a name. Two destructor flavours occur: a null-checked free
// (SubA) and a bare `delete[]` (SubB), so they are two different source types.
struct SubA {
    int* ptr;           // +0
    int a, b;
    tstring name;       // +0xc
    ~SubA() { if (ptr) delete[] ptr; }
};
struct SubB {
    int* ptr;
    int a, b;
    tstring name;
    ~SubB() { delete[] ptr; }
};

// Two abstract-looking base classes (vtables 0x5908ec and 0x5908f8, slots 1 and 2 are the empty function
// 0x477b70). Their scalar deleting destructors are slot 0.
struct IfaceA {
    IfaceA();
    virtual ~IfaceA() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
// @ 0x431400
IfaceA::IfaceA() {}

struct IfaceB {
    IfaceB();
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
// @ 0x431420
IfaceB::IfaceB() {}

// Record with two (array, name) pairs: 0x78 bytes; ~D at 0x431440, vector deleting destructor at 0x431480.
struct RecD {
    int pad[4];
    SubA s1;            // +0x10
    SubA s2;            // +0x2c
    int tail[12];
    ~RecD();
};
// @ 0x431440
RecD::~RecD() {}
// @ 0x431480
void delete_recd_array(RecD* p) { delete[] p; }

// 0x30-byte record, one (array, name) pair at +4: vector deleting destructor at 0x431540.
struct RecG {
    int head;
    SubA s;             // +4
    int tail[4];
};
// @ 0x431540
void delete_recg_array(RecG* p) { delete[] p; }

// 0x30-byte record: name at +0, then an (array, name) pair at +0x10 with an unconditional delete[]; 0x4315d0.
struct RecH {
    tstring name;       // +0
    SubB s;             // +0x10
    int tail;
};
// @ 0x4315d0
void delete_rech_array(RecH* p) { delete[] p; }

// 0x68-byte record: three names, two (array, name) pairs. ~RecI at 0x431660, vector deleting dtor 0x431810.
struct RecI {
    tstring n0, n1, n2; // +0, +0x10, +0x20
    SubA s1;            // +0x30
    SubA s2;            // +0x4c
    ~RecI();
};
// @ 0x431660
RecI::~RecI() {}

// A class whose only member is a tstring: its destructor is a bare tail jump to 0x42bc40 (0x4316b0).
struct NameHolder {
    tstring name;
    ~NameHolder();
};
// @ 0x4316b0
NameHolder::~NameHolder() {}

// Derived from IfaceB with implicit destructors. The constructors are only here to make cl emit the class's
// vtable and with it the vector deleting destructors (slot 0 of the real vtables).
struct RecF : IfaceB {
    int pad[4];
    tstring name;       // +0x14
    int tail[29];
    RecF();
};
RecF::RecF() {}
RecF* make_recf_array(int n) { return new RecF[n]; }     // makes cl emit ??_E (needs a `new[]`)
// @ 0x4316c0
void delete_recf_array(RecF* p) { delete[] p; }

struct RecJ : IfaceB {
    int body[0x2b];
    RecJ();
};
RecJ::RecJ() {}
RecJ* make_recj_array(int n) { return new RecJ[n]; }
// @ 0x431750
void delete_recj_array(RecJ* p) { delete[] p; }

// Owner of a heap array of RecG: frees it and clears the pointer (0x4317b0).
struct RecGList {
    int head;
    RecG* items;        // +4
    void clear();
};
// @ 0x4317b0
void RecGList::clear()
{
    if (items) {
        delete[] items;
        items = 0;
    }
}

// @ 0x431810
void delete_reci_array(RecI* p) { delete[] p; }

// 0x34-byte record: a name at +4 and an (array, name) pair at +0x18; 0x4318c0.
struct RecK {
    int head;
    tstring name;       // +4
    int pad;
    SubA s;             // +0x18
};
// @ 0x4318c0
void delete_reck_array(RecK* p) { delete[] p; }
