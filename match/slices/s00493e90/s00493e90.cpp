// Slice s00493e90: the quest/mission manager object around 0x493e90-0x494790 -- the entity pool
// purge/compact pair, two line/particle emitters, the 3D-listener refresh, and the four quest-status
// HUD helpers.
//
// Repo conventions: engine containers carry a leading dword, so their _Myfirst/_Mylast/_Myend live at
// +4/+8/+0xc.  Out-of-line callees are *declared only* so cl must emit a real call with `this` in ECX.
#pragma warning(disable:4530)
#include <new>
#include <string.h>
#include "tstring.h"

typedef unsigned size_type;

// -------------------------------------------------------------------------------------------------
// Out-of-line callees (bodies live in other slices).  Declared only.
// -------------------------------------------------------------------------------------------------
struct Mgr;
struct Mgr { void release(void* _p); };                    // 0x4175f0
void* alloc_53545e(unsigned _Size);                         // 0x53545e  operator new(unsigned)
void  ctor_48ae20(void* _this);                             // 0x48ae20
struct NodeAlloc {
    void* node_478f30();                                    // 0x478f30 (thiscall)
};
void* ident3_40cd60(void* _out);                             // 0x40cd60
void  fsound_stop_5342a6(int _h);                            // 0x5342a6  _FSOUND_StopSound@4
void  fsound_listener_5342dc(void* _pos, int, int, int, int, int, int, int); // 0x5342dc
void  mb_snprintf_415100(char* _buf, unsigned _n, const char* _fmt, ...); // 0x415100
struct GuiMgr {
    void set_o568(int _a, int _b);                           // 0x42cdb0 (thiscall)
    void text_set(tstring* _s, unsigned _color);             // 0x4334b0 (thiscall)
};
extern GuiMgr g_gui;                                        // 0x5d7080
struct StrTable {
    char* lookup(tstring* _s);                               // 0x43a1e0 (thiscall)
};
extern StrTable g_strtab;                                   // 0x5e2a18
struct DequeI {
    void push_back_493120(const int& _v);                    // 0x493120 (thiscall)
};
// FUN_00442650 is callee-cleanup; the call sites push in source order with a __thiscall receiver,
// the shape cl emits for a thiscall-to-__cdecl thunk.
// The draw receiver is a __thiscall object: `this` goes in ecx and is NOT a stack parameter, so
// the 5 stack dwords map to the 5 arguments below and the callee's `ret 0x14` cleans them.
struct DTxt {
    void draw(int _a, int _b, void* _c, float _d, int _e);                // 0x442650
};
extern DTxt g_dtxt;                                                    // 0x5aa128

extern char vtbl_591d58[];                                  // 0x591d58
extern int  g_5a9fd8;                                       // 0x5a9fd8
extern int  g_a17f28;                                       // 0xa17f28
extern char g_5d7fe0;                                       // 0x5d75e0
extern int  g_5aa184;                                       // 0x5aa184
extern int  g_5aa188;                                       // 0x5aa188
extern int  g_5aa190;                                       // 0x5aa190
extern Mgr g_5d6fa0;                                        // 0x5d6fa0

extern "C" const char s_ui_quest_completed_value[];          // 0x592650
extern "C" const char s_ui_quest_succeeded_value[];          // 0x59266c
extern "C" const char s_ui_quest_failed_value[];             // 0x592688

// -------------------------------------------------------------------------------------------------
// 4-byte-element container (its _Myfirst is at +4).
// -------------------------------------------------------------------------------------------------
struct Vec4 {
    int   _pad0;                                          // +0
    int*  _Myfirst;                                       // +4
    int*  _Mylast;                                        // +8
    int*  _Myend;                                         // +0xc
    size_type size()     { return _Myfirst != 0 ? (size_type)(_Mylast - _Myfirst) : 0; }
    size_type capacity() { return _Myfirst != 0 ? (size_type)(_Myend - _Myfirst) : 0; }
    void insert_nA(int* _Where, size_type _Count, const int& _Val);   // 0x4927d0
    void resize(size_type _Newsize, int _Val);                        // 0x43f310
    void kill_41ad20(int _n, int _pad);                               // 0x41ad20
    void push_back(const int& _Val);
};

// The functor/flag dword used by every vtable dispatch below is a plain void* payload argument.
struct VObjN {
    virtual void n0(); virtual void n1();
    virtual void n2(); virtual void n3(); virtual void n4(); virtual void n5();
    virtual void n6(); virtual void n7(); virtual char n8();
    virtual void n9(); virtual void n10(); virtual void n11();
    virtual void n12();
};
typedef void (__stdcall *PanelFn)(void* _this, unsigned char _v);
struct VObj {
    virtual void v00(void* =0); virtual void v01(void* =0);
    virtual void v2(void* =0); virtual void v3(void* =0);
    virtual void v4(void* =0); virtual void v5(void* =0);
    virtual void v6(void* =0); virtual void v7(void* =0);
    virtual char v8(void* =0); virtual void v9(void* =0);
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
    virtual void v40(void* =0); virtual void v41(void* =0);
    virtual void v42(void* =0); virtual void v43(void* =0);
    virtual void v44(void* =0); virtual void v45(void* =0);
    virtual void v46(void* =0); virtual void v47(void* =0);
    virtual void v48(void* =0); virtual void v49(void* =0);
    virtual void v50(void* =0); virtual void v51(void* =0);
};
typedef char vobj_pad_check[(sizeof(VObj) == 4) ? 1 : -1];

// -------------------------------------------------------------------------------------------------
// The manager.  Only the mangled names matter to the matcher; every body is complete.
// -------------------------------------------------------------------------------------------------
struct QMgr : public VObj {
    void  purge_493e90();                                  // 0x493e90
    void  purge_all_493f30();                              // 0x493f30
    void  emit_line_493f80(QMgr* _o, void* _p);            // 0x493f80
    void  emit_line2_494020(QMgr* _o, void* _p);           // 0x494020
    int   add_panel_494110(unsigned _flags);               // 0x494110
    void  switch_panel_494260(int _idx);                   // 0x494260
    void  set_params_4942f0(int _a, unsigned _b);          // 0x4942f0
    void  refresh_listener_494310();                       // 0x494310
    void  quest_done_494410(int _idx);                     // 0x494410
    void  quest_succeeded_494560(int _idx);                // 0x494560
    void  quest_failed_494680(int _idx);                   // 0x494680
};

// @ 0x493e90
void QMgr::purge_493e90()
    {
    char* self = (char*)this;
    Vec4* _v = (Vec4*)(self + 0x54);
    int  _n  = (int)_v->size();
    int _last = _n - 1;
    if (_last < 0)
        {
        _v->kill_41ad20(_last + 1, 0);
        return;
        }
    int _i = 0;
    do
        {
        int* _o = (int*)_v->_Myfirst[_i];
        if (*(int*)((char*)_o + 0x64) <= 0)
            {
            *((int*)((char*)_o + 0x5c)) = -1;
            *((int*)((char*)_o + 0x60)) = -1;
            g_5d6fa0.release(_o);
            _v->_Myfirst[_i] = _v->_Myfirst[_last];
            --_last;
            }
        else
            ++_i;
        }
    while (_i <= _last);
    _v->kill_41ad20(_last + 1, 0);
    }

// The line-emitter vector at +0x14 has no leading pad: _Myfirst/_Mylast/_Myend at +0x14/+0x18/+0x1c.
struct VecBare {
    int*  _Myfirst;
    int*  _Mylast;
    int*  _Myend;
    size_type size() { return _Myfirst != 0 ? (size_type)(_Mylast - _Myfirst) : 0; }
};

// The 0x68-byte-element pool the purge walks: same 4-byte-prefixed layout, 0x68 stride.
struct Vec68 {
    int   _pad0;                                          // +0
    char* _Myfirst;                                       // +4
    char* _Mylast;                                        // +8
    char* _Myend;                                         // +0xc
    size_type size() { return _Myfirst != 0 ? (size_type)((_Mylast - _Myfirst) / 0x68) : 0; }
};

// @ 0x493f30
void QMgr::purge_all_493f30()
    {
    Vec68* _v = static_cast<Vec68*>(static_cast<void*>(this));
    for (size_type _i = 0; _i < _v->size(); ++_i)
        ((QMgr*)(_v->_Myfirst + _i * 0x68))->purge_493e90();
    }

// @ 0x493f80
void QMgr::emit_line_493f80(QMgr* _o, void* _p)
    {
    char* self = (char*)this;
    char* other = (char*)_o;
    VecBare* _v = (VecBare*)(self + 0x14);
    int* _n = (int*)((NodeAlloc*)_v->_Myfirst[_v->size() - 1])->node_478f30();
    int* _d = (int*)_p;
    memcpy(_n + 1, _d, 0x10);
    _n[0] = 0;
    memcpy(_n + 9, other + 0x30, 0x10);
    memcpy(_n + 5, _n + 1, 0x10);
    float* _f = (float*)_n;
    _f[0x15] = (_f[3] + _f[2] + _f[1]) * 10.0f;
    }

// @ 0x494020
void QMgr::emit_line2_494020(QMgr* _o, void* _p)
    {
    char* self = (char*)this;
    char* other = (char*)_o;
    int _g[4];
    VecBare* _v = (VecBare*)(self + 0x14);
    int* _n = (int*)((NodeAlloc*)_v->_Myfirst[_v->size() - 1])->node_478f30();
    int* _d = (int*)_p;
    memcpy(_n + 1, _d, 0x10);
    _n[0] = 2;
    memcpy(_n + 0x11, other + 0x10, 0x10);
    float* _f = (float*)_n;
    _f[9]  = *(float*)(other + 0x30) - _f[0x11] * 1000.0f;
    _f[10] = *(float*)(other + 0x34) - _f[0x12] * 1000.0f;
    _f[0xb] = *(float*)(other + 0x38) - _f[0x13] * 1000.0f;
    _n[0xc] = _g[3];
    memcpy(_n + 5, _n + 1, 0x10);
    }

// @ 0x494110
int QMgr::add_panel_494110(unsigned _flags)
    {
    char* self = (char*)this;
    QMgr* _o = (QMgr*)alloc_53545e(0x188);
    if (_o == 0)
        _o = 0;
    else
        {
        ctor_48ae20(_o);
        *(int*)((char*)_o + 0x178) = 0;
        *((char*)_o + 0x121) = 0;
        *(void**)_o = (void*)vtbl_591d58;
        *((char*)_o + 0x180) = 0;
        *(int*)((char*)_o + 0x184) = 0;
        }
    _o->v22(0);
    unsigned _f = (_flags & 0x181c) | 0x200;
    _o->v29(&_f);
    _o->v39(0);
    float _ld[2] = { 1.0f, 0.75f };
    _o->v25(_ld);
    _o->v41(0);
    (*(VObj**)(self + 0x2c))->v50(_o);

    int _val = (int)_o;
    size_type _n = _o != 0 ? 0 : 0;
    (void)_n;
    Vec4* _v = (Vec4*)self;
    {
    size_type _sz = _v->_Myfirst != 0 ? (size_type)(_v->_Mylast - _v->_Myfirst) : 0;
    if (_v->_Myfirst != 0 && _sz < (size_type)(_v->_Myend - _v->_Myfirst))
        {
        *_v->_Mylast = _val;
        ++_v->_Mylast;
        }
    else
        _v->insert_nA(_v->_Mylast, 1, _val);
    }
    return (int)_v->size() - 1;
    }

// @ 0x494260
void QMgr::switch_panel_494260(int _idx)
    {
    char* self = (char*)this;
    g_gui.set_o568(0, 1);
    ((DequeI*)(self + 0x78))->push_back_493120(_idx);
    ((VObjN*)(void*)*(int*)(self + _idx * 4))->n2();

    unsigned _u = *(unsigned*)(self + 0x88) + *(unsigned*)(self + 0x84) - 1;
    unsigned _q = _u >> 2;
    unsigned _r = _u - (_q << 2);
    if (*(unsigned*)(self + 0x80) <= _q)
        _q -= *(unsigned*)(self + 0x80);
    int* _row = *(int**)(*(int*)(self + 0x7c) + _q * 4);
    int _elem = *(int*)((char*)_row + _r * 4);
    int _bl = (unsigned char)((VObjN*)(void*)*(int*)(self + _elem * 4))->n8();
    if (g_a17f28 != 0)
        {
        PanelFn _fn = (PanelFn)(*(void***)g_a17f28)[12];
        _fn((void*)g_a17f28, (unsigned char)_bl);
        }
    g_5d7fe0 = (char)_bl;
    }

// @ 0x4942f0
void QMgr::set_params_4942f0(int _a, unsigned _b)
    {
    unsigned _f = _b;
    _f |= 1;
    ((DTxt*)this)->draw(_a, (int)_f, (char*)this + 0x40, 1.0f, 0);    }

// @ 0x494310
void QMgr::refresh_listener_494310()
    {
    char* self = (char*)this;
    if (g_5a9fd8 != 0)
        return;

    Vec4* _v = (Vec4*)(self + 0x248);
    for (size_type _i = 0; _i < _v->size(); ++_i)
        fsound_stop_5342a6(_v->_Myfirst[_i]);

    _v->resize(0, 0);

    int _buf[3];
    _buf[0] = 0;
    _buf[1] = 0;
    _buf[2] = 0;
    void* _m = ident3_40cd60(_buf);
    int _mat[12];
    memcpy(_mat, _m, 0x30);
    memcpy(self + 0x10, _mat, 0x40);
    {
    int _a0 = *(int*)(self + 0x20);
    int _a1 = *(int*)(self + 0x24);
    int _a2 = *(int*)(self + 0x28);
    int _a3 = *(int*)(self + 0x30);
    int _a4 = *(int*)(self + 0x34);
    int _a5 = *(int*)(self + 0x38);
    fsound_listener_5342dc(self + 0x40, 0, _a0, _a2, _a1, _a3, _a5, _a4);
    }
    }

// @ 0x494410
void QMgr::quest_done_494410(int _idx)
    {
    char* self = (char*)this;
    char* _e = (char*)(*(int*)(self + 0x13a58) + _idx * 0x138);
    if ((*(unsigned*)(_e + 0x50) >> 1 & 1) != 0 ||
        ((*(unsigned*)(_e + 0x64) & 0x200) == 0 && (*(unsigned*)(_e + 0x64) & 0x100) != 0))
        {
        if ((*(unsigned*)(_e + 0x64) & 0x800) == 0)
            {
            tstring _val(*(const tstring*)(_e + 0x40));
            char _buf[0x200];
            {
            tstring _fmt(s_ui_quest_completed_value);
            char* _f = _val.buf ? _val.buf : (char*)&_val;
            char* _r = g_strtab.lookup(&_fmt) + 4;
            if (*(char**)_r != 0)
                _r = *(char**)_r;
            mb_snprintf_415100(_buf, 0x200, _r, _f);
            }
            {
            tstring _txt(_buf);
            g_gui.text_set(&_txt, 0xffd0f0c8);
            }
            ((DTxt*)0x5aa128)->draw(g_5aa184, 1, (void*)0x5aa168, 1.0f, 0);
            }
        if ((*(unsigned*)(_e + 0x50) >> 1 & 1) == 0)
            *(unsigned*)(_e + 0x64) |= 0x200;
        else
            *(int*)(_e + 0x64) = 0;
        }
    }

// @ 0x494560
void QMgr::quest_succeeded_494560(int _idx)
    {
    char* self = (char*)this;
    char* _e = (char*)(*(int*)(self + 0x13a58) + _idx * 0x138);
    *(unsigned*)(_e + 0x64) |= 0x1400;
    tstring _val(*(const tstring*)(*(int*)(self + 0x13a58) + _idx * 0x138 + 0x40));
    char _buf[0x200];
    {
    tstring _fmt(s_ui_quest_succeeded_value);
    char* _f = _val.buf ? _val.buf : (char*)&_val;
    char* _r = g_strtab.lookup(&_fmt) + 4;
    if (*(char**)_r != 0)
        _r = *(char**)_r;
    mb_snprintf_415100(_buf, 0x200, _r, _f);
    }
    {
    tstring _txt(_buf);
    g_gui.text_set(&_txt, 0xff90ff28);
    }
    ((DTxt*)0x5aa128)->draw(g_5aa188, 1, (void*)0x5aa168, 1.0f, 0);
    }

// @ 0x494680
void QMgr::quest_failed_494680(int _idx)
    {
    char* self = (char*)this;
    char* _e = (char*)(*(int*)(self + 0x13a58) + _idx * 0x138);
    *(unsigned*)(_e + 0x64) |= 0x1800;
    tstring _val(*(const tstring*)(*(int*)(self + 0x13a58) + _idx * 0x138 + 0x40));
    char _buf[0x200];
    {
    tstring _fmt(s_ui_quest_failed_value);
    char* _f = _val.buf ? _val.buf : (char*)&_val;
    char* _r = g_strtab.lookup(&_fmt) + 4;
    if (*(char**)_r != 0)
        _r = *(char**)_r;
    mb_snprintf_415100(_buf, 0x200, _r, _f);
    }
    {
    tstring _txt(_buf);
    g_gui.text_set(&_txt, 0xffd02028);
    }
    ((DTxt*)0x5aa128)->draw(g_5aa190, 1, (void*)0x5aa168, 1.0f, 0);
    }
