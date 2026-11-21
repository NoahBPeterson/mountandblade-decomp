// prsnt_table.h: PresentationTable, the presentations of presentations.txt (g_presentations 0xa75f2c, 0x14 bytes): the
// {records, count} pair of rec_table, then the record index of each of the engine's 3 named presentations.
// The records are the 0x24-byte PresentationRec (prsnt_rec.h).
#ifndef MB_PRSNT_TABLE_H
#define MB_PRSNT_TABLE_H
#include "./rec_table.h"
#include "./prsnt_rec.h"                         // PresentationRec

struct stream_base;
struct PresentationTable : rec_table<PresentationRec> {              // 0x14 bytes
    int used[3];                                    // +0x08 the record index of each named presentation (g_prsnt_names, -1 = missing)
    ~PresentationTable();                           // 0x4c6900 (rgl/mesh2/gui_edit.cpp): deletes the record array; the atexit destructor
    void load_used();                               // 0x4bdec0 (game/config/config.cpp)
    void fill_records(stream_base* s);                // 0x4c6950 (rgl/mesh2/gui_edit.cpp: reads the count and every record, then load_used)
};
extern PresentationTable g_presentations;           // 0xa75f2c (defined in rgl/mesh2/gui_edit.cpp)

#endif // MB_PRSNT_TABLE_H
