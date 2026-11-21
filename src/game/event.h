// event.h: Event, the 0x98-byte map event / battle record (vtable 0x5922cc, an IfaceB), the record of the pool at GameData +0xcc
// (Pool<Event>, include/pool.h; game_data.h `events`). ONE definition for src/game/mission_ui2 (stat_panel_x.cpp: constructor
// 0x48c210, slot 1 0x48c260), world_data (Pool<Event>::alloc 0x493700: `new Event[16]` pages, which is where the vector
// deleting destructor of the vtable is emitted), script (spawn_battle_icon 0x49bbb0, attach_party_center 0x49e9c0),
// inventory_panel (mission_ctx.cpp 0x514d40, map_panel.cpp 0x5191e0 / 0x51abd0). It was also declared as Rec98 (rec98.h).
// party2 called it BattleRec (bind_host 0x4b2770, detach 0x4b6180, commit 0x4b6870, update_nav_anim 0x4b71e0: members now) under that
// family's name: the same table at +0xcc, the same offsets (+0x10 type, +0x24 position, +0x54 / +0x58 the two sides,
// +0x60 / +0x80 the clocks the constructor starts).
// IfaceB's words are the pool's: f4 = 1 while the record is allocated, f8 = its own index, fc = its serial (0x493700).
// slot 1 (0x5922d0, no direct callers) is the IfaceB::slot1 override 0x48c260 (it initialises the event); slot 0 is the
// compiler's vector deleting destructor 0x4316c0 (world_data.cpp) and slot 2 IfaceB's empty one.
#ifndef GAME_EVENT_H
#define GAME_EVENT_H
#include "./tstring.h"
#include "../include/layout32.h"
#include "../include/iface.h"                   // IfaceB: the record base (vtable 0x5908f8)
#include "../include/vec.h"                     // Vec2
#include "../include/timer.h"                   // Timer: the two clocks
struct face;                                    // include/shape.h: the nav-mesh faces under the two positions
namespace rgl { struct strategic_entity; }
struct stream_base;

struct Event : IfaceB {
    int type;                                   // +0x10 0 = new (0x48c260); 1 = live, the icon owner present (0x49bbb0, 0x49e9c0, 0x514d40)
    tstring name;                               // +0x14 "An Event" (0x48c260), "" (0x48c210)
    Vec2 pos;                                   // +0x24 position (0x48c260 zeroes it; 0x49bbb0; 0x49e9c0 assigns it as one Vec2)
    float f2c, f30;                             // +0x2c the second position (a ship battle's landing point: bind_host 0x4b2770; the save writer 0x4e9ab0 writes the pair)
    float f34;                                  // +0x34 direction (0x48c260: (0, 1) turned by a random angle)
    float f38;                                  // +0x38
    float z;                                    // +0x3c height over the ground (bind_host 0x4b2770)
    float f40;                                  // +0x40 0.3 (0x48c260)
    float f44;                                  // +0x44 -10000.0 (0x48c260), the icon opacity (update_nav_anim 0x4b71e0)
    face* face1;                                // +0x48 the nav face under pos (bind_host 0x4b2770; Party::tick reads it on arrival)
    face* face2;                                // +0x4c the nav face under (f2c, f30) (bind_host, type 2)
    rgl::strategic_entity* entity;              // +0x50 the map icon's scene entity (spawn_battle_icon 0x49bbb0; detach 0x4b6180 removes it)
    int party_a;                                // +0x54 first side (0x49e9c0, 0x514d40, 0x51abd0)
    int party_b;                                // +0x58 other side
    char pad5c[0x60 - 0x5c];
    Timer born;                                 // +0x60 clock 1 (0x48c210); bind_host re-stamps it, update_nav_anim 0x4b71e0 runs the simulation off it
    union {
        int f70;                                // +0x70 (the save writer 0x4e9ab0 writes the word)
        float next_simulation_time;             //       seconds until the next commit (1.0 from bind_host; party_set_next_battle_simulation_time 0x4a8181)
    };
    float f74;                                  // +0x74 (bind_host clears it)
    int height;                                 // +0x78 the summed unit counts of both sides (attach_party_center 0x49e9c0)
    char pad7c[0x80 - 0x7c];
    Timer anim;                                 // +0x80 clock 0 (0x48c210): the battle sound's clock (update_nav_anim)
    int sound;                                  // +0x90 the playing battle sound, -1 = none (0x48c260, bind_host)
    int f94;                                    // +0x94 (no function of this object touches it)
    Event();                                    // 0x48c210
    void write(struct stream_base* out);          // 0x4e9ab0 (char_sheet/save_records.cpp)
    __declspec(noinline) void read(stream_base* s, int arg);   // 0x51e970 (party/game.cpp: the save reader, after the pool read its in-use flag and serial)
    virtual void slot1();                       // 1: 0x48c260 (init)
    // the battle-record members (src/game/party2/party_2.cpp, which called the record BattleRec)
    void bind_host();                           // 0x4b2770 re-seat on the map: faces, height, clocks
    void detach();                              // 0x4b6180 unwind the battle and release the record
    void commit();                              // 0x4b6870 run one simulation step
    void update_nav_anim(float unused);           // 0x4b71e0 per-frame sound, simulation clock and icon opacity
};
MB_LAYOUT32_ASSERT(assert_Event_size, sizeof(Event) == 0x98);

#endif // GAME_EVENT_H
