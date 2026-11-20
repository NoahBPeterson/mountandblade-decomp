// scalar4.h: the 4-byte element of the std::vector<scalar4> instantiated in rgl/mesh2/mesh_2.cpp (_Insert_n 0x4cc430, resize 0x4cd010,
// _Construct_n 0x4cd1c0): the bucket vector of the glyph hash map (HashTab::insert 0x4d3c00, Font::Font 0x4d99d0). The original's
// element is a class (its erase copies with a loop, not memmove: VC7.1's _Ptr_cat), i.e. the hash's std::list iterator; this
// placeholder stands in until that hash_map is a real one. ONE definition (vec_elems.h, world_app.cpp).
#ifndef MB_SCALAR4_H
#define MB_SCALAR4_H
struct scalar4 { union { int v; void* p; }; };   // p: a bucket's list node
#endif // MB_SCALAR4_H
