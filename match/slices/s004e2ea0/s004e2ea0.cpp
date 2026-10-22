// s004e2ea0: the slot / key-binding name helper of the options menu. Each of the 37 slot indices maps to its
// "ui_*" string-table key (default "unknown"); window-local text locals live in two 0x1c0-byte arrays.
// Provisional names; offsets from the disassembly of 0x4e2ea0.
#include <windows.h>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;    // MSVC x87 helper (this TU uses floats)

struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring() MB_NOTHROW;                               // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;                  // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;               // 0x42c110
    ~tstring() MB_NOTHROW;                              // 0x42bc40
    void operator=(const tstring& o) MB_NOTHROW;        // 0x42bc50
};

struct StrTable {                                       // 0x5e2a18
    tstring* lookup(tstring* key) MB_NOTHROW;           // 0x43a1e0
};
extern StrTable g_strtable;                             // 0x5e2a18

// @ 0x4e2ea0    tstring* slot_text(tstring* out, int i)  (ret 8)
tstring* slot_text(tstring* out, int i)
{
    tstring s("unknown");
    tstring l1[0x1c], l2[0x1c];
    switch (i) {
    case 0:
        l1[0] = tstring("ui_move_forward");
        s = *g_strtable.lookup(&l2[0x1b]);
        break;
    case 1:
        l1[4] = tstring("ui_move_backward");
        s = *g_strtable.lookup(&l2[0x13]);
        break;
    case 2:
        l1[3] = tstring("ui_move_left");
        s = *g_strtable.lookup(&l2[3]);
        break;
    case 3:
        l1[0x23] = tstring("ui_move_right");
        s = *g_strtable.lookup(&l2[0x23]);
        break;
    case 4:
        l1[5] = tstring("ui_action");
        s = *g_strtable.lookup(&l2[5]);
        break;
    case 5:
        l1[0x15] = tstring("ui_jump");
        s = *g_strtable.lookup(&l2[0x15]);
        break;
    case 6:
        l1[7] = tstring("ui_attack");
        s = *g_strtable.lookup(&l2[7]);
        break;
    case 7:
        l1[0x1d] = tstring("ui_defend");
        s = *g_strtable.lookup(&l2[0x1d]);
        break;
    case 8:
        l1[9] = tstring("ui_equip_next_weapon");
        s = *g_strtable.lookup(&l2[9]);
        break;
    case 9:
        l1[0x17] = tstring("ui_equip_next_shield");
        s = *g_strtable.lookup(&l2[0x17]);
        break;
    case 10:
        l1[0xb] = tstring("ui_sheath_weapon");
        s = *g_strtable.lookup(&l2[0xb]);
        break;
    case 11:
        l1[0x21] = tstring("ui_character_window");
        s = *g_strtable.lookup(&l2[0x21]);
        break;
    case 12:
        l1[0xd] = tstring("ui_inventory_window");
        s = *g_strtable.lookup(&l2[0xd]);
        break;
    case 13:
        l1[0x19] = tstring("ui_party_window");
        s = *g_strtable.lookup(&l2[0x19]);
        break;
    case 14:
        l1[0x1f] = tstring("ui_quests_window");
        s = *g_strtable.lookup(&l2[0x1f]);
        break;
    case 15:
        l1[0x10] = tstring("ui_game_log_window");
        s = *g_strtable.lookup(&l2[0x10]);
        break;
    case 16:
        l1[0x1b] = tstring("ui_leave_location_retreat");
        s = *g_strtable.lookup(&l2[0x1b]);
        break;
    case 17:
        l1[0x40] = tstring("ui_zoom");
        s = *g_strtable.lookup(&l2[0x40]);
        break;
    case 18:
        l1[0x59] = tstring("ui_view_outfit");
        s = *g_strtable.lookup(&l2[0x59]);
        break;
    case 19:
        l1[0x56] = tstring("ui_toggle_first_person_view");
        s = *g_strtable.lookup(&l2[0x56]);
        break;
    case 20:
        l1[0x57] = tstring("ui_view_orders");
        s = *g_strtable.lookup(&l2[0x57]);
        break;
    case 21:
        l1[0x04] = tstring("ui_order_hold");
        s = *g_strtable.lookup(&l2[0x04]);
        break;
    case 22:
        l1[0x3d] = tstring("ui_order_follow");
        s = *g_strtable.lookup(&l2[0x3d]);
        break;
    case 23:
        l1[0x6d] = tstring("ui_order_charge");
        s = *g_strtable.lookup(&l2[0x6d]);
        break;
    case 24:
        l1[0xd6] = tstring("ui_order_dismount_toggle");
        s = *g_strtable.lookup(&l2[0xd6]);
        break;
    case 25:
        l1[0xe2] = tstring("ui_order_advance");
        s = *g_strtable.lookup(&l2[0xe2]);
        break;
    case 26:
        l1[0xe6] = tstring("ui_order_fall_back");
        s = *g_strtable.lookup(&l2[0xe6]);
        break;
    case 27:
        l1[0x73] = tstring("ui_order_stand_closer");
        s = *g_strtable.lookup(&l2[0x73]);
        break;
    case 28:
        l1[0x65] = tstring("ui_order_spread_out");
        s = *g_strtable.lookup(&l2[0x65]);
        break;
    case 29:
        l1[0x9e] = tstring("ui_order_blunt_weapons_toggle");
        s = *g_strtable.lookup(&l2[0x9e]);
        break;
    case 30:
        l1[0xd7] = tstring("ui_order_hold_fire_toggle");
        s = *g_strtable.lookup(&l2[0xd7]);
        break;
    case 31:
        l1[0x10] = tstring("ui_everyone_control");
        s = *g_strtable.lookup(&l2[0x10]);
        break;
    case 32:
        l1[0x33] = tstring("ui_archers_control");
        s = *g_strtable.lookup(&l2[0x33]);
        break;
    case 33:
        l1[0x42] = tstring("ui_infantry_control");
        s = *g_strtable.lookup(&l2[0x42]);
        break;
    case 34:
        l1[0x72] = tstring("ui_cavalry_control");
        s = *g_strtable.lookup(&l2[0x72]);
        break;
    case 35:
        l1[0xab] = tstring("ui_others_control");
        s = *g_strtable.lookup(&l2[0xab]);
        break;
    case 36:
        l1[0xe4] = tstring("ui_quick_save");
        s = *g_strtable.lookup(&l2[0xe4]);
        break;
    }
    *out = s;
    return out;
}
