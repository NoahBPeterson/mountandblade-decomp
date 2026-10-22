// Slice s004d4554: Microsoft C++ 7.1 standard-library helper code emitted for the game's vectors:
// copy / fill / destroy range helpers for three element types (strides 0x50, 0x68 and 0xb0), as generated
// by the VC7.1 <memory>/<vector> sources (_Uninit_copy, _Destroy, _Ufill). Every function is complete and
// behaviourally equivalent to the original (same callee per element: ctor 0x4cd080 / 0x4cd100 / 0x4cd170,
// dtor 0x4cd6a0 / 0x4c8ba0 / 0x4c8ab0, copy of a 0x50-byte value 0x4cd080); none is byte-exact: the
// originals were emitted without /EHsc for the destroy loops and with cl's register-arg funclets.
#include <windows.h>
#include <new>

struct Elem50;
struct Elem68;
struct ElemB0;

extern void copy_one_50(Elem50* thisp, Elem50* src);        // 0x4cd080
extern void copy_one_68(Elem68* thisp, Elem68* src);        // 0x4cd100
extern void copy_one_b0(ElemB0* thisp, ElemB0* src);        // 0x4cd170
extern void destroy_one_50(Elem50* thisp);                  // 0x4cd6a0
extern void destroy_one_68(Elem68* thisp);                  // 0x4c8ba0
extern void destroy_one_b0(ElemB0* thisp);                  // 0x4c8ab0

struct Elem50 { char pad0[0x50]; };
struct Elem68 { char pad0[0x68]; };
struct ElemB0 { char pad0[0xb0]; };

void destroy_range_50(Elem50* first, Elem50* last);
void destroy_range_68(Elem68* first, Elem68* last);
void destroy_range_b0(ElemB0* first, ElemB0* last);

// @ 0x4d4554
Elem50* Elem50_copy(Elem50* dest, Elem50* first, Elem50* last)
{
    try {
        for (; first != last; ++first)
            copy_one_50(dest++, first);
        return dest;
    } catch (...) {
        destroy_range_50(first, last);
        throw;
    }
}

// @ 0x4d49c0
Elem50* Elem50_construct(Elem50* dest, Elem50* first, Elem50* last)
{
    for (; first != last; ++first)
        copy_one_50(dest++, first);
    return dest;
}

// @ 0x4d4a28
void destroy_range_50(Elem50* first, Elem50* last)
{
    for (; first != last; ++first)
        destroy_one_50(first);
}

// @ 0x4d4a50
Elem50* Elem50_fill(Elem50* dest, Elem50* first, Elem50* last, Elem50* val)
{
    (void)first;
    (void)last;
    copy_one_50(dest, val);
    return dest;
}

// @ 0x4d1160
Elem68* Elem68_construct(Elem68* dest, Elem68* first, Elem68* last)
{
    if (dest != 0)
        copy_one_68(first, last);
    return dest;
}

// @ 0x4d4ab8
void destroy_range_68(Elem68* first, Elem68* last)
{
    for (; first != last; ++first)
        destroy_one_68(first);
}

// @ 0x4d1180
ElemB0* ElemB0_construct(ElemB0* dest, ElemB0* first, ElemB0* last)
{
    if (dest != 0)
        copy_one_b0(first, last);
    return dest;
}

// @ 0x4d4ae0
ElemB0* ElemB0_copy(ElemB0* dest, ElemB0* first, ElemB0* last)
{
    for (; first != last; ++first)
        copy_one_b0(dest++, first);
    return dest;
}

// @ 0x4d4b4b
void destroy_range_b0(ElemB0* first, ElemB0* last)
{
    for (; first != last; ++first)
        destroy_one_b0(first);
}
