// dialog_manager.h: classes of dialog_manager.cpp (0x460160..0x461200): the conversation manager (DialogMgr), the
// dialog records read from the module file (DialogRec, Module System `dialogs`), the script blocks they carry
// (DialogBlock / Elem: condition and consequence operation lists), the string-register expander (ScriptState) and the
// parts of the game globals they touch. Game code. All names provisional; every offset is read by a function of
// this module (cited by address).
#ifndef GAME_DIALOG_MANAGER_H
#define GAME_DIALOG_MANAGER_H
#include "engine/core/tstring.h"

// ---- binary stream: the shared stream_base (slots used: 13..21 read, 0..12 write)
#include "engine/io/stream.h"


#include "game/data/str_table.h"

// ---- parties, troops and the game globals: the shared classes (Party / PartyMgr / Troop / GameData / g_game 0x5e2930) ----------------
#include "game/data/game_data.h"

// ---- the script interpreter's state object g_script (0xa70440): string registers, the {token} expander, ... (one class) ----------
#include "game/script/script_state.h"

// ---- script blocks and dialog records: the shared classes (Elem / DialogBlock / DialogRec) -----------------------------------------------------
#include "game/dialog/dialog_rec.h"

#include "game/dialog/dialog_mgr.h"                                    // DlgLine, DialogMgr (g_dialogs 0x8ed50c8)
#include "engine/io/text_stream.h"

#endif // GAME_DIALOG_MANAGER_H
