// A WndProc application on the platform API, the way src/dxframework/d3dapp.cpp uses it: register a class, create the
// window, run the PeekMessage / TranslateMessage / DispatchMessage loop, and receive WM_* messages produced from SDL
// events. Real SDL events are injected with SDL_PushEvent (the keyboard / mouse hardware stand-in); the test runs with
// SDL's dummy video driver unless MBP_TEST_REAL=1 (then a real window opens).
#include <SDL3/SDL.h>
#include <windows.h>
#include <dinput.h>
#include <mmsystem.h>
#undef max
#undef min
#include "../mb_sdl.h"
#include "check.h"
#include <string>
#include <vector>
#include <stdlib.h>

struct Seen { UINT msg; WPARAM wp; LPARAM lp; };
static std::vector<Seen> g_seen;
static int g_created = 0;

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
    g_seen.push_back({m, wp, lp});
    switch (m) {
    case WM_CREATE: ++g_created; return 0;
    case WM_DESTROY: PostQuitMessage(7); return 0;
    case WM_SETCURSOR: return TRUE;
    }
    return DefWindowProcA(h, m, wp, lp);
}
static const Seen* find(UINT m, int nth = 0)
{
    for (auto& s : g_seen) if (s.msg == m && nth-- == 0) return &s;
    return nullptr;
}
static void pump_all(int rounds = 5)
{
    MSG msg;
    for (int i = 0; i < rounds; ++i) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}

int main()
{
    if (!getenv("MBP_TEST_REAL")) setenv("SDL_VIDEODRIVER", "dummy", 1);
    setenv("MBP_NO_DIALOGS", "1", 1);

    // ---- class + window (the call sequence of CD3DApplication::Create) ----
    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof wc);
    wc.cbSize = sizeof wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hCursor = LoadCursorA(nullptr, (LPCSTR)IDC_ARROW);
    wc.hIcon = LoadIconA(nullptr, MAKEINTRESOURCEA(101));
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"MB Window";
    CHECK(RegisterClassExW(&wc) != 0);
    CHECK(RegisterClassExW(&wc) == 0);                         // duplicate class
    CHECK(GetSystemMetrics(SM_CXSCREEN) > 0 && GetSystemMetrics(SM_CYSCREEN) > 0);
    RECT rc;
    SetRect(&rc, 0, 0, 640, 480);
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE);
    const char* narrow_title = "Mount&Blade";                  // d3dapp passes the 8-bit title through the wide call
    HWND hwnd = CreateWindowExW(0, L"MB Window", (LPCWSTR)narrow_title, WS_OVERLAPPEDWINDOW, 100, 100, rc.right - rc.left,
                                rc.bottom - rc.top, nullptr, LoadMenuA(nullptr, MAKEINTRESOURCEA(1)), nullptr, nullptr);
    CHECK(hwnd != nullptr);
    CHECK(g_created == 1);
    SDL_Window* sw = mbp_sdl_window(hwnd);
    CHECK(sw != nullptr);
    CHECK(mbp_hwnd_from_sdl(sw) == hwnd);
    CHECK(strcmp(SDL_GetWindowTitle(sw), "Mount&Blade") == 0);
    CHECK(IsWindow(hwnd));
    RECT cr;
    CHECK(GetClientRect(hwnd, &cr) && cr.right == 640 && cr.bottom == 480);
    CHECK(GetWindowLongA(hwnd, GWL_STYLE) & WS_OVERLAPPEDWINDOW);
    CHECK(CreateWindowExA(0, "No Such Class", "x", 0, 0, 0, 10, 10, nullptr, nullptr, nullptr, nullptr) == nullptr);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
    pump_all();
    CHECK(find(WM_SIZE) != nullptr);
    CHECK(find(WM_SIZE) && LOWORD(find(WM_SIZE)->lp) == 640 && HIWORD(find(WM_SIZE)->lp) == 480);
    CHECK(find(WM_PAINT) != nullptr);

    // ---- keyboard: WM_KEYDOWN / WM_CHAR / WM_KEYUP ----
    SDL_WindowID wid = SDL_GetWindowID(sw);
    g_seen.clear();
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_KEY_DOWN; e.key.windowID = wid; e.key.scancode = SDL_SCANCODE_A; e.key.down = true;
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_TEXT_INPUT; e.text.windowID = wid; e.text.text = "a";
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_KEY_DOWN; e.key.windowID = wid; e.key.scancode = SDL_SCANCODE_RETURN; e.key.down = true;
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_KEY_UP; e.key.windowID = wid; e.key.scancode = SDL_SCANCODE_A; e.key.down = false;
    SDL_PushEvent(&e);
    pump_all();
    CHECK(find(WM_KEYDOWN) && find(WM_KEYDOWN)->wp == 'A');
    CHECK(find(WM_KEYDOWN) && ((find(WM_KEYDOWN)->lp >> 16) & 0xFF) == 0x1E);      // scan code = DIK_A
    CHECK(find(WM_CHAR) && find(WM_CHAR)->wp == 'a');
    CHECK(find(WM_KEYDOWN, 1) && find(WM_KEYDOWN, 1)->wp == 0x0D);
    CHECK(find(WM_CHAR, 1) && find(WM_CHAR, 1)->wp == '\r');                     // TranslateMessage: Enter -> WM_CHAR 13
    CHECK(find(WM_KEYUP) && find(WM_KEYUP)->wp == 'A');
    CHECK((GetAsyncKeyState('A') & 0x8000) == 0);                                // A went up; Return is still down
    CHECK((GetAsyncKeyState(0x0D) & 0x8000) != 0);

    // ---- mouse: WM_MOUSEMOVE / buttons / wheel ----
    g_seen.clear();
    SDL_zero(e);
    e.type = SDL_EVENT_MOUSE_MOTION; e.motion.windowID = wid; e.motion.x = 123; e.motion.y = 45; e.motion.xrel = 3; e.motion.yrel = -2;
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN; e.button.windowID = wid; e.button.button = SDL_BUTTON_LEFT; e.button.down = true; e.button.x = 123; e.button.y = 45; e.button.clicks = 1;
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_MOUSE_BUTTON_UP; e.button.windowID = wid; e.button.button = SDL_BUTTON_LEFT; e.button.down = false; e.button.x = 123; e.button.y = 45;
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN; e.button.windowID = wid; e.button.button = SDL_BUTTON_RIGHT; e.button.down = true; e.button.x = 5; e.button.y = 6;
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_MOUSE_WHEEL; e.wheel.windowID = wid; e.wheel.y = 1; e.wheel.x = 0;
    SDL_PushEvent(&e);
    pump_all();
    CHECK(find(WM_MOUSEMOVE) && GET_X_LPARAM(find(WM_MOUSEMOVE)->lp) == 123 && GET_Y_LPARAM(find(WM_MOUSEMOVE)->lp) == 45);
    CHECK(find(WM_SETCURSOR) != nullptr);
    CHECK(find(WM_LBUTTONDOWN) && (find(WM_LBUTTONDOWN)->wp & 1));                // MK_LBUTTON
    CHECK(find(WM_LBUTTONUP) != nullptr);
    CHECK(find(WM_RBUTTONDOWN) && (find(WM_RBUTTONDOWN)->wp & 2));
    CHECK(find(WM_MOUSEWHEEL) && (short)HIWORD(find(WM_MOUSEWHEEL)->wp) == 120);

    // ---- DirectInput sees the same events ----
    IDirectInput8A* di = nullptr;
    CHECK(DirectInput8Create(GetModuleHandleA(nullptr), 0x0800, IID_IDirectInput8A, (void**)&di, nullptr) == DI_OK && di);
    IDirectInputDevice8A *kb = nullptr, *mouse = nullptr;
    CHECK(di->CreateDevice(GUID_SysKeyboard, &kb, nullptr) == DI_OK);
    CHECK(kb->SetDataFormat(&c_dfDIKeyboard) == DI_OK);
    CHECK(kb->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY) == DI_OK);
    CHECK(kb->Acquire() == DI_OK);
    CHECK(di->CreateDevice(GUID_SysMouse, &mouse, nullptr) == DI_OK);
    CHECK(mouse->SetDataFormat(&c_dfDIMouse2) == DI_OK);
    CHECK(mouse->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE) == DI_OK);
    DIPROPDWORD pd;
    pd.diph.dwSize = sizeof pd; pd.diph.dwHeaderSize = sizeof(DIPROPHEADER); pd.diph.dwObj = 0; pd.diph.dwHow = 0; pd.dwData = 16;
    CHECK(mouse->SetProperty(DIPROP_BUFFERSIZE, &pd.diph) == DI_OK);
    CHECK(mouse->Acquire() == DI_OK);

    uint8_t keys[256];
    CHECK(kb->GetDeviceState(256, keys) == DI_OK);
    CHECK(keys[DIK_RETURN] == 0x80 && keys[DIK_A] == 0);                         // Return still held, A released
    SDL_zero(e);
    e.type = SDL_EVENT_KEY_DOWN; e.key.windowID = wid; e.key.scancode = SDL_SCANCODE_LCTRL; e.key.down = true;
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_KEY_DOWN; e.key.windowID = wid; e.key.scancode = SDL_SCANCODE_RCTRL; e.key.down = true;
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_KEY_DOWN; e.key.windowID = wid; e.key.scancode = SDL_SCANCODE_F9; e.key.down = true;
    SDL_PushEvent(&e);
    CHECK(kb->GetDeviceState(256, keys) == DI_OK);                               // the device pumps SDL itself
    CHECK(keys[DIK_LCONTROL] == 0x80 && keys[DIK_RCONTROL] == 0x80 && keys[DIK_F9] == 0x80);
    CHECK(kb->GetDeviceState(100, keys) == E_INVALIDARG);

    SDL_zero(e);
    e.type = SDL_EVENT_MOUSE_MOTION; e.motion.windowID = wid; e.motion.x = 130; e.motion.y = 50; e.motion.xrel = 7; e.motion.yrel = 5;
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_MOUSE_MOTION; e.motion.windowID = wid; e.motion.x = 131; e.motion.y = 50; e.motion.xrel = 1; e.motion.yrel = 0;
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_MOUSE_WHEEL; e.wheel.windowID = wid; e.wheel.y = -2;
    SDL_PushEvent(&e);
    // the game asks for 0x14-byte records (screen.cpp's DiObjectData: ofs, data, time, seq, app), as the 32-bit struct was
    struct Rec20 { DWORD dwOfs, dwData, dwTimeStamp, dwSequence, uAppData; } data[16];
    DWORD count = 16;
    HRESULT hr = mouse->GetDeviceData(0x14, (LPDIDEVICEOBJECTDATA)data, &count, 0);
    CHECK(hr == DI_OK);
    CHECK_MSG(count == 3, "count=%u", count);                                    // X, Y (coalesced), Z
    CHECK(count >= 3 && data[0].dwOfs == DIMOFS_X && (int)data[0].dwData == 8 && data[1].dwOfs == DIMOFS_Y && (int)data[1].dwData == 5);
    CHECK(count >= 3 && data[2].dwOfs == DIMOFS_Z && (int)data[2].dwData == -240);
    DIMOUSESTATE2 ms;
    CHECK(mouse->GetDeviceState(sizeof ms, &ms) == DI_OK);
    CHECK(ms.lX == 8 && ms.lY == 5 && ms.lZ == -240);                            // the immediate state accumulated the same deltas
    CHECK(mouse->GetDeviceState(sizeof ms, &ms) == DI_OK && ms.lX == 0 && ms.lY == 0);  // relative: reset after read
    CHECK(ms.rgbButtons[1] == 0x80);                                             // the right button is still down
    CHECK(mouse->Unacquire() == DI_OK && mouse->GetDeviceState(sizeof ms, &ms) == DIERR_NOTACQUIRED);
    kb->Release(); mouse->Release();
    CHECK(di->Release() == 0);

    // ---- resize / focus / close ----
    g_seen.clear();
    SDL_SetWindowSize(sw, 800, 600);
    SDL_zero(e);
    e.type = SDL_EVENT_WINDOW_RESIZED; e.window.windowID = wid; e.window.data1 = 800; e.window.data2 = 600;
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_WINDOW_FOCUS_LOST; e.window.windowID = wid;
    SDL_PushEvent(&e);
    SDL_zero(e);
    e.type = SDL_EVENT_WINDOW_FOCUS_GAINED; e.window.windowID = wid;
    SDL_PushEvent(&e);
    pump_all();
    CHECK(find(WM_SIZE, 0) != nullptr);
    bool saw800 = false;
    for (auto& s : g_seen) if (s.msg == WM_SIZE && LOWORD(s.lp) == 800 && HIWORD(s.lp) == 600) saw800 = true;
    CHECK(saw800);
    CHECK(GetClientRect(hwnd, &cr) && cr.right == 800 && cr.bottom == 600);
    CHECK(find(WM_ACTIVATE) && find(WM_ACTIVATE)->wp == 0);                       // WA_INACTIVE first
    CHECK(find(WM_ACTIVATE, 1) && find(WM_ACTIVATE, 1)->wp == 1);
    CHECK(find(WM_ACTIVATEAPP) != nullptr);

    // SetWindowPos / GetWindowRect / ClientToScreen
    SetWindowPos(hwnd, HWND_NOTOPMOST, 40, 60, 320, 240, SWP_SHOWWINDOW);
    RECT wr;
    GetWindowRect(hwnd, &wr);
    CHECK(wr.right - wr.left == 320 && wr.bottom - wr.top == 240);
    POINT pt = {10, 10};
    ClientToScreen(hwnd, &pt);
    CHECK(pt.x == wr.left + 10 && pt.y == wr.top + 10);
    CHECK(ShowCursor(FALSE) == -1 && ShowCursor(TRUE) == 0);                      // the display counter
    CHECK(SetCursor(nullptr) != (HCURSOR)1);
    SetCursor(LoadCursorA(nullptr, (LPCSTR)IDC_ARROW));

    // timers: WM_TIMER arrives after the interval
    g_seen.clear();
    UINT_PTR tid = SetTimer(hwnd, 77, 20, nullptr);
    CHECK(tid == 77);
    DWORD t0 = timeGetTime();
    MSG msg;
    bool got_timer = false;
    while (timeGetTime() - t0 < 500 && !got_timer) {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_TIMER && msg.wParam == 77) got_timer = true;
            DispatchMessageW(&msg);
        } else Sleep(2);
    }
    CHECK(got_timer);
    CHECK(KillTimer(hwnd, 77));

    // posted messages from another thread wake GetMessage
    PostMessageA(hwnd, WM_USER + 5, 11, 22);
    CHECK(GetMessageW(&msg, nullptr, 0, 0) && msg.message != WM_QUIT);
    while (msg.message != WM_USER + 5) { DispatchMessageW(&msg); GetMessageW(&msg, nullptr, 0, 0); }
    CHECK(msg.wParam == 11 && msg.lParam == 22);
    CHECK(SendMessageA(hwnd, WM_USER + 6, 1, 2) == 0 && find(WM_USER + 6) && find(WM_USER + 6)->wp == 1);   // SendMessage is synchronous

    // ---- close: WM_CLOSE -> DefWindowProc -> DestroyWindow -> WM_DESTROY -> PostQuitMessage -> GetMessage returns 0 ----
    g_seen.clear();
    SDL_zero(e);
    e.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED; e.window.windowID = wid;
    SDL_PushEvent(&e);
    int rc_quit = -1;
    int guard = 0;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (++guard > 1000) break;
    }
    rc_quit = (int)msg.wParam;
    CHECK(find(WM_CLOSE) != nullptr);
    CHECK(find(WM_DESTROY) != nullptr);
    CHECK(rc_quit == 7);
    CHECK(!IsWindow(hwnd));

    // MessageBox without a display answer: default button
    CHECK(MessageBoxA(nullptr, "test", "test", MB_OK) == IDOK);
    CHECK(MessageBoxA(nullptr, "test", "test", MB_YESNO) == IDYES);

    printf("%d checks, %d failed\n", g_checks, g_fails);
    return g_fails;
}
