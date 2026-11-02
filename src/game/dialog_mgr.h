// dialog_mgr.h: DialogMgr, the conversation manager (global g_dialogs at 0x8ed50c8): the dialog records read from dialogs.txt, the
// current conversation state (partner troop / party, current dialog) and the two 0x400-line tables of the shown / selected lines.
// ONE definition; dialog_manager.h (the methods 0x460620..0x461140), rgl/mesh2 (the editor's export), triggers (constructor),
// inventory_panel (loader) and mission5 / script (state readers) include it. Needs only tstring and DialogRec.
#ifndef GAME_DIALOG_MGR_H
#define GAME_DIALOG_MGR_H
#include "./tstring.h"
#include "../include/dialog_rec.h"

struct stream_base;

struct DlgLine {                                           // 0x1c bytes
    int dialog;
    int idx;
    unsigned flags;
    tstring text;
};

struct DialogMgr {
    DialogMgr();                                           // 0x4c2470 (triggers/trigger_vec.cpp)
    ~DialogMgr();                                          // 0x4c4090 (rgl/mesh2/rec_tables.cpp)
    int count;                                             // +0
    DialogRec* recs;                                       // +4
    int f8;                                                // +8 current state
    int fc;                                                // +0xc partner troop
    int f10;                                               // +0x10 partner party
    int f14;                                               // +0x14
    int cur;                                               // +0x18 current dialog
    int cur_idx;                                           // +0x1c
    int nres;                                              // +0x20
    DlgLine list[0x400];                                   // +0x24
    DlgLine sel[0x400];                                    // +0x7024
    char fe024;                                            // +0xe024
    tstring cur_text;                                      // +0xe028
    int get_c8();                                          // 0x460620
    int speaker();                                         // 0x460640
    int listener();                                        // 0x460670
    tstring line_text(int i);                              // 0x4606a0 (thiscall, ret 8)
    int line_flags(int i);                                 // 0x4606c0
    void read(stream_base* s);                             // 0x460ab0
    bool available(int idx, char flag);                    // 0x460b90
    void refresh_text();                                   // 0x460c10
    void select(DlgLine* l);                               // 0x460d80
    void collect(DlgLine* out, int max, int* n, char flag);   // 0x460de0
    void advance();                                        // 0x460ff0
    void start(int a, int b, int c, int d, char f);        // 0x4610b0 (ret 0x14)
    void advance_all(int* n);                              // 0x461140
};


extern DialogMgr g_dialogs;                                // 0x8ed50c8

#endif // GAME_DIALOG_MGR_H
