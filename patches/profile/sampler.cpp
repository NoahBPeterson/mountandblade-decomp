// patches/profile/sampler.cpp: a sampling profiler for the game's main thread (patch set "profile").
//
// Not part of the reconstruction: build with `tools/relink/build.py --patches profile`. A static constructor (it runs
// on the main thread during CRT start-up) starts a thread that, every millisecond, suspends the main thread, takes its
// EIP and copies the top of its stack, resumes it, and appends one record to profile_samples.bin in the game folder:
//
//     header: u32 'MBPS', u32 tick0, u32 filetime0 lo, u32 filetime0 hi  (the tick and the wall clock taken together)
//     then per sample: u32 tick (GetTickCount), u32 eip, u32 n, u32 addr[n]   -- addr: stack dwords inside our .text
//
// Every 2 s (and at start) it also rewrites profile_modules.txt with the loaded modules ("base size path" per line),
// so samples outside our image can be charged to a DLL (fmod.dll, dsound, wined3d, ...).
//
// Nothing that can take a lock runs while the main thread is suspended (only GetThreadContext and memcpy of committed
// stack memory); the file is written with WriteFile, never the C runtime. tools/relink/profile_report.py symbolises it
// and splits it into the load phases logged in rgl_log.txt (tools/relink/logwatch.py stamps those lines).
#include <windows.h>
#include <stdio.h>
#include <string.h>

namespace {

const unsigned STACK_DWORDS = 384;          // how much of the stack to scan for return addresses
const unsigned MAX_ADDRS = 48;

HANDLE g_main;
HANDLE g_out;
unsigned g_text_lo, g_text_hi;

void text_range()
{
    char* base = (char*)GetModuleHandleA(0);
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
    IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
    g_text_lo = (unsigned)base + sec[0].VirtualAddress;     // .text is the first section
    g_text_hi = g_text_lo + sec[0].Misc.VirtualSize;
}

void write_modules()
{
    // every image mapping's allocation base is a module handle: walk the address space (no psapi in this toolchain)
    FILE* f = fopen("profile_modules.txt", "w");
    if (!f)
        return;
    char* p = 0;
    MEMORY_BASIC_INFORMATION mbi;
    void* last = 0;
    while ((unsigned)p < 0x7fff0000u && VirtualQuery(p, &mbi, sizeof mbi)) {
        if (mbi.Type == MEM_IMAGE && mbi.AllocationBase != last) {
            last = mbi.AllocationBase;
            char path[MAX_PATH];
            char* b = (char*)mbi.AllocationBase;
            if (GetModuleFileNameA((HMODULE)b, path, sizeof path) && ((IMAGE_DOS_HEADER*)b)->e_magic == IMAGE_DOS_SIGNATURE) {
                IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(b + ((IMAGE_DOS_HEADER*)b)->e_lfanew);
                fprintf(f, "%08x %08x %s\n", (unsigned)b, (unsigned)nt->OptionalHeader.SizeOfImage, path);
            }
        }
        p = (char*)mbi.BaseAddress + mbi.RegionSize;
    }
    fclose(f);
}

DWORD WINAPI sampler(void*)
{
    DWORD last_mods = 0;
    static unsigned stack[STACK_DWORDS];
    static unsigned rec[3 + MAX_ADDRS];
    static char buf[1 << 16];
    unsigned used = 0;
    timeBeginPeriod(1);
    for (;;) {
        Sleep(1);
        if (GetTickCount() - last_mods > 2000) {     // on the sampler thread, with the main thread running
            write_modules();
            last_mods = GetTickCount();
        }
        CONTEXT ctx;
        ctx.ContextFlags = CONTEXT_CONTROL;
        if (SuspendThread(g_main) == (DWORD)-1)
            break;
        BOOL ok = GetThreadContext(g_main, &ctx);
        unsigned n = 0;
        if (ok) {
            MEMORY_BASIC_INFORMATION mbi;           // copy only what is committed above esp
            unsigned want = STACK_DWORDS * 4;
            if (VirtualQuery((void*)ctx.Esp, &mbi, sizeof mbi)) {
                unsigned avail = (unsigned)mbi.BaseAddress + (unsigned)mbi.RegionSize - ctx.Esp;
                if (avail < want)
                    want = avail & ~3u;
                memcpy(stack, (void*)ctx.Esp, want);
                n = want / 4;
            }
        }
        ResumeThread(g_main);
        if (!ok)
            continue;
        unsigned k = 0;
        for (unsigned i = 0; i < n && k < MAX_ADDRS; i++)
            if (stack[i] >= g_text_lo && stack[i] < g_text_hi)
                rec[3 + k++] = stack[i];
        rec[0] = GetTickCount();
        rec[1] = ctx.Eip;
        rec[2] = k;
        unsigned bytes = (3 + k) * 4;
        if (used + bytes > sizeof buf) {
            DWORD w;
            WriteFile(g_out, buf, used, &w, 0);
            used = 0;
        }
        memcpy(buf + used, rec, bytes);
        used += bytes;
        if (used > sizeof buf / 2) {                 // keep the file current: the game may be killed at any time
            DWORD w;
            WriteFile(g_out, buf, used, &w, 0);
            used = 0;
        }
    }
    return 0;
}

struct Start {
    Start()
    {
        text_range();
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &g_main,
                        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, 0);
        g_out = CreateFileA("profile_samples.bin", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, 0);
        if (g_main && g_out != INVALID_HANDLE_VALUE) {
            // header: 'MBPS', the tick and the wall clock (FILETIME) taken together, so ticks map to the log stamps
            unsigned hdr[4];
            FILETIME ft;
            hdr[0] = 0x5350424d;
            hdr[1] = GetTickCount();
            GetSystemTimeAsFileTime(&ft);
            hdr[2] = ft.dwLowDateTime;
            hdr[3] = ft.dwHighDateTime;
            DWORD w;
            WriteFile(g_out, hdr, sizeof hdr, &w, 0);
            CloseHandle(CreateThread(0, 0, sampler, 0, 0, 0));
        }
    }
} g_start;

}   // namespace

#pragma comment(lib, "winmm.lib")
