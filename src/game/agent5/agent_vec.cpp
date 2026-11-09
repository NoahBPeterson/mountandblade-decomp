//-----------------------------------------------------------------------------
// File: agent_vec.cpp  (0x4fe0b0..0x4fe860)
//
// Desc: the VC7.1 <vector> instantiations that sit between the map-text renderer and the editor dialog:
//       vector<ui_cell>::push_back / insert / _Insert_n, the 4-byte-element _Uninit_fill_n / _Uninit_copy,
//       vector<loading-screen config record>::_Insert_n and the row list's _Ucopy tail.
//       Compiled with the real library headers (/EHsc, /Gz for the __thiscall members, see matching.md).
//       Skin is the real skin record (src/include/skin.h); RowRec stands for UiRow
//       (src/game/char_sheet/screen.h): that header pulls in a different rgl header world and a
//       class with real members would inline its own copy constructors, so only its size and the
//       out-of-line copy / assign / destroy entry points are modelled here (see MIGRATION.md).
//-----------------------------------------------------------------------------
#include <windows.h>
#include <new>

#undef push_back       // windows.h's raster-op constant

// the 4-byte element of the 0x4fe0b0 vector: a class (so _Ptr_cat is non-scalar and the fills go through
// _Uninit_fill_n 0x4fe120 / _Uninit_copy 0x4fe4b0, which push_back and _Insert_n call), trivially copyable
struct ui_cell {
    int v;
};

// The original keeps the 4-byte element's fill/copy helpers out of line: declare them before <memory> is
// parsed so every use is a call to the out-of-line instance (0x4fe120 / 0x4fe4b0).
#include <xmemory>
namespace std {
void _Uninit_fill_n(ui_cell*, unsigned, const ui_cell&, allocator<ui_cell>&, _Nonscalar_ptr_iterator_tag);   // 0x4fe120 (cell_fill below)
ui_cell* _Uninit_copy(ui_cell*, ui_cell*, ui_cell*, allocator<ui_cell>&, _Nonscalar_ptr_iterator_tag);   // 0x4fe4b0 (cell_copy below)
}

#include <vector>
#include <algorithm>
#include <stdexcept>

// ---- element types --------------------------------------------------------------------------------------
// Skin (src/include/skin.h) is the 0x1f8-byte skin record: its copy constructor is the implicit one (0x4dc030, emitted out of line),
// operator= 0x4da410 (cls_map) and destructor 0x4d3720 (options_panel/world_app.cpp) are out of line too, so this
// instantiation calls them as the original does.
#include "../arr_set.h"
#include "../../include/vec_elems.h"
#include "../../include/skin.h"

// 0x20 bytes, NOT 0x1c8: the UI row record is the engine string plus a 4-dword vector (s004ebc10;
// copy ctor 0x4e42f0 copies the tstring and the vector). The size here decides the stride of every
// vector<RowRec> instantiation this file emits, and the linker folds those COMDATs with
// inventory_panel/widget_text.cpp's RowRec (= named_vec, 0x20). With a wrong 0x1c8 here, the folded
// _Uninit_copy walked 0x1c8 bytes per element inside the 0x20-byte row vector and the skeleton loader
// crashed reading text out of the heap (game/agent5 -> SkeletonBody::load -> Entries::add).
struct RowRec {
    int str[4];                                           // tstring name: pos, buf, cap, len
    void* vec[4];                                         // vector<E50>: alloc, first, last, end
    RowRec(const RowRec& o);                              // 0x4e42f0
};

typedef std::vector<ui_cell> int_vec;           // 0x4fe0b0/0x4fe190/0x4fe1e0 are vector<ui_cell> (not vector<int>:
                                                // the int instance fills inline and calls neither helper)
typedef std::vector<Skin> cfg_vec;
typedef std::vector<RowRec> row_vec;

// insert is out of line in the original: declare the specialization before the vector is instantiated so
// push_back's slow path is a call, then define it below.
template<> __declspec(noinline) std::vector<ui_cell>::iterator std::vector<ui_cell>::insert(std::vector<ui_cell>::iterator, const ui_cell&);

// _Insert_n is out of line too: declare the specialization before the insert body above is instantiated so
// the inline insert/fill helpers expand into it rather than the header template being instantiated here.
template<> void std::vector<ui_cell>::_Insert_n(std::vector<ui_cell>::iterator, std::vector<ui_cell>::size_type, const ui_cell&);

// The original's _Insert_n expands _Ucopy (-> 0x4fe4b0) and _Ufill (-> 0x4fe120) inline while the
// _Uninit_copy / _Uninit_fill_n calls they wrap stay calls; cl 13.10 will not inline the two-argument
// header helpers at this instantiation depth and folds the forwarding away instead.  Spelling out the
// two bodies (each is the header's own one-liner) reproduces the call sequence.
class cell_vec : public std::vector<ui_cell> {
public:
    __forceinline pointer ucopy(pointer _First, pointer _Last, pointer _Ptr)
    {
        return _Uninit_copy(_First, _Last, _Ptr, this->_Alval,
            std::_Ptr_cat(_First, _Ptr));
    }
    __forceinline pointer ufill(pointer _Ptr, size_type _Count, const ui_cell& _Val)
    {
        _Uninit_fill_n(_Ptr, _Count, _Val, this->_Alval,
            std::_Ptr_cat(_Ptr, _Ptr));
        return _Ptr + _Count;
    }
};

// ---- the vectors the engine works on (globals here keep the instantiations reachable) -------------------
static int_vec int_v;
const ui_cell* int_x;
ui_cell* int_where;
unsigned int_n;

static cfg_vec cfg_v;
Skin* cfg_where;
unsigned cfg_n;
const Skin* cfg_val;

static row_vec row_v;
const RowRec* row_val;

// ---- the int helpers -------------------------------------------------------------------------------------

// vector<ui_cell>::push_back wrapper (inlined caller of the member below)
void* __cdecl int_push_back() throw()
{
    int_v.push_back(*int_x);
    return 0;
}

// @ 0x4fe0b0  vector<ui_cell>::push_back (out of line)
template void std::vector<ui_cell>::push_back(const ui_cell&);

// @ 0x4fe190  vector<ui_cell>::insert(iterator, const ui_cell&)
template<> __declspec(noinline) std::vector<ui_cell>::iterator std::vector<ui_cell>::insert(
    std::vector<ui_cell>::iterator _Where, const ui_cell& _Val)
{
    size_type _Off = size() == 0 ? 0 : _Where - begin();
    _Insert_n(_Where, (size_type)1, _Val);
    return begin() + _Off;
}

// reached through insert(end, count, val) below
void* __cdecl int_insert() throw()
{
    int_v.insert(int_where, *int_x);
    return 0;
}

// reached through insert(where, count, val)
void* __cdecl int_insert_n() throw()
{
    int_v.insert(int_where, int_n, *int_x);
    return 0;
}

// @ 0x4fe1e0  vector<ui_cell>::_Insert_n (via the count overload)
template<> void std::vector<ui_cell>::_Insert_n(
    std::vector<ui_cell>::iterator _Where, std::vector<ui_cell>::size_type _Count,
    const ui_cell& _Val)
{
    ui_cell _Tmp = _Val;
    size_type _Capacity = capacity();

    if (_Count == 0)
        ;
    else if (max_size() - size() < _Count)
        _Xlen();
    else if (_Capacity < size() + _Count)
        {
        _Capacity = max_size() - _Capacity / 2 < _Capacity
            ? 0 : _Capacity + _Capacity / 2;
        if (_Capacity < size() + _Count)
            _Capacity = size() + _Count;
        ui_cell* _Newvec = this->_Alval.allocate(_Capacity);
        ui_cell* _Ptr = _Newvec;

        _TRY_BEGIN
        _Ptr = ((cell_vec*)this)->ucopy(_Myfirst,
            _ITER_BASE(_Where), _Newvec);   // copy prefix
        _Ptr = ((cell_vec*)this)->ufill(_Ptr, _Count, _Tmp);    // add new stuff
        ((cell_vec*)this)->ucopy(_ITER_BASE(_Where), _Mylast, _Ptr);    // copy suffix
        _CATCH_ALL
        _Destroy(_Newvec, _Ptr);
        this->_Alval.deallocate(_Newvec, _Capacity);
        _RERAISE;
        _CATCH_END

        _Count += size();
        if (_Myfirst != 0)
            {
            _Destroy(_Myfirst, _Mylast);
            this->_Alval.deallocate(_Myfirst, _Myend - _Myfirst);
            }
        _Myend = _Newvec + _Capacity;
        _Mylast = _Newvec + _Count;
        _Myfirst = _Newvec;
        }
    else if ((size_type)(_Mylast - _ITER_BASE(_Where)) < _Count)
        {
        ((cell_vec*)this)->ucopy(_ITER_BASE(_Where), _Mylast,
            _ITER_BASE(_Where) + _Count);   // copy suffix

        _TRY_BEGIN
        ((cell_vec*)this)->ufill(_Mylast, _Count - (_Mylast - _ITER_BASE(_Where)),
            _Tmp);  // insert new stuff off end
        _CATCH_ALL
        _Destroy(_ITER_BASE(_Where) + _Count, _Mylast + _Count);
        _RERAISE;
        _CATCH_END

        _Mylast += _Count;
        fill(_ITER_BASE(_Where), _Mylast - _Count, _Tmp);
        }
    else
        {
        ui_cell* _Oldend = _Mylast;
        _Mylast = ((cell_vec*)this)->ucopy(_Oldend - _Count, _Oldend,
            _Mylast);   // copy suffix
        copy_backward(_ITER_BASE(_Where), _Oldend - _Count, _Oldend);
        fill(_ITER_BASE(_Where), _ITER_BASE(_Where) + _Count, _Tmp);
        }
}

// ---- the 4-byte non-scalar element fill ------------------------------------------------------------------
// The 0x4fe0b0 vector's element is 4 bytes and a class (so _Ptr_cat is non-scalar), but trivially
// copyable: its _Uninit_fill_n/_Uninit_copy inline the copy to a 4-byte store and keep the EH frame
// (a throwing region with no call -- 0x4fe120, 0x4fe4b0). cl 13.10 does not inline the header's
// allocator::construct/_Uninit_fill_n at this instantiation depth, so the template body is written
// out here; it is byte-identical to the library instantiation.
// ui_cell (defined above with the element types) is that element

// @ 0x4fe120  _Uninit_fill_n for the 4-byte non-scalar element
void __cdecl cell_fill(ui_cell* _First, unsigned _Count, const ui_cell& _Val,
                       std::allocator<ui_cell>& _Al, std::_Nonscalar_ptr_iterator_tag)
{
    ui_cell* _Next = _First;
    try {
        for (; 0 < _Count; --_Count, ++_First)
            ::new ((void*)_First) ui_cell(_Val);
    } catch (...) {
        for (; _Next != _First; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
}

// @ 0x4fe4b0  _Uninit_copy for the 4-byte non-scalar element
ui_cell* __cdecl cell_copy(ui_cell* _First, ui_cell* _Last, ui_cell* _Dest,
                           std::allocator<ui_cell>& _Al, std::_Nonscalar_ptr_iterator_tag)
{
    ui_cell* _Next = _Dest;
    try {
        for (; _First != _Last; ++_Dest, ++_First)
            ::new ((void*)_Dest) ui_cell(*_First);
    } catch (...) {
        for (; _Next != _Dest; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
    return _Dest;
}

// ---- the Skin helper ---------------------------------------------------------------------------

// @ 0x4fe520  vector<Skin>::insert(where, count, val) -> _Insert_n
void* __cdecl cfg_insert() throw()
{
    cfg_v.insert(cfg_where, cfg_n, *cfg_val);
    return 0;
}

// ---- the UI row append -----------------------------------------------------------------------------------

// @ 0x4fe840  push_back on the row list: its tail copy is 0x4ebe10
void* __cdecl row_append() throw()
{
    row_v.push_back(*row_val);
    return 0;
}

// ---- keep the instantiations instantiated ---------------------------------------------------------------
void* __cdecl touch_all()
{
    int_insert();
    return 0;
}
