// Slice s00492b30: std::vector<E8>::_Insert_n, a deque map-grow/push_back pair, and the
// tooltip / text-panel object functions around 0x4930xx-0x4934xx.
//
// The original keeps the STL helper bodies (??_Xlen/_Ucopy/_Ufill, fill, copy_backward) OUT OF
// LINE in other functions, so they are only declared here: the compiler then must emit calls with
// exactly the original's argument shapes instead of inlining them.
#pragma warning(disable:4530)
#include <new>
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
struct E8 { int a, b; };                                  // 8-byte element
struct FPair { float a, b; };                             // 8-byte float pair

void vec8_fill(E8* _First, E8* _Last, const E8& _Val);        // 0x48ad70
void vec8_copy_backward(E8* _First, E8* _Last, E8* _Dest);    // 0x48ada0

struct Vec8 {
    int   _pad0;                                          // +0
    E8*   _Myfirst;                                       // +4
    E8*   _Mylast;                                        // +8
    E8*   _Myend;                                         // +0xc

    void _Xlen() const;                                   // 0x43e2a0
    E8*  _Ucopy(E8* _First, E8* _Last, E8* _Ptr);         // 0x48c570
    E8*  _Ufill(E8* _Ptr, size_type _Count, const E8& _Val); // 0x48ce90

    size_type size() const     { return _Myfirst == 0 ? 0 : (size_type)(_Mylast - _Myfirst); }
    size_type capacity() const { return (_Myend - _Myfirst); }
    size_type max_size() const { return (size_type)(-1) / sizeof(E8); }

    void _Insert_n(E8* _Where, size_type _Count, const E8& _Val);
    void resize(size_type _Newsize, E8 _Val);
};

// @ 0x492b30
void Vec8::_Insert_n(E8* _Where, size_type _Count, const E8& _Val)
    {	// insert _Count * _Val at _Where
    E8 _Tmp = _Val;	// in case _Val is in sequence
    size_type _Capacity = capacity();

    if (_Count == 0)
        ;
    else if (max_size() - size() < _Count)
        _Xlen();	// result too long
    else if (_Capacity < size() + _Count)
        {	// not enough room, reallocate
        _Capacity = max_size() - _Capacity / 2 < _Capacity
            ? 0 : _Capacity + _Capacity / 2;	// try to grow by 50%
        if (_Capacity < size() + _Count)
            _Capacity = size() + _Count;
        E8* _Newvec = (E8*)::operator new(_Capacity * sizeof(E8));
        E8* _Ptr = _Newvec;

        try
            {
            _Ptr = _Ucopy(_Myfirst, _Where, _Newvec);	// copy prefix
            _Ptr = _Ufill(_Ptr, _Count, _Tmp);	// add new stuff
            _Ucopy(_Where, _Mylast, _Ptr);	// copy suffix
            }
        catch (...)
            {
            ::operator delete(_Newvec);
            throw;
            }

        _Count += size();
        if (_Myfirst != 0)
            {	// destroy and deallocate old array
            ::operator delete(_Myfirst);
            }
        _Myend = _Newvec + _Capacity;
        _Mylast = _Newvec + _Count;
        _Myfirst = _Newvec;
        }
    else if ((size_type)(_Mylast - _Where) < _Count)
        {	// new stuff spills off end
        _Ucopy(_Where, _Mylast, _Where + _Count);	// copy suffix

        try
            {
            _Ufill(_Mylast, _Count - (_Mylast - _Where), _Tmp);	// insert new stuff off end
            }
        catch (...)
            {
            throw;
            }

        _Mylast += _Count;
        vec8_fill(_Where, _Mylast - _Count, _Tmp);	// insert up to old end
        }
    else
        {	// new stuff can all be assigned
        E8* _Oldend = _Mylast;
        _Mylast = _Ucopy(_Oldend - _Count, _Oldend, _Mylast);	// copy suffix
        vec8_copy_backward(_Where, _Oldend - _Count, _Oldend);	// copy hole
        vec8_fill(_Where, _Where + _Count, _Tmp);	// insert into hole
        }
    }

// @ 0x493090
void Vec8::resize(size_type _Newsize, E8 _Val)
    {	// grow to _Newsize, else drop the tail
    if (size() < _Newsize)
        _Insert_n(_Mylast, _Newsize - size(), _Val);
    else if (_Newsize < size())
        {
        E8* _First = _Myfirst + _Newsize;
        E8* _Last = _Mylast;
        if (_First != _Last)
            {
            size_type _n = _Mylast - _Last;
            _Mylast = (E8*)((char*)memmove(_First, _Last, _n * sizeof(E8)) + _n * sizeof(E8));
            }
        }
    }

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
struct DequeI {
    int       _pad0;                                      // +0
    int**     _Map;                                       // +4
    size_type _Mapsize;                                   // +8
    size_type _Myoff;                                     // +0xc
    size_type _Mysize;                                    // +0x10

    void _Xlen() const;                                   // 0x43e0e0
    size_type max_size() const { return (size_type)(-1) / sizeof(int) / 4; }

    void _Growmap(size_type _Count);
    void push_back(const int& _Val);
};

// @ 0x492d80
void DequeI::_Growmap(size_type _Count)
    {	// grow map by _Count pointers (_DEQUESIZ == 4)
    if (max_size() - _Mapsize < _Count)
        _Xlen();	// result too long

    size_type _Inc = _Mapsize / 2;	// try to grow by 50%
    if (_Inc < 8)
        _Inc = 8;
    if (_Count < _Inc && _Mapsize <= max_size() - _Inc)
        _Count = _Inc;

    size_type _Myboff = _Myoff / 4;
    int** _Newmap = (int**)::operator new((_Mapsize + _Count) * sizeof(int*));
    int** _Myptr = _Newmap + _Myboff;
    {
    size_type _n = _Mapsize - _Myboff;
    _Myptr = (int**)((char*)memmove(_Myptr, _Map + _Myboff, _n * sizeof(int*))
                     + _n * sizeof(int*));
    }
    if (_Myboff <= _Count)
        {	// increment greater than offset of initial block
        size_type _n = _Myboff;
        _Myptr = (int**)((char*)memmove(_Myptr, _Map, _n * sizeof(int*))
                         + _n * sizeof(int*));
        for (size_type i = 0; i < _Count - _Myboff; ++i)
            _Myptr[i] = 0;
        for (size_type i = 0; i < _Myboff; ++i)
            _Newmap[i] = 0;
        _Myptr += _Count - _Myboff;
        }
    else
        {	// increment not greater than offset of initial block
        size_type _n = _Count;
        _Myptr = (int**)((char*)memmove(_Myptr, _Map, _n * sizeof(int*))
                         + _n * sizeof(int*));
        size_type _n2 = _Myboff - _Count;
        _Myptr = (int**)((char*)memmove(_Newmap, _Map + _Count, _n2 * sizeof(int*))
                         + _n2 * sizeof(int*));
        for (size_type i = 0; i < _Count; ++i)
            _Myptr[i] = 0;
        _Myptr += _Count;
        }

    if (_Map != 0)
        ::operator delete(_Map);

    _Map = _Newmap;
    _Mapsize += _Count;
    }

// @ 0x493120
void DequeI::push_back(const int& _Val)
    {	// insert element at end
    if ((_Myoff + _Mysize) % 4 == 0 && _Mapsize <= (_Mysize + 4) / 4)
        _Growmap(1);
    size_type _Newoff = _Myoff + _Mysize;
    size_type _Block = _Newoff / 4;
    if (_Mapsize <= _Block)
        _Block -= _Mapsize;
    if (_Map[_Block] == 0)
        _Map[_Block] = (int*)::operator new(4 * sizeof(int));
    int* _Ptr = _Map[_Block] + _Newoff % 4;
    if (_Ptr != 0)
        *_Ptr = _Val;
    ++_Mysize;
    }

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
