// skill_cap.h: the per-skill record table (0x44 bytes each, g_skill_tbl at 0x8ee3100): the level cap at +0x34 is the only
// field read (Troop::nibble 0x4351a0 in src/game/agent, GameData::skill_level 0x437b20 in src/game/agent2). ONE definition
// for src/game. Names provisional.
#ifndef GAME_SKILL_CAP_H
#define GAME_SKILL_CAP_H
struct SkillCap { char pad[0x34]; int cap; char pad38[0x44 - 0x38]; };   // 0x44 bytes, cap at +0x34 (read by Troop::nibble)
extern SkillCap g_skill_tbl[];          // 0x8ee3100
#endif // GAME_SKILL_CAP_H
