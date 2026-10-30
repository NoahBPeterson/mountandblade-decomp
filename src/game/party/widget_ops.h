// widget_ops.h: the calls the party window and the loading screen make through a widget's vtable.
// The widget family (vtables 0x591c80 base, 0x599bf8 panel, 0x599d28 slot, 0x591ee8 icon, ...) is `MissionUi` in
// src/game/ui/mission_ui.h, but that header only names the slots mission_ui.cpp calls and gives some of them other
// signatures (+0x58 d22(void*), +0x64 set_xy, +0x74 d29(), +0x80 d32(int,int), +0xa4 get_text_size(tstring*, float*),
// nothing at +0xc8). This header is therefore a view: a pure interface, no data, laid over the same objects with a
// cast (WOPS). Each named slot cites the function that calls it; the vNN fillers are named by their offset.
#ifndef GAME_PARTY_WIDGET_OPS_H
#define GAME_PARTY_WIDGET_OPS_H
#include "../tstring.h"

struct WidgetOps {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c(); virtual void v10(); virtual void v14();
    virtual void v18(); virtual void v1c(); virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c(); virtual void v40(); virtual void v44();
    virtual void v48(); virtual void v4c(); virtual void v50(); virtual void v54();
    virtual void set_pos(float* xy);                    // +0x58 (0x501a20, 0x502320, 0x502230 area: widget position / offset pair)
    virtual void v5c(); virtual void v60();
    virtual void set_scale(float* xy);                  // +0x64 (0x502230, 0x502320)
    virtual void v68(); virtual void v6c(); virtual void v70();
    virtual void set_flags(unsigned* f);                // +0x74 (0x488a40 stores *f at +0x148; 0x501a20 passes 0x10)
    virtual void v78();
    virtual void add_image_a(int layer, void* mesh);    // +0x7c (0x502230: slot icon from the cloned "inv_slot" mesh)
    virtual void add_image(int layer, void* mesh);      // +0x80 (0x5029f0: item mesh on layer 2; 0x5024e0: strip icon on layer 0)
    virtual void v84(); virtual void v88(); virtual void v8c(); virtual void v90(); virtual void v94(); virtual void v98();
    virtual void v9c(); virtual void va0();
    virtual void set_text(float* size, const tstring* text);    // +0xa4 (0x501a20, 0x51c5a0; MissionUi::set_text 0x488b50 returns the text size in two floats)
    virtual void va8(); virtual void vac(); virtual void vb0(); virtual void vb4(); virtual void vb8(); virtual void vbc();
    virtual void vc0(); virtual void vc4();
    virtual void add_child(void* child);               // +0xc8 (0x501a20, 0x502320, 0x5024e0)
    // plain members of the widget base (src/game/inventory_panel/note_window.cpp reaches them through the same cast)
    void draw();                                   // 0x488d50 (this = widget)
    int  flag1();                                    // 0x50cc60 (this = widget)
};
#define WOPS(w) ((WidgetOps*)(w))


#endif // GAME_PARTY_WIDGET_OPS_H
