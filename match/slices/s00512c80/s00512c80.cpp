// Slice s00512c80: the inventory panel's slot-transfer engine (0x512c80) and its four small helpers --
// the "can this item go into this slot" test (0x513230), the slot value / slot flag writers (0x5132e0,
// 0x513320) and the pending-payment notification (0x513360).
//
// `this` is the inventory panel of s00500ad0 / s0050fe20 (mode +0x4, troop +0x14, the slot state table
// +0x364, the slot value table +0x664, the two stash tables for pages 2/3 (+0x9c0 flags / +0xa10 values,
// both addressed with `slot + page*10`), the total-cost label +0xa60, the pending value +0xa8c, the
// "holding an item" flag +0xa98, the held item +0xa9c (id +0xa9c, count +0xaa0), its save byte +0xaa4,
// its saved value +0xaa8 and the page it came from +0xaac).  Widget slots: +0xa4 set_text(tstring* out,
// tstring* text).
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---------------------------------------------------------------------------------------------------------
// the engine string (0x10 bytes; ctor 0x42bbe0, dtor 0x42bc40)
struct tstring {
    int pos;                                            // +0x00
    char* buf;                                          // +0x04
    int cap;                                            // +0x08
    int len;                                            // +0x0c
    tstring() MB_NOTHROW;                               // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;                  // 0x42bbe0 (ret 4)
    tstring(const tstring& o) MB_NOTHROW;               // 0x42c110
    ~tstring() MB_NOTHROW;                              // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// a tstring a callee writes in place (World::string_value writes into its first argument)
struct tstring_buffer {
    char bytes[sizeof(tstring)];
    tstring* get() { return (tstring*)bytes; }
    void destroy() { get()->~tstring(); }
};

// translation table at 0x5e2a18
struct StrTable {
    tstring* lookup(const tstring& key) MB_NOTHROW;     // 0x43a1e0 (ret 4)
};
extern StrTable g_strtable;                             // 0x5e2a18

// item kind record, 0x238 bytes (array at 0x5e29d0)
struct ItemKind {
    char pad0[0x1c8];
    union {
        unsigned flags;                                 // +0x1c8
        struct { unsigned char type; char tpad[3]; };
    };
    char pad1cc[0x238 - 0x1cc];
};
extern ItemKind* g_items;                               // 0x5e29d0

// item specifier (2 dwords; the low byte of +4 is the amount)
struct ItemRef {
    int id;                                             // +0x00
    int f4;                                             // +0x04 (low byte = amount)
    int  max_amount();                                  // 0x434b70
    void reduce(int n);                                 // 0x437510 (ret 4)
    void add_amount(int n);                             // 0x434be0 (ret 4)
};

// 0x508-byte character row of the troop table
struct Agent {
    tstring* display_name();                            // 0x43a480
    char fits_slot(ItemRef* item, int flag);            // 0x462c80 (ret 8)
};

struct GameData {
    char pad0[0x8c];
    int f8c;                                            // +0x8c (the "player" troop index)
    char pad90[0x13a00 - 0x90];
    char* chars;                                        // +0x13a00 (0x508-byte rows)
    tstring* string_value(tstring* out, int value);      // 0x48e9e0 (ret 8)
    char pick_event(int ev, int id, int flag);           // 0x435790 (ret 0xc)
};
extern GameData* g_game;                                // 0x5e2930

// the message log / notification sink at 0x5d7080
struct MsgLog {
    void lock();                                        // 0x48b2d0
    void unlock(int flag);                              // 0x42cd80 (ret 4)
    void add(tstring text, int color);                   // 0x452c00 (ret 0x14; text by value, first stack arg)
};
extern MsgLog g_msglog;                                 // 0x5d7080

// the object being carried by the mouse (0x5d75e4); vtable slot 0 = release(int)
struct Mesh {
    virtual void release(int flag);                     // +0x0
    void scale(float f);                                // 0x44f0c0 (ret 4)
};
extern Mesh* g_drag;                                    // 0x5d75e4

// widgets: 42 virtuals, set_text at +0xa4
struct UiWidget {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16();
    virtual char clicked_on(int which);                 // +0x44
    virtual void set_key_flags(int flags);              // +0x48
    virtual void d19(); virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23();
    virtual void d24(); virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28();
    virtual void d29(); virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33();
    virtual void d34(); virtual void d35(); virtual void d36(); virtual void d37(); virtual void d38();
    virtual void d39(); virtual void d40();
    virtual void set_text(tstring* out, tstring* text); // +0xa4
};

// ---------------------------------------------------------------------------------------------------------
// the inventory panel (only the fields this slice touches)
struct InventoryPanel {
    char pad0[4];
    int f4;                                             // +0x004 (mode)
    char pad8[0x14 - 0x8];
    int f14;                                            // +0x014 (troop)
    char pad18[0x364 - 0x18];
    unsigned flags[2][0x60];                            // +0x364 (pages 0/1 slot state, 0x300 bytes)
    unsigned values[2][0x60];                           // +0x664 (pages 0/1 slot value, 0x300 bytes)
    char pad964[0x9c0 - 0x964];
    unsigned fstash[2][10];                             // +0x9c0 (pages 2/3 state, reached as +0x970)
    unsigned vstash[2][10];                             // +0xa10 (pages 2/3 value, reached as +0x9c0)
    UiWidget* total_cost;                               // +0xa60
    char pada64[0xa8c - 0xa64];
    int f_a8c;                                          // +0xa8c (pending trade value)
    char pad_a90[8];
    int f_a98;                                          // +0xa98 (holding an item)
    ItemRef f_a9c;                                      // +0xa9c (the held item; count +0xaa0)
    char f_aa4;                                         // +0xaa4 (its saved flag)
    char padaa5[3];
    int f_aa8;                                          // +0xaa8 (its saved value)
    int f_aac;                                          // +0xaac (the page it came from)

    // --- callees defined in earlier slices ---
    char slot_flag(int page, int slot, unsigned mask);                  // 0x5126e0
    void slot_flags(int page, int idx, unsigned mask, char on);         // 0x510850 (ret 0x10)
    int  price_512960(ItemRef* item, int mode);                         // 0x512960 (ret 8)
    Mesh* make_mesh(ItemRef* item, int* out, int troop);                // 0x502b80 (ret 0xc)
    char try_place(ItemRef* item, int page, int slot);                  // 0x5134c0 (ret 0xc, s005134c0)

    // --- this slice ---
    void transfer_slot(ItemRef* item, int page, int slot);              // 0x512c80 (ret 0xc)
    char can_transfer(ItemRef* item, int mode, int flag);               // 0x513230 (ret 0xc)
    void set_slot_value(int page, int slot, int value);                 // 0x5132e0 (ret 0xc)
    void or_slot_flag(int page, int slot);                              // 0x513320 (ret 8)
    void add_cost(int delta);                                           // 0x513360 (ret 4)
};

// ---------------------------------------------------------------------------------------------------------
// @ 0x512c80
void InventoryPanel::transfer_slot(ItemRef* item, int page, int slot)
{
    ItemRef tmp;
    char fl;
    int sv;
    int hv;
    char hf;
    char buf[0x100];

    if (f_a98 != 0) {
        if (can_transfer(&f_a9c, page, slot))
            goto L_cd4;
        if (f_a98 != 0)
            goto L_304c;
    }
    if (item->id == -1)
        goto L_303e;
L_cd4:
    if (f4 == 2 && try_place(item, page, slot))
        goto L_303e;
    if (f_a98 == 0)
        goto L_de3;
    if (f_a9c.id == item->id && item->id >= 0 && g_items[item->id].type == 0x12 &&
        (f_a9c.f4 & 0xff) != 0 && (item->f4 & 0xff) != 0) {
        // same stackable: pour as much as fits from the hand into the held stack
        g_msglog.lock();
        int n = item->f4 & 0xff;
        int have = f_a9c.f4 & 0xff;
        int room = f_a9c.max_amount() - have;
        if (n > room)
            n = room;
        item->reduce(n);
        f_a9c.add_amount(n);
        if (f_a9c.max_amount() - (f_a9c.f4 & 0xff) > 0)
            return;
        f_a9c.id = -1;
        g_msglog.unlock(0);
        f_a98 = 0;
        return;
    }
L_de3:
    // pick the slot's contents up into the hand
    g_msglog.lock();
    tmp = *item;
    fl = slot_flag(page, slot, 1);
    sv = (page < 2) ? (int)values[page][slot] : (int)vstash[page - 2][slot];
    item->id = -1;
    or_slot_flag(page, slot);
    if (f_a98 == 0)
        goto L_f9d;
    hf = f_aa4;
    hv = f_aa8;
    slot_flags(page, slot, 1, hf);
    set_slot_value(page, slot, hf ? hv : 0x1e61);
    if (f4 == 2) {
        // trade mode: price the move, unless it stays on the same side
        if (page == 1) {
            if (f_aac == 1)
                goto L_f54;
        } else {
            if (f_aac != 1)
                goto L_f54;
        }
        {
            int price = hf ? hv : price_512960(&f_a9c, f_aac);
            char empty = (f_aa4 == 0);
            slot_flags(page, slot, 1, empty);
            set_slot_value(page, slot, empty ? price : 0x1e61);
            int ev;
            if (page == 1) {
                add_cost(price);
                ev = 6;
            } else {
                add_cost(-price);
                ev = 5;
            }
            g_game->pick_event(ev, f_a9c.id, f_aa4 != 0);
        }
    }
L_f54:
    // hand the held item back to the slot
    *item = f_a9c;
    or_slot_flag(page, slot);
    f_a9c.id = -1;
    if (g_drag != 0)
        g_drag->release(1);
    g_drag = 0;
    f_a98 = 0;
L_f9d:
    // take the slot's contents into the hand
    if (tmp.id < 0)
        return;
    f_a9c = tmp;
    f_aac = page;
    f_aa4 = fl;
    f_aa8 = sv;
    {
        int idx = g_game->f8c;
        if (page == 1 || page == 3) {
            if (fl == 0)
                idx = f14;
        } else {
            if (fl != 0)
                idx = f14;
        }
        Mesh* m = make_mesh(&f_a9c, &hv, idx);
        m->scale(2.0f);
        if (g_drag != 0)
            g_drag->release(1);
        g_drag = m;
        f_a98 = 1;
    }
    return;
L_303e:
    if (f_a98 == 0)
        return;
L_304c:
    if (page < 2)
        return;
    if (slot == 8 && g_items[f_a9c.id].type == 1) {
        if (page == 2) {
            tstring key("ui_notification_riding_skill_not_enough");
            g_msglog.add(*g_strtable.lookup(key), 0xffffddbb);
            return;
        }
    } else if (slot >= 0 && slot < 4 &&
               ((g_items[f_a9c.id].flags & 0xff) == 2 || (g_items[f_a9c.id].flags & 0xff) == 3 ||
                (g_items[f_a9c.id].flags & 0xff) == 4 || (g_items[f_a9c.id].flags & 0xff) == 8 ||
                (g_items[f_a9c.id].flags & 0xff) == 9 || (g_items[f_a9c.id].flags & 0xff) == 10 ||
                (g_items[f_a9c.id].flags & 0xff) == 16 || (g_items[f_a9c.id].flags & 0xff) == 17)) {
        if (page == 2) {
            tstring key("ui_notification_requirements_not_met");
            g_msglog.add(*g_strtable.lookup(key), 0xffffddbb);
            return;
        }
    } else if (((g_items[f_a9c.id].flags & 0xff) == 12 && slot == 4) ||
               ((g_items[f_a9c.id].flags & 0xff) == 13 && slot == 5) ||
               ((g_items[f_a9c.id].flags & 0xff) == 15 && slot == 7) ||
               ((g_items[f_a9c.id].flags & 0xff) == 14 && slot == 6)) {
        if (page == 2) {
            tstring key("ui_notification_requirements_not_met");
            g_msglog.add(*g_strtable.lookup(key), 0xffffddbb);
            return;
        }
    } else {
        return;
    }
    {
        tstring key("ui_troop_cant_use_item");
        sprintf(buf, g_strtable.lookup(key)->c_str(),
                ((Agent*)(g_game->chars + f14 * 0x508))->display_name()->c_str());
    }
    g_msglog.add(tstring(buf), 0xffff7700);
}

// ---------------------------------------------------------------------------------------------------------
// @ 0x513230
char InventoryPanel::can_transfer(ItemRef* item, int mode, int flag)
{
    char r = 1;
    if (mode == 0)
        return r;
    if (mode >= 2) {
        int idx = (mode == 2) ? g_game->f8c : f14;
        r = ((Agent*)(g_game->chars + idx * 0x508))->fits_slot(item, flag);
        if (flag != 9)
            return r;
        if (f4 != 2 && f4 != 4)
            return r;
    } else {
        if (mode != 1)
            return r;
        if (f4 != 3)
            return r;
    }
    if (g_items[item->id].type == 1)
        r = 0;
    return r;
}

// ---------------------------------------------------------------------------------------------------------
// @ 0x5132e0
void InventoryPanel::set_slot_value(int page, int slot, int value)
{
    if (page < 2)
        values[page][slot] = (unsigned)value;
    else
        vstash[page - 2][slot] = (unsigned)value;
}

// ---------------------------------------------------------------------------------------------------------
// @ 0x513320
void InventoryPanel::or_slot_flag(int page, int slot)
{
    if (page < 2)
        flags[page][slot] |= 2;
    else
        fstash[page - 2][slot] |= 2;
}

// ---------------------------------------------------------------------------------------------------------
// @ 0x513360
void InventoryPanel::add_cost(int delta)
{
    char buf[0x200];

    f_a8c += delta;
    if (f_a8c < 0) {
        tstring_buffer sv;
        tstring key("ui_notification_payment_value");
        sprintf(buf, g_strtable.lookup(key)->c_str(),
                g_game->string_value(sv.get(), -f_a8c)->c_str());
        sv.destroy();
    } else if (f_a8c > 0) {
        tstring_buffer sv;
        tstring key("ui_notification_payment_receive_value");
        sprintf(buf, g_strtable.lookup(key)->c_str(),
                g_game->string_value(sv.get(), f_a8c)->c_str());
        sv.destroy();
    } else {
        tstring key("ui_no_cost");
        sprintf(buf, g_strtable.lookup(key)->c_str());
    }
    {
        tstring_buffer out;
        tstring msg(buf);
        total_cost->set_text(out.get(), &msg);
    }
}
