// The character-file writer (0x4d86d0). It is a member of the game-data object (ecx = g_game, 0x5e2930):
// it takes the player troop record (player index at +0x8c, records at +0x13a00, stride 0x508) and emits one
// " key = value\n" line per field through the stream's +0x28 virtual (stream is the single stack argument,
// `ret 4`). Names and layouts are provisional, derived from the disassembly.
//
// Match status: complete but NOT byte-exact, and cannot be: the original carries two 18-byte binary
// instrumentation blocks (`jmp +0x10`, then 16 bytes "WL  " <n> 0 "WL  ") at 0x4d86da and 0x4d8bda, which
// no C++ source makes cl 13.10 emit (328 such blocks exist across the image; they also explain the only
// 36-byte size difference). Everything else reproduces: the source compiles to exactly the original's
// instructions and length once those two blocks are removed (1312 bytes vs 1348).
#include <stdio.h>
#include <string.h>
#include <windows.h>

// ---- tstring (16 bytes; see s0046ad50/tstring.h and symbols/slices/s0042b790.txt) ------------------------
struct tstring {
    int pos;                                                        // +0  read cursor
    char* buf;                                                      // +4  (null while empty)
    int cap;                                                        // +8
    int len;                                                        // +0xc
    tstring();                                                      // 0x42bbd0
    tstring(const tstring& o);                                      // 0x42c110
    ~tstring();                                                     // 0x42bc40
    void assign(const tstring& o);                                  // 0x42bc50
    void assign(const char* s);                                     // 0x42bcb0
    void spaces_to_underscores();                                   // 0x42bfb0
    void to_lower();                                                // 0x42c020
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// ---- the stream the writer fills (0x4d7d10/s004d7d10's stream_root) --------------------------------------
struct stream_text {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24();
    virtual void write(const char* text);                           // +0x28
};

// ---- skill table: 42 records of 0x44 bytes, name (tstring) first (0x8ee3110 .. 0x8ee3c38) ----------------
struct skill_entry {
    tstring name;                                                   // +0
    char pad10[0x44 - 0x10];
};
extern skill_entry g_skills[];                                      // 0x8ee3110

// ---- troop records (0x508 bytes each; array pointer at g_game+0x13a00) ----------------------------------
struct troop_rec {
    tstring name;                                                   // +0x00
    char pad10[0x58 - 0x10];
    int  strength;                                                  // +0x58
    int  agility;                                                   // +0x5c
    int  intelligence;                                              // +0x60
    int  charisma;                                                  // +0x64
    char pad68[0x80 - 0x68];
    float weapon[7];                                                // +0x80 (weapon proficiency per class)
    int  xp;                                                        // +0x9c
    char pada0[0xa4 - 0xa0];
    int  skill_points;                                              // +0xa4
    int  attribute_points;                                          // +0xa8
    int  weapon_points;                                             // +0xac
    char padb0[0x3b4 - 0xb0];
    int  money;                                                     // +0x3b4
    char pad3b8[0x3c0 - 0x3b8];
    __int64 face_key_1;                                             // +0x3c0
    __int64 face_key_2;                                             // +0x3c8
    char pad3d0[0x508 - 0x3d0];
    int  skill_level(int skill);                                    // 0x4351a0 (ecx = troop, ret 4)
};

struct game_data {
    char pad0[0x8c];
    int  player_troop;                                              // +0x8c
    char pad90[0x13a00 - 0x90];
    troop_rec* troops;                                              // +0x13a00
    void char_export_write(stream_text* stream);                    // 0x4d86d0
};
extern game_data* g_game;                                           // 0x5e2930

void mb_snprintf(char* buf, unsigned int n, const char* fmt, ...);  // 0x415100

// @ 0x4d86d0
void game_data::char_export_write(stream_text* stream)
{
    char line[0x400];
    troop_rec* troop = &troops[player_troop];

    sprintf(line, "%s = %ld\n", " charfile_version", 1);
    stream->write(line);

    tstring name(troop->name);
    name.spaces_to_underscores();
    mb_snprintf(line, 0x400, "%s = %s\n", "name", name.c_str());
    stream->write(line);

    sprintf(line, "%s = %ld\n", "xp", troop->xp);
    stream->write(line);
    sprintf(line, "%s = %ld\n", "money", troop->money);
    stream->write(line);
    stream->write("\n");

    sprintf(line, "%s = %ld\n", "attribute_points", troop->attribute_points);
    stream->write(line);
    sprintf(line, "%s = %ld\n", "skill_points", troop->skill_points);
    stream->write(line);
    sprintf(line, "%s = %ld\n", "weapon_points", troop->weapon_points);
    stream->write(line);
    stream->write("\n");

    sprintf(line, "%s = %ld\n", "strength", troop->strength);
    stream->write(line);
    sprintf(line, "%s = %ld\n", "agility", troop->agility);
    stream->write(line);
    sprintf(line, "%s = %ld\n", "intelligence", troop->intelligence);
    stream->write(line);
    sprintf(line, "%s = %ld\n", "charisma", troop->charisma);
    stream->write(line);
    stream->write("\n");

    tstring skill;
    for (int i = 0; i < 42; i++) {
        skill.assign(g_skills[i].name);
        skill.spaces_to_underscores();
        skill.to_lower();
        sprintf(line, "%s = %ld\n", skill.c_str(), troop->skill_level(i));
        stream->write(line);
    }
    stream->write("\n");

    char weapon_text[0x100];
    tstring weapon_name;
    for (int i = 0; i < 7; i++) {
        switch (i) {
        case 0: strcpy(weapon_text, "one_handed_weapons"); break;
        case 1: strcpy(weapon_text, "two_handed_weapons"); break;
        case 2: strcpy(weapon_text, "polearms"); break;
        case 3: strcpy(weapon_text, "archery"); break;
        case 4: strcpy(weapon_text, "crossbows"); break;
        case 5: strcpy(weapon_text, "throwing"); break;
        case 6: strcpy(weapon_text, "firearms"); break;
        }
        weapon_name.assign(weapon_text);
        weapon_name.spaces_to_underscores();
        sprintf(line, "%s = %ld\n", weapon_name.c_str(), (int)troop->weapon[i]);
        stream->write(line);
    }
    stream->write("\n");

    sprintf(line, "%s = %I64x\n", "face_key_1", troop->face_key_1);
    stream->write(line);
    sprintf(line, "%s = %I64x\n", "face_key_2", troop->face_key_2);
    stream->write(line);
}
