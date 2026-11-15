// event.h: Event, the 0x98-byte map event / battle record (vtable 0x5922cc, an IfaceB), the record of the pool at GameData +0xcc
// (Pool<Event>, include/pool.h; game_data.h `events`). ONE definition for src/game/mission_ui2 (stat_panel_x.cpp: constructor
// 0x48c210, slot 1 0x48c260), world_data (Pool<Event>::alloc 0x493700: `new Event[16]` pages, which is where the vector
// deleting destructor of the vtable is emitted), script (spawn_battle_icon 0x49bbb0, attach_party_center 0x49e9c0),
// inventory_panel (mission_ctx.cpp 0x514d40, map_panel.cpp 0x5191e0 / 0x51abd0). It was also declared as Rec98 (rec98.h).
// party2's BattleRec (battle_rec.h: bind_host 0x4b2770, detach 0x4b6180, commit 0x4b6870) is the same record under that
// family's name: the same table at +0xcc, the same offsets (+0x10 type, +0x24 position, +0x54 / +0x58 the two sides,
// +0x60 / +0x80 the clocks the constructor starts).
// IfaceB's words are the pool's: f4 = 1 while the record is allocated, f8 = its own index, fc = its serial (0x493700).
// slot 1 (0x5922d0, no direct callers) is the IfaceB::slot1 override 0x48c260 (it initialises the event); slot 0 is the
// compiler's vector deleting destructor (the same code as RecF's, 0x4316c0) and slot 2 IfaceB's empty one.
#ifndef GAME_EVENT_H
#define GAME_EVENT_H
#include "./tstring.h"
#include "../include/iface.h"                   // IfaceB: the record base (vtable 0x5908f8; Event is the same family as RecF)
#include "../include/vec.h"                     // Vec2

struct Event : IfaceB {
    int type;                                   // +0x10 0 = new (0x48c260); 1 = live, the icon owner present (0x49bbb0, 0x49e9c0, 0x514d40)
    tstring name;                               // +0x14 "An Event" (0x48c260), "" (0x48c210)
    Vec2 pos;                                   // +0x24 position (0x48c260 zeroes it; 0x49bbb0; 0x49e9c0 assigns it as one Vec2)
    char pad2c[0x34 - 0x2c];
    float f34;                                  // +0x34 direction (0x48c260: (0, 1) turned by a random angle)
    float f38;                                  // +0x38
    float z;                                    // +0x3c height over the ground (BattleRec::bind_host 0x4b2770)
    float f40;                                  // +0x40 0.3 (0x48c260)
    float f44;                                  // +0x44 -10000.0 (0x48c260), the icon opacity (BattleRec::update_nav_anim 0x4b71e0)
    char pad48[0x50 - 0x48];
    int f50;                                    // +0x50 the scene entity (0x49bbb0)
    int party_a;                                // +0x54 first side (0x49e9c0, 0x514d40, 0x51abd0)
    int party_b;                                // +0x58 other side
    char pad5c[0x60 - 0x5c];
    int f60;                                    // +0x60 clock 1 (0x48c210)
    char pad64[4];
    int f68;                                    // +0x68 g_ticks[1] low word (0x48c210)
    int f6c;                                    // +0x6c g_ticks[1] high word
    char pad70[0x78 - 0x70];
    int height;                                 // +0x78 the summed unit counts of both sides (attach_party_center 0x49e9c0)
    char pad7c[0x80 - 0x7c];
    int f80;                                    // +0x80 clock 0 (0x48c210)
    char pad84[4];
    int f88;                                    // +0x88 g_ticks[0] low word
    int f8c;                                    // +0x8c g_ticks[0] high word
    int f90;                                    // +0x90 -1 (0x48c260)
    int f94;                                    // +0x94 (no function of this object touches it)
    Event();                                    // 0x48c210
    virtual void slot1();                       // 1: 0x48c260 (init)
};
typedef char assert_Event_size[sizeof(Event) == 0x98 ? 1 : -1];

#endif // GAME_EVENT_H
