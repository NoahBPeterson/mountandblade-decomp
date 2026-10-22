// Slice s005236f0 (batch ds1): Game::load_game_data -- one 3172-byte loader that opens, in order,
// factions.txt, skills.txt, Data\skeleton_bodies.xml, troops.txt, scenes.txt, quests.txt, triggers.txt
// and simple_triggers.txt, rebuilding the corresponding record arrays, then copies the six combat
// parameter records out of the constants and appends one simple trigger per script emitter.
//
// The shape is the same as s0050d5e0's module loader: the two scratch tstrings and every stream are
// declared in one scope, each block ends with `stream.failed = 1;` and all streams are destroyed in
// reverse declaration order at the end. `this` is the Game/GameState object (layout shared with
// s0051f1a0 and s0051d920; offsets read off this slice's disassembly). Names are provisional.
#include <windows.h>
#include <stdio.h>
#include "rgl.h"

// ---- tstring (16 bytes; 0x42bbd0 / 0x42bbe0 / 0x42bc40) --------------------------------------------------
struct tstring {
    int pos;                                             // +0
    char* buf;                                           // +4
    int cap;                                             // +8
    int len;                                             // +0xc
    tstring();                                           // 0x42bbd0
    tstring(const char* s);                              // 0x42bbe0
    ~tstring();                                          // 0x42bc40
};

void mb_error(const char* fmt, ...);                     // 0x42cc80

// ---- text streams (three-level hierarchy 0x54a740 -> 0x54a890 -> 0x54ac88) -------------------------------
struct stream_root {
    virtual void s00();                                  // +0x00
    virtual void s01();
    virtual stream_root& s02(stream_root* o);
    virtual stream_root& s03(float* v);
    virtual stream_root& s04(char* v);
    virtual stream_root& s05(unsigned __int64* v);
    virtual stream_root& s06(__int64* v);
    virtual stream_root& s07(unsigned* v);
    virtual stream_root& s08(int* v);
    virtual stream_root& s09(unsigned char* v);
    virtual stream_root& s10(const char* s);
    virtual stream_root& s11(const tstring* s);
    virtual stream_root& s12(void* v);
    virtual stream_root& s13(stream_root* o);
    virtual stream_root& r_float(float* v);             // 14
    virtual stream_root& r_bool(char* v);               // 15
    virtual stream_root& r_uint(unsigned* v);           // 16
    virtual stream_root& r_u64(unsigned __int64* v);    // 17
    virtual stream_root& r_i64(__int64* v);             // 18
    virtual stream_root& r_int(int* v);                 // 19 -> +0x4c   0x415930
    virtual stream_root& r_byte(char* v);               // 20
    virtual stream_root& r_tstr(tstring* v);            // 21 -> +0x54   0x415880
    tstring name;                                        // +0x04
};

struct tstream2 : stream_root {                          // vtable 0x54a890
    int  state;                                          // +0x14
    char autoflush;                                      // +0x18
    int  f1c;                                            // +0x1c
    tstring buf;                                         // +0x20
};

struct text_stream : tstream2 {                          // vtable 0x54ac88
    FILE* fp;                                            // +0x30
    char  failed;                                        // +0x34
    text_stream()
    {
        autoflush = 0;
        f1c = 0;
        failed = 1;
        fp = 0;
        state = 0;
    }
    ~text_stream() {}
    virtual stream_root& r_int(int* v);                  // 0x415930
    virtual stream_root& r_tstr(tstring* v);             // 0x415880
    bool open_read_text(const char* path);               // 0x415d60 (ret 4)
};

// ---- records --------------------------------------------------------------------------------------------
struct Faction {                                         // 0x300 bytes
    char data[0x300];
    Faction();                                           // 0x5249f0
    ~Faction();                                          // 0x432010
    void read(text_stream* s, int count);                // 0x524a70 (ret 8)
};

struct TroopRec {                                        // 0x508 bytes
    char data[0x508];
    TroopRec();                                          // 0x524710
    ~TroopRec();                                         // 0x431c40
    void read(text_stream* s);                           // 0x524420 (ret 4)
};

struct SceneRec {                                        // 0x494 bytes, polymorphic (vtable 0x590904)
    virtual ~SceneRec();                                 // scalar/vector deleting destructor
    char data[0x490];
    SceneRec();                                          // 0x5248e0
    void read(text_stream* s);                           // 0x524d60 (ret 4)
};

struct QuestRec {                                        // 0x138 bytes
    char data[0x138];
    QuestRec();                                          // 0x524860
    ~QuestRec();                                         // 0x431b10
    void read(text_stream* s);                           // 0x524690 (ret 4)
};

struct SkillRec {                                        // 0x44 bytes, table at 0x8ee3100
    char pad0[0x34];
    int  f34;                                            // +0x34
    int  f38;                                            // +0x38
    int  pad3c;                                          // +0x3c
    int  f40;                                            // +0x40
};
void read_skill(text_stream* s, SkillRec* r);            // 0x5243c0 (cdecl)

// ---- trigger vectors ------------------------------------------------------------------------------------
struct TriggerVec {                                      // at Game+0x139e8
    int  count;                                          // +0
    void* array;                                         // +4
    int  f8;                                             // +8
    void resize(text_stream* s, const char* name);       // 0x4c1c90 (ret 8)
};
struct GameVec {                                         // at Game+0x139f4
    int  count;                                          // +0
    void* array;                                         // +4
    int  f8;                                             // +8
    int  resize(text_stream* s, const char* name, int extra);   // 0x4c1e10 (ret 0xc)
};

// a 0x30-byte simple-trigger record: int at +0 followed by the 0x2c-byte statement at +4
struct TriggerStmt {
    int  a;                                              // +0
    char pad4[0x2c - 4];
    void init(int op, int idx);                          // 0x524360 (ret 8)
};
struct SimpleTrigger {
    int         f0;                                      // +0 (the emitter delay, bit-copied)
    TriggerStmt stmt;                                    // +4
};

// ---- script emitters (0xa75eb0 array, count 0xa75eb4) ----------------------------------------------------
struct Emitter {                                         // 0x34 bytes
    char  pad0[0x14];
    float f14;                                           // +0x14
    char  pad18[0x34 - 0x18];
};
extern Emitter* g_emitters;                              // 0xa75eb0
extern int      g_emitter_count;                         // 0xa75eb4
// skill table at the absolute address 0x8ee3100 (kept in the symbol name so the equivalence
// resolver's hex-name path can find it; the table itself lives in the high .data section)
extern SkillRec g_08ee3100[42];

// ---- skeleton-body table at 0xa180e8 --------------------------------------------------------------------
struct SkeletonList {
    void load_bodies(tstring* path);                     // 0x50fc60 (ret 4)
    void fix();                                          // 0x4cd850
};
extern SkeletonList g_skeletons;                         // 0xa180e8

// ---- the game object ------------------------------------------------------------------------------------
struct Game {
    char       pad0[0x139e8];
    TriggerVec triggers;                                 // +0x139e8
    GameVec    simple;                                   // +0x139f4
    TroopRec*  troops;                                   // +0x13a00
    int        troop_count;                              // +0x13a04
    char       pad13a08[0x13a48 - 0x13a08];
    Faction*   factions;                                 // +0x13a48
    int        faction_count;                            // +0x13a4c
    SceneRec*  scenes;                                   // +0x13a50
    int        scene_count;                              // +0x13a54
    QuestRec*  quests;                                   // +0x13a58
    int        quest_count;                              // +0x13a5c
    char       pad13a60[0x13b64 - 0x13a60];
    float      combat[24];                               // +0x13b64 six 4-float records

    const char* module_path(const char* name);           // 0x4639d0 (ret 4)
    void load_game_data(int extra_triggers);             // 0x5236f0 (ret 4)
};

// =========================================================================================================
// @ 0x5236f0    void Game::load_game_data(int extra_triggers)   (thiscall, ret 4)
void Game::load_game_data(int extra_triggers)
{
    tstring str1;                                        // +0x38
    tstring str2;                                        // +0x28

    // ---- factions.txt -----------------------------------------------------------------------------------
    text_stream s_factions;                              // +0x80
    {
        const char* path = module_path("factions.txt");
        if (!s_factions.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    stream_root& in_factions = s_factions;
    in_factions.r_tstr(&str1);
    in_factions.r_tstr(&str2);
    int version;
    in_factions.r_int(&version);
    in_factions.r_int(&faction_count);
    if (faction_count > 0x80)
        mb_error("Too Many Factions: %d (Max should be %d)", faction_count, 0x80);
    delete[] factions;
    factions = 0;
    factions = new Faction[faction_count];
    for (int i = 0; i < faction_count; i++)
        factions[i].read(&s_factions, faction_count);
    s_factions.failed = 1;

    // ---- skills.txt -------------------------------------------------------------------------------------
    text_stream s_skills;                                // +0x198
    {
        const char* path = module_path("skills.txt");
        if (!s_skills.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    int skill_count;
    s_skills.r_int(&skill_count);
    int i;
    for (i = 0; i < skill_count && i < 42; i++)
        read_skill(&s_skills, &g_08ee3100[i]);
    for (; i < 42; i++) {
        g_08ee3100[i].f34 = 0;
        g_08ee3100[i].f38 = 0;
        g_08ee3100[i].f40 = 0x100;
    }
    s_skills.failed = 1;

    // ---- Data\skeleton_bodies.xml -----------------------------------------------------------------------
    {
        tstring path("Data\\skeleton_bodies.xml");
        g_skeletons.load_bodies(&path);
    }
    g_skeletons.fix();

    // ---- troops.txt -------------------------------------------------------------------------------------
    text_stream s_troops;                                // +0xb8
    {
        const char* path = module_path("troops.txt");
        if (!s_troops.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    s_troops.r_tstr(&str1);
    s_troops.r_tstr(&str2);
    s_troops.r_int(&version);
    s_troops.r_int(&troop_count);
    delete[] troops;
    troops = 0;
    troops = new TroopRec[troop_count];
    for (int j = 0; j < troop_count; j++)
        troops[j].read(&s_troops);
    s_troops.failed = 1;

    // ---- scenes.txt -------------------------------------------------------------------------------------
    text_stream s_scenes;                                // +0xf0
    {
        const char* path = module_path("scenes.txt");
        if (!s_scenes.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    s_scenes.r_tstr(&str1);
    s_scenes.r_tstr(&str2);
    s_scenes.r_int(&version);
    s_scenes.r_int(&scene_count);
    delete[] scenes;
    scenes = 0;
    scenes = new SceneRec[scene_count];
    for (int k = 0; k < scene_count; k++)
        scenes[k].read(&s_scenes);
    s_scenes.failed = 1;

    // ---- quests.txt -------------------------------------------------------------------------------------
    text_stream s_quests;                                // +0x48
    {
        const char* path = module_path("quests.txt");
        if (!s_quests.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    s_quests.r_tstr(&str1);
    s_quests.r_tstr(&str2);
    s_quests.r_int(&version);
    s_quests.r_int(&quest_count);
    delete[] quests;
    quests = 0;
    quests = new QuestRec[quest_count];
    for (int n = 0; n < quest_count; n++)
        quests[n].read(&s_quests);
    s_quests.failed = 1;

    // ---- triggers.txt -----------------------------------------------------------------------------------
    text_stream s_triggers;                              // +0x160
    {
        const char* path = module_path("triggers.txt");
        if (!s_triggers.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    s_triggers.r_tstr(&str1);
    s_triggers.r_tstr(&str2);
    s_triggers.r_int(&version);
    triggers.resize(&s_triggers, "Game Triggers");
    s_triggers.failed = 1;

    // ---- simple_triggers.txt ----------------------------------------------------------------------------
    text_stream s_simple;                                // +0x128
    {
        const char* path = module_path("simple_triggers.txt");
        if (!s_simple.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    s_simple.r_tstr(&str1);
    s_simple.r_tstr(&str2);
    s_simple.r_int(&version);
    int first = simple.resize(&s_simple, "Simple trigger", extra_triggers);
    s_simple.failed = 1;

    // one extra simple trigger per script emitter whose delay is non-negative
    if (g_emitter_count > 0) {
        Emitter* e = g_emitters;
        int off = first * 0x30;
        int idx = 0;
        do {
            if (e->f14 >= 0.0f) {
                SimpleTrigger* rec = (SimpleTrigger*)((char*)simple.array + off);
                rec->f0 = *(int*)&e->f14;
                rec->stmt.init(1, idx);
                off += 0x30;
            }
            idx++;
            e = (Emitter*)((char*)e + 0x34);
        } while (idx < g_emitter_count);
    }

    // ---- combat parameter records (six records of four floats) ------------------------------------------
    float* c = combat;
    c[2] = 3.0f;      c[3] = 3.0f;                       // record 0 tail
    c[16] = -100.0f;  c[17] = -50.0f;                    // record 4 head
    c[6] = 5.0f;      c[7] = 1.0f;                       // record 1 tail
    c[18] = 100.0f;   c[19] = -50.0f;                    // record 4 tail
    c[10] = 1.0f;     c[11] = -3.0f;                     // record 2 tail
    c[20] = -100.0f;  c[21] = 50.0f;                     // record 5 head
    c[14] = -5.0f;    c[15] = -3.0f;                     // record 3 tail
    c[22] = 100.0f;   c[23] = 50.0f;                     // record 5 tail
    c[0] = 23.0f;     c[1] = 0.2f;                       // record 0 head
    c[4] = 15.0f;     c[5] = 0.1f;                       // record 1 head
    c[8] = 11.0f;     c[9] = 0.9f;                       // record 2 head
    c[12] = 3.0f;     c[13] = 0.3f;                       // record 3 head
}
