// trigger_records.h (was slice s004d0620): creation of the mission-template trigger records (one heap object per record).
// Every record starts with the same head: vtable 0x5947d8, a zero word at +4, the 0x63 status word at +8
// and a tstring name at +0x1c; a derived record then writes its own vtable on top. Records built by
// constructors of other slices are declared opaque (size only); the others have their (tiny) constructors
// inlined into this function.
// NOTE: the original brackets the body with two inline-asm data tags (jmp +0x10 over "WL  ", 6, 0.. at the
// start; jmp +0x15 over "WL  ", 7, .. before the epilogue) that cannot be emitted from C++.
#ifndef OPTIONS_PANEL_TRIGGER_RECORDS_H
#define OPTIONS_PANEL_TRIGGER_RECORDS_H
#include <stddef.h>
#include "../tstring.h"
#include "../../include/frustum.h"

void* __cdecl operator new(size_t n);   // 0x53545e (scalar operator new)

#include <string.h>
#include "../head_rec.h"

// A record's slot 1 is `release` (0x4cb680), which frees the pointer at +0x178. The original allocates these records
// with the plain operator new 0x53545e and its constructors write only the head and the member objects; ours left the
// rest as raw heap and release freed garbage (runtime workaround, not in the original). Zero everything above the
// base's head (vptr, w04, w08, name) when the record is allocated, before any constructor runs: zeroing it in the
// constructor body wiped the members it had just built (Rec23c's frustum at +0x70, Rec118's tstring at +0x84).
#define MB_ZERO_TAIL(T) static void* operator new(size_t n) \
    { char* p = (char*)::operator new(n); if (p && n > 0x2c) memset(p + 0x2c, 0, n - 0x2c); return p; }

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
struct Rec118 : head_rec { MB_ZERO_TAIL(Rec118)   // vtable 0x595804
    char pad2c[0x58];
    tstring second;     // +0x84
    char tail[0x118 - 0x94];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Recacc : head_rec {   // vtable 0x595764
    char body[0xaa0];
    Recacc() { w04 = 0; }
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec38 : head_rec { MB_ZERO_TAIL(Rec38)   // vtable 0x595828
    char body[0xc];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 0; }    // slot 7: 0x4280e0 (xor al,al; ret), not head_rec's return 1
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec44 : head_rec { MB_ZERO_TAIL(Rec44)   // vtable 0x5957e0
    char body[0x18];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec34 : head_rec { MB_ZERO_TAIL(Rec34)   // vtable 0x59584c
    char body[0x8];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 0; }    // slot 7: 0x4280e0 (xor al,al; ret), not head_rec's return 1
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec454 : head_rec { MB_ZERO_TAIL(Rec454)   // vtable 0x595894
    char body[0x428];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec1208 : head_rec {   // vtable 0x595bfc
    char pad2c[0x8];
    rgl::frustum fr;    // +0x34 (0x120 bytes, canonical src/include/frustum.h)
    char pad154[0x1200 - 0x154];
    int f1200;
    char tail[0x1208 - 0x1204];
    Rec1208() { f1200 = -1; }
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec3c : head_rec { MB_ZERO_TAIL(Rec3c)   // vtable 0x595948
    char body[0x10];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Recd8 : head_rec { MB_ZERO_TAIL(Recd8)   // vtable 0x5958b8
    char body[0xac];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Recf0 : head_rec { MB_ZERO_TAIL(Recf0)   // vtable 0x5958dc
    char body[0xc4];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec1bc : head_rec { MB_ZERO_TAIL(Rec1bc)   // vtable 0x595900
    char body[0x190];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec23c : head_rec { MB_ZERO_TAIL(Rec23c)   // vtable 0x595c44
    char pad2c[0x44];
    rgl::frustum fr;    // +0x70
    char tail[0x23c - 0x190];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec48 : head_rec { MB_ZERO_TAIL(Rec48)   // vtable 0x59596c
    char body[0x1c];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec58 : head_rec { MB_ZERO_TAIL(Rec58)   // vtable 0x5959fc
    char body[0x2c];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec428 : head_rec { MB_ZERO_TAIL(Rec428)   // vtable 0x595c68
    // Without the zeroed tail, slot +0x04 (`release`, 0x4cb680) freed whatever sat at +0x178 as a CellGrid and its
    // tstring destructor freed garbage (crash 0x3c6d9166). The original's constructor (inline at 0x4d0bc1) writes only
    // the head and the frustum.
    char pad2c[0x8];
    rgl::frustum fr;    // +0x34
    char tail[0x428 - 0x154];
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
    // head_rec now declares the nine slots the original's table at 0x595c68 has (slot +0x08 is FaceGen::init, +0x0c
    // CamView::update_model), so this record only overrides slot 0 with its own vfunc.
};

extern RecCam* g_rec_9c;   // 0xa7399c
#include "../world_data/world_data.h"   // canonical: g_windows (0xa73980)
extern RecItem90* g_rec_90;   // 0xa73990
extern Rec98b* g_rec_88;   // 0xa73988
extern Rec38* g_rec_a4;   // 0xa739a4
extern Rec3c* g_rec_c4;   // 0xa739c4
extern Rec48* g_army_state;   // 0xa739e0
extern Rec428* g_menu_state;   // 0xa739f4
extern int g_trial_mode;   // 0x5e29c4
extern int g_5e29ac;   // 0x5e29ac

// @ 0x4d0620
void init_mission_trigger_records();
#endif
