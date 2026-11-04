// e68.h: E68, the 0x68-byte element of the vector at src/rgl/mesh2/mesh_2.cpp (vector<E68>, destructor calls the out-of-line
// free_materials 0x4c8330) and of the table walked by src/game/ui/mission_ui.cpp (Vec68::count 0x48ace0: (end - begin) / 0x68).
// ONE definition for src/rgl/mesh2 and src/game/ui. The member at +0x38 is the shared DialogBlock (copy constructor 0x4bd7a0), scalar4
// the 4-byte element of the vector at +0x54.
#ifndef MB_E68_H
#define MB_E68_H
#include <vector>
#include "../game/tstring.h"
#include "./dialog_rec.h"
struct stream_base;
struct BlockMat;
struct skin;

struct scalar4 { int v; };           // 4-byte scalar element (E68::v holds the block's materials, as pointers)

struct E68 {                                // 0x68: the map (tableau) block
    tstring s;                              // 0x00
    int b0, b1, b2, b3, b4;                 // 0x10
    int b5;                                 // 0x24
    float b6, b7, b8, b9;                   // 0x28 (metres: read in millimetres, 0x4c83a0)
    DialogBlock m;                          // 0x38
    std::vector<scalar4> v;                 // 0x54
    skin* t;                                // 0x64 the tableau material (0x4c83a0)
    E68() {}
    E68(const E68& o);                      // 0x4cd100 (mesh_2.cpp)
    E68& operator=(const E68& o);           // 0x4cd2f0 (mesh_2.cpp)
    void free_materials();                  // 0x4c8330 (src/rgl/mesh2/gui_edit.cpp)
    void read_terrain(stream_base* stream); // 0x4c83a0 (src/rgl/mesh2/gui_edit.cpp)
    ~E68() { free_materials(); }
};

#endif // MB_E68_H
