// Slice s004d0620: creation of the unnamed mission-template trigger records (one heap object per record).
// Everything below is *engine* code that this slice's function calls but that other slices own; it is
// declared only, so the calls here bind to the original routines.
#ifndef S004D0620_H
#define S004D0620_H
#include <stddef.h>

// RTTI descriptor used by scalar `new` (0x5a9864 in the game image).
extern unsigned char g_rtti_trigger_record;     // 0x5a9864

// @ 0x4d0620
void init_mission_trigger_records();

#endif
