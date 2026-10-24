// mission_3.h: classes of mission_3.cpp (0x48c330..0x48e5d0): views of the objects whose members sit in that object file and
// that no shared header declares (the game data, party, troop, item, quest and faction members are declared in the shared
// headers src/include/{game_data,party,troop,item_kind,game_recs}.h and src/game/{item_ref,slot_array}.h; the widget
// setters in src/game/config/widget_mgr.h). Names provisional; every offset cites the function that reads it.
#ifndef GAME_MISSION_UI2_MISSION_3_H
#define GAME_MISSION_UI2_MISSION_3_H
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../ui/mission_ui.h"

// ---- 0x48cec0 / 0x48cf40: the current site's records, as the script triggers see them -------------------------------
// The game data object (0x5e2930) as these two lookups see it: the index of the current prop record and two keys.
struct SiteKeys {
    char pad0[0x1a1f0];
    int cur;                                // +0x1a1f0 index of the current prop record, < 0 when none (0x48cec0, 0x48cf40)
    char pad1a1f4[0x1a1fc - 0x1a1f4];
    int key_a;                              // +0x1a1fc (0x48cec0: contexts -0x32, -0x33)
    int key_b;                              // +0x1a200 (0x48cf40: contexts -0x32, -0x46)
};
// A prop record's scene entity (Rec1c8::ent, an EntFlag* in src/include/rec1c8.h): two int vectors, begin/end at +4/+8 inside each 0x10-byte header.
struct EntVec {
    int pad0;                               // +0x00
    int* first;                             // +0x04
    int* last;                              // +0x08
    int* cap;                               // +0x0c
};
struct PropEnt {
    char pad0[0x108];
    EntVec a;                               // +0x108 (0x48cec0)
    EntVec b;                               // +0x118 (0x48cf40)
};

// ---- 0x48cfc0: a container of object lists ------------------------------------------------------------------------------
struct Owned {
    char pad0[0x2c];
    Owned* owner;                           // +0x2c
    char pad30[0x64 - 0x30];
    int refs;                               // +0x64
};
struct ListHolder {
    char pad0[0x38];
    int* lists;                             // +0x38 (begin/end pairs, stride 0x10)
    char pad3c[0x84 - 0x3c];
    int count;                              // +0x84
    void set_owner(void* owner);            // 0x48cfc0
};

#include "./field_panel.h"

// ---- 0x48d5f0, 0x48d690: containers of 0x1dc-byte elements and of 8-byte records ---------------------------------------------
struct ArrElem {
    virtual int v00();
    virtual int v04();
    virtual int v08(int arg);
    char pad8[0x1a0 - 4];
    int* a;                                 // +0x1a0
    int* b;                                 // +0x1a4
};
struct ArrHolder {
    char pad0[4];
    ArrElem* base;                          // +4
    int lookup(int index, int arg);         // 0x48d5f0
};
struct Pair690 { int a; int b; };
struct Cont690 {
    char pad0[0x44];
    char* base;                             // +0x44
    char* end;                              // +0x48
    int get(unsigned index);                // 0x48d690 (only the first dword is returned)
};

struct FieldGroup { int n; char pad[0x68]; };     // a 0x6c-byte force-group record: the member count first (0x48d940)
// ---- 0x48d940: a force label ---------------------------------------------------------------------------------------------
struct ForceLabel {
    char pad0[0x4c];
    int n0;                                 // +0x4c member count of the first 0x6c-byte force group (infantry)
    char pad50[0xb8 - 0x50];
    int n1;                                 // +0xb8 second group (archers)
    char padbc[0x124 - 0xbc];
    int n2;                                 // +0x124 third group (cavalry)
    void make_label(char* out);             // 0x48d940
};

// ---- 0x48db20..0x48e590: the widgets as the overlay setters see them -----------------------------------------------------------
// A child mesh of a widget (the five MissionUi tracks): only its colour word.
struct OvMesh {
    char pad0[0x3c];
    unsigned color;                         // +0x3c 0xAARRGGBB
};
// The widget vtable slots these setters call (the widget_mgr.h WVt names others).
struct OvVt {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28();
    virtual void set_value(int v);          // +0x2c
    virtual void v30();
    virtual void set_value_f(float v);      // +0x34
    virtual void v38(); virtual void v3c(); virtual void v40(); virtual void v44(); virtual void v48();
    virtual void v4c(); virtual void v50(); virtual void v54(); virtual void v58(); virtual void v5c();
    virtual void v60(); virtual void v64(); virtual void v68(); virtual void v6c(); virtual void v70();
    virtual void v74(); virtual void v78(); virtual void v7c(); virtual void v80(); virtual void v84();
    virtual void v88(); virtual void v8c(); virtual void v90();
    virtual void get_state(float* out);     // +0x94 (reads the widget's state back after a colour change)
    virtual void set_color(unsigned* c);    // +0x98
    virtual void set_color2(unsigned* c);   // +0x9c
    virtual void va0(); virtual void va4(); virtual void va8(); virtual void vac(); virtual void vb0();
    virtual void vb4(); virtual void vb8(); virtual void vbc();
    virtual int type();                     // +0xc0 widget kind (0 label, 1, 2/3 sliders / bars, 4, 5 list, 6, 7 panel, 8 check box, 9)
    virtual void vc4();
    virtual void vc8(int v);                // +0xc8
};
struct OvObj : OvVt {
    char pad4[0x108 - 4];
    OvMesh* m108;                           // +0x108 child meshes (MissionUi::tracks[0..4])
    OvMesh* m10c;                           // +0x10c
    OvMesh* m110;                           // +0x110
    OvMesh* m114;                           // +0x114
    char pad118[0x14c - 0x118];
    unsigned color14c;                      // +0x14c
    unsigned color150;                      // +0x150
    char pad154[0x160 - 0x154];
    int min160;                             // +0x160 (list widgets: lowest value)
    int max164;                             // +0x164
    float v168;                             // +0x168 value
    float lo16c;                            // +0x16c range
    float hi170;                            // +0x170
    char pad174[0x180 - 0x174];
    bool on180;                             // +0x180 (check box state)
    char pad181[0x184 - 0x181];
    int kind184;                            // +0x184
    char pad188[0x194 - 0x188];
    int f194;                               // +0x194
    char pad198[0x1a8 - 0x198];
    int f1a8;                               // +0x1a8
};
// The four child-mesh columns of a node (0x48db20)
struct OvNode {
    char pad0[0x108];
    FieldShape* s0;                         // +0x108
    FieldShape* s1;                         // +0x10c
    FieldShape* s2;                         // +0x110
    FieldShape* s3;                         // +0x114
    FieldShape* s4;                         // +0x118
};

extern int g_fixed_point_multiplier;       // 0xa70460 the script's fixed-point scale (set_limits divides by it)

#endif // GAME_MISSION_UI2_MISSION_3_H
