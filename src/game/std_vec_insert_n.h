// std_vec_insert_n.h: the out-of-line std::vector<rgl::mesh*>::_Insert_n at 0x410610 (rgl/particles/mesh_obj.cpp defines it). An
// explicit specialization declared without a definition makes the inlined push_back's slow path a call, as in the original. Unlike
// std_vec_out_of_line.h this leaves push_back itself to be expanded in the caller. (The int list's instance is 0x43e7b0,
// agent4/agent_logic.cpp: std_vec_out_of_line.h / int_vec_insert.h.)
#ifndef GAME_STD_VEC_INSERT_N_H
#define GAME_STD_VEC_INSERT_N_H
#include <vector>
namespace rgl { struct mesh; }
template<> void std::vector<rgl::mesh*, std::allocator<rgl::mesh*> >::_Insert_n(std::vector<rgl::mesh*, std::allocator<rgl::mesh*> >::iterator, size_t, rgl::mesh* const&);   // 0x410610
#endif
