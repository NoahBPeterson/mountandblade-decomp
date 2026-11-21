// rec_table.h: rec_table<T>, the {records, count} pair the module-file tables start with (rec34_table g_scripts 0xa75eb0,
// PresentationTable g_presentations 0xa75f2c, the mesh table 0x8ed4f58): a new[]-allocated array of T and its length.
// ONE definition. The three instantiations' constructors are identical, so the linker folded them into the one function
// at 0x4be520 (game/config/config.cpp).
#ifndef MB_REC_TABLE_H
#define MB_REC_TABLE_H

template <class T>
struct rec_table {                                  // {records, count} pair of the module-file tables
    T*    records;                                  // +0
    int   count;                                    // +4
    rec_table();                                    // 0x4be520 (game/config/config.cpp; the original is bracketed by WinLicense marker records)
};

#endif // MB_REC_TABLE_H
