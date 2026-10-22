// Slice s00502300: the character-sheet inventory window of the party screen. `this` is the
// party-window object (layout shared with s00501940). Widget slot offsets, tstring / string-table /
// item-record layouts and the callee conventions follow the verified neighbours (s004dfd50,
// s0048add0, s004fba40, s00462540, s00499050).  Names are provisional.
#include <windows.h>
#include <new>
#include "rgl.h"

using rgl::vector4;

// ---- engine types ------------------------------------------------------------------------------------
namespace rgl { struct mesh; }
using rgl::mesh;

// engine mesh: translated by 0x44f240 (ret 4, pointer argument)
namespace rgl {
struct mesh {
    void translate(const vector4* v);                       // 0x44f240 (ret 4)
};
}

// 0x10-byte tstring: { pos, buffer, capacity, length }.
struct tstring {
    int pos;                                                // +0
    char* buf;                                              // +4
    int cap;                                                // +8
    int len;                                                // +0xc
    tstring(const char* s);                                 // 0x42bbe0 (ret 4)
    tstring() { pos = 0; len = 0; cap = 0; buf = 0; }       // 0x42bbd0 (inline)
    tstring(const tstring& o);                              // 0x42c110 (ret 4)
    ~tstring();                                             // 0x42bc40
};

// string table at 0x5e2a18: lookup returns a pointer to its named entry (its text at +4).
struct strtab {
    tstring* lookup(const tstring& name);                   // 0x43a1e0 (ret 4)
};
extern strtab g_strtab;                                     // 0x5e2a18
static strtab* const g_strtab_p = (strtab*)0x5e2a18;        // 0x5e2a18

// the object table at 0xa75f40: 0x4991d0 builds a display mesh from an (index, item) pair.
struct obj_tbl {
    mesh* make_mesh_from(const int* index, const void* from);   // 0x4991d0
};
extern obj_tbl g_obj;                                       // 0xa75f40

// the mesh of an item reference (cdecl, ret 8): def[0] = item index, def[1] = amount/modifier
mesh* make_item_mesh(const int* def, const float* scale);   // 0x4fba40

// ---- item records ------------------------------------------------------------------------------------
// item reference: item index, amount (low byte) and modifier (byte 3) in the troop record's 8-byte
// equipment slots
struct ItemRef {
    int item;                                               // +0
    unsigned m4;                                            // +4
    int  stat_1fc();                                        // 0x462960
    int  stat_200();                                        // 0x434eb0
    int  stat_204();                                        // 0x462a10
    int  max_amount();                                      // 0x434b70
};
// the sound sub-object at +0x228 of an item record; its own 0x10 bytes make the record stride 0x238
struct ItemSound {
    char pad[0x10];
    void play(int id);                                      // 0x48add0 (ret 4)
};
struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                                    // +0x1c8 (low byte = modifier class)
    char pad1cc[0x208 - 0x1cc];
    float f208;                                             // +0x208 (base armor value)
    char pad20c[0x228 - 0x20c];
    ItemSound snd;                                          // +0x228
};
extern ItemKind* g_items;                                   // 0x5e29d0 (pointer to the item table)
#define ITEMS g_items
#define ITEM_STRIDE 0x238

// Troop record (stride 0x508)
struct TroopRec {
    char pad0[0x4a4];
    ItemRef slots[8];                                       // +0x4a4
    void release();                                         // 0x487c00 (base destructor)
    void* destroy_troop(unsigned char flag);                // 0x502300 (deleting destructor)
};
#define TROOP_STRIDE 0x508

// the troop-count accessor of the troop table entry (0x462b60)
struct TroopEntry {
    int count();                                            // 0x462b60
};

// The world / game-data object at 0x5e2930.
struct World {
    char pad8c[0x8c];
    int   cur_troop;                                        // +0x8c
    int   hero_troop;                                       // +0x90
    char pad94[0x13a00 - 0x94];
    char* troops;                                           // +0x13a00 (stride 0x508)
    char pad13a04[0x1a1f4 - 0x13a04];
    int   f_1a1f4;                                          // +0x1a1f4
    int   f_1a1f8;                                          // +0x1a1f8
    int   f_1a1fc;                                          // +0x1a1fc
    int   f_1a200;                                          // +0x1a200
    int   f_1a204;                                          // +0x1a204
};
extern World* g_game;                                       // 0x5e2930

#define TROOP(i) ((char*)g_game->troops + (i) * TROOP_STRIDE)
#define SLOT(i, k) ((ItemRef*)(TROOP(i) + 0x4a4 + (k) * 8))

extern float g_layout_unit;                                 // 0x599ae8 (0.088f: one row of the grid)
extern int   g_equip_page;                                  // 0xa75f68 (negative: nothing selected)

// the spawn sliders reset by 0x502b80; the last pair is written as one 64-bit value (sign extension)
struct SpawnSliders {
    int lo;                                                 // 0xa70448
    int hi;                                                 // 0xa7044c
    long long pos;                                          // 0xa70450 (lo word = troop, hi word = its sign)
};
extern SpawnSliders g_sliders;

// ---- cstdlib -----------------------------------------------------------------------------------------
extern "C" int sprintf(char* dst, const char* fmt, ...);    // 0x536f8f

// ---- widgets (slot numbers from the call sites) -------------------------------------------------------
struct UiText {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21();
    virtual void set_xy(float* v);                          // +0x58 (index 22)
    virtual void d23(); virtual void d24();
    virtual void set_scale(float* v);                       // +0x64 (index 25)
    virtual void d26(); virtual void d27(); virtual void d28();
    virtual void set_xyz(float* v);                         // +0x74 (index 29)
    virtual void d30(); virtual void d31();
    virtual void add_image(int idx, void* m);               // +0x80 (index 32; slot icon / strip mesh)
    virtual void d33(); virtual void d34(); virtual void d35(); virtual void d36(); virtual void d37();
    virtual void d38(); virtual void d39(); virtual void d40();
    virtual void set_text(tstring* out, const tstring* name);// +0xa4 (index 41)
    virtual void d42(); virtual void d43(); virtual void d44(); virtual void d45(); virtual void d46();
    virtual void d47(); virtual void d48(); virtual void d49();
    virtual void add_child(void* child);                    // +0xc8 (index 50)
    virtual void d51(); virtual void d52(); virtual void d53(); virtual void d54(); virtual void d55();
    char  pad_e0[0xe0 - 4];                                 // the vtable occupies +0..+0xdc
    float x0;                                               // +0xe0
    float y0;                                               // +0xe4
};

// plain text widget (ctor 0x48ae20, size 0x178)
struct UiText2 : UiText {
    char pad[0x178 - 0xe8];
    UiText2();                                              // 0x48ae20
    virtual ~UiText2();                                     // 0x487c00
};

// the page block 0x502320 allocates (vptr 0x599bf8, ctor 0x4931a0, size 0x188): the "paper doll" page.
struct UiPanel : UiText2 {
    char pad2[0x188 - 0x178];
    UiPanel();                                              // 0x4931a0
};

// the icon the ground/stash strip builds (size 0x188): the 0x48b1c0 ctor initialises the intermediate's
// fields, while the concrete class below only has to install its own vtable (0x591ee8)
struct UiIconBase : UiText2 {
    char pad2[0x188 - 0x178];
    UiIconBase();                                           // 0x48b1c0
};
struct UiIcon2 : UiIconBase { };

// label parameter refresh (0x4e4c70): a cdecl free function taking (widget, tstring by value)
void label_refresh(void* w, tstring name);

// the slot widget 0x502230 allocates (vptr 0x599d28, ctor 0x48ae20, size 0x178); the page builder
// pokes two of its dwords, +0x300 and +0x600
struct UiSlot : UiText2 {
    char pad300[0x300 - 0x178];
    int  f300;                                              // +0x300
    char pad304[0x600 - 0x304];
    int  f600;                                              // +0x600
};
UiText2* make_slot(void* o, void* slot_record);             // 0x502230 (ret 4)

// the inventory window object; the helpers below are members (this in ecx, like the original)
struct PartyWindow {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17();
    virtual void on_input(unsigned mask);                   // +0x48
    virtual void d19(); virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23();
    virtual void d24(); virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28();
    virtual void d29(); virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33();
    virtual void d34(); virtual void d35();
    virtual void refresh();                                 // +0x90
    virtual void d37(); virtual void d38(); virtual void d39(); virtual void d40(); virtual void d41();
    virtual void d42(); virtual void d43(); virtual void d44(); virtual void d45(); virtual void d46();
    virtual void d47(); virtual void d48(); virtual void d49(); virtual void d50(); virtual void d51();
    virtual void d52(); virtual void d53(); virtual void d54(); virtual void d55(); virtual void d56();
    virtual void d57(); virtual void d58(); virtual void d59(); virtual void d60(); virtual void d61();
    virtual void d62(); virtual void d63(); virtual void d64(); virtual void d65(); virtual void d66();
    virtual void d67(); virtual void d68(); virtual void d69(); virtual void d70(); virtual void d71();
    virtual void d72(); virtual void d73(); virtual void d74(); virtual void d75(); virtual void d76();
    virtual void d77(); virtual void d78(); virtual void d79(); virtual void d80(); virtual void d81();
    virtual void d82(); virtual void d83(); virtual void d84(); virtual void d85(); virtual void d86();
    virtual void d87(); virtual void d88(); virtual void d89(); virtual void d90(); virtual void d91();
    virtual void d92(); virtual void d93(); virtual void d94(); virtual void d95(); virtual void d96();
    virtual void d97(); virtual void d98(); virtual void d99(); virtual void d100(); virtual void d101();
    virtual void d102(); virtual void d103(); virtual void d104(); virtual void d105(); virtual void d106();
    virtual void d107(); virtual void d108(); virtual void d109(); virtual void d110(); virtual void d111();
    virtual void d112(); virtual void d113(); virtual void d114(); virtual void d115(); virtual void d116();
    virtual void d117(); virtual void d118(); virtual void d119(); virtual void d120(); virtual void d121();
    virtual void d122(); virtual void d123(); virtual void d124(); virtual void d125(); virtual void d126();
    virtual void d127(); virtual void d128(); virtual void d129(); virtual void d130(); virtual void d131();
    virtual void d132(); virtual void d133(); virtual void d134(); virtual void d135(); virtual void d136();
    virtual void d137(); virtual void d138(); virtual void d139(); virtual void d140(); virtual void d141();
    virtual void d142(); virtual void d143(); virtual void d144(); virtual void d145(); virtual void d146();
    virtual void d147(); virtual void d148(); virtual void d149(); virtual void d150(); virtual void d151();
    virtual void d152(); virtual void d153(); virtual void d154(); virtual void d155(); virtual void d156();
    virtual void d157(); virtual void d158(); virtual void d159(); virtual void d160(); virtual void d161();
    virtual void d162(); virtual void d163(); virtual void d164(); virtual void d165(); virtual void d166();
    virtual void d167(); virtual void d168(); virtual void d169(); virtual void d170(); virtual void d171();
    virtual void d172(); virtual void d173();
    virtual void on_message(int a, int b);                  // +0x2b0
    int   f4;                                               // +0x4 (non-zero: the other hero shown)
    char  pad8[0x14 - 0x8];
    int   troop;                                            // +0x14
    char  pad18[0x2c - 0x18];
    void* f2c;                                              // +0x2c (gui manager)
    void* f30;                                              // +0x30
    float layout_x;                                         // +0x34
    void* lists[4];                                         // +0x38 (armor / encumbrance labels)
    char  pad4c[0x964 - 0x48];
    int   f964;                                             // +0x964 (worn-equipment rows)
    int   f968;                                             // +0x968 (ground rows, <= 2)
    void* f96c;                                             // +0x96c (the icon container)
    // the ground/stash page blocks: 10 slot widgets at +0x970 / +0x9c0 (0x28 bytes apart) and the
    // matching state dwords; the second block ends at +0xa88 (the two strip icons below)
    char  page0[0x28];                                      // +0x970
    char  page1[0x28];                                      // +0x998
    char  pad9c0[0xa84 - 0x9c0];
    void* f_a84;                                            // +0xa84 (page 0 character icon)
    void* f_a88;                                            // +0xa88 (page 1 character icon)
    char  pad_a8c[0xac0 - 0xa8c];
    int   f_ac0;                                            // +0xac0 (page 0 character mesh spec)
    int   f_ac4;                                            // +0xac4 (page 1 character mesh spec)
    // the in-slice helpers are members (the originals take `this` in ecx); declarations only
    void  init_window();                                    // 0x5025a0
    void  refresh_armor(void** labels, int troop);          // 0x502620
    void  refresh_page(int page);                           // 0x502920
    void  refresh_stash(int row);                           // 0x502c30
    void  place_slot(UiText* w, int* item, int idx, int* ref, int page, char flag);  // 0x5029f0
    mesh* make_mesh(int* item, float* scale, int troop);     // 0x502b80
    void  add_strip_icon(char* slot, const tstring& name, int troop, int kind);  // 0x5024e0
};

// the 8 worn equipment slots of the page builder (dwords at page * 0x180 + 0x64)
#define PAGE_BLOCK(p) ((void**)((char*)this + (p) * 0x180 + 0x64))

// ---- forward declarations (non-static so the compiler emits their bodies) ----------------------------
void* build_page(PartyWindow* o, int page);

// ---- code -------------------------------------------------------------------------------------------
// @ 0x502300 ------------------------------------------------------------------------------------------
// destroying destructor of a troop record (base dtor 0x487c00, then the conditional free)
void* TroopRec::destroy_troop(unsigned char flag)
{
    release();
    if (flag & 1)
        ::operator delete(this);
    return this;
}

// @ 0x502320 ------------------------------------------------------------------------------------------
// "paper doll" page builder (name for the marker: build_page)
void* build_page(PartyWindow* o, int page)
{
    World* g = g_game;
    int troop = g->cur_troop;
    if (page != 0)
        troop = o->troop;
    int count;
    if (troop == g->hero_troop && o->f4 == 0)
        count = 9;
    else
        count = ((TroopEntry*)TROOP(troop))->count();
    int n = count;
    float pos[2];
    pos[0] = 0.0f;
    pos[1] = (float)((n + 2) / 3) * g_layout_unit;

    UiPanel* p = new UiPanel;
    float xy[2];
    xy[0] = 0.0f;
    xy[1] = 0.0f;
    p->set_xy(xy);
    float sc[2];
    sc[0] = pos[1];
    sc[1] = 0.088f;
    p->set_scale(sc);

    int col = 0, row = 0;
    for (int i = 0; i < n; i++) {
        UiText2* w = make_slot(o, SLOT(troop, i));
        void** icons = (void**)((char*)o + page * 0x180 + 0x64);
        icons[i] = w;
        float q[2];
        q[0] = (float)col * g_layout_unit;
        q[1] = pos[1] - 0.088f;
        w->set_xy(q);
        col++;
        if (col >= 3) {
            col = 0;
            row++;
        }
        p->add_child(w);
        UiSlot* s = (UiSlot*)w;
        s->f300 = 2;
        s->f600 = 0x12fd1;
    }
    return p;
}

// strip-icon entry (0x5024e0): the global at 0xa75f68 holds the selected equipment index (negative: nothing
// selected). On first use the 0x188 icon is created (0x48b1c0 fields + the 0x591ee8 vtable), named through
// label_refresh, hung in the window's icon container, then given the empty image and the mesh the object
// table builds from (the selected index and the icon's own slot-derived index).
// @ 0x5024e0 ------------------------------------------------------------------------------------------
void PartyWindow::add_strip_icon(char* slot, const tstring& name, int troop, int kind)
{
    int sel = g_equip_page;
    if (sel < 0)
        return;
    if (*(void**)slot == 0) {
        *(void**)slot = new UiIcon2;
        label_refresh(*(void**)slot, name);
        ((UiText*)f96c)->add_child(*(void**)slot);
    }
    (*(UiText**)slot)->add_image(0, 0);
    int idx = kind + troop * 4;
    (*(UiText**)slot)->add_image(0, g_obj.make_mesh_from(&sel, &idx));
}

// @ 0x5025a0 ------------------------------------------------------------------------------------------
void PartyWindow::init_window()
{
    for (int i = 0; i < f964; i++)
        refresh_page(i);
    for (int i = 0; i < f968; i++)
        refresh_stash(i);
    refresh_armor((void**)((char*)this + 0xa64), g_game->cur_troop);
    if (f968 == 2)
        refresh_armor((void**)((char*)this + 0xa74), troop);
}

// @ 0x502620 ------------------------------------------------------------------------------------------
void PartyWindow::refresh_armor(void** labels, int troop)
{
    int head = 0, body = 0;
    float enc = 0.0f;
    int legs = 0;
    for (int i = 0; i < 8; i++) {
        ItemRef* r = SLOT(troop, i);
        if (r->item < 0)
            continue;
        ItemKind* it = ITEMS + r->item;
        if (i >= 4) {
            head += r->stat_1fc();
            body += r->stat_200();
            legs += r->stat_204();
        }
        float v = it->f208;
        unsigned mod = it->type_flags & 0xff;
        if (mod == 5 || mod == 6 || mod == 10 || mod == 18) {
            int amt = (int)(r->m4 & 0xff);
            int mx = r->max_amount();
            v = v * (float)(mx - amt) / (float)r->max_amount();
        } else if (mod == 7 && (int)(r->m4 & 0xffff) <= 0) {
            v = 0.0f;
        }
        enc += v;
    }
    char buf[0x210];
    {
        tstring key("ui_head_armor");
        tstring* val = g_strtab_p->lookup(key);
        sprintf(buf, val->buf ? val->buf : (char*)&val->buf, head);
    }
    {
        tstring s(buf);
        ((UiText*)labels[0])->set_text(&s, &s);
    }
    {
        tstring key("ui_body_armor");
        tstring* val = g_strtab_p->lookup(key);
        sprintf(buf, val->buf ? val->buf : (char*)&val->buf, body);
    }
    {
        tstring s(buf);
        ((UiText*)labels[1])->set_text(&s, &s);
    }
    {
        tstring key("ui_leg_armor");
        tstring* val = g_strtab_p->lookup(key);
        sprintf(buf, val->buf ? val->buf : (char*)&val->buf, legs);
    }
    {
        tstring s(buf);
        ((UiText*)labels[2])->set_text(&s, &s);
    }
    {
        tstring key("ui_encumbrance");
        tstring* val = g_strtab_p->lookup(key);
        sprintf(buf, val->buf ? val->buf : (char*)&val->buf, enc);
    }
    {
        tstring s(buf);
        ((UiText*)labels[3])->set_text(&s, &s);
    }
}

// @ 0x502920 ------------------------------------------------------------------------------------------
void PartyWindow::refresh_page(int page)
{
    World* g = g_game;
    int troop = g->cur_troop;
    if (page != 0)
        troop = this->troop;
    int count;
    if (troop == g->hero_troop && f4 == 0)
        count = 9;
    else
        count = ((TroopEntry*)TROOP(troop))->count();
    char* widget = (char*)this + page * 0x180 + 0x64;
    char* state = (char*)this + page * 0x180 + 0x364;
    int slot = troop * TROOP_STRIDE + 0xb4;
    for (int i = 0; i < count; i++) {
        place_slot(*(UiText**)widget, (int*)(g_game->troops + slot), i, (int*)state, page, 0);
        widget += 4;
        slot += 8;
    }
}

// @ 0x5029f0 ------------------------------------------------------------------------------------------
// place_slot: param 2 is the slot widget, param 3 the slot's item reference, param 4 the slot index, param
// 5 the state dwords of the page (bit 1 = "needs a mesh"), param 6 the page, param 7 the new-menu flag.
void PartyWindow::place_slot(UiText* w, int* item, int idx,
                             int* ref, int page, char flag)
{
    unsigned flags = ref[idx];
    if ((flags & 2) == 0)
        return;
    int troop = g_game->cur_troop;
    if (item[0] == -1) {
        *(char*)((char*)w + 0x11e) = 0;
        if (flag != 0 && idx < 9)
            *(char*)((char*)w + 0x11c) = 0;
    } else {
        if (page == 1) {
            if ((flags & 1) == 0)
                troop = this->troop;
        } else if ((flags & 1) != 0) {
            troop = this->troop;
        }
        float sc = 0.088f;
        mesh* m = make_mesh(item, &sc, troop);
        vector4 off(0.044f, 0.044f, 0.0f);
        m->translate(&off);
        w->add_image(2, m);
        *(char*)((char*)w + 0x11e) = 1;
        if (flag != 0 && idx < 9)
            *(char*)((char*)w + 0x11c) = 1;
    }
    ref[idx] = flags & ~2u;
    // the two character figures of the page are redrawn whenever a new-style (ground/stash) slot is
    // refreshed
    if (f_a84 != 0 && flag != 0) {
        add_strip_icon((char*)((char*)this + 0xa84), tstring("inv_i_character"),
                       g_game->cur_troop, f_ac0);
    }
    if (f_a88 != 0 && flag != 0) {
        add_strip_icon((char*)((char*)this + 0xa88), tstring("inv_i_character2"),
                       this->troop, f_ac4);
    }
}

// @ 0x502b80 ------------------------------------------------------------------------------------------
// make_mesh: param 2 is the caller's scale (the original also materialises its own 0.088f), param 3 the
// troop the spawn sliders are set from.
mesh* PartyWindow::make_mesh(int* item, float* scale, int troop)
{
    mesh* m = 0;
    if (item[0] >= 0) {
        float s = 0.088f;
        m = make_item_mesh(item, &s);
        g_game->f_1a1f4 = -1;
        g_game->f_1a1f8 = troop;
        g_game->f_1a204 = -10000;
        g_game->f_1a200 = (int)m;
        g_game->f_1a1fc = 0;
        g_sliders.lo = -1;
        g_sliders.hi = -1;
        g_sliders.pos = troop;
        g_items[item[0]].snd.play(-0x32);
    }
    return m;
}

// @ 0x502c30 ------------------------------------------------------------------------------------------
void PartyWindow::refresh_stash(int row)
{
    int troop = g_game->cur_troop;
    if (row != 0)
        troop = this->troop;
    char* page = (char*)this + row * 0x28;
    int n = 0;
    char* slot = page + 0x970;
    for (; n < 10; n++, slot += 4) {
        if (*(void**)slot != 0) {
            place_slot(*(UiText**)slot, (int*)SLOT(troop, n), n, (int*)(page + 0x9c0), row, 1);
        }
    }
}
