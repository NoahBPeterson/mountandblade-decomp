// std_vec_push_back.h: std::vector<int>::push_back kept out of line (0x440b30, agent3/agent_combat.cpp). Separate from
// std_vec_out_of_line.h because most files expand push_back inline (0x51eb90, 0x51eda0); only a file whose original calls 0x440b30
// includes this (mesh_poly_ray.cpp, 0x52fbc0).
#ifndef GAME_STD_VEC_PUSH_BACK_H
#define GAME_STD_VEC_PUSH_BACK_H
#include <vector>
template<> void std::vector<int, std::allocator<int> >::push_back(const int&);                 // 0x440b30 (agent3/agent_combat.cpp)
#endif
