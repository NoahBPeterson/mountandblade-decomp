// port/runtime/main.cpp: the portable build's entry point (ticket 004 milestone 2). It hands the process to the game's own
// WinMain (src/game/cls_map/cls_map.cpp, 0x4d9450) the way the Windows loader's CRT startup does: the command line
// without the program name, SW_SHOWDEFAULT, a non-null instance handle. Ticket 007's platform layer will put SDL's
// initialisation in front of it; until then every Win32 call WinMain makes lands in port/stubs/platform_stubs.cpp.
#include <windows.h>
#include "../src/game/tstring.h"
static_assert(sizeof(tstring) == MB_TSTRING_SIZE, "MB_TSTRING_SIZE (port_prelude.h) must be sizeof(tstring)");
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <execinfo.h>
#include <fcntl.h>
#include <sys/ucontext.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);

static HINSTANCE__ g_port_instance;    // GetModuleHandle(NULL)'s stand-in: only compared against NULL and passed back

// MB_CRASH_BT=1: on SIGSEGV / SIGBUS / SIGILL / SIGFPE / SIGABRT (malloc errors) write the fault address and a backtrace to stderr (the macOS crash
// report is not always written, e.g. when ReportCrash is throttled), then die with the signal as before.
static int g_bt_fd = 2;   // MB_CRASH_BT=<path>: the backtrace goes to that file instead (stderr may be closed by then)
static void crash_bt(int sig, siginfo_t* si, void* ctx)
{
    char head[1024];
    int n = snprintf(head, sizeof head, "\n---- signal %d at address %p ----\n", sig, si ? si->si_addr : nullptr);
    write(g_bt_fd, head, n > 0 ? (size_t)n : 0);
#if defined(__APPLE__) && defined(__aarch64__)
    if (ctx) {
        // the interrupted pc / lr and registers (backtrace() below starts at the caller of a frameless leaf)
        const __darwin_arm_thread_state64& ss = ((ucontext_t*)ctx)->uc_mcontext->__ss;
        n = snprintf(head, sizeof head, "pc %llx lr %llx fp %llx sp %llx\n", (unsigned long long)__darwin_arm_thread_state64_get_pc(ss),
                     (unsigned long long)__darwin_arm_thread_state64_get_lr(ss), (unsigned long long)__darwin_arm_thread_state64_get_fp(ss),
                     (unsigned long long)__darwin_arm_thread_state64_get_sp(ss));
        write(g_bt_fd, head, n > 0 ? (size_t)n : 0);
        n = 0;
        for (int i = 0; i < 29; ++i)
            n += snprintf(head + n, sizeof head - n, "x%d=%llx%s", i, (unsigned long long)ss.__x[i], i % 6 == 5 ? "\n" : " ");
        n += snprintf(head + n, sizeof head - n, "\n");
        write(g_bt_fd, head, (size_t)n);
    }
#endif
    void* frames[64];
    int k = backtrace(frames, 64);
    backtrace_symbols_fd(frames, k, g_bt_fd);
    signal(sig, SIG_DFL);
    raise(sig);
}

int main(int argc, char** argv)
{
    if (const char* e = getenv("MB_CRASH_BT")) {
        if (*e && *e != '0') {
            if (strcmp(e, "1") != 0)
                g_bt_fd = open(e, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            struct sigaction sa;
            memset(&sa, 0, sizeof sa);
            sa.sa_sigaction = crash_bt;
            sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
            int sigs[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT};
            for (int sg : sigs)
                sigaction(sg, &sa, nullptr);
        }
    }
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
