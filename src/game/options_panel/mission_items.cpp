// Slice s004ce960: item / mission-object record types and the per-frame updater that lives next to them.
// 0x4ce960 / 0x4ceb30 / 0x4cef20 / 0x4cef80 / 0x4cee50 / 0x4cee50 are constructors of 0x60..0x2c8-byte
// records that all start with the same "vtbl then tstring at +0x1c, id 99" base; 0x4cea10 is the big
// initialiser of the 0x328-byte mission-parameter block, 0x4ceb90 the per-frame "process the placed
// objects" pass, and 0x4ceea0 / 0x4ceec0 its deleting dtor and releasing dtor. The three loops at the end
// (0x4cf070 / 0x4cf0a0 / 0x4cf0d0) walk arrays of 0x50 / 0x68 / 0xb0-byte records and call their readers.
// ALL NAMES PROVISIONAL. Layouts and field offsets reproduced from the disassembly of this slice only.
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <new>
#include "../../rgl/rgl_math.h"
#include "../../include/frustum.h"
#include "../tstring.h"
using rgl::vector4;

// ---- globals --------------------------------------------------------------------------------------------
extern int g_ticks[];                                           // 0x5e2948 / 0x5e294c (two-dword clock)
extern char* g_game;                                            // 0x5e2930 (the GameData instance)
extern char* g_menus;                                           // 0x5e29b0 (the menu array)
extern char* g_strtable;                                        // 0x5e2a18 (LangTable)
extern int g_5e2930_dummy;

struct LangTable {
    tstring* find(const tstring* key);                          // 0x4c6010
};
struct expander {
    tstring expand(const tstring& in);                          // 0x4606d0 (ret 8: hidden ret + in)
};
extern expander g_5a70440;

#include "../../include/crt.h"   // canonical: operator delete (0x535712)
void conv_clear(void* p);                                    // 0x4c8bf0 (releases an owned block)

// opaque record types used by the loops at the end of the slice
struct rec50 { int pad[0x50 / 4]; void read(void* p); };       // reader 0x4cd270, stride 0x50
struct rec68 { int pad[0x68 / 4]; void read(void* p); };       // reader 0x4cd2f0, stride 0x68
struct recb0 { int pad[0xb0 / 4]; void read(void* p); };       // reader 0x4cd380, stride 0xb0

// ---- 0x60-byte item base (vtbl 0x5947d8, id 99, tstring at +0x1c) ----------------------------------------
// A polymorphic base so MSVC emits the base-vtable store; the ctor puts id 99 in place after the name ctor.
struct item_base {
    virtual ~item_base() { }
    int f4;                                                     // +0x04 = 0
    int f8;                                                     // +0x08 = 99
    char padc[0x10];
    tstring name;                                               // +0x1c
    item_base() : name(), f8(99) { }
};

struct MissionBlocker : item_base {                             // 0x2c8 bytes, vtable 0x59616c
    char pad2c[0x8];
    int f34, f38, f3c, f40, f44, f48, f4c;
    int clock;                                                  // +0x50 = g_ticks[0]
    int clock_hi;                                               // +0x54 = g_ticks[1]
    float f58;                                                  // +0x58 = 1.0f
    int f5c;                                                    // +0x5c = 0
    char pad60[0xd4 - 0x60];
    int f_d4;                                                   // +0xd4 = 0
    int f_d8;                                                   // +0xd8 = 0
    char pad_dc[0xf8 - 0xdc];
    rgl::frustum fst;                                                // +0xf8 (0x41ef60)
    char pad1cc[0x2a8 - 0x1cc];
    int f_2a8;                                                  // +0x2a8 = 0
    char pad2ac[0x4];
    int f_2b0;                                                  // +0x2b0 = g_ticks[0]
    int f_2b4;                                                  // +0x2b4 = g_ticks[1]
    int f_2b8;                                                  // +0x2b8 = 0
    char pad2bc[0x4];
    int f_2c0;                                                  // +0x2c0 = g_ticks[0]
    int f_2c4;                                                  // +0x2c4 = g_ticks[1]
    MissionBlocker();                                           // 0x4ce960
};

// @ 0x4ce960
MissionBlocker::MissionBlocker()
{
    f4 = 0;
    f34 = 0; f38 = 0; f3c = 0; f48 = 0;
    clock = g_ticks[0];
    clock_hi = g_ticks[1];
    f58 = 1.0f;
    f40 = -1;
    f5c = 0;
    f_2a8 = 0;
    f_2b0 = g_ticks[0];
    f_2b4 = g_ticks[1];
    f_2b8 = 0;
    f_2c0 = g_ticks[0];
    f_2c4 = g_ticks[1];
    f_d4 = 0;
    f_d8 = 0;
}

// ---- the 0x328-byte mission parameter block (0x4cea10) ---------------------------------------------------
struct MissionParams {
    char pad0[0xd4];
    int f_d4;                                                   // +0xd4 = 0
    int f_d8;                                                   // +0xd8 = 0
    char pade0[0x110];
    vector4 v_1f0;                                              // +0x1f0 (0, 0, 10, w)
    vector4 v_218;                                              // +0x218 (0, 0, 10, w)
    int f_268;                                                  // +0x268 = 0
    float f_26c;                                                // +0x26c = -0.1f
    int f_270;                                                  // +0x270 = 0
    int f_274;                                                  // +0x274 = 0
    float f_278;                                                // +0x278 = 1.1f
    int f_284;                                                  // +0x284 = 0
    float f_288;                                                // +0x288 = 97.0f
    float f_28c;                                                // +0x28c = -1e6f
    int f_29c;                                                  // +0x29c = 0
    int f_2a0;                                                  // +0x2a0 = -1
    char pad2a4[0x28];
    char b_2e0;                                                 // +0x2e0 = 0
    char b_2e1;                                                 // +0x2e1 = 0
    float f_2ec;                                                // +0x2ec = 1.1f
    char b_2f0;                                                 // +0x2f0 = 0
    char b_2f1;                                                 // +0x2f1 = 0
    char b_2f2;                                                 // +0x2f2 = 0
    char b_2f8;                                                 // +0x2f8 = 0
    char b_2f9;                                                 // +0x2f9 = 0
    int f_300;                                                  // +0x300 = 0
    int f_314;                                                  // +0x314 = 0
    int f_318;                                                  // +0x318 = 0
    int f_31c;                                                  // +0x31c = 0
    int f_320;                                                  // +0x320 = -1
    int f_324;                                                  // +0x324 = -1
    void init();                                                // 0x4cea10
};

char* g_mission_params;                                         // unknown owner; init called from here

// @ 0x4cea10
void MissionParams::init()
{
    char* base = (char*)this;
    void* p1 = base + 0x1f0;
    void* p2 = base + 0x218;
    *(vector4*)p1 = vector4(0.0f, 0.0f, 10.0f);
    *(vector4*)p2 = vector4(0.0f, 0.0f, 10.0f);
    *(float*)(base + 0x278) = 1.1f;
    *(float*)(base + 0x2ec) = 1.1f;
    *(int*)(base + 0x324) = -1;
    *(char*)(base + 0x2e1) = 0;
    *(char*)(base + 0x2e0) = 0;
    *(char*)(base + 0x2f1) = 0;
    *(char*)(base + 0x2f2) = 0;
    *(int*)(base + 0x29c) = 0;
    *(int*)(base + 0x2a0) = -1;
    *(float*)(base + 0x288) = 97.0f;
    *(int*)(base + 0x268) = 0;
    *(float*)(base + 0x26c) = -0.1f;
    *(int*)(base + 0x284) = 0;
    *(int*)(g_game + 0x1a20c) = 0;
    *(char*)(base + 0x2f8) = 0;
    *(int*)(base + 0x270) = 0;
    *(int*)(base + 0x274) = 0;
    *(float*)(base + 0x28c) = -1e6f;
    *(char*)(base + 0x2f0) = 0;
    *(char*)(base + 0x2f9) = 0;
    *(int*)(base + 0x300) = 0;
    *(int*)(base + 0x314) = 0;
    *(int*)(base + 0x318) = 0;
    *(int*)(base + 0x31c) = 0;
    *(int*)(base + 0x320) = -1;
}

// ---- the remaining item constructors (0x4ceb30 / 0x4cef20 / 0x4cef80) --------------------------------------
struct ItemA : item_base {                                      // vtable 0x596190
    char pad2c[0x8];
    int f34, f38, f3c, f40, f44, f48, f4c;
    int clock, clock_hi; float f58; int f5c;
    ItemA();                                                    // 0x4ceb30
};
struct ItemB : item_base {                                      // vtable 0x5961e4
    char pad2c[0x8];
    int f34, f38, f3c, f40, f44, f48, f4c;
    int clock, clock_hi; float f58; int f5c;
    ItemB();                                                    // 0x4cef20
};
struct ItemC : item_base {                                      // vtable 0x596208
    char pad2c[0x8];
    int f34, f38, f3c, f40, f44, f48, f4c;
    int clock, clock_hi; float f58; int f5c;
    ItemC();                                                    // 0x4cef80
};

// @ 0x4ceb30
ItemA::ItemA()
{
    f4 = 0;
    f34 = 0; f38 = 0; f3c = 0; f48 = 0;
    clock = g_ticks[0];
    clock_hi = g_ticks[1];
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// @ 0x4cef20
ItemB::ItemB()
{
    f4 = 0;
    f34 = 0; f38 = 0; f3c = 0; f48 = 0;
    clock = g_ticks[0];
    clock_hi = g_ticks[1];
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// @ 0x4cef80
ItemC::ItemC()
{
    f4 = 0;
    f34 = 0; f38 = 0; f3c = 0; f48 = 0;
    clock = g_ticks[0];
    clock_hi = g_ticks[1];
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// ---- 0x9c-byte item with four empty vector-ish pairs (0x4cee50) -------------------------------------------
// 0x4ceea0 / 0x4ceec0 are the deleting dtor / releasing dtor of a class whose vtable takes precedence, so this
// variant carries a virtual dtor of its own (the shared base vtable 0x5947d8 + id 99, then its own vtable).
struct ItemD_vt {                                               // virtual-dtor base of 0x4cee50's record
    virtual ~ItemD_vt();                                        // 0x4ceec0 (releasing dtor)
    int f4;                                                     // +0x04 = 0
    int f8;                                                     // +0x08 = 99
    char padc[0x10];
    tstring name;                                               // +0x1c
    ItemD_vt() : name() { f4 = 0; f8 = 99; }
};
struct ItemD : ItemD_vt {                                       // vtable 0x5961c0
    char pad2c[0x7c - 0x2c];
    int f_7c;                                                   // +0x7c = 0 (vector begin)
    int f_80;                                                   // +0x80 = 0 (vector end)
    int f_84;                                                   // +0x84 = 0 (vector cap)
    int f_88;
    int f_8c;                                                   // +0x8c = 0 (vector begin)
    int f_90;                                                   // +0x90 = 0 (vector end)
    int f_94;                                                   // +0x94 = 0 (vector cap)
    char pad98[0x4];
    ItemD();                                                    // 0x4cee50
};

// @ 0x4cee50
ItemD::ItemD()
{
    f_7c = 0;
    f_80 = 0;
    f_84 = 0;
    f_8c = 0;
    f_90 = 0;
    f_94 = 0;
}

// @ 0x4ceec0
ItemD_vt::~ItemD_vt()
{
    ItemD* self = (ItemD*)this;
    if (self->f_8c)
        operator delete((void*)self->f_8c);
    self->f_8c = 0; self->f_90 = 0; self->f_94 = 0;
    if (self->f_7c)
        operator delete((void*)self->f_7c);
    self->f_7c = 0; self->f_80 = 0; self->f_84 = 0;
}

// ---- per-frame update of the placed objects (0x4ceb90) --------------------------------------------------
// The item arrays start at this+0x60; index 0xa0 is the count and the per-slot ids live at +0xa4.
struct mission_obj_vt {
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23(); virtual void v24();
    virtual void v25(); virtual void v26(); virtual void v27(); virtual void v28(); virtual void v29();
    virtual void v30(); virtual void v31(); virtual void v32(); virtual void v33(); virtual void v34();
    virtual void v35(); virtual void v36(); virtual void v37(); virtual void v38(); virtual void v39();
    virtual void v40(); virtual void v41(); virtual void v42(); virtual void v43(); virtual void v44();
    virtual void v45(); virtual void v46();
    void v_add8(int* out);                                      // +0xb0
    void v_addc();                                              // +0xbc
    void v_add4();                                              // +0xb4
    void v_add8b();                                             // +0xb8
    void v_58(float a, float b);                                // +0x58
};

struct SceneObj {
    int pad0[16];
    int zeros;                                                  // +0x40 = 0
    int f44;
    mission_obj_vt* vt;
    void v_add4();                                              // +0xa4 (thiscall, ret 4)
};

#include "../ui/snd_channel.h"                               // snd_channel: play 0x4875e0 (thiscall, ret 4)

struct placed_rec {
    char pad0[0x20];
    int flags;                                                  // +0x20 (bit 4 = skip v_b4)
    char pad24[0x30 - 0x24];
    int f30;
    char pad34[0x34];
};

struct scene_group {
    int vt;
    char pad4[0x1c];
    int flags;                                                  // +0x20
    char pad24[0x30];
    int count;                                                  // +0x54
    placed_rec* recs;                                           // +0x58
    void v64(float a, float b);                                 // +0x64
};

struct scene_handler {
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23(); virtual void v24();
    virtual void v25(); virtual void v26();
    void relayout(const tstring& s);                          // 0x48f020 (thiscall, ret 4)
    void v_add8(int* out);                                      // +0xa4 (thiscall, ret 8)
    virtual void v64(float a, float b);                         // +0x64
};

struct MissionUpdater {
    item_base* items[0x10];                                     // +0x00 .. +0x3c (the objects at +0x60)
    char pad0[0x20];
    int count;                                                  // +0x40 (at +0xa0)
    int ids[0x10];                                              // +0x44
    char pade4[0x4];
    scene_handler* scene;                                       // +0x88 (at +0xe8, 0x4cec0d)
    scene_group* group;                                         // +0x8c (at +0xec, 0x4cee0c)
    void update();                                              // 0x4ceb90
};

// @ 0x4ceb90
void MissionUpdater::update()
{
    int i = *(int*)(g_game + 0x13aec);
    scene_group* g = (scene_group*)(i * 0x60 + (int)g_menus);
    tstring* t = ((LangTable*)g_strtable)->find((tstring*)((char*)g + 0x10));
    if (t == 0)
        t = (tstring*)g;
    tstring tmp = g_5a70440.expand(*t);
    scene->relayout(tmp.strip_tags());
    tmp.~tstring();
    for (int k = 0; k < 16; k++) {
        mission_obj_vt* o = (mission_obj_vt*)items[k];
        o->v_add8(0);
        o->v_addc();
    }
    int n = g->count;
    int idx = 0;
    while (idx < n && count < 0x10) {
        *(char*)(g_game + 0x14054) = 0;
        snd_channel* ch = (snd_channel*)((char*)g->recs + idx * 0x68 + 0x30);
        if (ch->play(8)) {
            SceneObj* obj = (SceneObj*)((char*)g->recs + idx * 0x68);
            tstring* nm = ((LangTable*)g_strtable)->find((tstring*)((char*)obj + 0x20));
            if (nm == 0)
                nm = (tstring*)obj;
            tstring tmp2 = g_5a70440.expand(*nm);
            tstring tmp3 = tmp2.strip_tags();
            scene_handler* h = scene;
            h->v_add8((int*)&obj);
            tmp3.~tstring();
            tmp2.~tstring();
            if (!(g->recs[idx].flags & 0x10))
                ((mission_obj_vt*)items[count])->v_add4();
            if (*(char*)(g_game + 0x14054))
                ((mission_obj_vt*)items[count])->v_add8b();
            ids[count] = idx;
            count++;
        }
        idx++;
    }
    float f = (float)count * /*0x54ab1c*/ 0.02f;
    if (!(f > /*0x5961bc*/ 1000.0f))
        f = 0.24f;
    for (int k = 0; k < count; k++) {
        mission_obj_vt* o = (mission_obj_vt*)items[k];
        o->v_58((1.0f - k * 0.5f) /* 0x549d70 */, f);
    }
    float f2 = (float)count * 0.02f;
    if (!(f2 > 1000.0f))
        f2 = 1000.0f;
    scene_group* gg = group;
    gg->v64(0.75f /* 0x3f400000 */, f2);
    ((void (*)(scene_group*))0x488d50)(gg);
}

// ---- the array walkers (0x4cf070 / 0x4cf0a0 / 0x4cf0d0) --------------------------------------------------
// @ 0x4cf070
void walk50(rec50* begin, rec50* end, void* arg)
{
    for (; begin != end; begin++)
        begin->read(arg);
}

// @ 0x4cf0a0
void walk68(rec68* begin, rec68* end, void* arg)
{
    for (; begin != end; begin++)
        begin->read(arg);
}

// @ 0x4cf0d0
void walkb0(recb0* begin, recb0* end, void* arg)
{
    for (; begin != end; begin++)
        begin->read(arg);
}

// 0x4cefe0: append one element (stride 0x2c) with the capacity check.
struct vec44 {
    char pad0[4];
    char* begin;                                                // +0x04
    char* end;                                                  // +0x08
    char* cap;                                                  // +0x0c
    int size() { if (!begin) return 0; return (int)((end - begin) / (ptrdiff_t)0x2c); }
    unsigned capacity() { if (!begin) return 0; return (unsigned)((cap - begin) / (ptrdiff_t)0x2c); }
    void push_back(char* v);                                    // 0x4cefe0 (ret 4)
    void grow(char** out, char* at, char* v);                   // 0x4ccef0 (thiscall)
};

extern char* uninit_fill_n(char* dst, int n, char* src, vec44* self, char* src2);   // 0x4c5010

// @ 0x4cefe0
void vec44::push_back(char* v)
{
    int n = size();
    if (begin && (unsigned)n < capacity()) {
        uninit_fill_n(end, 1, v, this, v);
        end += 0x2c;
        return;
    }
    grow(&v, end, v);
}
