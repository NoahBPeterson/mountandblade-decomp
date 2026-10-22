// Slice s004b9e70: dialog refresh of the "create game" options page.
//
// One function: 0x4b9e70, a thiscall member of the create-game options page. It shows
// or hides ~70 dialog controls depending on the selected game mode and enables or
// disables the sub-option buttons. The page object keeps its dialog HWND at +0x374
// (same page layout as s004b3ce0); the game configuration object (global at 0x5e2930,
// cf. s004b3ce0) keeps the selected game mode at +0x1a094 and the selected sub-option
// (sub-mode button) at +0x1a1d0.
//
// The original is a straight run of macro expansions:
//   * ShowWindow(GetDlgItem(hDlg, id), test ? SW_SHOW : SW_HIDE) with the condition
//     spelled differently per group, which shows in the codegen:
//       mode 1        cmp eax,1          (single ==, setne/dec/and materialisation)
//       mode 2        cmp edx,ebp, ebp=2 (counted from 2),
//       mode 1..3     cmp eax,1 / cmp eax,2 / cmp eax,ebp, ebp=3
//       mode 3        cmp ecx,ebp, ebp=3
//       mode 0        test ecx,ecx
//       mode 4        cmp ecx,ebp, ebp=4
//     hence the `!= X ? SW_HIDE : SW_SHOW` spelling used below for every group but the
//     first: it yields the `setne` + `dec`/`and 5` sequence of the original.
//   * EnableWindow(...) for the sub-option buttons. The first three disable themselves
//     once selected, the last one (0x4a6) is enabled whenever the host page is not
//     shown, the others enable on the matching sub-option.
//
// windows.h declares GetDlgItem / ShowWindow / EnableWindow as dllimport, so each call
// goes through its IAT slot (0x54932c, 0x54934c, 0x5493a8), as in the original.

#include <windows.h>

// 0x54932c GetDlgItem (imported thunk)
// 0x54934c ShowWindow (imported thunk)
// 0x5493a8 EnableWindow (imported thunk)

// Game modes selected by the "single player / multiplayer" radio buttons of the page.
enum GameMode {
    MODE_CAMPAIGN = 0,          // single-player campaign page
    MODE_QUICK = 1,             // quick battle
    MODE_CUSTOM = 2,            // custom battle
    MODE_MULTIPLAYER = 3,       // multiplayer client (join)
    MODE_MULTIPLAYER_HOST = 4,  // multiplayer host (create server)
};

struct GameConfig {             // object pointed to by the global at 0x5e2930
    char pad[0x1a094];
    int  game_mode;             // 0x1a094  selected game mode (GameMode)
    char pad2[0x1a1d0 - 0x1a094 - 4];
    int  sub_option;            // 0x1a1d0  selected sub-option of the host page
};

extern GameConfig* g_config;    // 0x5e2930

struct CreateGameOptionsPage {
    char pad[0x374];
    HWND hDlg;                  // 0x374  dialog window of the page
    void refresh();             // 0x4b9e70
};

// @ 0x4b9e70
void CreateGameOptionsPage::refresh()
{
    // Quick-battle only.
    ShowWindow(GetDlgItem(hDlg, 0x46e), g_config->game_mode == MODE_QUICK ? SW_SHOW : SW_HIDE);

    // Custom battle only.
    ShowWindow(GetDlgItem(hDlg, 0x474), g_config->game_mode != MODE_CUSTOM ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x475), g_config->game_mode != MODE_CUSTOM ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x476), g_config->game_mode != MODE_CUSTOM ? SW_HIDE : SW_SHOW);

    // Shared by quick battle, custom battle and the multiplayer client.
    ShowWindow(GetDlgItem(hDlg, 0x472), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x47d), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x47f), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x473), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x47e), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x480), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x477), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x481), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x482), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x4af), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);

    // Multiplayer client (join) only.
    ShowWindow(GetDlgItem(hDlg, 0x4cd), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4c8), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4cf), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4c9), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4ce), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4ca), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4cb), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);

    // Single-player campaign only.
    ShowWindow(GetDlgItem(hDlg, 0x439), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x43a), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x43b), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x44e), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x44f), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x435), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x436), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x437), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x478), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x479), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x47a), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x48b), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x48c), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x48d), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x48e), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x48f), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x490), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x491), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x492), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x493), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x438), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x431), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x434), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x433), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x47b), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x47c), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x453), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x454), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);

    // Multiplayer host (create server) only.
    ShowWindow(GetDlgItem(hDlg, 0x494), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4ae), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x497), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x498), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x499), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4a5), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4a3), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4a4), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x49b), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4aa), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4a7), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4a6), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4a8), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x49d), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4ac), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4ad), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);

    // Sub-option buttons of the host page. The first three are the radio buttons: the
    // selected one is disabled (you cannot re-select it), every other one enabled.
    EnableWindow(GetDlgItem(hDlg, 0x4a5), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 0 ? FALSE : TRUE);
    EnableWindow(GetDlgItem(hDlg, 0x4a3), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 1 ? FALSE : TRUE);
    EnableWindow(GetDlgItem(hDlg, 0x4a4), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 2 ? FALSE : TRUE);
    // The rest are only usable with the matching sub-option selected.
    EnableWindow(GetDlgItem(hDlg, 0x49b), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 1 ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, 0x4aa), g_config->game_mode == MODE_MULTIPLAYER_HOST && (g_config->sub_option == 0 || g_config->sub_option == 1) ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, 0x4a7), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 0 ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, 0x4a6), !(g_config->game_mode != MODE_MULTIPLAYER_HOST));
    EnableWindow(GetDlgItem(hDlg, 0x4a8), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 2 ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, 0x49d), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 1 ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, 0x4ac), g_config->game_mode == MODE_MULTIPLAYER_HOST && (g_config->sub_option == 0 || g_config->sub_option == 1) ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, 0x4ad), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 1 ? TRUE : FALSE);
}
