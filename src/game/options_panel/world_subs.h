// world_subs.h: two sub-objects of the world object (g_world 0x5e292c, `Big` in world_app.cpp, `World` in hud.cpp), embedded at
// +0x63d0 and +0x6698. One definition for world_app.cpp (the constructor 0x4d3720 calls init 0x4be6b0 / 0x4d1360 on
// them) and hud.cpp (the destructor 0x4d51d0 calls dtor 0x4be6f0 / 0x4cb780 on the same offsets of the same object).
// The extents are the gaps up to the next member the constructor initialises (+0x667c, +0x67d8).
#ifndef GAME_OPTIONS_PANEL_WORLD_SUBS_H
#define GAME_OPTIONS_PANEL_WORLD_SUBS_H
struct Sub63d0 {
    char pad[0x667c - 0x63d0];
    void init();                                    // 0x4be6b0
    void dtor();                                    // 0x4be6f0
};
struct Sub6698 {
    char pad[0x67d8 - 0x6698];
    void init();                                    // 0x4d1360
    void dtor();                                    // 0x4cb780
};
#endif
