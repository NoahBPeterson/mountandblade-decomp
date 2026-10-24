#ifndef SCREEN_INIT_H
#define SCREEN_INIT_H
// Slice s004ec4d0: the campaign screen's UI construction and the map-camera entity pass.
//  0x4ec4d0  Screen::init_screen  rebuilds the whole campaign UI: resets the GUI, creates the map bar
//                                overlay (+0x2c), the button overlay (+0x34) with its terrain map and the six
//                                translated menu buttons, two hint widgets, the paused banner (+0x5c), the
//                                wait-hint widget (+0x58) and the three-dimensional map widget (+0x30).
//  0x4ecce0  camera_mark_entities scales the camera zoom into g_game+0x13acc and walks the party list,
//                                setting +0x8b5 = 1 on every party whose scene object (+0x59c0) exists and
//                                whose flags (+0xa0) have 0x400 but not 0x80000.
// flags /O2 /MT /Gy /TP
#include <new>
#include <string.h>
#include "rgl.h"

// ---- engine string (16 bytes, see s0042a1f0/tstring.h) --------------------------------------------------
#include "../tstring.h"

// ---- widgets ------------------------------------------------------------------------------------------
// the 0x178-byte widget (base ctor 0x48ae20; see s004dada0)
struct Widget {
    Widget();                                   // 0x48ae20
    virtual void w00(int); virtual void w01(int); virtual void w02(int); virtual void w03(int);
    virtual void w04(int); virtual void w05(int); virtual void w06(int); virtual void w07(int);
    virtual void w08(int); virtual void w09(int); virtual void w10(int); virtual void w11(int);
    virtual void w12(int); virtual void w13(int); virtual void w14(int); virtual void w15(int);
    virtual void w16(int); virtual void w17(int); virtual void w18(int); virtual void w19(int);
    virtual void w20(int); virtual void w21(int);
    virtual void set_pos(const float* p);       // +0x58
    virtual void w23(int); virtual void w24(int);
    virtual void set_scale(const float* p);     // +0x64
    virtual void w26(int); virtual void w27(int);
    virtual void set_size(const float* p);      // +0x6c
    virtual void w29(int);
    virtual void set_flag(const int* f, int extra);   // +0x74
    virtual void w31(int);
    virtual void set_mesh(void* m, int kind);   // +0x7c
    virtual void w32(int); virtual void w33(int); virtual void w34(int); virtual void w35(int);
    virtual void w36(int); virtual void w37(int); virtual void w38(int); virtual void w39(int);
    virtual void w40(int); virtual void w41(int); virtual void w42(int); virtual void w43(int);
    virtual void w44(int); virtual void w45(int); virtual void w46(int); virtual void w47(int);
    virtual void w48(int);
    virtual void set_color(const int* c);       // +0x98
    virtual void w50(int); virtual void w51(int);
    virtual void set_text(const void* a, const void* b);    // +0xa4

    char pad0[0x121 - 4];
    char f121;                                  // +0x121
    char pad122[0x178 - 0x122];
    int f178;                                   // +0x178
};

// the overlay window (base ctor 0x4931a0, 0x190 bytes; see s004db660).  The two overlays built here use
// vtables 0x592848 and 0x597050; only +0x188 is read back by this slice.
struct Overlay {
    Overlay();                                  // 0x4931a0
    virtual void n00(int); virtual void n01(int); virtual void n02(int); virtual void n03(int);
    virtual void n04(int); virtual void n05(int); virtual void n06(int); virtual void n07(int);
    virtual void n08(int); virtual void n09(int); virtual void n10(int); virtual void n11(int);
    virtual void n12(int); virtual void n13(int); virtual void n14(int); virtual void n15(int);
    virtual void n16(int); virtual void n17(int); virtual void n18(int); virtual void n19(int);
    virtual void n20(int); virtual void n21(int);
    virtual void set_pos(const float* p);       // +0x58
    virtual void n23(int); virtual void n24(int);
    virtual void set_scale(const float* p);     // +0x64
    virtual void n26(int); virtual void n27(int);
    virtual void set_size(const float* p);      // +0x6c
    virtual void n29(int);
    virtual void set_flag(const int* f);        // +0x74
    virtual void n31(int);
    virtual void set_mesh(void* m, int kind);   // +0x7c
    virtual void n33(int); virtual void n34(int); virtual void n35(int); virtual void n36(int);
    virtual void n37(int); virtual void n38(int); virtual void n39(int); virtual void n40(int);
    virtual void n41(int); virtual void n42(int); virtual void n43(int); virtual void n44(int);
    virtual void n45(int); virtual void n46(int); virtual void n47(int); virtual void n48(int);
    virtual void n49(int);
    virtual void set_color(const int* c);       // +0x98
    virtual void n51(int); virtual void n52(int);
    virtual void set_text(const void* a, const void* b);    // +0xa4
    virtual void n54(int);
    virtual void n55(int); virtual void n56(int); virtual void n57(int); virtual void n58(int);
    virtual void n59(int); virtual void n60(int); virtual void n61(int); virtual void n62(int);
    virtual void n63(int); virtual void n64(int); virtual void n65(int); virtual void n66(int);
    virtual void n67(int); virtual void n68(int); virtual void n69(int); virtual void n70(int);
    virtual void n71(int); virtual void n72(int); virtual void n73(int); virtual void n74(int);
    virtual void n75(int); virtual void n76(int); virtual void n77(int); virtual void n78(int);
    virtual void n79(int); virtual void n80(int); virtual void n81(int); virtual void n82(int);
    virtual void n83(int); virtual void n84(int); virtual void n85(int); virtual void n86(int);
    virtual void n87(int); virtual void n88(int); virtual void n89(int); virtual void n90(int);
    virtual void n91(int); virtual void n92(int);
    virtual void set_text2(const void* a, const void* b);   // +0xac
    virtual void add_child(void* c);            // +0xc8

    char padC8[0x184 - 0xc8];
    int first;                                  // +0x184
    int last;                                   // +0x188
};

// std::vector<T> (0x10 bytes): control block, first, last, end
template <class T> struct vec {
    void* alloc;
    T* first;
    T* last;
    T* end;
    void push_back(const T& v);                 // 0x4927d0
    void push_back2(const T& v);                // 0x4936b0 (std::vector<T>::push_back)
    void push_back3(const T& v, int size);      // 0x40f8a0 (incremental copy_push_back)
};

// ---- the campaign screen (`this` of 0x4ec4d0 / 0x4ecce0) -----------------------------------------------
struct Screen {
    virtual void s00(int); virtual void s01(int); virtual void s02(int); virtual void s03(int);
    virtual void s04(int); virtual void s05(int); virtual void s06(int); virtual void s07(int);
    virtual void s08(int); virtual void s09(int);
    int f4;                                     // +0x4
    char pad8[0x2c - 8];
    Overlay* overlay;                           // +0x2c   map bar
    Widget* f30;                                // +0x30   map view (the 3D panel)
    Widget* f34;                                // +0x34   menu buttons
    Widget* f38;                                // +0x38
    Widget* f3c;                                // +0x3c   ui_party
    Widget* f40;                                // +0x40   ui_inventory
    Widget* f44;                                // +0x44   ui_character
    int f48;                                    // +0x48   ui_terrain quad
    Widget* f4c;                                // +0x4c   ui_camp
    Widget* f50;                                // +0x50   ui_quests
    Widget* f54;                                // +0x54   paused banner
    Widget* f58;                                // +0x58   wait hint
    Widget* f5c;                                // +0x5c   first hint
    Widget* f60;                                // +0x60   second hint
    char pad64[0x170 - 0x64];
    int labels[0x48];                           // +0x170 (copied into the map widget at +0x1ac)
    char pad290[0x2b8 - 0x170 - 0x120];
    float f2b8;                                 // +0x2b8  camera zoom
    char pad2bc[0x2d4 - 0x2bc];
    char f2d4;                                  // +0x2d4
    char f2d5;                                  // +0x2d5
    void init_screen();                         // 0x4ec4d0
    void camera_mark_entities();                // 0x4ecce0
};

// the 0x358-byte map widget (vptr 0x598698, base ctor 0x48ae20)
struct MapWidget : Widget {
    int f17c;                                   // +0x17c
    char pad180[0x1ac - 0x180];
    int labels[0x48];                           // +0x1ac
    int f2dc;                                   // +0x2dc (unused here)
};

// ---- game objects -------------------------------------------------------------------------------------
struct Rect {                                   // the 0x10-byte region copied into Screen+0x290
    int x, y, w, h;
};
struct Game {
    char pad0[0x78];
    int side_party;                             // +0x78
    char pad7c[0x88 - 0x7c];
    char f88;                                   // +0x88
    char pad89[0x139d4 - 0x89];
    int parties_end;                            // +0x139d4 (party-list end handle)
    char pad139d8[0x13acc - 0x139d8];
    float f13acc;                               // +0x13acc (scaled zoom)
    char pad13ad0[0x1a090 - 0x13ad0];
    int f1a090;                                 // +0x1a090
};

struct GuiMgr { void reset(); void add_root(void* p); };
struct ResMgr { void* get_object(const tstring& n); };
struct StrTab {                                 // 0x5e2a18
    char pad0[0xdf34];
    const void* find(const tstring& name);      // 0x43a1e0
};

#define g_gui     (*(GuiMgr*)0x5d7080)
#define g_resmgr  (*(ResMgr*)0x5d6fa0)
#define g_strtab  (*(StrTab*)0x5e2a18)

extern Game* g_game;                            // 0x5e2930
extern char g_5e212c;                           // 0x5e212c
extern char g_5d8788;                           // 0x5d8788
extern int g_5a9fb0;                            // 0x5a9fb0 (no terrain camera)
extern int g_5e28fc;                            // 0x5e28fc
extern int g_a732b4;                            // 0xa732b4 (current scene)
extern int g_a70438;                            // 0xa70438 (last-loaded scene)
extern float g_590bb8;                          // 0x590bb8
extern float g_549cdc;                          // 0x549cdc (1.0f)
extern float g_54b3b4;                          // 0x54b3b4

// ---- callees -------------------------------------------------------------------------------------------
void ctor_4931a0(void* p);                      // 0x4931a0 (ecx = object)
void __fastcall ctor_494310(void* p);           // 0x494310 (ecx = 0x5aa128: the campaign UI root)
void* clone_41c6d0(void* m);                    // 0x41c6d0
void* __fastcall ui_terrain_quad(Overlay* o, void* name);   // 0x4e5570
Widget* new_medium_button(Overlay* o, void* text, float x, float y, float z);   // 0x4dce70
struct vector2 { float x, y; vector2() {} vector2(float a, float b) { x = a; y = b; } };
Widget* make_hint(const tstring& text, int a, Overlay* o, vector2 p, vector2 s);   // 0x4dada0
void __fastcall ctor_4ed630(MapWidget* w);       // 0x4ed630

// the party manager at g_game+0xf8
struct PartyMgr {
    int first_id();                             // 0x460d20 (first live party handle)
    void next(unsigned* id);                    // 0x460ca0 (advance *id)
    int index_of(int id);                       // 0x48ad00 (handle -> dense index)
    char* at(int idx) {                         // block table at +0x138d0, 0x5ac8-byte records
        void** page = *(void***)((char*)this + 0x138d0);
        return (char*)page[idx >> 4] + (idx & 0xf) * 0x5ac8;
    }
};
struct PartyRec {                               // 0x5ac8-byte party record
    char pad0[0xa0];
    long long flags;                            // +0xa0
    char padA8[0x8b5 - 0xa8];
    char f8b5;                                  // +0x8b5 (marked)
    char padA6[0x59c0 - 0x8b6];
    int scene_obj;                              // +0x59c0
};

struct GameScene {                              // g_psys (0xa70248)
    void finish(int scene);                     // 0x4711e0
    void add_scene_obj(void* p);                // 0x468c20
    void flush();                               // 0x463cc0
    void reset();                               // 0x47fef0
};
#define g_psys (*(GameScene*)0xa70248)

static Widget* new_widget(int size)
{
    Widget* w = (Widget*)operator new(size);
    if (w != 0)
        new (w) Widget();
    else
        w = 0;
    return w;
}

#endif
