// eff_obj.h: the effect object (the wind / light record, 0x7c bytes) that the world's scene container hands out through
// 0x478f30 (EffMgr::create, CellGrid::spawn_wind). ONE definition for src/game/world_data (world_data.cpp: the wind spawn)
// and src/game/char_sheet (terrain_page_update.cpp: the flash light of the terrain page); layout from s00494e40 / 0x4952f0.
// Needs rgl::vector4 (16 bytes) declared by the includer's engine header.
#ifndef MB_GAME_EFF_OBJ_H
#define MB_GAME_EFF_OBJ_H

struct EffObj {
    int f0;                                                     // +0x00
    rgl::vector4 pos;                                           // +0x04
    rgl::vector4 f14;                                           // +0x14
    rgl::vector4 f24;                                           // +0x24
    char pad34[0x44 - 0x34];
    rgl::vector4 f44;                                           // +0x44
    float f54;                                                  // +0x54 (the first dword of f44's successor)
    char pad58[0x70 - 0x58];
    int f70;                                                    // +0x70
    int f74;                                                    // +0x74
    char pad78[0x7c - 0x78];
};
struct EffMgr { EffObj* create(); };                            // 0x478f30 (CellGrid::spawn_wind)

#endif // MB_GAME_EFF_OBJ_H
