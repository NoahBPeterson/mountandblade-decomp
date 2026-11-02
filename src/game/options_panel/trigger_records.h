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

#include "../screen_records.h"   // the record classes (vtables 0x595740..0x596c68)
#include "../conv_screen.h"      // Rec1208: the conversation screen

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
