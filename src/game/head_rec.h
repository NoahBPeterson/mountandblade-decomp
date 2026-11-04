// head_rec.h: the base of the "head record" family (editor / module / trigger tables): vtable 0x5947d8, a status int at
// +4, the constant 0x63 at +8 and a name tstring at +0x1c (ctor 0x4c2c50 inlined; layout evidence in
// src/rgl/mesh2/head_rec.h). ONE definition for src/game (options_panel/trigger_records.h, triggers/item_rec.h,
// options_panel/mission_items.cpp) and src/rgl (mesh2/head_rec.h).
//
// THE NINE SLOTS ARE THE IMAGE'S OWN, dumped from 0x5947d8 and from every derived table in the family
// (0x596124 ItemRec, 0x596148 ItemRec2, 0x59616c MissionBlocker, 0x596190 ItemA, 0x5961c0 ItemD, 0x5961e4 ItemB,
// 0x596208 ItemC):
//
//   slot 0  0x4c4c90  ~head_rec            releases the name tstring and, when asked, the object. Each derived
//                                          class's deleting destructor follows from it (the item family shares one,
//                                          0x4ce930, by ICF).
//   slot 1  0x477b70  reset_screen()       `ret` - an empty function (the base's default; most derived classes
//                                          override it with a screen reset)
//   slot 2  0x477b70  init_screen()       the same empty function
//   slot 3  0x408030  frame_update(float)  `ret 4` - takes its argument (the frame time) and returns (shape_base::scale)
//   slot 4  0x477b70  slot_empty_4()       the same empty function (MissionBlocker overrides it with
//                                          MissionBlocker::slot_empty_4, 0x4cea10)
//   slot 5  0x408030  write_stream(w_stream*)  `ret 4` (the same function as slot 3)
//   slot 6  0x408030  read_stream(w_stream*)  `ret 4` (the same function as slot 3)
//   slot 7  0x5363c8  pure_7() = 0          __purecall: the base declares this one pure and EVERY derived class
//                                          overrides it (0x48b170, or 0x4280e0 `return 0` in four of the record classes)
//   slot 8  0x48b170  always_true()        `mov al,1; ret` (MissionUi::always1; MissionBlocker overrides it with
//                                          static_entity::awake, 0x4280e0)
//
// Slots 3/5/6 hold one function three times and slot 1/2/4 another, so cl must fold each group to a single address
// (cmpobj-verified). A stub where the original has a function is never equivalent: the only empty bodies here are
// the ones the original itself has (0x477b70).
//
// TESTABILITY: the item-family records are never constructed by any code in the binary (no reference to their
// constructors exists; each of their vtables is referenced only by its own constructor's vptr store), so their
// equivalence rows compare vtable bytes rather than executed behaviour. See docs/Papercuts.md.
#ifndef MB_GAME_HEAD_REC_H
#define MB_GAME_HEAD_REC_H
#include "./tstring.h"
#include <stddef.h>
#include <string.h>
void* __cdecl operator new(size_t n);   // 0x53545e (scalar operator new)

// 0x4c6e00 (src/rgl/mesh2/gui_edit.cpp): resets the current module screen. Most derived records put it in slot 1.
struct w_stream;                // rgl/mesh2/head_rec.h: the serializer slots 5 / 6 take
void reset_gui();

struct head_rec {
    virtual ~head_rec() {}                                          // slot 0  0x4c4c90
    virtual void reset_screen() {}                                  // slot 1  0x477b70 (empty)
    virtual void init_screen() {}                                  // slot 2  0x477b70 (the same function)
    virtual void frame_update(float) {}                           // slot 3  0x408030 (ret 4)
    virtual void slot_empty_4() {}                                  // slot 4  0x477b70 (the same function)
    virtual void write_stream(w_stream*) {}                        // slot 5  0x408030 (the same function)
    virtual void read_stream(w_stream*) {}                        // slot 6  0x408030 (the same function)
    // slot 7: pure here (the base table 0x5947d8 holds __purecall there); EVERY derived class overrides it - 0x48b170
    // (`return 1`) in most, 0x4280e0 (`return 0`) in ResultWindow, C4c3050, Rec34 and Rec38.
    virtual char pure_7() = 0;                                      // slot 7: __purecall; every derived class overrides it
    virtual char always_true() { return 1; }                        // slot 8  0x48b170
    int     w04;               // +4     0x4c2c50
    int     w08;               // +8     0x63
    char    pad0c[4];          // +0xc
    int     w10;               // +0x10 (the party / object the screen shows, set by whoever opens it)
    int     w14;               // +0x14 (a troop / object id some screens keep)
    int     w18;               // +0x18 (the menu state a screen returns to; the screens' slot 3 passes it to set_state)
    tstring name;              // +0x1c
    head_rec() : name() { w04 = 0; w08 = 0x63; }
    struct no_w04 {};
    head_rec(no_w04) : name() { w08 = 0x63; }               // for derived ctors that store w04 themselves, later
};

// A record's slot 1 is `release` (0x4cb680), which frees the pointer at +0x178. The original allocates these records
// with the plain operator new 0x53545e and its constructors write only the head and the member objects; ours left the
// rest as raw heap and release freed garbage (runtime workaround, not in the original). Zero everything above the
// base's head (vptr, w04, w08, name) when the record is allocated, before any constructor runs: zeroing it in the
// constructor body wiped the members it had just built (TerrainPage's frustum at +0x70, Rec118's tstring at +0x84).
#define MB_ZERO_TAIL(T) static void* operator new(size_t n) \
    { char* p = (char*)::operator new(n); if (p && n > 0x2c) memset(p + 0x2c, 0, n - 0x2c); return p; }

#endif // MB_GAME_HEAD_REC_H
