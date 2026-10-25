// elem_calls.h: the three vector elements of mesh_2.cpp (E50 0x50, E68 0x68, E176 0xb0; src/include/vec_elems.h and e68.h
// define them) as the option panel's loaders call them: their copy constructor and copy assignment are out of line in the
// original (mesh_2.cpp: ??0E50 0x4cd080, ??0E68 0x4cd100, ??0E176 0x4cd170, operator= 0x4cd270 / 0x4cd2f0 / 0x4cd380). The
// canonical definitions are inline, so a body here would be inlined; these are the same classes declared without bodies and
// without members (only the size matters to the callers).
#ifndef OPTIONS_PANEL_ELEM_CALLS_H
#define OPTIONS_PANEL_ELEM_CALLS_H
struct E50 {
    int pad[0x50 / 4];
    E50(const E50& o);   // 0x4cd080
    E50& operator=(const E50& o);   // 0x4cd270
};
struct E68 {
    int pad[0x68 / 4];
    E68(const E68& o);   // 0x4cd100
    E68& operator=(const E68& o);   // 0x4cd2f0
};
struct E176 {
    int pad[0xb0 / 4];
    E176(const E176& o);   // 0x4cd170
    E176& operator=(const E176& o);   // 0x4cd380
};
#endif
