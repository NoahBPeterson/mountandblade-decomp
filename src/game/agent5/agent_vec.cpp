//-----------------------------------------------------------------------------
// File: agent_vec.cpp  (0x4fe0b0..0x4fe860)
//
// Desc: the VC7.1 <vector> instantiations that sit between the map-text renderer and the editor dialog:
//       vector<NotePage>::push_back / insert / _Insert_n and its _Uninit_fill_n / _Uninit_copy (ui/note_page.h),
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

#define NOTE_PAGE_INSTANCES             // this file defines the vector<NotePage> instances note_page.h declares
#include "../ui/note_page.h"    // NotePage: the element of the 0x4fe0b0 vector, and its out-of-line instances

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
#include "../../include/vec_xlen.h"

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

typedef std::vector<Skin> cfg_vec;
typedef std::vector<RowRec> row_vec;

// ---- the vectors the engine works on (globals here keep the instantiations reachable) -------------------
static cfg_vec cfg_v;
Skin* cfg_where;
unsigned cfg_n;
const Skin* cfg_val;

static row_vec row_v;
const RowRec* row_val;

// ---- std::vector<NotePage> (the link pages of Font::layout_map 0x4fd700, agent_hud.cpp) -----------------------------
typedef std::vector<NotePage> page_vec;

// @ 0x4fe0b0  vector<NotePage>::push_back: the header's; its slow path calls insert below
template void page_vec::push_back(const NotePage&);

// @ 0x4fe190  vector<NotePage>::insert(iterator, const NotePage&): the header's body (declared in note_page.h)
template<> __declspec(noinline) page_vec::iterator page_vec::insert(page_vec::iterator _Where, const NotePage& _Val)
{
    size_type _Off = size() == 0 ? 0 : _Where - begin();
    _Insert_n(_Where, (size_type)1, _Val);
    return begin() + _Off;
}

// @ 0x4fe1e0  vector<NotePage>::_Insert_n: the header's (instantiated by insert above). Not byte-exact: the original
// expands _Ucopy / _Ufill / copy_backward / fill inline (only _Uninit_copy 0x4fe4b0 / _Uninit_fill_n 0x4fe120 stay
// calls), this compiler calls them from inside the try block.
template void page_vec::_Insert_n(page_vec::iterator, page_vec::size_type, const NotePage&);

namespace std {
// @ 0x4fe120  _Uninit_fill_n for NotePage (memory's allocator form, with the element copy inline)
template<> void _Uninit_fill_n(NotePage* _First, unsigned _Count, const NotePage& _Val,
                               allocator<NotePage>& _Al, _Nonscalar_ptr_iterator_tag)
{
    NotePage* _Next = _First;
    _TRY_BEGIN
    for (; 0 < _Count; --_Count, ++_First)
        ::new ((void*)_First) NotePage(_Val);
    _CATCH_ALL
    for (; _Next != _First; ++_Next)
        _Al.destroy(_Next);
    _RERAISE;
    _CATCH_END
}

// @ 0x4fe4b0  _Uninit_copy for NotePage
template<> NotePage* _Uninit_copy(NotePage* _First, NotePage* _Last, NotePage* _Dest,
                                  allocator<NotePage>& _Al, _Nonscalar_ptr_iterator_tag)
{
    NotePage* _Next = _Dest;
    _TRY_BEGIN
    for (; _First != _Last; ++_Dest, ++_First)
        ::new ((void*)_Dest) NotePage(*_First);
    _CATCH_ALL
    for (; _Next != _Dest; ++_Next)
        _Al.destroy(_Next);
    _RERAISE;
    _CATCH_END
    return _Dest;
}
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
