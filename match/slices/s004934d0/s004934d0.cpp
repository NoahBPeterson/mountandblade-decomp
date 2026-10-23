// Slice s004934d0: the big game-manager object around 0x4934d0-0x493e90 -- its constructors, the
// pool/hash helpers, and the two 8-byte-container grow helpers (reserve / set).
//
// Conventions used in this repo: the engine containers carry a leading dword, so their
// _Myfirst/_Mylast/_Myend live at +4/+8/+0xc.  Every out-of-line callee is *declared only* (its
// body lives in another slice) so cl must emit a real call with `this` in ECX and the arguments on
// the stack, exactly as the original does.
#pragma warning(disable:4530)
#include <new>
#include <string.h>
#include "tstring.h"

typedef unsigned size_type;

// -------------------------------------------------------------------------------------------------
// Out-of-line callees (bodies live in other slices).  Declared only.
// -------------------------------------------------------------------------------------------------
void* __cdecl alloc_arr_535482(unsigned _Size);                                   // 0x535482
void  __stdcall vec_ctor_iter_401800(void* _First, unsigned _Size,
                                     unsigned _Count, void* _Fn);                 // 0x401800
void  elem_ctor_48c210(void* _This);                                              // 0x48c210
struct ListNode {
    void* _Buynode();                                 // 0x46c0c0 (thiscall)
};

extern long long g_5e2948[];                                                      // 0x5e2948 (array of int64)
extern char vtbl_592644[];                                                        // 0x592644

// -------------------------------------------------------------------------------------------------
// 4-byte-element container (its _Myfirst is at +4).  Used by push_back/insert sites in this slice.
// -------------------------------------------------------------------------------------------------
struct E8 { int a, b; };

struct Vec4 {
    int   _pad0;                                          // +0
    int*  _Myfirst;                                       // +4
    int*  _Mylast;                                        // +8
    int*  _Myend;                                         // +0xc
    size_type size() const     { return (_Mylast - _Myfirst); }
    size_type capacity() const { return (_Myend - _Myfirst); }

    __declspec(noinline) void push_back(const int& _Val);                      // 0x4936b0
    void insert_nA(int* _Where, size_type _Count, const int& _Val);   // 0x4927d0
    void insert_nB(int* _Where, size_type _Count, const int& _Val);   // 0x43e7b0
    void insert_nC(int* _Where, size_type _Count, const int& _Val);   // 0x41a2c0
};

// @ 0x4936b0
void Vec4::push_back(const int& _Val)
    {
    size_type _n = _Myfirst != 0 ? (size_type)(_Mylast - _Myfirst) : 0;
    if (_Myfirst != 0 && _n < (size_type)(_Myend - _Myfirst))
        {
        *_Mylast = _Val;
        ++_Mylast;
        }
    else
        insert_nA(_Mylast, 1, _Val);
    }

// -------------------------------------------------------------------------------------------------
// 8-byte-element container (its _Myfirst is at +4); the grow helpers used by 0x493c40/0x493d70.
// -------------------------------------------------------------------------------------------------
struct Vec8 {
    int   _pad0;                                          // +0
    E8*   _Myfirst;                                       // +4
    E8*   _Mylast;                                        // +8
    E8*   _Myend;                                         // +0xc
    size_type size() const     { return _Myfirst == 0 ? 0 : (size_type)(_Mylast - _Myfirst); }
    size_type capacity() const { return (_Myend - _Myfirst); }

    void resize(size_type _Newsize, E8 _Val);             // 0x493090 (in slice s00492b30)
    void reserve(int* _pNewsize);                         // 0x493c40
    void set_next(int* _pIdx, const E8& _Val);            // 0x493d70
};

// Opaque view of the same 8-byte container: callers go through this so cl cannot see the body of
// reserve (the original lives in a different translation unit and the callers reload _n).
struct Vec8Op {
    int   _pad0;
    long long* _Myfirst;
    E8*   _Mylast;
    E8*   _Myend;
    void reserve(int* _pNewsize);                         // 0x493c40
};

// @ 0x493c40
void Vec8::reserve(int* _pNewsize)
    {
    int _n = *_pNewsize;
    if (size() < _n)
        {
        if (_n < 0x10)          _n = 0x10;
        else if (_n < 0x20)     _n = 0x20;
        else if (_n < 0x40)     _n = 0x40;
        else if (_n < 0x80)     _n = 0x80;
        else if (_n < 0x100)    _n = 0x100;
        else if (_n < 0x200)    _n = 0x200;
        else if (_n < 0x400)    _n = 0x400;
        else if (_n < 0x800)    _n = 0x800;
        else if (_n < 0x1000)   _n = 0x1000;
        else if (_n < 0x2000)   _n = 0x2000;
        else if (_n < 0x4000)   _n = 0x4000;
        else if (_n < 0x8000)   _n = 0x8000;
        else if (_n < 0x10000)  _n = 0x10000;

        int _old = (int)size();
        E8 _z = { 0, 0 };
        resize((size_type)_n, _z);
        for (int i = _old; i < _n; ++i)
            {
            _Myfirst[i].a = 0;
            _Myfirst[i].b = 0;
            }
        }
    }

// @ 0x493d70
void Vec8::set_next(int* _pIdx, const E8& _Val)
    {
    int _n = *_pIdx + 1;
    ((Vec8Op*)this)->reserve(&_n);
    ((Vec8Op*)this)->_Myfirst[*_pIdx] = *(long long*)&_Val;
    }

// -------------------------------------------------------------------------------------------------
// vtable stand-in: slot [vt+0x5c] is reached through the object vtable.
// -------------------------------------------------------------------------------------------------
struct VObj {
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
};

// -------------------------------------------------------------------------------------------------
// The manager object.  Only the mangled names matter to the matcher; bodies are complete.
// -------------------------------------------------------------------------------------------------
struct BigMgr : VObj {
    void  init_48f180();                                  // 0x48f180 (declared only)
    void  thunk_4936a0();                                 // 0x4936a0
    void* ctor_4934d0();                                  // 0x4934d0
    void  pool_alloc_493700();                            // 0x493700
    void* pool_alloc2_493830();                           // 0x493830
    void* hash_insert_493960();                           // 0x493960
    void  add_panel_493a30(void* _obj);                   // 0x493a30
    void* find_or_create_493df0(int* _key);               // 0x493df0
    void* create_489b20(int* _key);                       // 0x489b20 (declared only)
};

// @ 0x4936a0
void BigMgr::thunk_4936a0()
    {
    init_48f180();
    }

// @ 0x4934d0
void* BigMgr::ctor_4934d0()
    {
    char* self = (char*)this;

    *(void**)(self) = (void*)vtbl_592644;
    new ((tstring*)(self + 0xc)) tstring();
    new ((tstring*)(self + 0x20)) tstring();
    new ((tstring*)(self + 0x54)) tstring();

    {
    char* p = self + 0xa8;
    for (int i = 0; i < 0x40; ++i)
        {
        *(int*)(p + 0x00) = 0;
        *(int*)(p + 0x04) = -1;
        *(int*)(p + 0x08) = 0;
        *(int*)(p + 0x0c) = 0;
        *(int*)(p + 0x10) = 0;
        *(int*)(p + 0x14) = 0;
        *(int*)(p + 0x18) = 0;
        p += 0x20;
        }
    }

    *(int*)(self + 0x8bc) = 0;
    *(int*)(self + 0x8c0) = 0;
    *(int*)(self + 0x8c4) = 0;
    *(int*)(self + 0x8d0) = 1;
    *(long long*)(self + 0x8d8) = g_5e2948[1];
    *(int*)(self + 0x8f8) = 1;
    *(long long*)(self + 0x900) = g_5e2948[1];
    *(int*)(self + 0x908) = 0;
    *(long long*)(self + 0x910) = g_5e2948[0];
    *(int*)(self + 0x938) = 1;
    *(long long*)(self + 0x940) = g_5e2948[1];
    *(int*)(self + 0x948) = 1;
    *(long long*)(self + 0x950) = g_5e2948[1];
    *(int*)(self + 0x59b0) = 1;
    *(long long*)(self + 0x59b8) = g_5e2948[1];
    *(int*)(self + 0x59dc) = 0;
    *(int*)(self + 0x59e0) = 0;
    *(int*)(self + 0x59e4) = 0;
    *(int*)(self + 0x59f8) = 0;
    *(int*)(self + 0x59fc) = 0;
    *(int*)(self + 0x5a00) = 0;

    for (int i = 0; i < 8; ++i)
        new ((tstring*)(self + 0x5a28 + i * 0x10)) tstring();

    *(int*)(self + 0x5aa8) = -1;
    *(int*)(self + 0x5ab0) = 1;
    *(long long*)(self + 0x5ab8) = g_5e2948[1];

    init_48f180();

    *(long long*)(self + 0x59b8) = g_5e2948[*(int*)(self + 0x59b0)] - 100000;
    *(char*)(self + 0x5a04) = 0;
    return self;
    }

// @ 0x493700
void BigMgr::pool_alloc_493700()
    {
    char* self = (char*)this;
    Vec4* _v1 = (Vec4*)(self + 4);                        // int indices (this+8)
    Vec4* _v2 = (Vec4*)(self + 0x14);                     // block pointers (this+0x18)

    if (_v1->_Myfirst == 0 || _v1->size() == 0)
        {
        int* _arr = (int*)alloc_arr_535482(0x984);
        char* _p;
        if (_arr == 0)
            _p = 0;
        else
            {
            _arr[0] = 0x10;
            _p = (char*)(_arr + 1);
            vec_ctor_iter_401800(_p, 0x98, 0x10, (void*)elem_ctor_48c210);
            }
        int _tmp = (int)_p;
        _v2->push_back(_tmp);

        for (int i = 0; i < 0x10; ++i)
            {
            char* blk = _p + i * 0x98;
            int _val = *(int*)(self) + i;
            *(int*)(blk + 0x04) = 0;
            *(int*)(blk + 0x08) = _val;
            size_type _n = _v1->_Myfirst != 0 ? (size_type)(_v1->_Mylast - _v1->_Myfirst) : 0;
            if (_v1->_Myfirst != 0 && _n < (size_type)(_v1->_Myend - _v1->_Myfirst))
                {
                *_v1->_Mylast = _val;
                ++_v1->_Mylast;
                }
            else
                _v1->insert_nB(_v1->_Mylast, 1, _val);
            }
        *(int*)(self) += 0x10;
        }

    unsigned _idx = (unsigned)_v1->_Mylast[-1];     // unsigned: the original uses shr, not sar, on the popped index
    if (_v1->_Myfirst != 0 && _v1->size() != 0)
        --_v1->_Mylast;

    void** _blocks = *(void***)(self + 0x18);
    char* blk = (char*)_blocks[_idx >> 4] + (_idx & 0xf) * 0x98;
    *(int*)(blk + 4) = 1;
    *(int*)(blk + 0xc) = *(int*)(self + 0x24);
    ++*(int*)(self + 0x24);
    }

// @ 0x493830
void* BigMgr::pool_alloc2_493830()
    {
    char* self = (char*)this;
    Vec4* _va = (Vec4*)(self + 0x138bc);                  // int indices (this+0x138c0)
    Vec4* _vb = (Vec4*)(self + 0x138cc);                  // block pointers (this+0x138d0)

    int* _arr = (int*)alloc_arr_535482(0x5ac84);
    char* _p;
    if (_arr == 0)
        _p = 0;
    else
        {
        _arr[0] = 0x10;
        _p = (char*)(_arr + 1);
        char* q = _p;
        for (int i = 0; i < 0x10; ++i, q += 0x5ac8)
            ((BigMgr*)q)->ctor_4934d0();
        }

    {
    int _val = (int)_p;
    size_type _nb = _vb->_Myfirst != 0 ? (size_type)(_vb->_Mylast - _vb->_Myfirst) : 0;
    if (_vb->_Myfirst != 0 && _nb < (size_type)(_vb->_Myend - _vb->_Myfirst))
        {
        *_vb->_Mylast = _val;
        ++_vb->_Mylast;
        }
    else
        _vb->insert_nA(_vb->_Mylast, 1, _val);
    }

    for (int i = 0; i < 0x10; ++i)
        {
        *(int*)(_p + i * 0x5ac8 + 4) = 0;
        int _val = *(int*)(self + 0x138b8) + i;
        size_type _na = _va->_Myfirst != 0 ? (size_type)(_va->_Mylast - _va->_Myfirst) : 0;
        if (_va->_Myfirst != 0 && _na < (size_type)(_va->_Myend - _va->_Myfirst))
            {
            *_va->_Mylast = _val;
            ++_va->_Mylast;
            }
        else
            _va->insert_nB(_va->_Mylast, 1, _val);
        }
    *(int*)(self + 0x138b8) += 0x10;
    return self;
    }

// @ 0x493960
void* BigMgr::hash_insert_493960()
    {
    char* self = (char*)this;
    Vec4* _va = (Vec4*)(self + 0x138bc);
    Vec4* _vb = (Vec4*)(self + 0x138cc);

    if (_va->size() == 0)
        pool_alloc2_493830();

    int _idx = _va->_Mylast[-1];
    if (_va->_Myfirst != 0 && _va->size() != 0)
        --_va->_Mylast;

    void** _blocks = (void**)_vb->_Myfirst;
    char* blk = (char*)_blocks[_idx >> 4] + (_idx & 0xf) * 0x5ac8;
    *(int*)(blk + 4) = 1;
    *(int*)(blk + 8) = *(int*)(self + 0x138dc);

    int _h = *(int*)(self + 0x138dc);
    int _slot = _h % 0x2717;
    while (*(int*)(self + _slot * 8) != -1)
        _slot = (_slot + 0x4f) % 0x2717;
    *(int*)(self + _slot * 8) = _h;
    *(int*)(self + _slot * 8 + 4) = _idx;
    ++*(int*)(self + 0x138dc);
    return *(void**)(blk + 8);
    }

// @ 0x493a30
void BigMgr::add_panel_493a30(void* _obj)
    {
    char* self = (char*)this;
    Vec4* _v = (Vec4*)(self + 0x178);

    size_type _n = _v->_Myfirst != 0 ? (size_type)(_v->_Mylast - _v->_Myfirst) : 0;
    if (_v->_Myfirst != 0 && _n < (size_type)(_v->_Myend - _v->_Myfirst))
        {
        *_v->_Mylast = (int)_obj;
        ++_v->_Mylast;
        }
    else
        _v->insert_nA(_v->_Mylast, 1, *(const int*)&_obj);

    *(void**)((char*)_obj + 0x154) = this;
    ((VObj*)this)->v23(_obj);
    }

// @ 0x493df0
void* BigMgr::find_or_create_493df0(int* _key)
    {
    char* self = (char*)this;
    Vec4* _v = (Vec4*)(self + 0x54);

    void* _found = 0;
    for (size_type i = 0; i < _v->size(); ++i)
        {
        if (*(int*)((char*)(void*)_v->_Myfirst[i] + 0x60) == *_key)
            {
            _found = (void*)_v->_Myfirst[i];
            break;
            }
        }
    if (_found != 0)
        return _found;

    void* _new = create_489b20(_key);
    int _val = (int)_new;
    size_type _n = _v->_Myfirst != 0 ? (size_type)(_v->_Mylast - _v->_Myfirst) : 0;
    if (_v->_Myfirst != 0 && _n < (size_type)(_v->_Myend - _v->_Myfirst))
        {
        *_v->_Mylast = _val;
        ++_v->_Mylast;
        }
    else
        _v->insert_nC(_v->_Mylast, 1, _val);
    return _new;
    }

// -------------------------------------------------------------------------------------------------
// 0x493db0: writes an int64 element into the 8-byte container that lives at this+0x40.
// -------------------------------------------------------------------------------------------------
struct Outer40 {
    char _pad[0x40];
    Vec8 _v;                                              // _Myfirst at +0x44
    void set40(int _idx, int _val);
};

// @ 0x493db0
void Outer40::set40(int _idx, int _val)
    {
    Vec8Op* _p = (Vec8Op*)((char*)this + 0x40);
    int _n = _idx + 1;
    _p->reserve(&_n);
    _p->_Myfirst[_idx] = _val;
    }

// -------------------------------------------------------------------------------------------------
// 0x493ab0: constructor of the view/params object (returns this).
// -------------------------------------------------------------------------------------------------
struct ViewParams {
    char _pad[0x1c];
    void ctor_46b940();                                   // 0x46b940 (declared only)
};

struct ViewObj {
    void set_478830(float* _a, float* _b, int _c, int _d);  // 0x478830 (thiscall, declared only)
    void* ctor_493ab0();
};

// @ 0x493ab0
void* ViewObj::ctor_493ab0()
    {
    char* self = (char*)this;

    {
    void* _n0 = ((ListNode*)(self + 0x2c))->_Buynode();
    *(void**)(self + 0x30) = _n0;
    *(int*)(self + 0x34) = 0;
    *(int*)(self + 0x38) = 0x400;
    *(int*)(self + 0x3c) = 0;
    }
    {
    void* _n1 = ((ListNode*)(self + 0x40))->_Buynode();
    *(void**)(self + 0x44) = _n1;
    *(int*)(self + 0x48) = 0;
    *(int*)(self + 0x4c) = 0x400;
    *(int*)(self + 0x50) = 0;
    }

    *(int*)(self + 0x58) = 0;
    *(int*)(self + 0x5c) = 0;
    *(int*)(self + 0x60) = 0;
    *(int*)(self + 0x80) = 0;
    *(int*)(self + 0x84) = 0;
    *(int*)(self + 0x88) = 0;
    *(int*)(self + 0x90) = 0;
    *(int*)(self + 0x94) = 0;
    *(int*)(self + 0x98) = 0;
    *(int*)(self + 0xa4) = 0;
    *(int*)(self + 0xa8) = 0;
    *(int*)(self + 0xac) = 0;
    *(int*)(self + 0xbc) = 0;
    *(int*)(self + 0xc0) = 0;
    *(int*)(self + 0xc4) = 0;
    *(int*)(self + 0xcc) = 0;
    *(int*)(self + 0xd0) = 0;
    *(int*)(self + 0xd4) = 0;

    ((ViewParams*)(self + 0xd8))->ctor_46b940();

    *(int*)(self + 0x330) = 0;
    *(int*)(self + 0x334) = 0;
    *(int*)(self + 0x338) = 0;
    *(int*)(self + 0x340) = 0;
    *(int*)(self + 0x9c) = 1;

    float _g[4];
    {
    float _x[2] = { 1000.0f, 1000.0f };
    float _y[2] = { -1000.0f, -1000.0f };
    set_478830(_y, _x, 0, 0);
    }

    *(float*)(self + 0x30c) = 1.0f;
    *(float*)(self + 0x310) = 0.0f;
    *(float*)(self + 0x314) = 0.0f;
    *(float*)(self + 0x318) = _g[3];
    *(float*)(self + 0x31c) = 0.0f;
    *(float*)(self + 0x320) = 0.0f;
    *(float*)(self + 0x324) = 0.0f;
    *(float*)(self + 0x328) = _g[3];

    *(int*)(self + 0xb0) = 0;
    *(int*)(self + 0xb4) = 0;
    *(int*)(self + 0x64) = 0;
    *(float*)(self + 0x68) = 0.0f;
    *(float*)(self + 0x6c) = 0.0f;
    *(float*)(self + 0x70) = 0.0f;
    *(float*)(self + 0x74) = _g[3];
    *(int*)(self + 0x78) = 0;

    return self;
    }
