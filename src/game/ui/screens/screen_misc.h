#ifndef SCREEN_H
#define SCREEN_H
// Slice s004ebc10: the frame driver of the 0x4e... screen module (mission/map screen).
//   0x4ebc10 take_screenshot   builds the next free "mb<N>.bmp" path, saves the frame (0x4bb9c0), logs the
//                              translated result message.
//   0x4ebdc0 allocator<RagModel>::destroy   destroys one 0x20-byte RagModel (src/engine/physics/rag_model.h).
//   0x4ebe10 _Uninit_copy<RagModel*>        copy-constructs [first,last) into dest (0x4e42f0), returning the new end.
//   0x4ebe7a (funclet)                      the EH cleanup of _Uninit_copy: destroys [saved, current) then rethrows.
//   0x4ebea0 GuiScreen::GuiScreen    ctor of the screen object (1.1 MB: 6 "hint" vector tables, 3 std::vectors,
//                              vec_table 0x417cb0, rgl::mesh, scene, a 0x800-byte matrix array).
//   0x4ebfd0 GuiScreen::process   per-frame mouse/camera update and key-message dispatch.
//   0x4ec460 frame_hook        the engine's end-of-frame callback: screenshot / leave-mission triggers.
// Names and field names are provisional; offsets come from this function and the neighbouring slices.
#include <windows.h>
#include <stdio.h>
#include <new>
#include <d3d9.h>
#include <d3dx9.h>
#include "engine/math/rgl_math.h"
#include "game/ui/screens/character_sheet/cs_common.h"
#include "game/ui/screens/character_sheet/cs_mat.h"

using rgl::vector4;

#include "engine/math/mat4_include.h"
using rgl::mat4;
using rgl::mat3;
// (identity3 0x40cd60: cs_mat.h)

#include "engine/render/mesh.h"
using rgl::mesh;

#include "engine/core/tstring.h"

#include "game/data/str_table.h"
#include "game/input/key_names.h"
#include "game/data/str_table.h"                          // StrTable g_strtable (0x5e2a18)
#include "game/ui/gui_manager.h"

bool __cdecl save_screenshot(const char* file_name, D3DXIMAGE_FILEFORMAT format);  // 0x4bb9c0 (2-arg cdecl)
void take_screenshot(void);
#include "engine/io/text_stream.h"                  // mb_snprintf 0x415100
#include "engine/core/crt.h"
#include "engine/math/float_consts.h"
// (0x4e42f0 allocator<RagModel>::construct: cs_common.h)
// the MSVC 7.1 formulation of _AddressOfReturnAddress
#define _AddressOfReturnAddress() ((void*)((char*)&_AddressOfReturnAddress_ + 4))
int _AddressOfReturnAddress_;

#include "engine/core/vec_table.h"     // vec_table (8 bytes: data, count; ctor 0x417cb0). It was a 0x260-byte stand-in here.
#include "engine/core/clock_table.h"
#include "game/ui/ui_globals.h"      // UI / core globals

#include "engine/core/timer.h"   // Timer (elapsed 0x432c10: seconds since the record was touched)


// 0x4babf0 is a file-local helper: cl's register convention for `static` functions hands it its out
// pointer in ESI. Defining it here (body shape taken from 0x4babf0) makes the caller emit
// `lea esi,[esp+0x18]; call ...` instead of a normal stack argument.
extern "C" __declspec(dllimport) long __stdcall SHGetFolderPathA(void*, int, void*, unsigned long, char*);
__declspec(noinline) static void screenshots_path(char* out)
{
    char home[0x104];
    char path[0x104];
    SHGetFolderPathA(0, 5, 0, 0, home);
    sprintf(path, "%s\\Mount&Blade Screenshots", home);
    CreateDirectoryA(path, 0);
    sprintf(path, "%s", out);
    sprintf(out, "%s\\Mount&Blade Screenshots\\%s", home, path);
}

using rgl::scene;

struct GameMgr;

// ---- globals --------------------------------------------------------------------------------------------







// the mouse device g_keyboard.state (0xa170d0) is drained through IDirectInputDevice8A::GetDeviceData (DIDEVICEOBJECTDATA records)

#include "engine/core/clock_table.h"   // g_ticks (0x5e2948), read as float pairs by the cast
extern float g_four_tenths;             // 0x54b3b8  0.4f

extern float zero;                      // 0x549ca8  0.0f
extern float g_eps;                     // 0x54a6f0  0.01f
extern float g_ninety_nine;             // 0x54a5cc  0.99f
extern float g_seventy_four;            // 0x5985f4  0.74f

// (leave_module 0x4ebb80 is menu_variables_loaded: cs_common.h)

#endif
