// Slice s004f46d0: the out-of-line instantiations of the engine's std::vector<0x60>/<0x20> growth helpers and
// their exception machinery (VC7.1 with /EHsc: _Reserve does `new`, so the try/catch handler deletes the fresh
// block and rethrows).
//
//   0x4f46d0  vec60::resize(n): grow the 0x60-byte element vector (begin +4, end +8, cap +0xc) when its
//             capacity is below n: allocate n * 0x60, copy the old range with the per-element copy loop, free
//             the old block and install the new one.
//   0x4f47d8  the handler funclett of 0x4f46d0: delete the fresh block, rethrow.
//   0x4f47f0  the copy loop of 0x4f46d0: uninitialized_copy(first, last, dest, src) over 0x60-byte elements.
//   0x4f4856  the no-catch funclett: rethrow.
//   0x4f4860  vec20::resize(n): the same for 0x20-byte elements.
//   0x4f4947  its handler funclett: delete + rethrow.
//   0x4f4960  its copy loop over 0x20-byte elements.
//   0x4f49c6  the no-catch funclett: rethrow.
// The element-wise copy is performed by the engine's copy constructors at 0x40f5b0 (0x60) / 0x40f5f0 (0x20).
#include "rgl.h"
#include <new>

struct elem60 { char b[0x60]; elem60(const elem60& o); };   // 0x40f5b0
struct elem20 { char b[0x20]; elem20(const elem20& o); };   // 0x40f5f0

struct vec60 {                                  // begin +4, end +8, cap +0xc (alloc word at +0)
    int alloc;
    elem60* begin;
    elem60* end;
    elem60* cap;
    void resize(unsigned n);                    // 0x4f46d0
};
struct vec20 {
    int alloc;
    elem20* begin;
    elem20* end;
    elem20* cap;
    void resize2(unsigned n);                   // 0x4f4860
};

void* operator_new_(unsigned n);                 // 0x53545e
void  operator_delete_(void* p);                 // 0x535712
void  rethrow_();                                // 0x5368e6 (_CxxThrowException, no object)
// @ 0x4f47f0  (uninitialized_copy: first, last, dest, tag)
elem60* copy60(elem60* first, elem60* last, elem60* dest, void*)
{
    for (; first != last; ++dest, ++first)
        new (dest) elem60(*first);
    return dest;
}

// @ 0x4f4960
elem20* copy20(elem20* first, elem20* last, elem20* dest, void*)
{
    for (; first != last; ++dest, ++first)
        new (dest) elem20(*first);
    return dest;
}

// @ 0x4f46d0
void vec60::resize(unsigned n)
{
    if (n > 0x2aaaaaa)
        throw 0;
    unsigned have = begin ? (unsigned)(end - begin) : 0;
    if (have >= n)
        return;
    elem60* fresh = (elem60*)operator_new_(n * 0x60);
    try {
        copy60(begin, end, fresh, this);
    } catch (...) {
        operator_delete_(fresh);
        throw;
    }
    unsigned old = begin ? (unsigned)(end - begin) : 0;
    if (begin)
        operator_delete_(begin);
    cap = fresh + n;
    end = fresh + old;
    begin = fresh;
}

// @ 0x4f4860
void vec20::resize2(unsigned n)
{
    if (n > 0x7ffffff)
        throw 0;
    unsigned have = begin ? (unsigned)(end - begin) : 0;
    if (have >= n)
        return;
    elem20* fresh = (elem20*)operator_new_(n * 0x20);
    try {
        copy20(begin, end, fresh, this);
    } catch (...) {
        operator_delete_(fresh);
        throw;
    }
    unsigned old = begin ? (unsigned)(end - begin) : 0;
    if (begin)
        operator_delete_(begin);
    cap = fresh + n;
    end = fresh + old;
    begin = fresh;
}
