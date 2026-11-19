// KERNEL / shell / registry / GDI / dialog subset on real data: the game directory (argv[1]) with mixed-case backslash
// paths, threads and events, critical sections, the registry store, DIB + TrueType text, the virtual launcher dialog.
#include <windows.h>
#include <shlobj.h>
#include <mmsystem.h>
#include <io.h>
#include <mb_crt_extra.h>
#undef max
#undef min
#include "../mb_paths.h"
#include "../mb_sdl.h"
#include "check.h"
#include <string>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <process.h>
#include <sys/stat.h>

static std::string slurp(FILE* f)
{
    std::string s;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) s.append(buf, n);
    return s;
}

// ---- threads ----
static HANDLE g_ev_auto, g_ev_manual;
static std::atomic<int> g_thread_ran{0};
static DWORD WINAPI worker(LPVOID p) { g_thread_ran += (int)(intptr_t)p; SetEvent(g_ev_auto); return 42; }
static CRITICAL_SECTION g_cs;
static long g_counter = 0;
static DWORD WINAPI counter_thread(LPVOID) { for (int i = 0; i < 20000; ++i) { EnterCriticalSection(&g_cs); EnterCriticalSection(&g_cs); ++g_counter; LeaveCriticalSection(&g_cs); LeaveCriticalSection(&g_cs); } return 0; }
static std::atomic<int> g_bt{0};
static void __cdecl begin_thread_fn(void* arg) { g_bt = (int)(intptr_t)arg; SetEvent(g_ev_manual); }

// ---- dialogs ----
static INT_PTR CALLBACK launcher_proc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
    static bool inited = false;
    if (m == WM_INITDIALOG) {
        inited = true;
        HWND list = GetDlgItem(h, 0x420);
        SendMessageA(list, 0x14b, 0, 0);                                    // CB_RESETCONTENT
        const char* names[] = {"Native", "Other"};
        for (int i = 0; i < 2; ++i) { int r = (int)SendMessageA(list, 0x14a, 0, (LPARAM)names[i]); SendMessageA(list, 0x151, (WPARAM)r, i); }
        int r = (int)SendMessageA(list, 0x158, (WPARAM)-1, (LPARAM)"other");  // CB_FINDSTRINGEXACT, case-insensitive
        SendMessageA(list, 0x14e, (WPARAM)r, 0);                              // CB_SETCURSEL
        PostMessageA(h, WM_COMMAND, 0x10000, (LPARAM)list);
        return 1;
    }
    if (m == WM_COMMAND && LOWORD(wp) == 0x410) {
        int idx = (int)SendMessageA(GetDlgItem(h, 0x420), 0x147, 0, 0);      // CB_GETCURSEL
        int sel = (int)SendMessageA(GetDlgItem(h, 0x420), 0x150, (WPARAM)idx, 0);
        SetDlgItemTextA(h, 0x500, sel == 1 ? "picked-other" : "picked-wrong");
        char buf[64];
        GetDlgItemTextA(h, 0x500, buf, sizeof buf);
        EndDialog(h, strcmp(buf, "picked-other") == 0 ? 0x410 : 99);
        return 1;
    }
    return 0;
}
static INT_PTR CALLBACK ignoring_proc(HWND h, UINT m, WPARAM, LPARAM) { return 0; }

int main(int argc, char** argv)
{
    setenv("SDL_VIDEODRIVER", "dummy", 1);
    setenv("MBP_NO_DIALOGS", "1", 1);
    std::string game = argc > 1 ? argv[1] : "game";
    char tmpl[] = "/tmp/mbp_test_XXXXXX";
    std::string tmp = mkdtemp(tmpl);
    setenv("MB_USER_DIR", tmp.c_str(), 1);
    CHECK(chdir(game.c_str()) == 0);

    // ================= the game's file layer: mixed-case backslash paths ==================
    char out[1024];
    mbp_paths_force_scan(1);                                   // exercise the directory index even on a case-insensitive volume
    int ex = mbp_resolve_path("modules\\NATIVE\\sceneobj", out, sizeof out);
    CHECK(ex == 1 && std::string(out) == "Modules/Native/SceneObj");
    CHECK(mbp_resolve_path("DATA\\Ground_Specs.TXT", out, sizeof out) == 1 && std::string(out) == "Data/ground_specs.txt");
    CHECK(mbp_resolve_path(".\\Modules\\\\native\\MODULE.INI", out, sizeof out) == 1 && std::string(out) == "Modules/Native/module.ini");
    CHECK(mbp_resolve_path("DATA\\brand_new_file.txt", out, sizeof out) == 0 && std::string(out) == "Data/brand_new_file.txt");   // parent resolved, leaf kept
    CHECK(mbp_resolve_path("nodir\\x\\y.txt", out, sizeof out) == 0 && std::string(out) == "nodir/x/y.txt");
    CHECK(mbp_resolve_path("C:\\Windows\\x", out, sizeof out) == 0 && std::string(out) == "/Windows/x");   // drive-root path
    FILE* f = mb_fopen("DATA\\Ground_Specs.TXT", "r");
    CHECK(f != nullptr);
    if (f) { std::string s = slurp(f); CHECK(s.size() > 10); fclose(f); }
    // text mode "r" strips CR of CRLF; "rb" keeps it (module.ini has CRLF line ends)
    f = mb_fopen("modules\\native\\module.ini", "r");
    CHECK(f != nullptr);
    if (f) { std::string s = slurp(f); CHECK(s.find('\r') == std::string::npos && s.find('\n') != std::string::npos); fclose(f); }
    f = mb_fopen("MODULES\\NATIVE\\MODULE.INI", "rb");
    CHECK(f != nullptr);
    if (f) { std::string s = slurp(f); CHECK(s.find("\r\n") != std::string::npos); fclose(f); }
    f = mb_fopen("modules\\native\\module.ini", "rt");
    if (f) { char line[256]; CHECK(fgets(line, sizeof line, f) && strchr(line, '\n') && !strchr(line, '\r')); CHECK(fseek(f, 0, SEEK_SET) == 0); fclose(f); }
    CHECK(mb_fopen("data\\does_not_exist.txt", "r") == nullptr);
    mbp_paths_force_scan(0);
    CHECK(mb_access("DATA\\FALLS.TXT", 0) == 0);

    // FindFirstFileA family: sorted, case-insensitive, wildcard
    WIN32_FIND_DATAA fd;
    HANDLE fh = FindFirstFileA("Modules\\*", &fd);
    CHECK(fh != INVALID_HANDLE_VALUE);
    std::vector<std::string> names;
    if (fh != INVALID_HANDLE_VALUE) {
        do names.push_back(fd.cFileName); while (FindNextFileA(fh, &fd));
        CHECK(GetLastError() == 18);                                    // ERROR_NO_MORE_FILES
        FindClose(fh);
    }
    bool has_native = false;
    for (auto& n : names) if (n == "Native") has_native = true;
    CHECK(has_native && names[0] == "." && names[1] == "..");
    fh = FindFirstFileA("modules\\native\\*.INI", &fd);
    CHECK(fh != INVALID_HANDLE_VALUE && std::string(fd.cFileName) == "module.ini" && !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.nFileSizeLow > 0);
    if (fh != INVALID_HANDLE_VALUE) FindClose(fh);
    fh = FindFirstFileA(".\\Skins\\mb_skin_xx_nonexistent.smf", &fd);
    CHECK(fh == INVALID_HANDLE_VALUE && GetLastError() == 2);
    struct _finddata_t fdt;
    intptr_t h2 = _findfirst("Modules\\*", &fdt);
    CHECK(h2 != -1);
    int found_dir = 0;
    if (h2 != -1) { do if (!strcmp(fdt.name, "Native") && (fdt.attrib & _A_SUBDIR)) found_dir = 1; while (_findnext(h2, &fdt) == 0); _findclose(h2); }
    CHECK(found_dir);
    struct _stat st;
    CHECK(_stat("MODULES\\native\\Module.INI", &st) == 0 && st.st_size > 0);
    CHECK(GetFileAttributesA("MODULES\\NATIVE") & FILE_ATTRIBUTE_DIRECTORY);
    CHECK(GetFileAttributesA("nothing_here") == 0xFFFFFFFFu);

    // shell folders + directories + files in the user data dir
    char folder[260], path[600];
    CHECK(SHGetFolderPathA(0, CSIDL_APPDATA, 0, 0, folder) == S_OK);
    CHECK(std::string(folder) == tmp);
    sprintf(path, "%s\\Mount&Blade", folder);
    CHECK(CreateDirectoryA(path, 0));
    CHECK(!CreateDirectoryA(path, 0) && GetLastError() == ERROR_ALREADY_EXISTS);
    sprintf(path, "%s\\Mount&Blade\\rgl_config.txt", folder);
    f = mb_fopen(path, "wb");
    CHECK(f != nullptr);
    if (f) { fputs("hello\n", f); fclose(f); }
    sprintf(path, "%s\\mount&blade\\RGL_CONFIG.TXT", folder);       // case differs
    CHECK(GetFileAttributesA(path) != 0xFFFFFFFFu);
    char dst[600];
    sprintf(dst, "%s\\Mount&Blade\\copy.txt", folder);
    CHECK(CopyFileA(path, dst, TRUE));
    CHECK(!CopyFileA(path, dst, TRUE) && GetLastError() == 80);
    char dst2[600];
    sprintf(dst2, "%s\\Mount&Blade\\moved.txt", folder);
    CHECK(MoveFileA(dst, dst2));
    CHECK(mb_rename(dst2, path) == -1);                              // the CRT does not replace an existing file
    CHECK(mb_remove(dst2) == 0);
    CHECK(DeleteFileA(path) && !DeleteFileA(path));
    char dirp[600];
    sprintf(dirp, "%s\\Mount&Blade", folder);
    CHECK(RemoveDirectoryA(dirp));
    CHECK(SHGetFolderPathA(0, CSIDL_PERSONAL, 0, 0, folder) == S_OK && std::string(folder) == tmp + "/Documents");
    CHECK(SHGetFolderPathA(0, 0x7777, 0, 0, folder) == E_INVALIDARG);

    // registry: the game's calls (world_app.cpp / options_dlg.cpp)
    HKEY key = 0;
    DWORD disp = 0;
    CHECK(RegCreateKeyExA((HKEY)0x80000001, "Software\\MountAndBladeKeys", 0, 0, 0, 0x20007, 0, &key, &disp) == 0 && disp == 1);
    CHECK(RegSetValueExA(key, "last_module", 0, REG_SZ, (const BYTE*)"Native", 7) == 0);
    DWORD dv = 1234;
    CHECK(RegSetValueExA(key, "Volume", 0, REG_DWORD, (const BYTE*)&dv, 4) == 0);
    RegCloseKey(key);
    key = 0;
    CHECK(RegCreateKeyExA(HKEY_CURRENT_USER, "software\\mountandbladekeys", 0, 0, 0, 0x20007, 0, &key, &disp) == 0 && disp == 2);
    char val[64] = {0};
    DWORD size = sizeof val, type = 0;
    CHECK(RegQueryValueExA(key, "LAST_MODULE", 0, &type, (LPBYTE)val, &size) == 0 && type == REG_SZ && std::string(val) == "Native" && size == 7);
    size = 2;
    CHECK(RegQueryValueExA(key, "last_module", 0, &type, (LPBYTE)val, &size) == 234 && size == 7);   // ERROR_MORE_DATA tells the size
    DWORD got = 0; size = 4;
    CHECK(RegQueryValueExA(key, "volume", 0, &type, (LPBYTE)&got, &size) == 0 && type == REG_DWORD && got == 1234);
    CHECK(RegQueryValueExA(key, "missing", 0, &type, (LPBYTE)val, &size) == ERROR_FILE_NOT_FOUND);
    HKEY k2 = 0;
    CHECK(RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Nope", 0, KEY_READ, &k2) == ERROR_FILE_NOT_FOUND);
    RegCloseKey(key);
    { std::string rp = tmp + "/Mount&Blade/registry.ini"; FILE* rf = fopen(rp.c_str(), "rb"); CHECK(rf != nullptr); if (rf) { std::string s = slurp(rf); fclose(rf); CHECK(s.find("[HKCU\\Software\\MountAndBladeKeys]") != std::string::npos && s.find("last_module=sz:Native") != std::string::npos); } }

    // ================= time =================
    LARGE_INTEGER q0, q1, fq;
    QueryPerformanceFrequency(&fq);
    QueryPerformanceCounter(&q0);
    DWORD t0 = timeGetTime();
    Sleep(60);
    QueryPerformanceCounter(&q1);
    double elapsed = (double)(q1.QuadPart - q0.QuadPart) * 1000.0 / (double)fq.QuadPart;
    CHECK(elapsed >= 55 && elapsed < 400);
    CHECK(timeGetTime() - t0 >= 55);

    // ================= events / threads / critical sections =================
    g_ev_auto = CreateEventA(0, FALSE, FALSE, 0);
    g_ev_manual = CreateEventA(0, TRUE, FALSE, 0);
    CHECK(WaitForSingleObject(g_ev_auto, 0) == WAIT_TIMEOUT);
    SetEvent(g_ev_auto);
    CHECK(WaitForSingleObject(g_ev_auto, 0) == WAIT_OBJECT_0);
    CHECK(WaitForSingleObject(g_ev_auto, 0) == WAIT_TIMEOUT);          // auto-reset consumed
    SetEvent(g_ev_manual);
    CHECK(WaitForSingleObject(g_ev_manual, 0) == WAIT_OBJECT_0 && WaitForSingleObject(g_ev_manual, 0) == WAIT_OBJECT_0);
    ResetEvent(g_ev_manual);
    CHECK(WaitForSingleObject(g_ev_manual, 30) == WAIT_TIMEOUT);
    DWORD tid = 0;
    HANDLE th = CreateThread(0, 0, worker, (LPVOID)5, 0, &tid);
    CHECK(th != nullptr);
    CHECK(WaitForSingleObject(g_ev_auto, 2000) == WAIT_OBJECT_0);
    CHECK(WaitForSingleObject(th, 2000) == WAIT_OBJECT_0);             // a finished thread is signalled
    CHECK(g_thread_ran == 5);
    CHECK(CloseHandle(th));
    CHECK(_beginthread(begin_thread_fn, 0x40000, (void*)9) != (uintptr_t)-1);
    CHECK(WaitForSingleObject(g_ev_manual, 2000) == WAIT_OBJECT_0 && g_bt == 9);   // the loader-thread pattern (cls_map.cpp)
    HANDLE both[2] = {g_ev_auto, g_ev_manual};
    SetEvent(g_ev_auto);
    CHECK(WaitForMultipleObjects(2, both, TRUE, 100) == WAIT_OBJECT_0);
    CHECK(WaitForMultipleObjects(2, both, FALSE, 0) == 1);             // only the manual event stays set
    InitializeCriticalSection(&g_cs);
    HANDLE ths[4];
    for (auto& t : ths) t = CreateThread(0, 0, counter_thread, 0, 0, 0);
    CHECK(WaitForMultipleObjects(4, ths, TRUE, 20000) == WAIT_OBJECT_0);
    CHECK(g_counter == 80000);                                          // recursive, mutually exclusive
    for (auto& t : ths) CloseHandle(t);
    DeleteCriticalSection(&g_cs);
    CHECK(CloseHandle(g_ev_auto) && CloseHandle(g_ev_manual));
    CHECK(!CloseHandle((HANDLE)1234));
    DWORD_PTR pm = 0, sm = 0;
    CHECK(GetProcessAffinityMask(GetCurrentProcess(), &pm, &sm) && pm >= 1 && pm == sm);
    CHECK(GetCurrentThreadId() != 0);

    // ================= processes (binkplay.exe: a Windows program that cannot run here) =================
    STARTUPINFOA si; PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof si); si.cb = sizeof si; memset(&pi, 0, sizeof pi);
    CHECK(CreateProcessA("binkplay.exe", (LPSTR)" paradox.bik /P /I2 /Z /J /U1 /W-1 /H-1 /C /B2", 0, 0, FALSE, 0, 0, 0, &si, &pi));
    CHECK(WaitForSingleObject(pi.hProcess, 0x57e4) == WAIT_OBJECT_0);
    DWORD code = 99;
    CHECK(GetExitCodeProcess(pi.hProcess, &code) && code == 0);
    CloseHandle(pi.hProcess);
    memset(&pi, 0, sizeof pi);
    char cmd[] = "/bin/sh -c \"exit 3\"";
    CHECK(CreateProcessA("/bin/sh", cmd, 0, 0, FALSE, 0, 0, 0, &si, &pi));
    CHECK(WaitForSingleObject(pi.hProcess, 5000) == WAIT_OBJECT_0);
    CHECK(GetExitCodeProcess(pi.hProcess, &code) && code == 3);
    CloseHandle(pi.hProcess);

    // ================= memory, strings, code pages =================
    HGLOBAL g = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 32);
    char* gp = (char*)GlobalLock(g);
    CHECK(gp && gp[31] == 0);
    strcpy(gp, "abc");
    GlobalUnlock(g);
    GlobalFree(g);
    void* va = VirtualAlloc(0, 1 << 16, 0x3000, 4);
    CHECK(va != nullptr);
    ((char*)va)[100] = 1;
    CHECK(VirtualFree(va, 0, 0x8000));
    MEMORYSTATUS ms;
    GlobalMemoryStatus(&ms);
    CHECK(ms.dwTotalPhys > (1u << 28) && ms.dwAvailPhys > 0);
    char b[64];
    CHECK(wsprintfA(b, "%s|%d|%lu|%5.3s|%x|%c|%%", "str", -5, 4000000000ul, "abcdef", 255, 'z') > 0 && std::string(b) == "str|-5|4000000000|  abc|ff|z|%");
    CHECK(lstrcpynA(b, "0123456789", 5) == b && std::string(b) == "0123");
    CHECK(MulDiv(3, 7, 2) == 11 && MulDiv(-3, 7, 2) == -11 && MulDiv(1, 1, 0) == -1);
    wchar_t w[16];
    CHECK(MultiByteToWideChar(65001, 0, "\xC3\xA9x", -1, w, 16) == 3 && w[0] == 0xE9 && w[1] == 'x');
    char a8[16];
    CHECK(WideCharToMultiByte(0, 0, w, -1, a8, 16, 0, 0) == 3 && (unsigned char)a8[0] == 0xE9);
    CHECK(lstrcmpiA("AbC", "aBc") == 0);
    char mod[512];
    CHECK(GetModuleFileNameA(0, mod, sizeof mod) > 0);
    CHECK(LoadLibraryA("riched20.dll") != nullptr);
    CHECK(LoadLibraryA("nonexistent_xyz.dll") == nullptr && GetLastError() == 126);

    // ================= GDI: DIB section + TrueType text (the d3dfont.cpp bake sequence) =================
    HDC dc = CreateCompatibleDC(nullptr);
    SetMapMode(dc, MM_TEXT);
    CHECK(GetDeviceCaps(dc, LOGPIXELSY) == 96);
    int nHeight = -MulDiv(9, (int)(GetDeviceCaps(dc, LOGPIXELSY) * 1.0f), 72);
    HFONT font = CreateFontA(nHeight, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, "Arial");
    CHECK(font != nullptr);
    if (font) {
        HGDIOBJ oldf = SelectObject(dc, font);
        SIZE sz;
        CHECK(GetTextExtentPoint32A(dc, "W", 1, &sz) && sz.cx > 4 && sz.cy > 8);
        SIZE sz2;
        GetTextExtentPoint32A(dc, "WWWW", 4, &sz2);
        CHECK(sz2.cx > sz.cx * 3);
        BITMAPINFO bmi;
        memset(&bmi.bmiHeader, 0, sizeof bmi.bmiHeader);
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); bmi.bmiHeader.biWidth = 128; bmi.bmiHeader.biHeight = -128;
        bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biCompression = BI_RGB; bmi.bmiHeader.biBitCount = 32;
        DWORD* bits = nullptr;
        HBITMAP bmp = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, (void**)&bits, nullptr, 0);
        CHECK(bmp != nullptr && bits != nullptr);
        HGDIOBJ oldb = SelectObject(dc, bmp);
        SetTextColor(dc, RGB(255, 255, 255));
        SetBkColor(dc, 0);
        SetTextAlign(dc, TA_TOP);
        CHECK(ExtTextOutA(dc, 2, 2, ETO_OPAQUE, nullptr, "W", 1, nullptr));
        int lit = 0, bright = 0;
        for (int i = 0; i < 128 * 128; ++i) { if (bits[i] & 0xFF) ++lit; if ((bits[i] & 0xFF) > 200) ++bright; }
        CHECK_MSG(lit > 20 && bright > 5, "lit=%d bright=%d", lit, bright);       // the glyph's alpha, white on black: low byte = coverage
        BITMAP bm;
        CHECK(GetObjectA(bmp, sizeof bm, &bm) == sizeof bm && bm.bmWidth == 128 && bm.bmHeight == 128 && bm.bmBits == bits);
        SelectObject(dc, oldb);
        SelectObject(dc, oldf);
        DeleteObject(bmp);
        DeleteObject(font);
    }
    // a .bmp from the game dir through the loader (case-insensitive), blitted between memory DCs (options_dlg.cpp's paint)
    HBITMAP logo = (HBITMAP)LoadImageA(0, "MAIN.BMP", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    CHECK(logo != nullptr);
    if (logo) {
        BITMAP bm;
        GetObjectA(logo, sizeof bm, &bm);
        CHECK(bm.bmWidth > 0 && bm.bmHeight > 0);
        HDC mem = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(mem, logo);
        BITMAPINFO bmi;
        memset(&bmi.bmiHeader, 0, sizeof bmi.bmiHeader);
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); bmi.bmiHeader.biWidth = 16; bmi.bmiHeader.biHeight = -16;
        bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biBitCount = 32;
        DWORD* px = nullptr;
        HBITMAP target = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, (void**)&px, nullptr, 0);
        HGDIOBJ oldt = SelectObject(dc, target);
        CHECK(BitBlt(dc, 0, 0, 16, 16, mem, 0, 0, SRCCOPY));
        int nz = 0;
        for (int i = 0; i < 256; ++i) if (px[i]) ++nz;
        CHECK(nz > 0);
        SelectObject(dc, oldt);
        DeleteObject(target);
        SelectObject(mem, old);
        DeleteDC(mem);
        DeleteObject(logo);
    }
    CHECK(LoadImageA(0, MAKEINTRESOURCEA(105), IMAGE_BITMAP, 0, 0, 0) == nullptr);   // no embedded resources
    DeleteDC(dc);

    // ================= dialogs: the launcher runs headless =================
    INT_PTR r = DialogBoxParamA(0, MAKEINTRESOURCEA(0xaa), nullptr, launcher_proc, 0);
    CHECK_MSG(r == 0x410, "launcher returned %lld", (long long)r);          // WM_INITDIALOG populated the combo, "Play" returned 0x410
    r = DialogBoxParamA(0, MAKEINTRESOURCEA(0xc1), nullptr, ignoring_proc, 0);
    CHECK(r == IDCANCEL);                                                   // an unattended dialog is cancelled
    HWND dlg = CreateDialogParamA(0, MAKEINTRESOURCEA(0xc0), nullptr, launcher_proc, 0);
    CHECK(dlg != nullptr);
    CheckRadioButton(dlg, 100, 103, 102);
    CHECK(IsDlgButtonChecked(dlg, 102) == BST_CHECKED && IsDlgButtonChecked(dlg, 101) == BST_UNCHECKED);
    SetDlgItemInt(dlg, 200, 42, FALSE);
    BOOL ok = FALSE;
    CHECK(GetDlgItemInt(dlg, 200, &ok, FALSE) == 42 && ok);
    SendDlgItemMessageA(dlg, 300, 0x406, 0, MAKELONG(0, 10));              // TBM_SETRANGE
    SendDlgItemMessageA(dlg, 300, 0x405, 1, 20);                           // TBM_SETPOS beyond the range clamps
    CHECK(SendDlgItemMessageA(dlg, 300, 0x400, 0, 0) == 10);
    CHECK(DestroyWindow(dlg) && !IsWindow(dlg));

    // clipboard (SDL's, under the dummy video driver it is process-local)
    CHECK(OpenClipboard(0));
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, 16);
    strcpy((char*)GlobalLock(hg), "line1\r\nline2");
    GlobalUnlock(hg);
    CHECK(EmptyClipboard());
    CHECK(SetClipboardData(CF_TEXT, hg) == hg);
    CHECK(IsClipboardFormatAvailable(CF_TEXT));
    HANDLE ch = GetClipboardData(CF_TEXT);
    CHECK(ch != nullptr && std::string((const char*)GlobalLock(ch)) == "line1\r\nline2");
    CHECK(CloseClipboard());
    CHECK(!CloseClipboard());

    // cleanup of the sandbox
    std::string cmdrm = "rm -rf '" + tmp + "'";
    system(cmdrm.c_str());
    printf("%d checks, %d failed\n", g_checks, g_fails);
    return g_fails;
}
