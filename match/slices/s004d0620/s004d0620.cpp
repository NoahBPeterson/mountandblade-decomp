// Slice s004d0620: creation of the unnamed mission-template trigger records (one heap object per record).
// Every record starts with the same head: vtable 0x5947d8, a zero word at +4, the 0x63 status word at +8
// and a tstring name at +0x1c; a derived record then writes its own (here identical) vtable on top.
// Record 7 (+0x118) also has a second tstring at +0x2c and a frustum at +0x3c; the +0x38 record is only
// this head. The remaining record kinds are built by constructors that belong to other slices, so this file
// only declares them.
// NOTE: reconstruction only; see nonmatching.txt — the original was built without RTTI, so the scalar
// `new` calls a bare `operator new` + ctor instead of cl's `??_U@YAPAXI@Z` RTTI path emitted here.
#include <windows.h>
#include <d3d9.h>
#include "rgl.h"
#include "tstring.h"

struct frustum {
    char data[0xcc];
};

// The record head, shared by all of them (0x4c2c50 is the canonical out-of-line version).
struct trigger_head {
    void* vtable;       // +0 (0x5947d8)
    int pad04;          // +4
    int status;         // +8 (0x63)
    char pad0c[0x10];
    tstring name;       // +0x1c
    trigger_head() : vtable((void*)0x5947d8), pad04(0), status(0x63) {}
};

// +0x118 record: name at +0x1c, second string at +0x2c, frustum at +0x3c.
struct record_118 : trigger_head {
    tstring second;     // +0x2c
    frustum fr;         // +0x3c
};

// +0x38 record: only the head.
struct record_38 : trigger_head {
};

// @ 0x4d0620
void init_mission_trigger_records()
{
    record_38* r = new record_38();
    r->status = 0x63;
}
