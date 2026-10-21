// Slice s00431960: compiler-generated (vector) deleting destructors and destructor bodies of engine record
// classes (continuing s00431050), the std::vector<Item> helpers they use, and two tiny helpers.
// Class names and field names are provisional; only layouts, sizes and destruction orders are verified.
#include <new>
#include <vector>
#include <list>

// The engine's 16-byte string (see s00431050): only its destructor matters here.
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    ~tstring();         // 0x42bc40
};

// Two small abstract bases (vtables 0x5908ec and 0x5908f8, see s00431050).
struct IfaceA {
    IfaceA();
    virtual ~IfaceA() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct IfaceB {
    IfaceB();
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};

// Polymorphic owned object (only its virtual destructor is called).
struct Poly {
    virtual ~Poly();
};

// 0x1c8-byte polymorphic record, kept in a std::vector (helpers 0x431a30 / 0x431a60).
struct Item {
    virtual ~Item();
    char body[0x1c4];
};
template class std::vector<Item>;       // 0x431a30 _Destroy, 0x431a60 _Tidy

// Tail objects with out-of-line destructors.
struct Tail { char body[0x18]; ~Tail(); };      // 0x42d390
struct Tail2 { char body[0x20]; ~Tail2(); };    // 0x4c4d10

struct Owner { Poly* p; ~Owner() { delete p; } };

// 0x48-byte record: name, vector<int> at +0x10, owned polymorphic object at +0x24.
struct RecA {
    tstring name;               // +0
    std::vector<int> v;         // +0x10
    int pad20;
    Owner owned;                // +0x24
    char tail[0x20];
};
// @ 0x431960
void delete_reca_array(RecA* p) { delete[] p; }

// 0x34-byte record: vector<int> at +0, list<int> at +0x10, vector<int> at +0x20. ~RecP 0x431ab0.
struct RecP {
    std::vector<int> a;         // +0
    std::list<int> l;           // +0x10
    int pad1c;
    std::vector<int> b;         // +0x20
    int tail;
    ~RecP();
};
// @ 0x431ab0
RecP::~RecP() {}
// @ 0x431d30
void delete_recp_array(RecP* p) { delete[] p; }

// A class that owns eight names, a vector and more names (non-virtual destructor body 0x431b10).
struct RecQ {
    tstring n0, n1, n2, n3, n4;     // +0 .. +0x40
    int pad50;
    tstring n5;                     // +0x54
    char pad64[0x88 - 0x64];
    tstring names[8];               // +0x88
    char pad108[0x128 - 0x108];
    std::vector<int> v;             // +0x128
    ~RecQ();
};
// @ 0x431b10
RecQ::~RecQ() {}

// IfaceA-derived record (0x5ac8 bytes). ~RecR 0x431b90, vector deleting destructor 0x431cb0.
struct RecR : IfaceA {
    char pad4[8];
    tstring n0;                     // +0xc
    char pad1c[4];
    tstring n1;                     // +0x20
    char pad30[0x54 - 0x30];
    tstring n2;                     // +0x54
    char pad64[0x8b8 - 0x64];
    std::vector<int> v0;            // +0x8b8
    char pad8c8[0x59d8 - 0x8c8];
    std::vector<int> v1;            // +0x59d8
    char pad59e8[0xc];
    std::vector<int> v2;            // +0x59f4 (ptr at +0x59f8)
    char pad5a04[0x5a28 - 0x5a04];
    tstring names[8];               // +0x5a28
    char tail[0x5ac8 - 0x5aa8];
    RecR();
};
RecR::RecR() {}
// 0x431b90 RecR::~RecR (implicit)
RecR* make_recr_array(int n) { return new RecR[n]; }
// @ 0x431cb0
void delete_recr_array(RecR* p) { delete[] p; }

// A class with eight names and a vector (non-virtual destructor body 0x431c40).
struct RecS {
    tstring n0;                     // +0
    tstring n1;                     // +0x10
    tstring n2;                     // +0x20
    char pad30[0x420 - 0x30];
    tstring names[8];               // +0x420
    char pad4a0[0x4f4 - 0x4a0];
    std::vector<int> v;             // +0x4f4
    ~RecS();
};
// @ 0x431c40
RecS::~RecS() {}

// IfaceB-derived record (0x494 bytes) with a vector<Item>. ~RecY 0x431e10, vector deleting dtor 0x431ec0.
struct RecY : IfaceB {
    char pad4[0xc];
    tstring n0;                     // +0x10
    tstring n1;                     // +0x20
    char pad30[4];
    tstring n2;                     // +0x34
    tstring n3;                     // +0x44
    char pad54[0x68 - 0x54];
    tstring n4;                     // +0x68
    char pad78[0x104 - 0x78];
    std::vector<Item> items;        // +0x104
    char pad114[0x418 - 0x114];
    tstring n5;                     // +0x418
    char pad428[0xc];
    tstring n6;                     // +0x434
    char pad444[4];
    tstring n7;                     // +0x448
    char pad458[0xc];
    tstring n8;                     // +0x464
    std::vector<int> v;             // +0x474
    tstring n9;                     // +0x484
    RecY();
    virtual ~RecY();
};
RecY::RecY() {}
// @ 0x431e10
RecY::~RecY() {}
RecY* make_recy_array(int n) { return new RecY[n]; }
// @ 0x431ec0
void delete_recy_array(RecY* p) { delete[] p; }

// IfaceB-derived record (0x5928 bytes) with a vector<int> at +0x1c and a Tail2 at +0x53c0.
struct RecZ : IfaceB {
    char pad4[0x18];
    std::vector<int> v;             // +0x1c
    char pad2c[0x53c0 - 0x2c];
    Tail2 t;                        // +0x53c0
    char tail[0x5928 - 0x53e0];
    RecZ();
};
RecZ::RecZ() {}
RecZ* make_recz_array(int n) { return new RecZ[n]; }
// @ 0x431f40
void delete_recz_array(RecZ* p) { delete[] p; }

// A class with eight names, a vector and a Tail (non-virtual destructor body 0x432010).
struct RecT {
    tstring n0;                     // +0
    int pad10;
    tstring n1;                     // +0x14
    char pad24[0x234 - 0x24];
    Tail t;                         // +0x234
    std::vector<int> v;             // +0x24c
    char pad25c[0x20];
    tstring names[8];               // +0x27c
    char pad2fc[0x5330 - 0x2fc];
    ~RecT();
};
// @ 0x432010
RecT::~RecT() {}

// @ 0x432080
struct BigObj { char pad[0x5340]; void reset(); };
void BigObj::reset()
{
    *(int*)(pad + 0x5338) = 0;
    *(char*)(pad + 0x5330) = 0;
    *(int*)(pad + 0x532c) = 0;
    *(int*)(pad + 0x5324) = 0;
    *(int*)(pad + 0x284) = 0;
}

// Key-code test: false for the first code of each triple group_base() (s00432130) returns, 0xc7 + 3k (key_home 0xc7,
// 0xca, key_right 0xcd, key_down 0xd0, key_delete 0xd3, ... in Module System header_triggers numbering).
// @ 0x4320b0
int not_group_base(int x)
{
    int r = 1;
    if (x == 0xc7 || x == 0xca || x == 0xcd || x == 0xd0 || x == 0xd3 || x == 0xd6 || x == 0xd9 ||
        x == 0xdc || x == 0xdf || x == 0xe2 || x == 0xe5 || x == 0xe8 || x == 0xeb)
        r = 0;
    return r;
}
