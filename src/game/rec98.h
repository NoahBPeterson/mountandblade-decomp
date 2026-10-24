// rec98.h: the 0x98-byte record of the pool whose pages of 16 sit at GameData+0xe4 (the map battle / event records).
// One definition for the views world_data/world_data.h (RecPool::alloc_record 0x493700: the pool bookkeeping words,
// constructor 0x48c210), party2/camera.cpp (Mission::spawn_battle_icon 0x49bbb0, bind_host 0x4b2770),
// inventory_panel/mission_ctx.cpp (the map screen's party pick, 0x514d40) and inventory_panel/map_info.cpp (the panel
// text; there the slice called it MapAgent: AgentPages::slot is 0x48ef20 on GameData+0xcc). Same class by:
//   * every view indexes it as pages[id >> 4] + (id & 0xf) * 0x98 over the page table at GameData+0xe4 (0x493700 builds the
//     pages with `new Rec98[16]`, 0x49bbb0 / 0x514d40 / 0x51abd0 read them back);
//   * the offsets overlap without conflict (+0x10 type, +0x54 / +0x58 the two sides) and 0x4b2770 is the same method
//     in party2/party_2.h (`BattleRec::bind_host`) and stat_panel_x.h (`Event`, constructor 0x48c210): the same class under
//     two more names, left to their own families.
#ifndef GAME_REC98_H
#define GAME_REC98_H
struct Rec98 {
    int pad0;
    int f4;                                         // +0x04 pool: 1 while the record is allocated (0x493700)
    int f8;                                         // +0x08 pool: the record's own id (0x493700)
    int fc;                                         // +0x0c pool: serial number (0x493700)
    int f10;                                        // +0x10 (=1 -> icon owner present; 0x49bbb0, 0x514d40)
    char pad14[0x24 - 0x14];
    float f24;                                      // +0x24 (x; 0x49bbb0)
    float f28;                                      // +0x28 (y)
    char pad2c[0x3c - 0x2c];
    float f3c;                                      // +0x3c (z base)
    char pad40[0x50 - 0x40];
    int f50;                                        // +0x50 (scene entity; 0x49bbb0)
    int f54;                                        // +0x54 (agent id A / first side; 0x514d40, 0x51abd0)
    int f58;                                        // +0x58 (agent id B / other side)
    char pad5c[0x98 - 0x5c];
    Rec98();                                        // 0x48c210
    ~Rec98();
    void bind_host();                               // 0x4b2770 (thiscall)
};
typedef char assert_Rec98_size[sizeof(Rec98) == 0x98 ? 1 : -1];
#endif
