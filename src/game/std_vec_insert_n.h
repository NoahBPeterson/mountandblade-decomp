// std_vec_insert_n.h: the out-of-line std::vector<int>::_Insert_n (0x410610 / 0x43e7b0: the linker folded the instances of every
// four-byte-element vector into it; rgl/particles/mesh_obj.cpp defines it). An explicit specialization declared without a
// definition makes the inlined push_back's slow path a call, as in the original. Unlike std_vec_out_of_line.h this leaves
// push_back itself to be expanded in the caller.
#ifndef GAME_STD_VEC_INSERT_N_H
#define GAME_STD_VEC_INSERT_N_H
#include <vector>
template<> void std::vector<int, std::allocator<int> >::_Insert_n(std::vector<int, std::allocator<int> >::iterator, size_t, const int&);   // 0x410610
#endif
