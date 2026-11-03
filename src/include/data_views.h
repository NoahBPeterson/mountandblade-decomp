// data_views.h: the names of the original's globals that still lie INSIDE a bigger object whose real type has no member for them yet
// (the music player at 0xa73a10, the meta mission at 0xa73300, the clock table's step row at 0x5e2970, the shadow map's scene).
// Each is replaced by a real member of the outer type when it gets one.
#ifndef MB_DATA_VIEWS_H
#define MB_DATA_VIEWS_H

extern char g_ovl_005e2948[];   // 0x005e2948
extern char g_ovl_00a73300[];   // 0x00a73300
extern char g_ovl_00a73a10[];   // 0x00a73a10
extern char g_ovl_00a76630[];   // 0x00a76630
struct ShadowMap;
extern ShadowMap g_shadow_map;         // (defined in game_state.cpp)

#define g_ticks_dt ((__int64*)(g_ovl_005e2948 + 0x28))   // 0x005e2970
#define g_a70260 ((char*)((char*)&g_shadow_map + 0x18))   // 0x00a70260
#define g_battle_advantage (*(int*)(g_ovl_00a73300 + 0x1c))   // 0x00a7331c
#define g_battle_flags (*(unsigned*)(g_ovl_00a73300 + 0x164))   // 0x00a73464
#define g_a76644 (*(unsigned*)(g_ovl_00a76630 + 0x14))   // 0x00a76644 (Terrain::flags14; src/game/cls_map cannot include terrain.h)
#define g_track_count (*(int*)(g_ovl_00a73a10 + 0x4))   // 0x00a73a14

#endif // MB_DATA_VIEWS_H
