// externs.h: objects and functions of other translation units that the party-window and game modules call.
// Each declaration cites the address; shapes (calling conventions) are read off the call sites. A class or
// function that another src/ header also declares is declared again here with the shape these call sites show
// (the headers differ in argument types), see MIGRATION.md.
#ifndef GAME_PARTY_EXTERNS_H
#define GAME_PARTY_EXTERNS_H
#include "../../include/image_addrs.h"
#include "../tstring.h"
#include "../../rgl/stream.h"

#include "../str_table.h"
extern StrTable g_strtab;                   // 0x5e2a18  // old name of g_strtable, removed after the names merge

// the engine log (global 0x5a9f60): stream slot 10 (+0x28) writes a C string
inline stream_base* log_stream_ptr() { return (stream_base*)&orig::g_log; }

#endif // GAME_PARTY_EXTERNS_H
