// rec_table.h: rec_table, the {records, count} pair the module-file tables start with (rec34_table g_scripts 0xa75eb0,
// PresentationTable g_presentations 0xa75f2c, the mesh table 0x8ed4f58). ONE definition.
#ifndef MB_REC_TABLE_H
#define MB_REC_TABLE_H

struct rec_table {                                  // {records, count} pair of the module-file tables
    void* records;                                  // +0
    int   count;                                    // +4
    rec_table();                                    // 0x4be520 (game/config/config.cpp; the original is bracketed by WinLicense marker records)
};

#endif // MB_REC_TABLE_H
