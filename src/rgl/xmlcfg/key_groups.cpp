//-----------------------------------------------------------------------------
// File: key_groups.cpp  (0x4320b0..0x432350)
//
// Desc: the key-code group tables: the first code of each triple of ids (0x97.., 0xc7..0xed). The original compiled
//       these two functions in their own object; id_matches_dir (game_util.cpp, 0x4326d0) calls group_base as an
//       external function. Functions in ascending address order. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "key_groups.h"

// Key-code test: false for the first code of each triple group_base() returns, 0xc7 + 3k.
// @ 0x4320b0
int __cdecl not_group_base(int x)
{
    int r = 1;
    if (x == 0xc7 || x == 0xca || x == 0xcd || x == 0xd0 || x == 0xd3 || x == 0xd6 || x == 0xd9 ||
        x == 0xdc || x == 0xdf || x == 0xe2 || x == 0xe5 || x == 0xe8 || x == 0xeb)
        r = 0;
    return r;
}

// Key codes in the Module System's header_triggers numbering: key_home 0xc7, key_up 0xc8, key_page_up 0xc9,
// key_left 0xcb, key_right 0xcd, key_end 0xcf, key_down 0xd0, key_page_down 0xd1, key_insert 0xd2, key_delete 0xd3,
// mouse buttons 0xe0..0xe7.
// @ 0x432130
int __cdecl group_base(int id)
{
    int r = -1;
    if (id == 0xc7 || id == 0xc8 || id == 0xc9) {
        r = 0xc7;
    } else {
        if (id == 0x97 || id == 0x98 || id == 0x99) return 0x97;
        if (id == 0xca || id == 0xcb || id == 0xcc) return 0xca;
        if (id == 0xcd || id == 0xce || id == 0xcf) return 0xcd;
        if (id == 0xd0 || id == 0xd1 || id == 0xd2) return 0xd0;
        if (id == 0xd3 || id == 0xd4 || id == 0xd5) return 0xd3;
        if (id == 0xd6 || id == 0xd7 || id == 0xd8) return 0xd6;
        if (id == 0xd9 || id == 0xda || id == 0xdb) return 0xd9;
        if (id == 0xdc || id == 0xdd || id == 0xde) return 0xdc;
        if (id == 0xdf || id == 0xe0 || id == 0xe1) return 0xdf;
        if (id == 0xe2 || id == 0xe3 || id == 0xe4) return 0xe2;
        if (id == 0xe5 || id == 0xe6 || id == 0xe7) return 0xe5;
        if (id == 0xe8 || id == 0xe9 || id == 0xea) return 0xe8;
        if (id == 0xeb || id == 0xec || id == 0xed) return 0xeb;
    }
    return r;
}
