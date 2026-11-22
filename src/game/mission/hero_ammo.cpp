//-----------------------------------------------------------------------------
// File: hero_ammo.cpp  (0x513eb0..0x513f80)
//
// Desc: GameData::clear_hero_ammo, split from mission_context.cpp. It sits between mission_ctx's count_hero_items and
//       setup but was compiled without the party accessor bodies those two see (it reloads the party page vector
//       [esi+0x138d0] after the Map10007::lookup call 0x48ad00): a separate original unit (docs/codegen.md 16, "a
//       same-unit callee body can hide a translation-unit boundary"). Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "platform/image_addrs.h"
#include "game/mission/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): listed directly (the resolution audit reads only the source's own includes)
#include "game/config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "game/common/game_globals.h"   // g_trial_mode (canonical)
#include "game/mission/mission_context.h"
#include "engine/render/render_batch.h"   // g_batch (0x5fe898): counter 0x418828
#include "game/terrain/terrain.h"   // Terrain, g_terrain (0xa76630)
#include "game/ui/window_mgr.h"      // g_windows slot 11 (0xa739ac)
#include "game/common/game_globals.h"      // g_bit (0x5a8e68)
#include "game/mission/inv_common.h"      // (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "game/data/str_table.h"
#include "game/mission/mission.h"
#include "game/mission/placed_obj.h"      // PlacedObj: the by-value argument of SceneSub::f (constructor 0x437830)
#include "game/data/game_data.h"
#include "game/party/party.h"
#include "game/data/troop.h"
#include "game/ui/gui_manager.h"
#include "game/ui/widgets/ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/ui/widgets/container.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/campaign/map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/campaign/campaign_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/common/shared_vars.h"
#include "game/input/game_keys.h"
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object

// @ 0x513eb0
void GameData::clear_hero_ammo(unsigned id)
{
    GameData* g = this;
    PartyMgr* m = &g->parties;
    if (id < m->serial) {
        int idx = m->lookup(id);
        if (idx >= 0 && m->atu(idx)->type == 1) {
            Party* p = party_at(m, m->lookup(id));
            for (int i = 0; i < p->nstacks; i++) {
                if ((g->troops[p->stacks[i].troop].flags30 & 0x10) != 0 && p->stacks[i].wounded != 0)
                    p->stacks[i].wounded = 0;
            }
        }
    }
}
