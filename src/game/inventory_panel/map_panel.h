// map_panel.h: the campaign map screen's target picker, info panel and label helpers (0x5191e0..0x51c090). `this` is the
// CampaignScreen (../campaign_screen.h): its map widget f30, the redraw cooldown f2cc (0x51abd0 resets it), the label fade
// timer f2d0 (0x51a420 resets it), the hover flags f2d4 / f2d5 and the picked party f2d8. MapPanel only names these
// functions' class (it has no members of its own).
#ifndef GAME_INVENTORY_PANEL_MAP_PANEL_H
#define GAME_INVENTORY_PANEL_MAP_PANEL_H
#include "../campaign_screen.h"

struct MapPanel : CampaignScreen {
    void update(int type, int id);           // 0x51abd0
    void update_target();                    // 0x5191e0
    void set_label(int id);                  // 0x51a420 ret 4
    int  draw_party_line(int index);         // 0x51c090 ret 4
};
#endif
