// emit_ids.h: the table of effect / emitter ids the mission sets up (0x8ed5098, 12 ints up to the dialog manager at 0x8ed50c8):
// [2] / [3] the default voice effects (agent_logic), [4] dense fog, [6] thin fog (terrain_b mission post_spawn),
// [4] is also the default effect id agent_logic compares against, [7..9] the three ground fog layers. No includes.
#ifndef MB_EMIT_IDS_H
#define MB_EMIT_IDS_H
extern int g_emit_id[12];                       // 0x8ed5098
#endif // MB_EMIT_IDS_H
