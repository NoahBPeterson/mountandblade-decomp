// mbinput.cpp: drive the game's menus from inside the wine bottle (macOS cannot post into wine's input queue).
// Built with tools/matching/cl.sh, run with the game's own bottle:
//   mbinput.exe key 36          -- one keystroke (VK code)
//   mbinput.exe slowclick X Y  -- absolute point, press held across several frames
//   mbinput.exe click X Y       -- absolute screen point, left click
//   mbinput.exe move DX DY      -- relative mouse motion (the menus read the pointer's movement)
//   mbinput.exe focus           -- bring the game window to the front
#include <windows.h>
#include <stdlib.h>
#include <string.h>

static HWND game_window() {
    HWND h = FindWindowA(NULL, "Mount&Blade");
    if (!h) h = FindWindowA(NULL, "Mount & Blade");
    return h;
}

static void do_key(int vk) {
    keybd_event((BYTE)vk, 0, 0, 0);
    Sleep(60);
    keybd_event((BYTE)vk, 0, KEYEVENTF_KEYUP, 0);
}

static void do_click(int x, int y) {
    SetCursorPos(x, y);
    Sleep(120);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    Sleep(110);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

static void do_slowclick(int x, int y) {
    SetCursorPos(x, y);
    Sleep(300);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    Sleep(400);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    Sleep(200);
}

static void do_move(int dx, int dy) {
    INPUT in[1];
    ZeroMemory(in, sizeof(in));
    in[0].type = INPUT_MOUSE;
    in[0].mi.dx = dx;
    in[0].mi.dy = dy;
    in[0].mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, in, sizeof(INPUT));
}

int main(int argc, char** argv) {
    HWND h = game_window();
    if (!argc || !argv[1]) return 2;
    if (h && (!strcmp(argv[1], "click") || !strcmp(argv[1], "slowclick") || !strcmp(argv[1], "key"))) {
        SetForegroundWindow(h);
        Sleep(150);
    }
    if (!strcmp(argv[1], "key") && argc > 2) {
        do_key(atoi(argv[2]));
    } else if (!strcmp(argv[1], "click") && argc > 3) {
        do_click(atoi(argv[2]), atoi(argv[3]));
    } else if (!strcmp(argv[1], "slowclick") && argc > 3) {
        do_slowclick(atoi(argv[2]), atoi(argv[3]));
    } else if (!strcmp(argv[1], "move") && argc > 3) {
        do_move(atoi(argv[2]), atoi(argv[3]));
    } else if (!strcmp(argv[1], "focus")) {
        // nothing else
    } else {
        return 2;
    }
    return 0;
}
