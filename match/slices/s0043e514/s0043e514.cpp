// Slice s0043e514: VC7.1 <vector>/<deque> instantiations, continued from s0043db60 (tails of the
// vector<tstring>::_Insert_n at 0x43e310), plus a tiny Skeleton helper. Built WITHOUT an EH switch (the STL
// try/catch alone makes cl emit the frames; with /EHa cl also keeps the ~E56 call out of line).
//   0x43e514  cold tail of vector<tstring>::_Insert_n 0x43e310 (neighbour slice s0043db60)
//   0x43e6e0  std::_Destroy_range<E56*>                     (E56 = 0x38-byte record holding two vector<int>)
//   0x43e590  std::deque<E32>::_Growmap(size_t)             (_DEQUESIZ == 1, max_size/1 == 0x7ffffff)
//   0x43e740  Skeleton::ensure_updated()
//   0x43e760  std::vector<int>::_Buy(size_t)
//   0x43e7b0  std::vector<int>::_Insert_n(where, n, val)    (+ catch funclet / cold tail 0x43e910)
//   0x43ea00  std::vector<Rec1c8>::_Insert_n                (+ funclets/cold tails 0x43ebc9, 0x43ec91)
//   0x43ed20  std::vector<int>::vector(const vector<int>&)  (+ catch funclet 0x43edd9)
#include <vector>
#include <deque>
#include <memory>
#include <new>

// ---- element types ------------------------------------------------------------------------------------
struct E12 { int v[3]; };      // 0x0c
struct E32 { int v[8]; };      // 0x20 (deque element)

// The engine's 16-byte string; destructor 0x42bc40, range destroyer 0x42d360 (s0042d360).
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring() {}
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
};

// 0x1c8-byte polymorphic record (vtable 0x590b84, copy ctor 0x437bb0, deleting dtor 0x437870): see s004375b0
// ("Agent") and s00431960 ("Item"). Layout reduced to what the copy/destroy paths touch.
struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct Rec1c8 : IfaceB {
    char body1[0x190 - 0x10];
    tstring name;                               // +0x190
    char body2[0x1c8 - 0x1a0];
    Rec1c8(const Rec1c8& o);                    // 0x437bb0
    Rec1c8& operator=(const Rec1c8& o);         // 0x437d60
};

// 0x38-byte record: three words, two vector<int> (+0xc, +0x1c), three words. Copy ctor 0x43f510 (s0043edf0).
struct E56 {
    int a, b, c;
    std::vector<int> v1;       // +0x0c
    std::vector<int> v2;       // +0x1c
    int d, e, f;
};

// ---- skeleton (animator) -----------------------------------------------------------------------------
// *(agent+0x250)->0x304: shares its layout with s00422130 (Animator, update 0x422130) and s00421aa0
// (update_poses 0x422080).
struct Skeleton {
    int nbones;                                 // +0
    char* recs;                                 // +4 (0x1dc-byte records)
    char pad8[0x50 - 8];
    char done;                                  // +0x50
    char pad51[0x64 - 0x51];
    int has_poses;                              // +0x64
    void update();                              // 0x422130
    void update_poses();                        // 0x422080
    void ensure_updated();                      // 0x43e740
};

// @ 0x43e740
void Skeleton::ensure_updated()
{
    if (!done) {
        if (has_poses)
            update_poses();
        else
            update();
    }
}

// ---- instantiations ---------------------------------------------------------------------------------
// 0x43e310 (neighbour slice) with its cold tail 0x43e514 in the same COMDAT
template void std::vector<tstring>::_Insert_n(std::vector<tstring>::iterator, size_t, const tstring&);
// 0x43e590
// @ 0x43e590
template void std::deque<E32>::_Growmap(size_t);
// 0x43e760
// @ 0x43e6e0
template void std::_Destroy_range(E56*, E56*, std::allocator<E56>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x43e760
template bool std::vector<int>::_Buy(size_t);
// 0x43e7b0 (+ 0x43e910)
// @ 0x43e7b0
template void std::vector<int>::_Insert_n(std::vector<int>::iterator, size_t, const int&);
// 0x43ea00 (+ 0x43ebc9, 0x43ec91)
// @ 0x43ea00
template void std::vector<Rec1c8>::_Insert_n(std::vector<Rec1c8>::iterator, size_t, const Rec1c8&);
// 0x43ed20 (+ 0x43edd9)
// @ 0x43ed20
template std::vector<int>::vector(const std::vector<int>&);
