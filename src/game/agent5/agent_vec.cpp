//-----------------------------------------------------------------------------
// File: agent_vec.cpp  (0x4fe0b0..0x4fe860)
//
// Desc: the VC7.1 <vector> instantiations that sit between the map-text renderer and the editor dialog:
//       vector<NotePage>::push_back / insert / _Insert_n and its _Uninit_fill_n / _Uninit_copy (ui/note_page.h),
//       vector<Skin>::_Insert_n and vector<RagModel>::_Ucopy (src/include/rag_model.h).
//       Compiled with the real library headers (/EHsc, /Gz for the __thiscall members, see matching.md).
//       The elements are the real records: NotePage, Skin (src/include/skin.h), RagModel (src/include/rag_model.h).
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

// RagModel (src/include/rag_model.h) is the 0x20-byte ragdoll model record: a tstring and a vector<E50>. The linker folds
// this file's vector<RagModel> COMDATs with inventory_panel/widget_text.cpp's (its vector<RagModel>::_Insert_n 0x50d260 calls
// _Ucopy 0x4fe840), so the element size decides the stride of both: a wrong one (0x1c8) once made the folded _Uninit_copy
// walk past the rows and crashed the skeleton loader (rag_model.h asserts 0x20).
#include "../../include/rag_model.h"

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

// ---- std::vector<Skin> (Font's skin list: vector<Skin>::resize 0x50d170, inventory_panel/widget_text.cpp) ---------------

// @ 0x4fe520  vector<Skin>::_Insert_n: the header's (its caller is resize 0x50d170)
template void std::vector<Skin>::_Insert_n(std::vector<Skin>::iterator, std::vector<Skin>::size_type, const Skin&);

// ---- std::vector<RagModel> (the skeleton list: vector<RagModel>::_Insert_n 0x50d260, inventory_panel/widget_text.cpp) ------

// @ 0x4fe840  vector<RagModel>::_Ucopy<RagModel*>: the header's member template; it calls _Uninit_copy 0x4ebe10
template RagModel* std::vector<RagModel>::_Ucopy<RagModel*>(RagModel*, RagModel*, RagModel*);
