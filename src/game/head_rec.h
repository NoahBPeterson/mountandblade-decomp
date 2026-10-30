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
//   slot 1  0x477b70  slot_empty_1()       `ret` - an empty function (the base's default; most derived classes
//                                          override it with a screen reset)
//   slot 2  0x477b70  slot_empty_2()       the same empty function
//   slot 3  0x408030  scale_3(const float*)  `ret 4` - takes its argument and returns (shape_base::scale)
//   slot 4  0x477b70  slot_empty_4()       the same empty function (MissionBlocker overrides it with
//                                          MissionParams::init, 0x4cea10)
//   slot 5  0x408030  scale_5(const float*)  `ret 4` (the same function as slot 3)
//   slot 6  0x408030  scale_6(const float*)  `ret 4` (the same function as slot 3)
//   slot 7  0x5363c8  pure_7() = 0          __purecall: the base declares this one pure and EVERY derived class
//                                          overrides it (all seven tables hold 0x48b170 there)
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

// 0x4c6e00 (src/rgl/mesh2/gui_edit.cpp): resets the current module screen. Most derived records put it in slot 1.
void reset_gui();

struct head_rec {
    virtual ~head_rec() {}                                          // slot 0  0x4c4c90
    virtual void slot_empty_1() {}                                  // slot 1  0x477b70 (empty)
    virtual void slot_empty_2() {}                                  // slot 2  0x477b70 (the same function)
    virtual void scale_3(const float*) {}                           // slot 3  0x408030 (ret 4)
    virtual void slot_empty_4() {}                                  // slot 4  0x477b70 (the same function)
    virtual void scale_5(const float*) {}                           // slot 5  0x408030 (the same function)
    virtual void scale_6(const float*) {}                           // slot 6  0x408030 (the same function)
    // slot 7: the BASE table (0x5947d8) holds 0x5363c8 (__purecall) there, while EVERY derived table in the family
    // holds 0x48b170 (always1). Declaring it pure would make ~30 derived classes abstract for no gain, so it is
    // modelled concretely with always1's body: every derived table then matches, and only the base's own table
    // (which nothing instantiates) would differ in that slot.
    virtual char pure_7() { return 1; }
    virtual char always_true() { return 1; }                        // slot 8  0x48b170
    int     w04;               // +4     0x4c2c50
    int     w08;               // +8     0x63
    char    pad0c[0x10];       // +0xc
    tstring name;              // +0x1c
    head_rec() : name() { w04 = 0; w08 = 0x63; }
    struct no_w04 {};
    head_rec(no_w04) : name() { w08 = 0x63; }               // for derived ctors that store w04 themselves, later
};

#endif // MB_GAME_HEAD_REC_H
