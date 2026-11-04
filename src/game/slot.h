// slot.h: the input device record at 0xa170c8 (global g_keyboard), 0x848 bytes: the DirectInput object and its keyboard and
// mouse devices, the per-scan-code key table (held state, then the previous frame's snapshot), the mouse deltas and the
// clock helpers of the per-frame update. ONE definition for every module that reads a key or calls into it:
// mesh_slot/slot.cpp (read_keys, read_state, update, is_ready), config/config.cpp (init_devices), options_panel (key_name),
// mission5 (mesh_3 / mission_5 read key[] and now[] directly), script, char_sheet, inventory_panel, party2.
#ifndef MB_GAME_SLOT_H
#define MB_GAME_SLOT_H
#include <stddef.h>
#include "./tstring.h"

struct IDirectInput8A;
struct IDirectInputDevice8A;
struct HWND__;

struct Slot {
    IDirectInput8A* dinput;                                 // +0   0x4bc430 (DirectInput8Create)
    IDirectInputDevice8A* kb;                               // +4   the keyboard device (0x100-byte state)
    IDirectInputDevice8A* state;                            // +8   the mouse device (0x14-byte state, c_dfDIMouse2)
    int down[0x100];                                        // +0xc  held state per scan code (down[0xe0..0xe7] = mouse buttons, filled by read_state)
    int prev[0x100];                                        // +0x40c the previous frame's snapshot of down (taken at the top of update; a press is down && !prev)
    float axis[3];                                          // +0x80c mouse look axes (x, y, wheel)
    float f818;                                             // +0x818
    float f81c;                                             // +0x81c
    int clock820;                                           // +0x820
    __int64 t828;                                           // +0x828
    int clock830;                                           // +0x830
    __int64 t838;                                           // +0x838
    char b840;                                              // +0x840
    char pad841[0x848 - 0x841];

    Slot();                                                 // both tick snapshots = clock 0's time, the held-key table and look axes cleared (the original's static initializer 0x547b50)
    void update(float dt);                                  // 0x487930 (thiscall, ret 4)
    int read_keys();                                        // 0x487640
    int read_state();                                       // 0x4876e0
    int is_ready(int i);                                    // 0x487900 (thiscall, ret 4)
    int init_devices(HWND__* hwnd);                        // 0x4bc430 (config/config.cpp)
    tstring key_name(int key);                              // 0x4d7610 (options_panel/options_panel.cpp)
};
typedef char slot_size_check[(sizeof(Slot) == 0x848) ? 1 : -1];
extern Slot g_keyboard;                                     // 0xa170c8

#endif // MB_GAME_SLOT_H
