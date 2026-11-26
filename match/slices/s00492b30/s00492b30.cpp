// Slice s00492b30: std::vector<E8>::_Insert_n, a deque map-grow/push_back pair, and the
// tooltip / text-panel object functions around 0x4930xx-0x4934xx.
//
// The original keeps the STL helper bodies (??_Xlen/_Ucopy/_Ufill, fill, copy_backward) OUT OF
// LINE in other functions, so they are only declared here: the compiler then must emit calls with
// exactly the original's argument shapes instead of inlining them.
#pragma warning(disable:4530)
#include <new>
#include <vector>
#include <deque>
#include <string.h>
#include "tstring.h"

typedef unsigned size_type;

// -------------------------------------------------------------------------------------------------
// Out-of-line engine callees (their bodies live in other slices).  Declared only, so every call
// below stays a real call with `this` in ECX and arguments on the stack.
// -------------------------------------------------------------------------------------------------
struct PanelBase  { void init(); };                       // 0x48ae20
struct Tail       { void dtor(); };                       // 0x42d390
struct BufferBase { void dtor(); };                       // 0x487c00
struct CursorObj  { void reset(); };                      // 0x492a20
struct Obj475d50  { void dtor(); };                       // 0x475d50
struct Mgr490340  { void clear(); };                      // 0x490340
struct Obj4c6e30  { void release(); };                    // 0x4c6e30
struct Obj4317b0  { void reset(); };                      // 0x4317b0
struct RecD       { void* scalar_delete(unsigned); };     // 0x431480  (ret 4)
// UTF-8 -> code points (0x487b00).  The original uses cl's static register convention, so the
// arguments are ordered to put the tstring in EDI and the count out-pointer in EDX.
static __declspec(noinline) void decode_utf8_487b00(int* _Out, const tstring* _s, int* _Buf,
                                                    const int* _Maxn)
    {
    *_Out = 0;
    int _n = _s->len;
    int _i = 0;
    if (_n > 0)
        {
        do
            {
            if (*_Out >= *_Maxn)
                break;
            unsigned char _c = _s->buf[_i];
            int _ch;
            if (_c >= 0xf0 && _c < 0xf5)
                {
                const unsigned char* _q = (const unsigned char*)_s->buf + _i;
                _ch = (((((_q[0] & 7) << 6) | (_q[1] & 0x3f)) << 6 | (_q[2] & 0x3f)) << 6) | (_q[3] & 0x3f);
                _i += 4;
                }
            else if (_c >= 0xe0 && _c < 0xf0)
                {
                const unsigned char* _q = (const unsigned char*)_s->buf + _i;
                _ch = ((((_q[0] & 0xf) << 6) | (_q[1] & 0x3f)) << 6) | (_q[2] & 0x3f);
                _i += 3;
                }
            else if (_c >= 0xc2 && _c < 0xe0)
                {
                const unsigned char* _q = (const unsigned char*)_s->buf + _i;
                _ch = ((_q[0] & 0x1f) << 6) | (_q[1] & 0x3f);
                _i += 2;
                }
            else
                {
                _i++;
                if (_c >= 0x80)
                    continue;
                _ch = (signed char)_c;
                }
            _Buf[*_Out] = _ch;
            (*_Out)++;
            }
        while (_i < _n);
        }
    }
struct Obj488720  { void update(); };                     // 0x488720
struct VecTstring {
    void push_back(const tstring&);                       // 0x43f2a0
};
struct StrFail    { void fail() const; };                 // 0x43e2a0 (vector<T> too long)
struct DeqFail    { void fail() const; };                 // 0x43e0e0 (deque<T> too long)

// =================================================================================================
// 0x492b30 / 0x493090: a class whose first dword is not part of the vector, so the vector's
// _Myfirst/_Mylast/_Myend live at +4/+8/+0xc.
// =================================================================================================
// The element is an __int64: its _Ucopy is the out-of-line memmove forwarder 0x48c570 (scalar element) while
// _Ufill 0x48ce90 and fill 0x48ad70 move it as two dwords. The vector's first dword is its allocator.
struct FPair { float a, b; };                             // 8-byte float pair

// @ 0x492b30
template void std::vector<__int64>::_Insert_n(std::vector<__int64>::iterator, size_t, const __int64&);

// @ 0x493090
template void std::vector<__int64>::resize(size_t, __int64);

// =================================================================================================
// 0x492ee0: vector<T*> style "destroy back element and free it, then pop" helper.
// =================================================================================================
struct PtrVec14 {
    char _pad[0x14];
    void** _Myfirst;                                      // +0x14
    void** _Mylast;                                       // +0x18
    size_type size() const { return (_Mylast - _Myfirst); }
    void pop_back_delete();
};

// @ 0x492ee0
void PtrVec14::pop_back_delete()
    {
    size_type _n = size();
    void* _Ptr = _Myfirst[_n - 1];
    if (_Ptr != 0)
        {
        ((Obj475d50*)_Ptr)->dtor();
        ::operator delete(_Ptr);
        }
    if (_Myfirst != 0 && size() != 0)
        --_Mylast;
    }

// =================================================================================================
// 0x492f40: the tooltip/text-panel destructor.  It walks a fixed set of tstring members and a few
// heap arrays (the original's offsets are given in each comment).
// =================================================================================================
struct TextPanel {
    void dtor_492f40();
};

// @ 0x492f40
void TextPanel::dtor_492f40()
    {
    char* self = (char*)this;
    if (*(char*)(self + 0x1a05c) != 0)
        ((CursorObj*)self)->reset();

    {
    tstring* p = (tstring*)(self + 0x1a290);
    for (int n = 5; n != 0; --n)
        {
        --p;
        p->~tstring();
        }
    }
    ((tstring*)(self + 0x1a210))->~tstring();

    if (*(void**)(self + 0x1a1bc) != 0)
        ::operator delete(*(void**)(self + 0x1a1bc));
    *(void**)(self + 0x1a1bc) = 0;
    *(void**)(self + 0x1a1c0) = 0;
    *(void**)(self + 0x1a1c4) = 0;

    ((tstring*)(self + 0x1a080))->~tstring();
    ((tstring*)(self + 0x1a06c))->~tstring();

    {
    tstring* p = (tstring*)(self + 0x13eec);
    for (int n = 0x32; n != 0; --n)
        {
        --p;
        p->~tstring();
        }
    }

    ((tstring*)(self + 0x13b50))->~tstring();
    ((tstring*)(self + 0x13ab8))->~tstring();
    ((tstring*)(self + 0x13aa8))->~tstring();
    ((tstring*)(self + 0x13a70))->~tstring();

    if (*(void**)(self + 0x13a64) != 0)
        ::operator delete(*(void**)(self + 0x13a64));
    *(void**)(self + 0x13a64) = 0;
    *(void**)(self + 0x13a68) = 0;
    *(void**)(self + 0x13a6c) = 0;

    ((Obj4317b0*)(self + 0x139f4))->reset();

    if (*(void**)(self + 0x139ec) != 0)
        ((RecD*)*(void**)(self + 0x139ec))->scalar_delete(3);

    ((Mgr490340*)(self + 0xf8))->clear();
    ((Obj4c6e30*)(self + 0xcc))->release();
    ((tstring*)(self + 0xb8))->~tstring();

    if (*(void**)(self + 0x38) != 0)
        ::operator delete(*(void**)(self + 0x38));
    *(void**)(self + 0x38) = 0;
    *(void**)(self + 0x3c) = 0;
    *(void**)(self + 0x40) = 0;

    ((tstring*)(self + 0x1c))->~tstring();
    ((tstring*)(self + 8))->~tstring();
    }

// =================================================================================================
// 0x492d80 / 0x493120: std::deque<int> map grow + push_back (the deque hangs off +4 of its owner).
// =================================================================================================
// @ 0x492d80
template void std::deque<int>::_Growmap(size_t);

// @ 0x493120
template void std::deque<int>::push_back(const int&);

// =================================================================================================
// Vtable stand-in: the engine objects below reach members through their vtable, so the slots must
// exist at the exact byte offsets the original uses ([vt+0x2c], [vt+0x6c], [vt+0xa4]).
// =================================================================================================
struct VObj {
    // Each slot takes up to two pointer arguments so every call shape in this slice compiles;
    // only the byte offset of the slot in the vtable matters for matching.
    virtual void v00(void* =0); virtual void v01(void* =0);
    virtual void v02(void* =0); virtual void v03(void* =0);
    virtual void v04(void* =0); virtual void v05(void* =0);
    virtual void v06(void* =0); virtual void v07(void* =0);
    virtual void v08(void* =0); virtual void v09(void* =0);
    virtual void v10(void* =0); virtual void v11(void* =0);
    virtual void v12(void* =0); virtual void v13(void* =0);
    virtual void v14(void* =0); virtual void v15(void* =0);
    virtual void v16(void* =0); virtual void v17(void* =0);
    virtual void v18(void* =0); virtual void v19(void* =0);
    virtual void v20(void* =0); virtual void v21(void* =0);
    virtual void v22(void* =0); virtual void v23(void* =0);
    virtual void v24(void* =0); virtual void v25(void* =0);
    virtual void v26(void* =0); virtual void v27(void* =0);
    virtual void v28(void* =0); virtual void v29(void* =0);
    virtual void v30(void* =0); virtual void v31(void* =0);
    virtual void v32(void* =0); virtual void v33(void* =0);
    virtual void v34(void* =0); virtual void v35(void* =0);
    virtual void v36(void* =0); virtual void v37(void* =0);
    virtual void v38(void* =0); virtual void v39(void* =0);
    virtual void v40(void* =0); virtual void v41(void* =0, void* =0);
};

extern char vtbl_5924a0[];                                // 0x5924a0
extern char vtbl_592570[];                                // 0x592570

// @ 0x4931a0
struct PanelA {
    void* ctor_4931a0();
};

void* PanelA::ctor_4931a0()
    {
    ((PanelBase*)this)->init();
    *(void**)this = (void*)vtbl_5924a0;
    *(int*)((char*)this + 0x17c) = 0;
    *(int*)((char*)this + 0x180) = 0;
    *(int*)((char*)this + 0x184) = 0;
    return this;
    }

// @ 0x4931d0
struct PanelB {
    void* ctor_4931d0();
};

void* PanelB::ctor_4931d0()
    {
    char* self = (char*)this;
    ((PanelBase*)this)->init();
    *(int*)(self + 0x178) = 0;
    *(char*)(self + 0x121) = 0;
    *(void**)self = (void*)vtbl_592570;
    *(int*)(self + 0x190) = 0;
    *(int*)(self + 0x194) = 0;
    *(int*)(self + 0x198) = 0;
    *(char*)(self + 0x180) = 0;
    *(int*)(self + 0x188) = 0;
    *(int*)(self + 0x164) = 0;
    *(int*)(self + 0x160) = 0;
    {
    FPair _p0 = { 0.2265f, 0.029f };
    *(FPair*)(self + 0x1b0) = _p0;
    FPair _p1 = { 0.018f, 0.018f };
    *(FPair*)(self + 0x1bc) = _p1;
    *(float*)(self + 0x1b8) = 0.0135f;
    FPair _p2 = { 0.02f, 0.01f };
    *(FPair*)(self + 0x1c4) = _p2;
    }
    return self;
    }

// @ 0x4932a0
struct PanelC : VObj {
    void set_mode();
};

void PanelC::set_mode()
    {
    char* self = (char*)this;
    if (*(char*)(self + 0x180) == 0)
        *(int*)(self + 0x178) = 1;
    else
        *(int*)(self + 0x178) = 0;
    *(char*)(self + 0x11f) = 0;
    if (*(int*)(self + 0x178) != 0)
        {
        *(char*)(self + 0x11e) = 1;
        *(char*)(self + 0x11d) = 0;
        }
    else
        {
        *(char*)(self + 0x11e) = 0;
        *(char*)(self + 0x11d) = 1;
        }
    ((Obj488720*)self)->update();
    }

// @ 0x493300
int panel_count_493300()
    {
    return 5;
    }

struct PanelD : VObj {
    void set_index(int _v);
};

// The 16-byte-element vector at +0x18c (leading dword, so _Myfirst is at +0x190).
struct E16 { int a, b, c, d; };
struct Vec16 {
    int   _pad0;                                          // +0x18c
    E16*  _Myfirst;                                       // +0x190
    E16*  _Mylast;                                        // +0x194
    E16*  _Myend;                                         // +0x198
    size_type size() const { return _Myfirst == 0 ? 0 : (size_type)(_Mylast - _Myfirst); }
};

// @ 0x493310
void PanelD::set_index(int _v)
    {
    char* self = (char*)this;
    Vec16* _v16 = (Vec16*)(self + 0x18c);
    int _lo = *(int*)(self + 0x160);
    *(int*)(self + 0x15c) = _v;
    if (_v < _lo)
        *(int*)(self + 0x15c) = _lo;
    else if (_v > *(int*)(self + 0x164))
        *(int*)(self + 0x15c) = *(int*)(self + 0x164);

    int _cur = *(int*)(self + 0x15c);
    if (_cur < _lo)
        *(int*)(self + 0x15c) = _lo;
    else if ((size_type)_cur > _v16->size())
        *(int*)(self + 0x15c) = _v16->size();

    int _tmp[2];
    v41(_tmp, (void*)((char*)_v16->_Myfirst + *(int*)(self + 0x15c) * 16));
    }

// @ 0x4933b0
struct PanelE : VObj {
    void  reset_tail();
    void* delete_4933d0(unsigned _flags);
};

void PanelE::reset_tail()
    {
    char* self = (char*)this;
    ((Tail*)(self + 0x18c))->dtor();
    *(int*)(self + 0x164) = 0;
    *(int*)(self + 0x15c) = 0;
    }

// @ 0x4933d0
void* PanelE::delete_4933d0(unsigned _flags)
    {
    char* self = (char*)this;
    ((Tail*)(self + 0x18c))->dtor();
    ((BufferBase*)self)->dtor();
    if (_flags & 1)
        ::operator delete(self);
    return self;
    }

// @ 0x493400
struct PanelF : VObj {
    void append_493400(const tstring* _name);
};

void PanelF::append_493400(const tstring* _name)
    {
    char* self = (char*)this;
    Vec16* _v16 = (Vec16*)(self + 0x18c);
    int _count;
    int _maxn = 0x800;
    int _buf[0x800];
    decode_utf8_487b00(&_count, _name, _buf, &_maxn);
    float _scale = (float)_count * 0.0077999993f;
    if (_scale > 0.21f)
        _scale = 0.21f / _scale * 0.02f;
    else
        _scale = 0.02f;
    if (_scale < *(float*)(self + 0x140))
        {
        float _pair[2];
        _pair[0] = _scale;
        _pair[1] = _scale;
        v27(_pair);
        }
    ((VecTstring*)(self + 0x18c))->push_back(*_name);
    ++*(int*)(self + 0x164);
    if (_v16->size() == 1)
        v11((void*)0);
    }

// @ 0x492c96
// No source of its own: this address is the compiler-generated `__catch` funclet of 0x492b30
// (it frees the temporary vector buffer and rethrows) plus the two non-reallocating insert
// branches that the compiler laid out after it.  It is fully covered by the 0x492b30 source above.
