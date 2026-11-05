// int_vec_insert.h: the definition of IntVec::insert_n (0x43e7b0) for the sources that call it (or IntVec::push_back, whose slow path it is).
// It forwards to the library's out-of-line std::vector<int>::_Insert_n; the explicit specialization declaration lives here rather than in
// int_vec.h because declaring it changes the codegen of sources that merely include int_vec.h (Pool<RecJ>::reset in terrain_b/poly_mesh.cpp).
#ifndef GAME_INT_VEC_INSERT_H
#define GAME_INT_VEC_INSERT_H
#include "./int_vec.h"
#include <vector>
#include "./std_vec_insert_n.h"
struct IntStdVec : std::vector<int> {
    __forceinline void insert_n(int* p, size_t n, const int& v) { _Insert_n(iterator(p), n, v); }
};
inline void IntVec::insert_n(int* where, unsigned count, const int* val)   // 0x43e7b0
{
    ((IntStdVec*)this)->insert_n(where, count, *val);
}
#endif
