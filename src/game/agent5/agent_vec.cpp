//-----------------------------------------------------------------------------
// File: agent_vec.cpp  (0x4fe0b0..0x4fe860)
//
// Desc: the VC7.1 <vector> instantiations that sit between the map-text renderer and the editor dialog:
//       vector<int>::push_back / insert / _Insert_n, the 4-byte-element _Uninit_fill_n / _Uninit_copy,
//       vector<loading-screen config record>::_Insert_n and the row list's _Ucopy tail.
//       Compiled with the real library headers (/EHsc, /Gz for the __thiscall members, see matching.md).
//       CfgRec / RowRec stand for the loading_config (src/game/cls_map/cls_map.h) and UiRow
//       (src/game/char_sheet/screen.h) records: those headers pull in a different rgl header world and a
//       class with real members would inline its own copy constructors, so only the sizes and the
//       out-of-line copy / assign / destroy entry points are modelled here (see MIGRATION.md).
//-----------------------------------------------------------------------------
#include <windows.h>
#include <new>
#include <vector>
#include <algorithm>
#include <stdexcept>

#undef push_back       // windows.h's raster-op constant

// ---- element types --------------------------------------------------------------------------------------
struct loading_config {                         // src/game/cls_map/cls_map.h: the 0x1f8-byte record, whose copy constructor is the one 0x4dc030 defines
    char pad[0x1f8];
    loading_config(const loading_config& o);      // 0x4dc030
};
struct CfgRec : loading_config {                // 0x1f8 bytes (s004dbf7e / s004e40f0)
    CfgRec(const CfgRec& o) : loading_config(o) {}   // 0x4dc030 (the inherited copy)
    CfgRec& operator=(const CfgRec& o); // 0x4e4340
    ~CfgRec();                                  // 0x4d3720
};

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

typedef std::vector<int> int_vec;
typedef std::vector<CfgRec> cfg_vec;
typedef std::vector<RowRec> row_vec;

// ---- the vectors the engine works on (globals here keep the instantiations reachable) -------------------
static int_vec int_v;
const int* int_x;
int* int_where;
unsigned int_n;

static cfg_vec cfg_v;
CfgRec* cfg_where;
unsigned cfg_n;
const CfgRec* cfg_val;

static row_vec row_v;
const RowRec* row_val;

// ---- the int helpers -------------------------------------------------------------------------------------

// @ 0x4fe0b0  vector<int>::push_back
void* __cdecl int_push_back() throw()
{
    int_v.push_back(*int_x);
    return 0;
}

// @ 0x4fe190  vector<int>::insert(iterator, const int&) -- reached through insert(end, count, val) below
void* __cdecl int_insert() throw()
{
    int_v.insert(int_where, *int_x);
    return 0;
}

// @ 0x4fe1e0  vector<int>::_Insert_n (via the count overload)
void* __cdecl int_insert_n() throw()
{
    int_v.insert(int_where, int_n, *int_x);
    return 0;
}

// ---- the 4-byte non-scalar element fill ------------------------------------------------------------------
// The 0x4fe0b0 vector's element is 4 bytes and a class (so _Ptr_cat is non-scalar), but trivially
// copyable: its _Uninit_fill_n/_Uninit_copy inline the copy to a 4-byte store and keep the EH frame
// (a throwing region with no call -- 0x4fe120, 0x4fe4b0). cl 13.10 does not inline the header's
// allocator::construct/_Uninit_fill_n at this instantiation depth, so the template body is written
// out here; it is byte-identical to the library instantiation.
struct ui_cell {                                        // 4-byte provisional element
    int v;
};

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

// ---- the CfgRec helper ---------------------------------------------------------------------------

// @ 0x4fe520  vector<CfgRec>::insert(where, count, val) -> _Insert_n
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
