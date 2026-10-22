// Slice s00507310: party-screen item/character view update (provisional name
// Mission::show_selected_character).  `this` is the Mission (layout shared with the neighbouring
// s005063e0 slice); the function shows the item selected in the party screen: it creates the
// "party_i_character" map quad on first use, draws the selected item's mesh there, and writes the
// selected troop's translated name into the character label at +0x7c.  Names are provisional.
#include <windows.h>

// ---- engine string (layout from the neighbouring slices) ----------------------------------------------
struct tstring {
    int pos;                                                // +0
    char* buf;                                              // +4
    int cap;                                                // +8
    int len;                                                // +0xc
    tstring();                                              // 0x42bbd0
    tstring(const char* s);                                 // 0x42bbe0
    ~tstring();                                             // 0x42bc40
};

// ---- string table at 0x5e2a18 -------------------------------------------------------------------------
struct string_table {
    tstring* find(const tstring* key);                      // 0x4c6010 (null when absent)
};
extern string_table g_strtable;                             // 0x5e2a18

// ---- object table at 0xa75f40 (display mesh factory, s00499050 / s00502300) ---------------------------
namespace rgl { struct mesh; }
struct obj_tbl {
    rgl::mesh* make_mesh_from(const int* index, const void* from);   // 0x4991d0 (ret 8)
};
extern obj_tbl g_obj;                                       // 0xa75f40

// ---- widgets ------------------------------------------------------------------------------------------
struct Widget {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23(); virtual void d24();
    virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28(); virtual void d29();
    virtual void d30(); virtual void d31();
    virtual void add_image(int idx, void* m);               // +0x80
    virtual void d33(); virtual void d34(); virtual void d35(); virtual void d36(); virtual void d37();
    virtual void d38(); virtual void d39(); virtual void d40();
    virtual void set_text(void* out, const tstring* s);     // +0xa4
    virtual void d42(); virtual void d43(); virtual void d44(); virtual void d45(); virtual void d46();
    virtual void d47(); virtual void d48(); virtual void d49();
    virtual void add_child(void* child);                    // +0xc8
};

// plain text widget (ctor 0x48ae20, size 0x178)
struct UiText : Widget {
    char pad[0x178 - 4];
    UiText();                                               // 0x48ae20
};

// label parameter refresh: (widget, tstring by value), cdecl
void label_refresh(void* w, tstring s);                     // 0x4e4c70

// ---- game data ----------------------------------------------------------------------------------------
struct vec2f { float x, y; };

// a 0x508-byte troop/character record: translated id at +0x20
struct TroopRec {
    char pad0[0x20];
    tstring name;                                           // +0x20
    char pad30[0x508 - 0x30];
    tstring* display_name() {                               // 0x43a480 (inlined here)
        tstring* n = g_strtable.find(&name);
        return n ? n : (tstring*)this;
    }
};

struct GameData {
    char pad0[0x8c];
    int player;                                             // +0x8c
    char pad90[0x13a00 - 0x90];
    TroopRec* chars;                                        // +0x13a00 (stride 0x508)
};
extern GameData* g_game;                                    // 0x5e2930

// the selected item index; negative = nothing selected
extern int g_item_sel;                                      // 0xa75f6c

// ---- mission ------------------------------------------------------------------------------------------
struct Mission {
    char pad00[0x2c];
    Widget* mgr;                                            // +0x2c (widget manager / parent container)
    char pad30[0x7c - 0x30];
    Widget* w7c;                                            // +0x7c (character name label)
    char pad80[0x390c - 0x80];
    Widget* w390c;                                          // +0x390c (item map quad)
    void show_selected_character(int troop, int slot);      // 0x507310
};

// @ 0x507310
void Mission::show_selected_character(int troop, int slot)
{
    int item = g_item_sel;
    if (item < 0)
        return;

    if (!w390c) {
        w390c = new UiText;
        label_refresh(w390c, "party_i_character");
        mgr->add_child(w390c);
    }

    int t = troop;
    troop = slot + troop * 2;
    w390c->add_image(0, g_obj.make_mesh_from(&item, &troop));

    TroopRec* rec = (TroopRec*)((char*)g_game->chars + t * 0x508);
    vec2f size;
    w7c->set_text(&size, rec->display_name());
}
