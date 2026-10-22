// Slice s004d8c20: the character export path helper, the settings object's record helpers and the per-stat
// accessor wrappers (0x4d9260..0x4d9450). Very provisional: these are the map / settings-panel helpers seen from
// their call sites; the exact signatures are not pinned down yet (see partial.txt).
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// The engine's 16-byte string (layout verified in s0042c0f0).
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const tstring&);                    // 0x42c110
    ~tstring();                                 // 0x42bc40
    tstring& operator=(const tstring&);         // 0x42bc50
    void assign(const char*);                   // 0x42bcb0
    char equals(const char*) const;             // 0x42bd10
    char equals(const tstring&) const;          // 0x42bdd0
    void to_lower();                            // 0x42c020
    void spaces_to_underscores();               // 0x42bfb0
    void underscores_to_spaces();               // 0x42bf80
    const char* c_str() const { return buf ? buf : (char*)&buf; }
};
struct Cls_5e2a18;                              // the attribute-name map, 0x5e2a18 (0x1c0010 bytes)
// name -> cell lookup on Cls_5e2a18; returns the cell with that name, or 0.  0x4c6010
void* __stdcall Cls_5e2a18_lookup(Cls_5e2a18* self, tstring* name);
// 0x4d10f0: destroys the record range in the object `thiz` (frees the tstring of each record).
void __cdecl FUN_004d10f0(void* thiz, int a, int last, int unused, int b);
// 0x4d11a0 / 0x4d1230 / 0x4d12c0: record-range helpers of the settings object (prov).
void __cdecl FUN_004d11a0(void* thiz, int a, int b);
void __cdecl FUN_004d1230(void* thiz, int a, int b);
void __cdecl FUN_004d12c0(void* thiz, int a, int b);
// 0x4d4140: the settings panel's handler (thiscall; the hidden this is 0x8ee3c48).
int __cdecl FUN_004d4140(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern int g_game;                               // 0x5e2930 (the global game object)

// ---- the game's hash table<int, Glyph> (same table class as s0043ac40's IntHash) --------------------------
// The value is a 0x1c-byte glyph record: the uv quad and the metrics (see s004520d0's Glyph).
struct Rec { float u0, v0, u1, v1, xoff, yoff, xadv; };
struct HNode {
    HNode* next;                                 // +0
    HNode* prev;                                 // +4
    int    key;                                  // +8
    Rec    value;                                // +0xc
};
struct HIter {
    HNode* p;
    HIter() {}
    HIter(HNode* q) : p(q) {}
    HIter(const HIter& o) : p(o.p) {}
};
struct HOut { HNode* node; char inserted; };
struct node_pair { int key; Rec value; };        // 0x20 (the value_type)

struct HashTab {
    char pad0[8];
    HNode* end_;                                 // +8  (list sentinel)
    char pad0c[0x24 - 0xc];
    HIter find(const int& key);                  // 0x43b210 (hidden result, ret 8)
    void insert(HOut* out, const int* key);      // 0x4d3c00 (ret 8; the original insert returns `out` in eax)
    Rec* at(const int& key);                     // 0x4d92c0 (the original symbol is operator[])
};

// @ 0x4d92c0
Rec* HashTab::at(const int& key)
{
    HIter it = find(key);
    if (it.p == end_) {
        HOut out;
        node_pair v = { key, Rec() };
        insert(&out, &v.key);
        return &out.node->value;
    }
    return &it.p->value;
}

// ---- std::vector<E68>::erase(first, last) (element 0x68, iterator returned through a hidden pointer) ----
struct E68 { char pad[0x68]; };
struct vec_it {
    E68* p;
};
struct ptr_tag { };                              // std::_Nonscalar_ptr_iterator_tag
E68*  copy_opt(E68* first, E68* last, E68* dest, ptr_tag tag);            // 0x4cd530
void  destroy_range(E68* first, E68* last, void* alloc, ptr_tag tag);     // 0x4cd570
struct E68vec {
    char pad0[8];
    E68* last;                                   // +8
    vec_it erase(vec_it first, vec_it last);     // 0x4d9390
};

// @ 0x4d9390
vec_it E68vec::erase(vec_it first, vec_it last)
{
    if (first.p != last.p) {
        ptr_tag tag;
        E68* p = copy_opt(last.p, this->last, first.p, tag);
        destroy_range(p, this->last, this, tag);
        this->last = p;
    }
    return first;
}

// @ 0x4d9260
// sprintf(out, "CharExport\\%s.txt", <active character's name>);
void __stdcall FUN_004d9260(char* out)
{
    int c = *(int*)(g_game + 0x8c) * 0x508 + *(int*)(g_game + 0x13a00);
    void* e = Cls_5e2a18_lookup((Cls_5e2a18*)0x5e2a18, (tstring*)(c + 0x20));
    if (e == 0) e = (void*)c;
    char* name = (char*)e + 4;
    if (*(char**)name != 0) name = *(char**)name;
    sprintf(out, "CharExport\\%s.txt", name);
}

// @ 0x4d9370
// Destroys the record range ending at `last`, then writes it back (hidden-this dispatch).
void __stdcall FUN_004d9370(void* thiz, int first, int last)
{
    FUN_004d10f0(thiz, first, last, first, last);
}

// @ 0x4d93e0
void __stdcall FUN_004d93e0(void* thiz, int a, int b)
{
    FUN_004d11a0(thiz, a, b);
}

// @ 0x4d9400
void __stdcall FUN_004d9400(void* thiz, int a, int b)
{
    FUN_004d1230(thiz, a, b);
}

// @ 0x4d9420
void __stdcall FUN_004d9420(void* thiz, int a, int b)
{
    FUN_004d12c0(thiz, a, b);
}

// @ 0x4d9440
// Settings panel window procedure: enters the shared handler with the panel as `this`.
int __stdcall FUN_004d9440(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return FUN_004d4140(hwnd, msg, wParam, lParam);
}

// ---- character import reader (0x4d8c20) --------------------------------------------------------------
// A skill record in the global skill table: name at +0, 0x44 bytes per entry (42 entries, 0x8ee3110).
struct skill_rec {
    tstring name;
    char    pad[0x34];
};
extern skill_rec g_skills[42];                   // 0x8ee3110
extern int       g_xp_table[];                   // 0x5937c8

// 0x432510: clamp *v into [*lo, *hi) and return it.
int __cdecl clamp_index(const int* v, const int* lo, const int* hi);
// 0x48bf90: sets skill `idx` of the troop to `value`.
void __fastcall set_skill(void* troop, int dummy, int idx, int value);

struct troop_rec {
    tstring name;                                // +0x00
    char    pad10[0x48];                         // +0x10
    int     strength;                            // +0x58
    int     agility;                             // +0x5c
    int     intelligence;                        // +0x60
    int     charisma;                            // +0x64
    char    pad68[0x18];                         // +0x68
    float   weapons[7];                          // +0x80
    int     xp;                                  // +0x9c
    int     pad_a0;                              // +0xa0
    int     skill_points;                        // +0xa4
    int     attribute_points;                    // +0xa8
    int     weapon_points;                       // +0xac
    int     level;                               // +0xb0
    char    pad_b4[0x300];                       // +0xb4
    int     money;                               // +0x3b4
    char    pad_3b8[0x10];                       // +0x3b8
    unsigned __int64 face_key;                   // +0x3c8
};

struct import_entry {
    tstring name;                                // +0x00
    char    pad10[0x0c];                         // +0x10
    tstring value;                               // +0x1c
};
struct import_data {
    import_entry entries[4096];                  // 0x2c000 bytes
    int          count;                          // +0x2c000
};

struct Game {
    void import_char(int data);                  // 0x4d8c20
};

static const char* const weapon_names[7] = {
    "one_handed_weapons", "two_handed_weapons", "polearms", "archery",
    "crossbows", "throwing", "firearms"
};

// @ 0x4d8c20
void Game::import_char(int data)
{
    int idx = *(int*)((char*)this + 0x8c);
    troop_rec* troop = (troop_rec*)(*(int*)((char*)this + 0x13a00) + idx * 0x508);
    import_data* imp = (import_data*)data;

    for (int i = 0; i < imp->count; i++) {
        import_entry* e = &imp->entries[i];
        tstring key = e->name;
        tstring value = e->value;

        if (key.equals("name")) {
            value.underscores_to_spaces();
            troop->name = value;
            continue;
        }
        if (key.equals("xp")) {
            int xp = atoi(value.c_str());
            if (xp < 0) {
                xp = 0;
            } else if (xp >= 0x26635cb0) {
                xp = 0x26635caf;
            }
            troop->xp = xp;
            troop->level = 1;
            int j = 1;
            while (j < 0x40) {
                if (xp <= g_xp_table[j]) break;
                troop->level = j;
                if (xp <= g_xp_table[j + 1]) break;
                troop->level = j + 1;
                if (xp <= g_xp_table[j + 2]) break;
                j += 3;
                troop->level = j - 1;
            }
            continue;
        }
        if (key.equals("money")) {
            troop->money = atoi(value.c_str());
            continue;
        }
        if (key.equals("attribute_points")) {
            troop->attribute_points = atoi(value.c_str());
            continue;
        }
        if (key.equals("skill_points")) {
            troop->skill_points = atoi(value.c_str());
            continue;
        }
        if (key.equals("weapon_points")) {
            troop->weapon_points = atoi(value.c_str());
            continue;
        }
        if (key.equals("strength")) {
            troop->strength = atoi(value.c_str());
            continue;
        }
        if (key.equals("agility")) {
            troop->agility = atoi(value.c_str());
            continue;
        }
        if (key.equals("intelligence")) {
            troop->intelligence = atoi(value.c_str());
            continue;
        }
        if (key.equals("charisma")) {
            troop->charisma = atoi(value.c_str());
            continue;
        }
        if (key.equals("face_key_1")) {
            sscanf(value.c_str(), "%I64x", &troop->face_key);
            continue;
        }
        if (key.equals("face_key_2")) {
            sscanf(value.c_str(), "%I64x", &troop->face_key);
            continue;
        }

        // Anything else is a skill / weapon proficiency name.
        key.to_lower();
        bool found = false;
        tstring tmp;
        for (int j = 0; j < 42 && !found; j++) {
            tmp = g_skills[j].name;
            tmp.spaces_to_underscores();
            tmp.to_lower();
            if (tmp.equals(key)) {
                int v = atoi(value.c_str());
                int lo = 0, hi = 0xb;
                int r = clamp_index(&v, &lo, &hi);
                set_skill(troop, 0, j, r);
                found = true;
            }
        }
        if (!found) {
            tstring wname;
            for (int j = 0; j < 7 && !found; j++) {
                wname.assign(weapon_names[j]);
                wname.spaces_to_underscores();
                if (wname.equals(key)) {
                    int v = atoi(value.c_str());
                    int lo = 0, hi = 0x2bc;
                    int r = clamp_index(&v, &lo, &hi);
                    troop->weapons[j] = (float)r;
                    found = true;
                }
            }
        }
    }
}
