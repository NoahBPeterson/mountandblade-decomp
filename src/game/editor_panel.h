// editor_panel.h: the application object (g_app_obj 0x8ee3c48, GameApp of application.h) as the map editor's dialog procedure
// code sees it. One definition for agent5/editor_dialog.cpp (the terrain dialog body 0x4fe860 and the two mode-check helpers) and
// inventory_panel/widget_text.cpp (the dialog thunk 0x50d250 calls terrain_dialog on 0x8ee3c48). Both views put the module
// HINSTANCE at +0x358 and the editor dialog window at +0x374, i.e. GameApp::m_hInstance / m_hMapEditorDlg.
#ifndef GAME_EDITOR_PANEL_H
#define GAME_EDITOR_PANEL_H
#include "application.h"
struct EditorPanel : GameApp {
    void update_checks();          // 0x4bf640
    void update_object_type();          // 0x4c7bf0
    LRESULT terrain_dialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);   // 0x4fe860
};
#endif
