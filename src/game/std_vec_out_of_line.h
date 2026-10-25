// std_vec_out_of_line.h: std::vector instances the original keeps out of line. An explicit specialization declared
// without a definition makes every use a call to the out-of-line instance, which another module defines
// (an explicit instantiation there), so the callers link against it under its std:: name. Callers that hold the
// vector as a layout-identical view class (IntVec, I64Vec) cast the view to the std::vector.
#ifndef GAME_STD_VEC_OUT_OF_LINE_H
#define GAME_STD_VEC_OUT_OF_LINE_H
#include <vector>
template<> void std::vector<int, std::allocator<int> >::resize(size_t, int);                  // 0x43f310 (agent3/agent_combat.cpp)
template<> void std::vector<__int64, std::allocator<__int64> >::resize(size_t, __int64);      // 0x493090 (world_data/world_data.cpp)
template<> void std::vector<int, std::allocator<int> >::push_back(const int&);                 // 0x440b30 (agent3/agent_combat.cpp)
template<> void std::vector<int, std::allocator<int> >::_Insert_n(std::vector<int, std::allocator<int> >::iterator, size_t, const int&);   // 0x410610 (rgl/particles/mesh_obj.cpp; also the vector<T*> instance)
// the protected _Insert_n of a std::vector<int>, for the pointer-vector views that call the shared instance
struct int_stdvec : std::vector<int> {
    __forceinline void insert_n(int* p, size_t n, const int& v) { _Insert_n(iterator(p), n, v); }
};
template<> std::vector<int, std::allocator<int> >& std::vector<int, std::allocator<int> >::operator=(const std::vector<int, std::allocator<int> >&);   // 0x43f0a0 (agent3/agent_combat.cpp)
#endif
