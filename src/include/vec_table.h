// vec_table.h: the 32-entry table of zeroed 16-byte vectors (ctor 0x417cb0 allocates 0x200 bytes with operator new[] and stores
// count = 0x20), 8 bytes: data +0, count +4. ONE definition for src/rgl (resource.h: frame_set::table, ctor body in
// resource_manager.cpp) and src/game/char_sheet (screen.h: the table embedded at Screen+0x4183d8, followed by the mesh at +0x4183e0).
// vector4 is only forward-declared: it is rgl::vector4 of rgl_math.h or of match/include/rgl.h depending on the includer.
#ifndef MB_VEC_TABLE_H
#define MB_VEC_TABLE_H
namespace rgl { struct vector4; }

struct vec_table {
    rgl::vector4* data;                        // +0
    int count;                                 // +4
    vec_table();                               // 0x417cb0
};

#endif // MB_VEC_TABLE_H
