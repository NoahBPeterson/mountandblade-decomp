// Slice s004fe0b0: MSVC 7.1 <vector> insertion helpers and the std::copy append tail of the UI row list.
// Library instantiations: the file drives the real VC7.1 headers (/EHsc, the original has throwing
// funclets; /Gz for the __thiscall members) and pins the element types and entry points.
//   0x4fe0b0  vector<int>::push_back(const int&)          -- spare capacity -> 0x4fe120 _Uninit_fill_n
//   0x4fe190  vector<int>::insert(iterator, const int&)   -- byte-exact
//   0x4fe1e0  vector<int>::_Insert_n                      -- its _Ucopy/_Ufill funclets follow it
//   0x4fe120  _Uninit_fill_n for int      0x4fe184 raise funclet      0x4fe393 catch (...) + rethrow
//   0x4fe4b0  _Uninit_copy funclet
//   0x4fe516  raise funclet of the loading_config _Insert_n; 0x4fe6e9 catch, 0x4fe7b1 funclet
//   0x4fe520  vector<loading_config>::insert(where, count, val) -> _Insert_n of the 0x1f8-byte record
//             (copy ctor 0x4dc030, in-place copy 0x4e4340, fill 0x4f6360, destroy 0x4d3720,
//             move_backward 0x4da8c0, destroy_range 0x4dc190)
//   0x4fe840  append one 0x1c8-byte UI row (s004ebc10) through 0x4ebe10, the row list's std::copy tail
#include <windows.h>
#include <new>
#include <vector>
#include <algorithm>
#include <stdexcept>

#undef push_back       // windows.h's raster-op constant

// ---- element types --------------------------------------------------------------------------------------
struct loading_config {                                 // 0x1f8 bytes (s004dbf7e / s004e40f0)
    char pad[0x1f8];
    loading_config(const loading_config& o);            // 0x4dc030
    loading_config& operator=(const loading_config& o); // 0x4e4340
    ~loading_config();                                  // 0x4d3720
};

struct UiRow {                                          // 0x1c8 bytes (s004ebc10)
    char pad[0x1c8];
    UiRow(const UiRow& o);                              // 0x4e42f0
};

typedef std::vector<int> int_vec;
typedef std::vector<loading_config> cfg_vec;
typedef std::vector<UiRow> row_vec;

// ---- the vectors the engine works on (globals here keep the instantiations reachable) -------------------
static int_vec int_v;
const int* int_x;
int* int_where;
unsigned int_n;

static cfg_vec cfg_v;
loading_config* cfg_where;
unsigned cfg_n;
const loading_config* cfg_val;

static row_vec row_v;
const UiRow* row_val;

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

// ---- the loading_config helper ---------------------------------------------------------------------------

// @ 0x4fe520  vector<loading_config>::insert(where, count, val) -> _Insert_n
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
