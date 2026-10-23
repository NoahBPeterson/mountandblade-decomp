// Slice s004d0620: creation of the mission-template trigger records (one heap object per record).
// Every record starts with the same head: vtable 0x5947d8, a zero word at +4, the 0x63 status word at +8
// and a tstring name at +0x1c; a derived record then writes its own vtable on top. Records built by
// constructors of other slices are declared opaque (size only); the others have their (tiny) constructors
// inlined into this function.
// NOTE: the original brackets the body with two inline-asm data tags (jmp +0x10 over "WL  ", 6, 0.. at the
// start; jmp +0x15 over "WL  ", 7, .. before the epilogue) that cannot be emitted from C++.
#ifndef S004D0620_H
#define S004D0620_H
#include <stddef.h>
#include "tstring.h"

void* __cdecl operator new(size_t n);   // 0x53545e (scalar operator new)

struct frustum {
    char data[0xd4];
    frustum();      // 0x41ef60
};

struct head_rec {
    virtual void vfunc();               // only the vptr store matters (vtable 0x5947d8)
    int w04;                            // +4
    int w08;                            // +8
    char pad0c[0x10];
    tstring name;                       // +0x1c
    head_rec() { w04 = 0; w08 = 0x63; }
};
struct RecCam { char body[0x2dc]; RecCam(); };   // ctor 0x4c49f0
struct RecMenu { char body[0x98]; RecMenu(); };   // ctor 0x4c2c50
struct RecProfile { char body[0xcc]; RecProfile(); };   // ctor 0x4c3050
struct RecItem90 { char body[0x90]; RecItem90(); };   // ctor 0x4ce870
struct Rec98b { char body[0x98]; Rec98b(); };   // ctor 0x4c3290
struct Rec90b { char body[0x90]; Rec90b(); };   // ctor 0x4c2fb0
struct Rec80 { char body[0x80]; Rec80(); };   // ctor 0x4ce8d0
struct Rec3910 { char body[0x3910]; Rec3910(); };   // ctor 0x4c3160
struct RecBlocker { char body[0x328]; RecBlocker(); };   // ctor 0x4ce960
struct RecObj2 { char body[0x530]; RecObj2(); };   // ctor 0x4c4cc0
struct Rec238 { char body[0x238]; Rec238(); };   // ctor 0x4c31c0
struct RecItemA { char body[0xf8]; RecItemA(); };   // ctor 0x4ceb30
struct RecItemD { char body[0xb4]; RecItemD(); };   // ctor 0x4cee50
struct RecItemB { char body[0x60]; RecItemB(); };   // ctor 0x4cef20
struct RecItemC { char body[0x60]; RecItemC(); };   // ctor 0x4cef80
struct Rec118 : head_rec {   // vtable 0x595804
    char pad2c[0x58];
    tstring second;     // +0x84
    char tail[0x118 - 0x94];
    virtual void vfunc();
};
struct Recacc : head_rec {   // vtable 0x595764
    char body[0xaa0];
    Recacc() { w04 = 0; }
    virtual void vfunc();
};
struct Rec38 : head_rec {   // vtable 0x595828
    char body[0xc];
    virtual void vfunc();
};
struct Rec44 : head_rec {   // vtable 0x5957e0
    char body[0x18];
    virtual void vfunc();
};
struct Rec34 : head_rec {   // vtable 0x59584c
    char body[0x8];
    virtual void vfunc();
};
struct Rec454 : head_rec {   // vtable 0x595894
    char body[0x428];
    virtual void vfunc();
};
struct Rec1208 : head_rec {   // vtable 0x595bfc
    char pad2c[0x8];
    frustum fr;         // +0x34
    char pad108[0x1200 - 0x108];
    int f1200;
    char tail[0x1208 - 0x1204];
    Rec1208() { f1200 = -1; }
    virtual void vfunc();
};
struct Rec3c : head_rec {   // vtable 0x595948
    char body[0x10];
    virtual void vfunc();
};
struct Recd8 : head_rec {   // vtable 0x5958b8
    char body[0xac];
    virtual void vfunc();
};
struct Recf0 : head_rec {   // vtable 0x5958dc
    char body[0xc4];
    virtual void vfunc();
};
struct Rec1bc : head_rec {   // vtable 0x595900
    char body[0x190];
    virtual void vfunc();
};
struct Rec23c : head_rec {   // vtable 0x595c44
    char pad2c[0x44];
    frustum fr;         // +0x70
    char tail[0x23c - 0x144];
    virtual void vfunc();
};
struct Rec48 : head_rec {   // vtable 0x59596c
    char body[0x1c];
    virtual void vfunc();
};
struct Rec58 : head_rec {   // vtable 0x5959fc
    char body[0x2c];
    virtual void vfunc();
};
struct Rec428 : head_rec {   // vtable 0x595c68
    char pad2c[0x8];
    frustum fr;         // +0x34
    char tail[0x428 - 0x108];
    virtual void vfunc();
};

extern RecCam* g_rec_9c;   // 0xa7399c
extern RecMenu* g_main_menu;   // 0xa73980
extern RecProfile* g_profile;   // 0xa73984
extern RecItem90* g_rec_90;   // 0xa73990
extern Rec98b* g_rec_88;   // 0xa73988
extern Rec90b* g_rec_8c;   // 0xa7398c
extern Rec80* g_rec_94;   // 0xa73994
extern Rec118* g_rec_98;   // 0xa73998
extern Recacc* g_rec_a0;   // 0xa739a0
extern Rec38* g_rec_a4;   // 0xa739a4
extern Rec44* g_rec_a8;   // 0xa739a8
extern Rec34* g_rec_ac;   // 0xa739ac
extern Rec3910* g_rec_b0;   // 0xa739b0
extern Rec454* g_rec_b4;   // 0xa739b4
extern RecBlocker* g_rec_b8;   // 0xa739b8
extern RecObj2* g_rec_bc;   // 0xa739bc
extern Rec1208* g_rec_c0;   // 0xa739c0
extern Rec3c* g_rec_c4;   // 0xa739c4
extern Recd8* g_rec_c8;   // 0xa739c8
extern Recf0* g_rec_cc;   // 0xa739cc
extern Rec1bc* g_rec_d0;   // 0xa739d0
extern Rec238* g_rec_d4;   // 0xa739d4
extern Rec23c* g_rec_d8;   // 0xa739d8
extern RecItemA* g_rec_dc;   // 0xa739dc
extern Rec48* g_army_state;   // 0xa739e0
extern Rec58* g_rec_e4;   // 0xa739e4
extern RecItemD* g_rec_e8;   // 0xa739e8
extern RecItemB* g_rec_ec;   // 0xa739ec
extern RecItemC* g_rec_f0;   // 0xa739f0
extern Rec428* g_menu_state;   // 0xa739f4
extern int g_trial_mode;   // 0x5e29c4
extern int g_5e29ac;   // 0x5e29ac

// @ 0x4d0620
void init_mission_trigger_records();
#endif
