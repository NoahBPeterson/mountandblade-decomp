// vec_xlen.h: std::vector<int>::_Xlen (0x43e2a0), the one out-of-line instance of the library's "vector<T> too long" report
// (the linker folded every vector<T>::_Xlen into it); defined by an explicit instantiation in game/agent3/agent_combat.cpp.
// An explicit specialization declared without a definition makes every use a call. _Xlen is protected, so xlen_vec (never
// constructed) reaches it from the vectors that are written out here, with `this` in ECX as in the original.
// VEC_XLEN is for a TU where cl knew the call never returns, VEC_XLEN_CALL for one where it is an ordinary call.
#ifndef MB_VEC_XLEN_H
#define MB_VEC_XLEN_H
#include <vector>
template<> void std::vector<int, std::allocator<int> >::_Xlen() const;      // 0x43e2a0
struct xlen_vec : std::vector<int> {
    __forceinline void xlen_call() const { _Xlen(); }
    __declspec(noreturn) __forceinline void xlen() const { _Xlen(); }
};
#define VEC_XLEN_CALL(p) (((const xlen_vec*)(p))->xlen_call())
#define VEC_XLEN(p) (((const xlen_vec*)(p))->xlen())
#endif // MB_VEC_XLEN_H
