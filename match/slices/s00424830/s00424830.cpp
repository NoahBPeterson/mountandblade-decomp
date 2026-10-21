// Slice s00424830: VC7.1 <vector>/<memory> instantiations for two POD element types: the 0x24-byte E36
// (vector<E36>::_Insert_n, _Uninit_copy, _Uninit_fill_n) and the 0x88-byte E136 (resize, erase, _Insert_n,
// _Uninit_copy). Built with /EHa. The 9-byte stubs after _Uninit_* and the cold tails after _Insert_n are
// catch(...) funclets that cl emits in the parent's COMDAT, with no symbol of their own.
//
// These bodies do not come out of the stock <vector> with cl 13.10.3052 (it leaves allocator::construct and
// the _Ucopy/_Ufill wrappers out of line, and stops inlining std::copy for elements of >= 128 bytes), so the
// vector members are written out as a small class with the same members as the VC7.1 <vector> (the same trick as
// vec32 in s0040e060). _Xlen must stay an undefined external (0x43e2a0): with a body cl sees it never returns.
#include <vector>
#include <new>
#include <memory>

struct E36  { int v[9]; };      // 0x24
struct E136 { int v[34]; };     // 0x88

namespace std {
// @ 0x424bc0
template<> E36* _Uninit_copy<E36*, E36*, allocator<E36> >(E36* _First, E36* _Last, E36* _Dest, allocator<E36>& _Al, _Nonscalar_ptr_iterator_tag)
{
    E36* _Next = _Dest;
    try {
        for (; _First != _Last; ++_Dest, ++_First)
            new ((void*)_Dest) E36(*_First);
    } catch (...) {
        for (; _Next != _Dest; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
    return _Dest;
}
// @ 0x424c40
template<> void _Uninit_fill_n<E36*, unsigned int, E36, allocator<E36> >(E36* _First, unsigned int _Count, const E36& _Val, allocator<E36>& _Al, _Nonscalar_ptr_iterator_tag)
{
    E36* _Next = _First;
    try {
        for (; 0 < _Count; --_Count, ++_First)
            new ((void*)_First) E36(_Val);
    } catch (...) {
        for (; _Next != _First; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
}
// @ 0x425140
template<> E136* _Uninit_copy<E136*, E136*, allocator<E136> >(E136* _First, E136* _Last, E136* _Dest, allocator<E136>& _Al, _Nonscalar_ptr_iterator_tag)
{
    E136* _Next = _Dest;
    try {
        for (; _First != _Last; ++_Dest, ++_First)
            new ((void*)_Dest) E136(*_First);
    } catch (...) {
        for (; _Next != _Dest; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
    return _Dest;
}
// 0x4251c0 (next slice): _Uninit_fill_n<E136*>
template<> void _Uninit_fill_n<E136*, unsigned int, E136, allocator<E136> >(E136* _First, unsigned int _Count, const E136& _Val, allocator<E136>& _Al, _Nonscalar_ptr_iterator_tag);
}

#define VEC_COMMON(T) \
    std::allocator<T> _Alval; \
    T *_Myfirst, *_Mylast, *_Myend; \
    typedef unsigned size_type; \
    typedef T* iterator; \
    size_type capacity() const { return _Myfirst == 0 ? 0 : _Myend - _Myfirst; } \
    size_type size() const { return _Myfirst == 0 ? 0 : _Mylast - _Myfirst; } \
    size_type max_size() const { return _Alval.max_size(); } \
    __forceinline T* _Ucopy(T* f, T* l, T* p) { return std::_Uninitialized_copy(f, l, p, _Alval); } \
    __forceinline T* _Ufill(T* p, size_type n, const T& v) { std::_Uninitialized_fill_n(p, n, v, _Alval); return p + n; } \
    void _Destroy(T* f, T* l) { for (; f != l; ++f) std::_Destroy(f); } \
    void _Xlen() const;

#define SZ() (_Myfirst == 0 ? 0 : _Mylast - _Myfirst)

// _Insert_n body shared by both element types (the VC7.1 <vector> source, with size() written out).
#define INSERT_N_BODY(T) \
    T _Tmp = _Val; \
    size_type _Capacity = capacity(); \
    if (_Count == 0) \
        ; \
    else if (max_size() - SZ() < _Count) \
        _Xlen(); \
    else if (_Capacity < SZ() + _Count) { \
        _Capacity = max_size() - _Capacity / 2 < _Capacity ? 0 : _Capacity + _Capacity / 2; \
        if (_Capacity < SZ() + _Count) \
            _Capacity = SZ() + _Count; \
        T* _Newvec = _Alval.allocate(_Capacity); \
        T* _Ptr = _Newvec; \
        try { \
            _Ptr = _Ucopy(_Myfirst, _Where, _Newvec); \
            _Ptr = _Ufill(_Ptr, _Count, _Tmp); \
            _Ucopy(_Where, _Mylast, _Ptr); \
        } catch (...) { \
            _Destroy(_Newvec, _Ptr); \
            _Alval.deallocate(_Newvec, _Capacity); \
            throw; \
        } \
        _Count += SZ(); \
        if (_Myfirst != 0) { \
            _Destroy(_Myfirst, _Mylast); \
            _Alval.deallocate(_Myfirst, _Myend - _Myfirst); \
        } \
        _Myend = _Newvec + _Capacity; \
        _Mylast = _Newvec + _Count; \
        _Myfirst = _Newvec; \
    } else if ((size_type)(_Mylast - _Where) < _Count) { \
        _Ucopy(_Where, _Mylast, _Where + _Count); \
        try { \
            _Ufill(_Mylast, _Count - (_Mylast - _Where), _Tmp); \
        } catch (...) { \
            _Destroy(_Where + _Count, _Mylast + _Count); \
            throw; \
        } \
        _Mylast += _Count; \
        { \
            T* _Fe = _Mylast - _Count; \
            for (T* _Q = _Where; _Q != _Fe; ++_Q) \
                *_Q = _Tmp; \
        } \
    } else { \
        T* _Oldend = _Mylast; \
        _Mylast = _Ucopy(_Oldend - _Count, _Oldend, _Mylast); \
        { \
            T* _L = _Oldend - _Count; \
            T* _D = _Oldend; \
            while (_Where != _L) \
                *--_D = *--_L; \
        } \
        { \
            T* _Q = _Where; \
            T* _Fe = _Where + _Count; \
            for (; _Q != _Fe; ++_Q) \
                *_Q = _Tmp; \
        } \
    }

struct vec36 {
    VEC_COMMON(E36)
    void _Insert_n(iterator _Where, size_type _Count, const E36& _Val);   // 0x424830
};
// @ 0x424830
void vec36::_Insert_n(iterator _Where, size_type _Count, const E36& _Val)
{
    INSERT_N_BODY(E36)
}

__forceinline E136* copy_nonscalar(E136* f, E136* l, E136* d) { for (; f != l; ++d, ++f) *d = *f; return d; }

struct vec136 {
    VEC_COMMON(E136)
    struct iterator_ { E136* _Ptr; static iterator_ mk(E136* p) { iterator_ r; r._Ptr = p; return r; } };
    void insert(iterator w, size_type c, const E136& v) { _Insert_n(w, c, v); }
    void _Insert_n(iterator _Where, size_type _Count, const E136& _Val);   // 0x424db0
    void resize(size_type _Newsize, E136 _Val);                            // 0x424cc0
    iterator_ erase(iterator_ _First, iterator_ _Last);                    // 0x424d60
};
// @ 0x424db0
void vec136::_Insert_n(iterator _Where, size_type _Count, const E136& _Val)
{
    INSERT_N_BODY(E136)
}
// @ 0x424cc0
void vec136::resize(size_type _Newsize, E136 _Val)
{
    if (size() < _Newsize)
        insert(_Mylast, _Newsize - size(), _Val);
    else if (_Newsize < size())
        erase(iterator_::mk(_Myfirst + _Newsize), iterator_::mk(_Mylast));
}
// @ 0x424d60
vec136::iterator_ vec136::erase(iterator_ _First, iterator_ _Last)
{
    if (_First._Ptr != _Last._Ptr) {
        E136* _Ptr = copy_nonscalar(_Last._Ptr, _Mylast, _First._Ptr);
        _Destroy(_Ptr, _Mylast);
        _Mylast = _Ptr;
    }
    return _First;
}
