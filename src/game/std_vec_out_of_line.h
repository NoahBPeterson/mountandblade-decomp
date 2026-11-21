// std_vec_out_of_line.h: std::vector instances the original keeps out of line. An explicit specialization declared
// without a definition makes every use a call to the out-of-line instance, which another module defines
// (an explicit instantiation there), so the callers link against it under its std:: name.
#ifndef GAME_STD_VEC_OUT_OF_LINE_H
#define GAME_STD_VEC_OUT_OF_LINE_H
#include <vector>
#ifndef STD_VECTOR_INT_INSTANTIATED_HERE     // agent4/agent_logic.cpp instantiates these two itself
template<> void std::vector<int, std::allocator<int> >::resize(size_t, int);                  // 0x43f310 (agent4/agent_logic.cpp)
#endif
template<> void std::vector<__int64, std::allocator<__int64> >::resize(size_t, __int64);      // 0x493090 (world_data/world_data.cpp)
#ifndef STD_VECTOR_INT_INSTANTIATED_HERE
template<> void std::vector<int, std::allocator<int> >::_Insert_n(std::vector<int, std::allocator<int> >::iterator, size_t, const int&);   // 0x43e7b0 (agent4/agent_logic.cpp)
#endif
#ifndef STD_VECTOR_INT_INSTANTIATED_HERE
template<> std::vector<int, std::allocator<int> >& std::vector<int, std::allocator<int> >::operator=(const std::vector<int, std::allocator<int> >&);   // 0x43f0a0 (agent4/agent_logic.cpp)
#endif
#endif
