// patches/diag/rootwatch.cpp: log the GUI root-widget stack whenever it changes (patch set "diag").
//
// Diagnostic only. A static constructor starts a thread that every 20 ms reads g_gui's root vector (0x5d7080 in the
// original: std::vector<widget*>, first/last at +4/+8) and, when its contents change, appends one line to
// rootwatch.txt in the game folder: the time, the count, and each element's address and vtable. Reads only; a torn
// read during a push_back shows up as one odd line, never as a fault (the pointers are checked with IsBadReadPtr).
#include <windows.h>
#include <stdio.h>

struct gui_manager;
extern gui_manager g_gui;   // 0x5d7080 (strat/gui_manager.h)

namespace {

DWORD WINAPI watch(void*)
{
    unsigned prev[16] = {0};
    unsigned prev_n = ~0u;
    for (;;) {
        Sleep(20);
        unsigned** v = (unsigned**)((char*)&g_gui + 4);
        unsigned* first = v[0];
        unsigned* last = v[1];
        unsigned n = first ? (unsigned)(last - first) : 0;
        if (n > 16 || (n && IsBadReadPtr(first, n * 4)))
            continue;
        unsigned cur[16] = {0};
        for (unsigned i = 0; i < n; i++)
            cur[i] = first[i];
        if (n == prev_n && memcmp(cur, prev, sizeof cur) == 0)
            continue;
        prev_n = n;
        memcpy(prev, cur, sizeof cur);
        FILE* f = fopen("rootwatch.txt", "at");
        if (!f)
            continue;
        fprintf(f, "%lu n=%u", GetTickCount(), n);
        for (unsigned i = 0; i < n; i++) {
            unsigned vt = IsBadReadPtr((void*)cur[i], 4) ? 0 : *(unsigned*)cur[i];
            fprintf(f, "  [%u] %08x vt=%08x", i, cur[i], vt);
        }
        fprintf(f, "\n");
        fclose(f);
    }
    return 0;
}

struct Start {
    Start() { CloseHandle(CreateThread(0, 0, watch, 0, 0, 0)); }
} g_start;

}   // namespace
