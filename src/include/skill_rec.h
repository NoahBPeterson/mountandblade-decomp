// skill_rec.h: one skill of the skill table (0x8ee3100, stride 0x44) with its language-record accessors.
// ONE definition for src/rgl/mesh2 (gui_edit.cpp: translated 0x4c6100, desc_name 0x4c6190, translated_desc 0x4c61f0) and
// src/game/char_sheet (cs_detail.h: the level cap at +0x38 and the attribute nibble at +0x40, read by update_detail 0x4ef9f0).
#ifndef MB_SKILL_REC_H
#define MB_SKILL_REC_H
#include "../game/tstring.h"

struct SkillRec {
    tstring desc;                                   // +0x00
    tstring name;                                   // +0x10
    tstring key;                                    // +0x20
    int f30, f34;                                   // +0x30 / +0x34
    int cap;                                        // +0x38 (the level cap)
    int f3c;                                        // +0x3c
    unsigned attr;                                  // +0x40 (low nibble: 0-based attribute the skill needs)
    tstring translated();                           // 0x4c6100
    tstring* desc_name();                           // 0x4c6190 (the key's "_desc" translation)
    tstring translated_desc();                      // 0x4c61f0
};

#endif // MB_SKILL_REC_H
