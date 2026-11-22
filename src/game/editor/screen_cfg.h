// screen_cfg.h: the screen/device settings objects of 0x4ca550..0x4ca6c0: the window-mode toggle widget, the
// device-state request queue of the GUI (screen_cfg) and the device list. Shared by editor_selection_and_translation.cpp (0x4ca550 only) and
// poly_mesh_edge_face_ops.cpp. Slot numbers are from the call sites; only the slots used here are modelled.
// Merged from slice s004ca550.
#ifndef MESH2_SCREEN_CFG_H
#define MESH2_SCREEN_CFG_H
#include <deque>
#include <windows.h>
#include <d3d9.h>
#include "engine/core/tstring.h"
#include "game/ui/gui_manager.h"
#include "game/ui/window_mgr.h"

#include "engine/render/renderer_include.h"   // canonical: g_render.dev (0xa17f28)
#include "game/input/key_names.h"      // g_keyboard (0xa170c8)

// the window manager (game/ui/window_mgr.h, g_windows 0xa73980) applies the request queue (WindowMgrW::apply 0x4ca600, poly_mesh_edge_face_ops.cpp);

#endif
