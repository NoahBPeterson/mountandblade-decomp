// Slice s004c8ca0: exports the game's language data to CSV files under new_language\.
// Reconstruction of GameData::save_language_files(bool).  Names are provisional; layouts were
// derived from the disassembly of this function and the helper functions it calls.
#include <windows.h>
#include <stdio.h>
#include "tstring.h"

// ---------------------------------------------------------------------------------------------------------
// translation table (global at 0x5e2a18): string lookup + "key|value\n" writer
struct LangTable {
    tstring* find(const tstring* key);                                  // 0x4c6010
    void write(FILE* f, const tstring& key, const tstring& value);      // 0x4bd740
};
extern LangTable g_lang;                                                // 0x5e2a18

static inline void write_row(FILE* f, const tstring& key, const tstring& value)
{
    fputs(key.c_str(), f);                                              // 0x537ef4
    fputs("|", f);                                                      // 0x5946f8
    fputs(value.c_str(), f);                                            // 0x537ef4
    fputs("\n", f);                                                     // 0x54b118
}

// key lookup with fallback, used for the untranslated rows
static inline const tstring* find_or(const tstring& key, const tstring& fallback)
{
    const tstring* p = g_lang.find(&key);
    return p ? p : &fallback;
}

// "key" + suffix lookup with fallback (the plural / text / desc variants)
static inline const tstring* suffix_or(const tstring& key, const char* suffix, const tstring& fallback)
{
    tstring lookup = key + tstring(suffix);
    const tstring* p = g_lang.find(&lookup);
    return p ? p : &fallback;
}

// ---------------------------------------------------------------------------------------------------------
struct TroopRec;                                                        // 0x508 bytes
struct FactionRec;                                                      // 0x300
struct QuestRec;                                                        // 0x138
struct PartyRec;                                                        // 0x5ac8

struct PartyMgr {
    char pad0[0x138d0];
    PartyRec** pages;                                                   // +0x138d0
    char pad2[8];
    unsigned n_parties;                                                 // +0x138dc (GameData+0x139d4)
    unsigned first_party_id();                                          // 0x460d20
    void next_party_id(unsigned* id);                                   // 0x460ca0
    int find(unsigned id);                                              // 0x48ad00
    PartyRec* get_party(unsigned id) {
        int idx = find(id);
        return (PartyRec*)((char*)pages[idx >> 4] + (idx & 0xf) * 0x5ac8);
    }
};

struct GameData {
    char pad0[0x78];
    int party_id_78;                                                    // +0x78
    char pad7c[0x10];
    int player_troop;                                                   // +0x8c
    char pad90[0x68];
    PartyMgr pm;                                                        // +0xf8
    char pad_after_pm[0x13a00 - 0x139d8];
    TroopRec* chars;                                                    // +0x13a00
    int n_chars;                                                        // +0x13a04
    char pad_a08[0x40];
    FactionRec* factions;                                               // +0x13a48
    int n_factions;                                                     // +0x13a4c
    char pad_a50[8];
    QuestRec* quests;                                                   // +0x13a58
    int n_quests;                                                       // +0x13a5c
    void save_language_files(bool translated);                          // 0x4c8ca0
};

struct TroopRec {
    tstring name;                                                       // +0x00
    tstring plural;                                                     // +0x10
    tstring key;                                                        // +0x20
    char pad30[0x508 - 0x30];
    tstring translated();                                               // 0x4c62b0
    tstring translated_plural();                                        // 0x4c6330
    tstring* plural_name();                                             // 0x48d6d0
    const tstring* display_name() {
        const tstring* p = g_lang.find(&key);
        return p ? p : &name;
    }
};

struct ItemKind {
    tstring name;                                                       // +0x00
    tstring plural;                                                     // +0x10
    tstring key;                                                        // +0x20
    char pad30[0x238 - 0x30];
    tstring translated();                                               // 0x4c62b0
    tstring translated_plural();                                        // 0x4c6330
    const tstring* display_name() {
        return find_or(key, name);
    }
    const tstring* plural_name() {
        return suffix_or(key, "_pl", plural);
    }
};

struct PartyRec {
    char pad0[0xc];
    tstring name;                                                       // +0x0c
    bool flag;                                                          // +0x1c
    char pad1d[3];
    tstring key;                                                        // +0x20
    char pad30[0x8c - 0x30];
    int f8c;                                                            // +0x8c
    char pad90[0x5ac8 - 0x90];
    tstring translated();                                               // 0x4c6530
};

struct FactionRec {
    tstring name;                                                       // +0x00
    bool flag;                                                          // +0x10
    char pad11[3];
    tstring key;                                                        // +0x14
    char pad24[0x300 - 0x24];
    tstring translated();                                               // 0x4c6860
};

struct PartyTemplate {
    tstring name;                                                       // +0x00
    tstring key;                                                        // +0x10
    char pad20[0xb0 - 0x20];
    tstring translated();                                               // 0x4c6080
};

struct GameString {
    tstring name;                                                       // +0x00
    tstring key;                                                        // +0x10
    tstring translated();                                               // 0x4c6080
};

struct QuickString {
    tstring name;                                                       // +0x00
    tstring key;                                                        // +0x10
    tstring translated();                                               // 0x4c6080
};

struct QuestRec {
    tstring name;                                                       // +0x00
    tstring text;                                                       // +0x10
    char pad20[0x54 - 0x20];
    tstring key;                                                        // +0x54
    char pad64[0x138 - 0x64];
    tstring translated();                                               // 0x4c63f0
    tstring translated_text();                                          // 0x4c6470
    const tstring* text_name() {
        return suffix_or(key, "_text", text);
    }
};

struct DialogRec {
    tstring name;                                                       // +0x00
    tstring key;                                                        // +0x10
    char pad20[0x64 - 0x20];
    tstring translated();                                               // 0x4c6080
};

struct SkinElem {
    tstring name;                                                       // +0x00
    tstring key;                                                        // +0x10
    char pad20[0x34 - 0x20];
    tstring translated();                                               // 0x4c6080
};

struct SkinSet {
    char pad0[0x3c];
    int n_elems;                                                        // +0x3c
    SkinElem* elems;                                                    // +0x40
    char pad44[0x1f8 - 0x44];
};

struct ModRec {
    tstring name;                                                       // +0x00
    tstring key;                                                        // +0x10
    char pad20[0x28 - 0x20];
    tstring translated();                                               // 0x4c6080
};

struct SkillRec {
    tstring desc;                                                       // +0x00
    tstring name;                                                       // +0x10
    tstring key;                                                        // +0x20
    char pad30[0x44 - 0x30];
    tstring translated();                                               // 0x4c6100
    tstring translated_desc();                                          // 0x4c61f0
    const tstring* desc_name() {
        return suffix_or(key, "_desc", desc);
    }
};

struct MenuDoor {
    tstring name;                                                       // +0x00
    tstring text;                                                       // +0x10
    tstring key;                                                        // +0x20
    char pad30[0x68 - 0x30];
    tstring translated();                                               // 0x4c62b0
    tstring translated_door();                                          // 0x4c6d00
};

struct GameMenu {
    tstring name;                                                       // +0x00
    tstring key;                                                        // +0x10
    char pad20[0x54 - 0x20];
    int n_doors;                                                        // +0x54
    MenuDoor* doors;                                                    // +0x58
    char pad5c[4];
};

extern ItemKind* g_items;                                               // 0x5e29d0
extern int g_n_items;                                                   // 0x5e29d4
extern PartyTemplate* g_party_templates;                                // 0x5e29f8
extern int g_n_party_templates;                                         // 0x5e29fc
extern GameString* g_game_strings;                                      // 0xa75ea0
extern int g_n_game_strings;                                            // 0xa75ea8
extern QuickString* g_quick_strings;                                    // 0xa75ea4
extern int g_n_quick_strings;                                           // 0xa75eac
extern DialogRec* g_dialogs;                                            // 0x8ed50cc
extern int g_n_dialogs;                                                 // 0x8ed50c8
extern SkinSet* g_skins_begin;                                          // 0x8ed4f7c
extern SkinSet* g_skins_end;                                            // 0x8ed4f80
extern ModRec g_item_mods[];                                            // 0xa75f70 (43 x 0x28)
extern SkillRec g_skills[];                                             // 0x8ee3100 (42 x 0x44)
extern GameMenu* g_menus;                                               // 0x5e29b0
extern int g_n_menus;                                                   // 0x5e29b4
extern char g_lang_on;                                                  // 0xa181b8

// ---------------------------------------------------------------------------------------------------------
// @ 0x4c8ca0
void GameData::save_language_files(bool translated)
{
    CreateDirectoryA("new_language", 0);                                // 0x59606c

    if (n_chars > 0) {
        FILE* f = fopen("new_language\\troops.csv", "wt+");             // 0x596050 / 0x596068
        for (int i = 0; i < n_chars; i++) {
            if (i == player_troop)
                continue;
            if (translated) {
                g_lang.write(f, chars[i].key, chars[i].translated());
                tstring suffix("_pl");                                  // 0x5923b0
                g_lang.write(f, chars[i].key + suffix, chars[i].translated_plural());
            } else {
                g_lang.write(f, chars[i].key, *chars[i].display_name());
                g_lang.write(f, chars[i].key + tstring("_pl"),
                             *chars[i].plural_name());
            }
        }
        fclose(f);
    }

    if (pm.n_parties != 0) {
        FILE* f = fopen("new_language\\parties.csv", "wt+");            // 0x596034
        unsigned i = pm.first_party_id();
        while (i < pm.n_parties) {
            PartyRec* p = pm.get_party(i);
            if (p->f8c <= 0 && i != (unsigned)party_id_78) {
                if (translated) {
                    tstring v = pm.get_party(i)->translated();
                    write_row(f, pm.get_party(i)->key, v);
                } else {
                    const tstring* v;
                    if (g_lang_on)
                        v = &p->key;
                    else if (p->flag)
                        v = &p->name;
                    else
                        v = find_or(p->key, p->name);
                    write_row(f, pm.get_party(i)->key, *v);
                }
            }
            pm.next_party_id(&i);
        }
        fclose(f);
    }

    if (n_factions > 0) {
        FILE* f = fopen("new_language\\factions.csv", "wt+");           // 0x596018
        for (int i = 0; i < n_factions; i++) {
            if (translated) {
                tstring v = factions[i].translated();
                write_row(f, factions[i].key, v);
            } else {
                const tstring* v = &factions[i].name;
                if (!factions[i].flag)
                    v = find_or(factions[i].key, factions[i].name);
                write_row(f, factions[i].key, *v);
            }
        }
        fclose(f);
    }

    if (g_n_items > 0) {
        FILE* f = fopen("new_language\\item_kinds.csv", "wt+");         // 0x595ffc
        for (int i = 0; i < g_n_items; i++) {
            if (translated) {
                tstring v = g_items[i].translated();
                write_row(f, g_items[i].key, v);
                tstring suffix("_pl");
                write_row(f, g_items[i].key + suffix, g_items[i].translated_plural());
            } else {
                const tstring* v = find_or(g_items[i].key, g_items[i].name);
                write_row(f, g_items[i].key, *v);
                write_row(f, g_items[i].key + tstring("_pl"),
                          *g_items[i].plural_name());
            }
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\item_modifiers.csv", "wt+");     // 0x595fdc
        for (ModRec* r = g_item_mods; r < g_item_mods + 43; r++) {
            if (translated) {
                tstring v = r->translated();
                write_row(f, r->key, v);
            } else {
                write_row(f, r->key, *find_or(r->key, r->name));
            }
        }
        fclose(f);
    }

    if (g_n_party_templates > 0) {
        FILE* f = fopen("new_language\\party_templates.csv", "wt+");    // 0x595fb8
        for (int i = 0; i < g_n_party_templates; i++) {
            if (translated) {
                tstring v = g_party_templates[i].translated();
                write_row(f, g_party_templates[i].key, v);
            } else {
                write_row(f, g_party_templates[i].key,
                          *find_or(g_party_templates[i].key, g_party_templates[i].name));
            }
        }
        fclose(f);
    }

    if (g_n_game_strings > 0) {
        FILE* f = fopen("new_language\\game_strings.csv", "wt+");       // 0x595f98
        for (int i = 0; i < g_n_game_strings; i++) {
            if (translated) {
                tstring v = g_game_strings[i].translated();
                write_row(f, g_game_strings[i].key, v);
            } else {
                write_row(f, g_game_strings[i].key,
                          *find_or(g_game_strings[i].key, g_game_strings[i].name));
            }
        }
        fclose(f);
    }

    if (g_n_quick_strings > 0) {
        FILE* f = fopen("new_language\\quick_strings.csv", "wt+");      // 0x595f78
        for (int i = 0; i < g_n_quick_strings; i++) {
            if (translated) {
                tstring v = g_quick_strings[i].translated();
                write_row(f, g_quick_strings[i].key, v);
            } else {
                write_row(f, g_quick_strings[i].key,
                          *find_or(g_quick_strings[i].key, g_quick_strings[i].name));
            }
        }
        fclose(f);
    }

    if (n_quests > 0) {
        FILE* f = fopen("new_language\\quests.csv", "wt+");             // 0x595f60
        for (int i = 0; i < n_quests; i++) {
            if (translated) {
                tstring v = quests[i].translated();
                write_row(f, quests[i].key, v);
                tstring suffix("_text");                                // 0x5923b4
                write_row(f, quests[i].key + suffix, quests[i].translated_text());
            } else {
                write_row(f, quests[i].key, *find_or(quests[i].key, quests[i].name));
                write_row(f, quests[i].key + tstring("_text"),
                          *quests[i].text_name());
            }
        }
        fclose(f);
    }

    if (g_n_dialogs > 0) {
        FILE* f = fopen("new_language\\dialogs.csv", "wt+");            // 0x595f44
        for (int i = 0; i < g_n_dialogs; i++) {
            for (int j = 0; j < i; j++) {
                if (g_dialogs[i].key.equals(g_dialogs[j].key))
                    goto next_dialog;
            }
            if (translated) {
                tstring v = g_dialogs[i].translated();
                write_row(f, g_dialogs[i].key, v);
            } else {
                write_row(f, g_dialogs[i].key,
                          *find_or(g_dialogs[i].key, g_dialogs[i].name));
            }
        next_dialog:
            ;
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\skins.csv", "wt+");              // 0x595f2c
        for (unsigned i = 0; g_skins_begin && i < (unsigned)(g_skins_end - g_skins_begin); i++) {
            for (int j = 0; j < g_skins_begin[i].n_elems; j++) {
                if (translated) {
                    tstring v = g_skins_begin[i].elems[j].translated();
                    write_row(f, g_skins_begin[i].elems[j].key, v);
                } else {
                    write_row(f, g_skins_begin[i].elems[j].key,
                              *find_or(g_skins_begin[i].elems[j].key, g_skins_begin[i].elems[j].name));
                }
            }
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\skills.csv", "wt+");             // 0x595f14
        for (SkillRec* r = g_skills; r < g_skills + 42; r++) {
            if (translated) {
                tstring v = r->translated();
                write_row(f, r->key, v);
                tstring suffix("_desc");                                // 0x595ca4
                write_row(f, r->key + suffix, r->translated_desc());
            } else {
                write_row(f, r->key, *find_or(r->key, r->name));
                write_row(f, r->key + tstring("_desc"),
                          *r->desc_name());
            }
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\game_menus.csv", "wt+");         // 0x595ef8
        for (int i = 0; i < g_n_menus; i++) {
            write_row(f, g_menus[i].key, *find_or(g_menus[i].key, g_menus[i].name));
            for (int j = 0; j < g_menus[i].n_doors; j++) {
                if (translated) {
                    tstring v = g_menus[i].doors[j].translated();
                    write_row(f, g_menus[i].doors[j].key, v);
                    tstring suffix("_door");                            // 0x595d64
                    tstring lookup = g_menus[i].doors[j].key + suffix;
                    const tstring* p = g_lang.find(&lookup);
                    if (!p)
                        p = &g_menus[i].doors[j].text;
                    if (p->len > 2) {
                        tstring suffix2("_door");
                        tstring v2 = g_menus[i].doors[j].translated_door();
                        write_row(f, g_menus[i].doors[j].key + suffix2, v2);
                    }
                } else {
                    write_row(f, g_menus[i].doors[j].key,
                              *find_or(g_menus[i].doors[j].key, g_menus[i].doors[j].name));
                    const tstring* p = suffix_or(g_menus[i].doors[j].key, "_door",
                                                 g_menus[i].doors[j].text);
                    if (p->len > 2)
                        write_row(f, g_menus[i].doors[j].key + tstring("_door"), *p);
                }
            }
        }
        fclose(f);
    }
}
