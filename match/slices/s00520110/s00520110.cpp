// Mount & Blade 1.011 (mount&blade.exe) -- slice s00520110
//   00520110  TroopRec::apply_items   (refresh the two item-slot arrays of one troop: zero the stack
//                                      count of usable item kinds, restock the ammo kinds)
//   00520270  TroopRec::level_up      (recompute the two point budgets from the level, the attributes
//                                      and the skills, then spend them)
//   00520420  TroopRec::raise_skills  (spend the "hp" budget on random, not yet capped skills)
//   005204d0  TroopRec::raise_stats   (spend the "hp_max" budget on random attributes)
//
// Item slots are 8-byte records: {int kind_id; int flags;} where the low byte of flags is the stack
// count and the top byte is a quality/type tag.
//
// The 42 skills are stored as 4-bit fields in six words at +0x68; the per-skill cap table is a 0x44-byte
// record array at 0x8ee3100 with the cap at +0x34.

#include <windows.h>

typedef unsigned char byte;

// ---- the item table (0x5e29d0) -----------------------------------------------------------------------
struct ItemRec {                                    // 0x238-byte item kinds, table at 0x5e29d0
    char pad0[0x1c8];
    int  type;                                      // +0x1c8 (low byte = the type code)
    char pad1cc[0x210 - 0x1cc];
    int  f210;                                      // +0x210
};
extern char* g_items;                               // 0x5e29d0

static inline int item_type(int id)
{
    return *(int*)(g_items + id * 0x238 + 0x1c8) & 0xff;
}

// ---- one inventory slot (8 bytes) --------------------------------------------------------------------
struct ItemSlot {
    int  id;                                        // +0 (item kind, -1 = empty)
    union {
        int           flags;                        // +4 (low byte = count, high byte = quality tag)
        unsigned char num;                          // +4
    } v;
    int  free_space();                              // 0x434b70
    int  make();                                    // 0x434df0
    void take(int n);                               // 0x437510
};

// ---- the skill cap table (0x8ee3100) -----------------------------------------------------------------
struct SkillCap {                                   // 0x44 bytes
    char pad0[0x34];
    int  cap;                                       // +0x34
    char pad38[0x44 - 0x38];
};
extern SkillCap g_skill_tbl[];                      // 0x8ee3100

// The generic stat/skill setters of the character sheet; they take the troop in ecx.
struct StatPanelX {
    void set_stat(int idx, int val);                // 0x48be70
    void set_skill(int idx, int val);               // 0x48bf90
};

struct TroopRec {                                   // 0x508 bytes
    char     pad0[0x58];
    int      stats[4];                              // +0x58 strength, agility, intelligence, charisma
    unsigned skills[6];                             // +0x68 (42 4-bit skill levels)
    char     pad80[0xa4 - 0x80];
    int      hp;                                    // +0xa4 (skill points)
    int      hp_max;                                // +0xa8 (attribute points)
    char     padac[0xb0 - 0xac];
    int      level;                                 // +0xb0
    ItemSlot inv[0x60];                             // +0xb4 (0xb4 + 0x300 = 0x3b4)
    char     pad3b4[0x4a4 - 0x3b4];
    ItemSlot worn[10];                              // +0x4a4 (0x4a4 + 0x50 = 0x4f4)
    char     pad4f4[0x508 - 0x4f4];
    void     apply_items();                         // 0x520110
    void     level_up();                            // 0x520270
    void     raise_skills();                        // 0x520420
    void     raise_stats();                         // 0x5204d0
};

static __forceinline StatPanelX* panel(TroopRec* t) { return (StatPanelX*)t; }

// The clamped skill level: the stored 4-bit value, limited by the skill's own cap. The index type
// matters to the code generation (a signed index gets the signed /8 and %8 sequences).
template <class T>
static __forceinline int get_skill(TroopRec* t, T i)
{
    int v = (t->skills[i / 8] >> ((i % 8) * 4)) & 0xf;
    if (v < 0)
        return 0;
    int cap = g_skill_tbl[i].cap;
    if (v > cap)
        return cap;
    return v;
}

// The original inlines the per-slot body, which is why the slot id and the item type are each read
// twice; keep both reads so the same code shape comes out.
static __forceinline void init_slot(ItemSlot* s)
{
    int id = *(volatile int*)&s->id;
    if (id <= 0)
        return;
    int ty = item_type(id);
    if (ty == 10 || ty == 5 || ty == 6 || ty == 0x10 || ty == 0x11 || ty == 9 || ty == 0x12) {
        s->v.num = 0;
    } else if (ty == 7 || ty == 1 || ty == 0x13) {
        int r = s->make();
        *(short*)&s->v.flags = 0;
        s->v.flags |= r;
    }
}

// @ 0x520110 -----------------------------------------------------------------------------------------
void TroopRec::apply_items()
{
    ItemSlot* s = inv;
    for (int i = 0; i < 0x60; i++, s++) {
        if (s->id <= 0)
            continue;
        {
            int ty = item_type(s->id);
            if (!(ty == 9 || ty == 0x11 || ty == 0x10))
                continue;
        }
        init_slot(s);
    }

    s = worn;
    for (int i = 0; i < 10; i++, s++) {
        if (s->id <= 0)
            continue;
        {
            int ty = item_type(s->id);
            if (!(ty == 9 || ty == 0x11 || ty == 0x10))
                continue;
        }
        init_slot(s);
    }
}

// @ 0x520270 -----------------------------------------------------------------------------------------
void TroopRec::level_up()
{
    // The original writes the running value back after every subtraction (the compiler only keeps
    // those stores when the location is volatile), so model it that way.
    volatile int* hp_max_p = &hp_max;
    int v = level + 0x14;
    *hp_max_p = v;
    v -= stats[0]; *hp_max_p = v;
    v -= stats[1]; *hp_max_p = v;
    v -= stats[2]; *hp_max_p = v;
    v -= stats[3]; *hp_max_p = v;
    if (v < 0)
        *hp_max_p = 0;

    hp = level + 3;
    for (unsigned i = 0; i < 0x2a; i += 6)
        for (unsigned j = 0; j < 6; j++)
            hp -= get_skill(this, i + j);
    if (hp < 0)
        hp = 0;

    raise_stats();                                  // 0x5204d0
    raise_skills();                                 // 0x520420 (tail call)
}

// @ 0x520420 -----------------------------------------------------------------------------------------
void TroopRec::raise_skills()
{
    int n = 0;
    for (;;) {
        int s = rand() % 0x2a;                      // 0x536a9f
        if (hp < 1 || get_skill(this, s) >= g_skill_tbl[s].cap) {
            if (hp <= 0 || ++n > 10000)
                break;
            continue;
        }
        panel(this)->set_skill(s, get_skill(this, s) + 1);
        hp--;
    }
}

// @ 0x5204d0 -----------------------------------------------------------------------------------------
void TroopRec::raise_stats()
{
    while (hp_max) {
        int i = rand() % 4;                         // 0x536a9f
        panel(this)->set_stat(i, stats[i] + 1);
        hp_max--;
    }
}
