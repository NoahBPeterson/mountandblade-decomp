// port/runtime/main.cpp: the portable build's entry point (ticket 004 milestone 2). It hands the process to the game's own
// WinMain (src/game/cls_map/cls_map.cpp, 0x4d9450) the way the Windows loader's CRT startup does: the command line
// without the program name, SW_SHOWDEFAULT, a non-null instance handle. Ticket 007's platform layer will put SDL's
// initialisation in front of it; until then every Win32 call WinMain makes lands in port/stubs/platform_stubs.cpp.
#include <windows.h>
#include <string.h>
#include <stdlib.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);

static HINSTANCE__ g_port_instance;    // GetModuleHandle(NULL)'s stand-in: only compared against NULL and passed back

int main(int argc, char** argv)
{
    size_t n = 1;
    for (int i = 1; i < argc; ++i)
        n += strlen(argv[i]) + 1;
    char* cmd = (char*)calloc(n, 1);
    for (int i = 1; i < argc; ++i) {
        if (i > 1)
            strcat(cmd, " ");
        strcat(cmd, argv[i]);
    }
    int rc = WinMain(&g_port_instance, 0, cmd, 10 /* SW_SHOWDEFAULT */);
    free(cmd);
    return rc;
}
