// record_slots_head_rec.cpp: the slot 1/2/3 overrides of the head_rec record classes declared by game/data/head_rec_rgl_mesh2.h (vtables 0x595740..0x596c68).
// The image's tables hold there functions that our source defines under other classes' names (MissionScreen::init_*,
// Hud::on_*, ResultWindow::init, ...), whose `this` is a different view of the record than the 0x30..0x1200-byte record
// that owns the table. Each override is a forwarding stub: it compiles to a bare `jmp` to the function the image's
// slot names (declared by a minimal local view of its class; the class name must match the mangled name). Slot 3's
// base signature is frame_update(const float*) while the real functions take int/float: the stub forwards the 4-byte
// argument unchanged.
#include "game/data/head_rec_rgl_mesh2.h"
#include "game/campaign/campaign_screen.h"
#include "game/ui/gui_manager.h"     // g_gui

// CampaignScreen (game/campaign/campaign_screen.h)
void CampaignScreen::reset_screen() { g_gui.reset(); }

