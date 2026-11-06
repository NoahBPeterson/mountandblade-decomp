// map_panel.h: the campaign map screen object (MapPanel) and its view (MapView at MapPanel+0x30) as map_panel.cpp sees them.
// The two timers are two different floats: +0x2cc is the redraw cooldown 0x51abd0 resets, +0x2d0 the label fade timer 0x51a420 resets.
#ifndef GAME_INVENTORY_PANEL_MAP_PANEL_H
#define GAME_INVENTORY_PANEL_MAP_PANEL_H
namespace rgl { struct vector4; }


struct MapView {
    virtual void v00(); virtual void v08(); virtual void v10(); virtual void v18();
    virtual void v20(); virtual void v28(); virtual void v30(); virtual void v38();
    virtual void v40(); virtual void v48(); virtual void v50(); virtual void v58();
    virtual void v60(); virtual void v68(); virtual void v70(); virtual void v78();
    virtual void v80();
    virtual char is_key_down(int key);                              // +0x44
    char  pad04[0x274 - 4];                  // (the next member, the view/projection matrix used by to_screen, is at 0x274)
};

struct MapPanel {
    char      pad00[0x30];
    MapView*  view;                          // +0x30
    char      pad34[0x2cc - 0x34];
    float     timer;                         // +0x2cc  redraw cooldown (0x51abd0)
    float     f2d0;                          // +0x2d0  label fade timer (0x51a420)
    char      f2d4;                          // +0x2d4
    char      f2d5;                          // +0x2d5  label column is being hovered
    char      pad2d6[0x2d8 - 0x2d6];
    int       f2d8;                          // +0x2d8  currently picked party id
    void update(int type, int id);           // 0x51abd0
    void update_target();                    // 0x5191e0
    void set_label(int id);                  // 0x51a420 ret 4
    int  draw_party_line(int index);         // 0x51c090 ret 4
};
#endif
