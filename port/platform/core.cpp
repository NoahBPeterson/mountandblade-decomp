// port/platform/core.cpp: KERNEL32 over POSIX: time, sleep, critical sections, events, threads, processes, global memory,
// virtual memory, code-page conversion, lstr*, wsprintf, system information.
#include "mbp_internal.h"
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/sysctl.h>
#endif

using namespace mbp;

namespace {
thread_local DWORD t_last_error = 0;
std::mutex g_sync_mu;                  // one lock + one condition variable for every waitable object (few, low traffic)
std::condition_variable g_sync_cv;

unsigned long long now_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000000000ull + (unsigned long long)ts.tv_nsec;
}

struct Event : KObj { bool manual, signalled; };
struct Thread : KObj {
    LPTHREAD_START_ROUTINE fn; LPVOID arg; pthread_t th; bool done, closed, started; DWORD exit_code;
};
struct Process : KObj { pid_t pid; bool done; DWORD exit_code; bool pseudo; };
}   // namespace

namespace mbp {
void set_last_error(DWORD e) { t_last_error = e; }
static std::mutex g_obj_mu;
static std::map<void*, int>& objs() { static std::map<void*, int> m; return m; }
void kobj_add(void* p) { std::lock_guard<std::mutex> g(g_obj_mu); objs()[p] = 1; }
void kobj_remove(void* p) { std::lock_guard<std::mutex> g(g_obj_mu); objs().erase(p); }
bool kobj_live(void* p) { std::lock_guard<std::mutex> g(g_obj_mu); return objs().count(p) != 0; }
}

// =====================================================================================================================
// time
// =====================================================================================================================
extern "C" {
DWORD WINAPI timeGetTime(void) { return (DWORD)(now_ns() / 1000000ull); }
DWORD WINAPI GetTickCount() { return (DWORD)(now_ns() / 1000000ull); }
UINT WINAPI timeBeginPeriod(UINT) { return 0; }       // TIMERR_NOERROR: timers here are nanosecond-resolution already
UINT WINAPI timeEndPeriod(UINT) { return 0; }
BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER* f) { f->QuadPart = 1000000000ll; return TRUE; }
BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER* c) { c->QuadPart = (LONGLONG)now_ns(); return TRUE; }
void WINAPI Sleep(DWORD ms)
{
    struct timespec ts, rem;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&ts, &rem) == -1 && errno == EINTR)
        ts = rem;
}
static void fill_systemtime(SYSTEMTIME* st, const struct tm& tm, int ms)
{
    st->wYear = (WORD)(tm.tm_year + 1900); st->wMonth = (WORD)(tm.tm_mon + 1); st->wDayOfWeek = (WORD)tm.tm_wday;
    st->wDay = (WORD)tm.tm_mday; st->wHour = (WORD)tm.tm_hour; st->wMinute = (WORD)tm.tm_min;
    st->wSecond = (WORD)tm.tm_sec; st->wMilliseconds = (WORD)ms;
}
void WINAPI GetLocalTime(LPSYSTEMTIME st)
{
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    time_t t = ts.tv_sec; struct tm tm; localtime_r(&t, &tm);
    fill_systemtime(st, tm, (int)(ts.tv_nsec / 1000000));
}
void WINAPI GetSystemTime(LPSYSTEMTIME st)
{
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    time_t t = ts.tv_sec; struct tm tm; gmtime_r(&t, &tm);
    fill_systemtime(st, tm, (int)(ts.tv_nsec / 1000000));
}

// =====================================================================================================================
// last error, debug output
// =====================================================================================================================
DWORD WINAPI GetLastError() { return t_last_error; }
void WINAPI SetLastError(DWORD e) { t_last_error = e; }
void WINAPI OutputDebugStringA(LPCSTR s)
{
    static const bool on = getenv("MBP_DEBUG_OUTPUT") != nullptr;     // without a debugger attached Windows drops it
    if (on && s) fputs(s, stderr);
}

// =====================================================================================================================
// critical sections (recursive; the pthread mutex hangs off DebugInfo, which the 64-bit struct has room for)
// =====================================================================================================================
void WINAPI InitializeCriticalSection(LPCRITICAL_SECTION cs)
{
    memset(cs, 0, sizeof(*cs));
    pthread_mutex_t* m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(m, &a);
    pthread_mutexattr_destroy(&a);
    cs->DebugInfo = m;
}
void WINAPI EnterCriticalSection(LPCRITICAL_SECTION cs) { pthread_mutex_lock((pthread_mutex_t*)cs->DebugInfo); }
void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION cs) { pthread_mutex_unlock((pthread_mutex_t*)cs->DebugInfo); }
void WINAPI DeleteCriticalSection(LPCRITICAL_SECTION cs)
{
    if (cs->DebugInfo) {
        pthread_mutex_destroy((pthread_mutex_t*)cs->DebugInfo);
        free(cs->DebugInfo);
        cs->DebugInfo = nullptr;
    }
}

// =====================================================================================================================
// events, threads, processes: HANDLEs
// =====================================================================================================================
HANDLE WINAPI CreateEventA(LPSECURITY_ATTRIBUTES, BOOL manual, BOOL initial, LPCSTR)
{
    Event* e = new Event;
    e->magic = OBJ_EVENT;
    e->manual = manual != 0;
    e->signalled = initial != 0;
    kobj_add(e);
    return (HANDLE)e;
}
BOOL WINAPI SetEvent(HANDLE h)
{
    Event* e = as<Event>(h, OBJ_EVENT);
    if (!e) { t_last_error = 6; return FALSE; }       // ERROR_INVALID_HANDLE
    { std::lock_guard<std::mutex> g(g_sync_mu); e->signalled = true; }
    g_sync_cv.notify_all();
    return TRUE;
}
BOOL WINAPI ResetEvent(HANDLE h)
{
    Event* e = as<Event>(h, OBJ_EVENT);
    if (!e) { t_last_error = 6; return FALSE; }
    std::lock_guard<std::mutex> g(g_sync_mu);
    e->signalled = false;
    return TRUE;
}
}   // extern "C"

namespace {
bool poll_process(Process* p)
{
    if (p->done) return true;
    int st = 0;
    pid_t r = waitpid(p->pid, &st, WNOHANG);
    if (r == p->pid) {
        p->done = true;
        p->exit_code = WIFEXITED(st) ? (DWORD)WEXITSTATUS(st) : 0xC0000005u;
    } else if (r < 0 && errno == ECHILD) {
        p->done = true;
    }
    return p->done;
}
// true if the object is signalled now (and consumes an auto-reset event). Caller holds g_sync_mu.
bool is_signalled(HANDLE h, bool consume, bool* valid, bool* poll)
{
    *valid = true;
    *poll = false;
    if (Event* e = as<Event>(h, OBJ_EVENT)) {
        if (!e->signalled) return false;
        if (consume && !e->manual) e->signalled = false;
        return true;
    }
    if (Thread* t = as<Thread>(h, OBJ_THREAD)) return t->done;
    if (Process* p = as<Process>(h, OBJ_PROCESS)) {
        if (p->pseudo) return true;
        *poll = true;
        return poll_process(p);
    }
    *valid = false;
    return false;
}
DWORD wait_multi(DWORD n, const HANDLE* hs, BOOL all, DWORD ms)
{
    if (n == 0 || n > 64) { t_last_error = 87; return 0xFFFFFFFFu; }
    unsigned long long deadline = ms == INFINITE ? ~0ull : now_ns() + (unsigned long long)ms * 1000000ull;
    std::unique_lock<std::mutex> lk(g_sync_mu);
    for (;;) {
        bool need_poll = false;
        if (all) {
            bool every = true;
            for (DWORD i = 0; i < n; ++i) {
                bool valid, poll;
                bool s = is_signalled(hs[i], false, &valid, &poll);
                if (!valid) { t_last_error = 6; return 0xFFFFFFFFu; }
                need_poll |= poll;
                every &= s;
            }
            if (every) {
                for (DWORD i = 0; i < n; ++i) { bool v, p; is_signalled(hs[i], true, &v, &p); }
                return WAIT_OBJECT_0;
            }
        } else {
            for (DWORD i = 0; i < n; ++i) {
                bool valid, poll;
                if (is_signalled(hs[i], true, &valid, &poll)) return WAIT_OBJECT_0 + i;
                if (!valid) { t_last_error = 6; return 0xFFFFFFFFu; }
                need_poll |= poll;
            }
        }
        unsigned long long now = now_ns();
        if (ms != INFINITE && now >= deadline) return WAIT_TIMEOUT;
        if (ms == 0) return WAIT_TIMEOUT;
        unsigned long long slice = need_poll ? 5000000ull : ~0ull;     // child processes have no condition to wait on
        if (ms != INFINITE && deadline - now < slice) slice = deadline - now;
        if (slice == ~0ull) g_sync_cv.wait(lk);
        else g_sync_cv.wait_for(lk, std::chrono::nanoseconds(slice));
    }
}
void thread_done(Thread* t)
{
    bool free_it;
    {
        std::lock_guard<std::mutex> g(g_sync_mu);
        t->done = true;
        free_it = t->closed;
    }
    g_sync_cv.notify_all();
    if (free_it) { kobj_remove(t); delete t; }
}
void* thread_entry(void* p)
{
    Thread* t = (Thread*)p;
    t->exit_code = t->fn(t->arg);
    thread_done(t);
    return nullptr;
}
}   // namespace

extern "C" {
DWORD WINAPI WaitForSingleObject(HANDLE h, DWORD ms) { return wait_multi(1, &h, FALSE, ms); }
DWORD WINAPI WaitForMultipleObjects(DWORD n, const HANDLE* hs, BOOL all, DWORD ms) { return wait_multi(n, hs, all, ms); }

HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES, SIZE_T stack, LPTHREAD_START_ROUTINE fn, LPVOID arg, DWORD flags, LPDWORD tid)
{
    Thread* t = new Thread;
    t->magic = OBJ_THREAD; t->fn = fn; t->arg = arg; t->done = false; t->closed = false; t->exit_code = 0;
    kobj_add(t);
    pthread_attr_t a;
    pthread_attr_init(&a);
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
    if (stack) pthread_attr_setstacksize(&a, stack < 65536 ? 65536 : stack);
    int rc = pthread_create(&t->th, &a, thread_entry, t);
    pthread_attr_destroy(&a);
    if (rc != 0) { kobj_remove(t); delete t; t_last_error = 8; return nullptr; }       // ERROR_NOT_ENOUGH_MEMORY
    if (tid) *tid = (DWORD)(uintptr_t)t->th;
    return (HANDLE)t;
}
BOOL WINAPI CloseHandle(HANDLE h)
{
    if (!h || h == INVALID_HANDLE_VALUE || !kobj_live(h)) { t_last_error = 6; return FALSE; }
    uint32_t m = ((KObj*)h)->magic;
    if (m == OBJ_EVENT) { kobj_remove(h); delete (Event*)h; return TRUE; }
    if (m == OBJ_PROCESS) { kobj_remove(h); delete (Process*)h; return TRUE; }
    if (m == OBJ_THREAD) {
        Thread* t = (Thread*)h;
        bool free_it;
        { std::lock_guard<std::mutex> g(g_sync_mu); t->closed = true; free_it = t->done; }
        if (free_it) { kobj_remove(t); delete t; }
        return TRUE;
    }
    t_last_error = 6;
    return FALSE;
}
HANDLE WINAPI GetCurrentProcess() { return (HANDLE)(LONG_PTR)-1; }
HANDLE WINAPI GetCurrentThread() { return (HANDLE)(LONG_PTR)-2; }
DWORD WINAPI GetCurrentThreadId()
{
#if defined(__APPLE__)
    uint64_t id = 0; pthread_threadid_np(nullptr, &id); return (DWORD)id;
#else
    return (DWORD)gettid();
#endif
}
static unsigned cpu_count_()
{
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n < 1 ? 1u : (unsigned)n;
}
BOOL WINAPI GetProcessAffinityMask(HANDLE, DWORD_PTR* proc, DWORD_PTR* sys)
{
    unsigned n = cpu_count_();
    DWORD_PTR mask = n >= 64 ? ~(DWORD_PTR)0 : (((DWORD_PTR)1 << n) - 1);
    if (proc) *proc = mask;
    if (sys) *sys = mask;
    return TRUE;
}
DWORD_PTR WINAPI SetThreadAffinityMask(HANDLE, DWORD_PTR) { DWORD_PTR m = 0, s = 0; GetProcessAffinityMask(0, &m, &s); return m; }
BOOL WINAPI SetThreadPriority(HANDLE, int) { return TRUE; }      // priorities are scheduling hints; the OS default is kept

// ---- processes ------------------------------------------------------------------------------------------------------
static std::vector<std::string> split_command(const char* cmd)
{
    std::vector<std::string> out;
    std::string cur;
    bool inq = false, have = false;
    for (const char* p = cmd; *p; ++p) {
        if (*p == '"') { inq = !inq; have = true; }
        else if ((*p == ' ' || *p == '\t') && !inq) { if (have) out.push_back(cur); cur.clear(); have = false; }
        else { cur += *p; have = true; }
    }
    if (have) out.push_back(cur);
    return out;
}
BOOL WINAPI CreateProcessA(LPCSTR app, LPSTR cmdline, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID,
                           LPCSTR cwd, LPSTARTUPINFOA, LPPROCESS_INFORMATION pi)
{
    std::vector<std::string> argv = split_command(cmdline ? cmdline : "");
    std::string exe = app ? app : (argv.empty() ? "" : argv[0]);
    if (exe.empty()) { t_last_error = 87; return FALSE; }
    if (argv.empty()) argv.push_back(exe);            // the command line is the whole argv (its first word is argv[0]), as Windows passes it
    Process* p = new Process;
    kobj_add(p);
    p->magic = OBJ_PROCESS; p->pid = 0; p->done = false; p->exit_code = 0; p->pseudo = false;
    char host[4096];
    int exists = mbp_resolve_path(exe.c_str(), host, sizeof host);
    std::string low = exe;
    for (char& c : low) c = (char)tolower((unsigned char)c);
    bool windows_exe = low.size() > 4 && low.compare(low.size() - 4, 4, ".exe") == 0;
    if (windows_exe || !exists || access(host, X_OK) != 0) {
        // A Windows executable cannot run here (the game's only use is the binkplay.exe intro videos): the process "starts"
        // and exits at once with code 0, so the caller proceeds exactly as it does after a video that has ended.
        fprintf(stderr, "mb: CreateProcess(%s): not runnable on this platform, completing immediately\n", exe.c_str());
        p->pseudo = true; p->done = true;
    } else {
        std::vector<char*> av;
        for (auto& s : argv) av.push_back((char*)s.c_str());
        av.push_back(nullptr);
        char cwdhost[4096];
        if (cwd) mbp_resolve_path(cwd, cwdhost, sizeof cwdhost);
        pid_t pid = fork();
        if (pid < 0) { kobj_remove(p); delete p; t_last_error = 8; return FALSE; }
        if (pid == 0) {
            if (cwd && chdir(cwdhost) != 0) _exit(127);
            execv(host, av.data());
            _exit(127);
        }
        p->pid = pid;
    }
    if (pi) {
        memset(pi, 0, sizeof *pi);
        pi->hProcess = (HANDLE)p;
        pi->dwProcessId = (DWORD)p->pid;
    }
    return TRUE;
}
BOOL WINAPI GetExitCodeProcess(HANDLE h, LPDWORD code)
{
    Process* p = as<Process>(h, OBJ_PROCESS);
    if (!p) { t_last_error = 6; return FALSE; }
    std::lock_guard<std::mutex> g(g_sync_mu);
    bool d = p->pseudo || poll_process(p);
    if (code) *code = d ? p->exit_code : 259;       // STILL_ACTIVE
    return TRUE;
}
BOOL WINAPI TerminateProcess(HANDLE h, UINT code)
{
    if (h == GetCurrentProcess()) _exit((int)code);
    Process* p = as<Process>(h, OBJ_PROCESS);
    if (!p) { t_last_error = 6; return FALSE; }
    if (!p->pseudo && !p->done) kill(p->pid, SIGKILL);
    return TRUE;
}
void WINAPI ExitProcess(UINT code) { exit((int)code); }

// ---- modules ------------------------------------------------------------------------------------------------------
static HINSTANCE__ g_module;
HMODULE WINAPI GetModuleHandleA(LPCSTR name) { (void)name; return &g_module; }
DWORD WINAPI GetModuleFileNameA(HMODULE, LPSTR buf, DWORD size)
{
    char path[4096] = {0};
#if defined(__APPLE__)
    uint32_t n = sizeof path;
    if (_NSGetExecutablePath(path, &n) != 0) path[0] = 0;
#else
    ssize_t r = readlink("/proc/self/exe", path, sizeof path - 1);
    if (r > 0) path[r] = 0;
#endif
    size_t l = strlen(path);
    if (size == 0) return 0;
    if (l >= size) l = size - 1;
    memcpy(buf, path, l);
    buf[l] = 0;
    return (DWORD)l;
}
HMODULE WINAPI LoadLibraryA(LPCSTR name)
{
    // The Windows system DLLs the game loads (riched20 for the options dialog's rich edit, d3dx9, dinput8 ...) are provided
    // natively by this library, so the load succeeds with the process module; any other name is a real dlopen attempt.
    std::string n = name ? name : "";
    std::string low = n;
    for (char& c : low) c = (char)tolower((unsigned char)c);
    if (low.size() > 4 && low.compare(low.size() - 4, 4, ".dll") == 0) {
        static const char* provided[] = {"riched20", "riched32", "d3dx9", "d3d9", "dinput8", "kernel32", "user32", "gdi32", "ws2_32",
                                         "shell32", "winmm", "ole32", "oleaut32", "advapi32", "comctl32", "fmod", "newton", "skinmagic"};
        for (const char* p : provided)
            if (low.compare(0, strlen(p), p) == 0) return &g_module;
        t_last_error = 126;
        return nullptr;
    }
    void* h = dlopen(n.c_str(), RTLD_NOW);
    if (!h) { t_last_error = 126; return nullptr; }
    return (HMODULE)h;
}
BOOL WINAPI FreeLibrary(HMODULE h)
{
    if (h && h != &g_module) dlclose((void*)h);
    return TRUE;
}
void* WINAPI GetProcAddress(HMODULE h, LPCSTR name)
{
    void* p = dlsym(h && h != &g_module ? (void*)h : RTLD_DEFAULT, name);
    if (!p) t_last_error = 127;
    return p;
}

// ---- system info --------------------------------------------------------------------------------------------------
void WINAPI GlobalMemoryStatus(LPMEMORYSTATUS m)
{
    unsigned long long total = 0, avail = 0;
#if defined(__APPLE__)
    int64_t mem = 0; size_t sz = sizeof mem;
    if (sysctlbyname("hw.memsize", &mem, &sz, nullptr, 0) == 0) total = (unsigned long long)mem;
    avail = total / 2;
    int64_t pages = 0; sz = sizeof pages;
    if (sysctlbyname("vm.page_free_count", &pages, &sz, nullptr, 0) == 0 && pages > 0) avail = (unsigned long long)pages * (unsigned long long)getpagesize();
#else
    long pg = sysconf(_SC_PAGESIZE);
    total = (unsigned long long)sysconf(_SC_PHYS_PAGES) * (unsigned long long)pg;
    avail = (unsigned long long)sysconf(_SC_AVPHYS_PAGES) * (unsigned long long)pg;
#endif
    auto cl = [](unsigned long long v) -> SIZE_T { return (SIZE_T)(v > 0xFFFFFFFFull ? 0xFFFFFFFFull : v); };   // MEMORYSTATUS is the 32-bit report
    m->dwLength = sizeof *m;
    m->dwTotalPhys = cl(total); m->dwAvailPhys = cl(avail);
    m->dwTotalPageFile = cl(total * 2); m->dwAvailPageFile = cl(avail * 2);
    m->dwTotalVirtual = 0x7FFE0000; m->dwAvailVirtual = 0x7FFE0000 - 0x10000000;
    m->dwMemoryLoad = total ? (DWORD)(100 - (avail * 100) / total) : 0;
}
BOOL WINAPI GetVersionExA(LPOSVERSIONINFOA v)
{
    if (!v || v->dwOSVersionInfoSize < sizeof(OSVERSIONINFOA)) { t_last_error = 87; return FALSE; }
    v->dwMajorVersion = 10; v->dwMinorVersion = 0; v->dwBuildNumber = 19045; v->dwPlatformId = 2;   // what a current Windows reports
    v->szCSDVersion[0] = 0;
    return TRUE;
}

// ---- global / virtual memory ------------------------------------------------------------------------------------------
HGLOBAL WINAPI GlobalAlloc(UINT flags, SIZE_T bytes)
{
    void* p = (flags & GMEM_ZEROINIT) ? calloc(1, bytes ? bytes : 1) : malloc(bytes ? bytes : 1);
    if (!p) t_last_error = 8;
    return (HGLOBAL)p;           // handle == pointer: GlobalLock returns it, GMEM_MOVEABLE never moves
}
LPVOID WINAPI GlobalLock(HGLOBAL h) { return (LPVOID)h; }
BOOL WINAPI GlobalUnlock(HGLOBAL) { return FALSE; }         // lock count reaches 0 (NO_ERROR in GetLastError)
HGLOBAL WINAPI GlobalFree(HGLOBAL h) { free((void*)h); return nullptr; }

static std::mutex g_va_mu;
static std::map<void*, size_t> g_va;
LPVOID WINAPI VirtualAlloc(LPVOID addr, SIZE_T size, DWORD type, DWORD protect)
{
    (void)type;
    int prot = (protect == 0x01) ? PROT_NONE : (protect & 0xF0) ? (PROT_READ | PROT_WRITE | PROT_EXEC) : (PROT_READ | PROT_WRITE);
    void* p = mmap(addr, size, prot, MAP_PRIVATE | MAP_ANON | (addr ? MAP_FIXED : 0), -1, 0);
    if (p == MAP_FAILED) { t_last_error = 8; return nullptr; }
    std::lock_guard<std::mutex> g(g_va_mu);
    g_va[p] = size;
    return p;
}
BOOL WINAPI VirtualFree(LPVOID addr, SIZE_T size, DWORD type)
{
    std::lock_guard<std::mutex> g(g_va_mu);
    auto it = g_va.find(addr);
    if (type & 0x8000) {                         // MEM_RELEASE
        if (it == g_va.end()) { t_last_error = 87; return FALSE; }
        munmap(addr, it->second);
        g_va.erase(it);
        return TRUE;
    }
    if (type & 0x4000) {                         // MEM_DECOMMIT
        madvise(addr, size, MADV_DONTNEED);
        return TRUE;
    }
    t_last_error = 87;
    return FALSE;
}

// ---- code pages -------------------------------------------------------------------------------------------------------
int WINAPI MultiByteToWideChar(UINT cp, DWORD, LPCSTR s, int cb, LPWSTR w, int cch)
{
    if (!s) return 0;
    if (cb < 0) cb = (int)strlen(s) + 1;
    std::wstring out;
    if (cp == 65001) {
        for (int i = 0; i < cb;) {
            unsigned char c = (unsigned char)s[i];
            unsigned cpnt; int n;
            if (c < 0x80) { cpnt = c; n = 1; }
            else if ((c >> 5) == 6 && i + 1 < cb) { cpnt = ((c & 0x1Fu) << 6) | (s[i + 1] & 0x3Fu); n = 2; }
            else if ((c >> 4) == 14 && i + 2 < cb) { cpnt = ((c & 0xFu) << 12) | ((s[i + 1] & 0x3Fu) << 6) | (s[i + 2] & 0x3Fu); n = 3; }
            else if ((c >> 3) == 30 && i + 3 < cb) { cpnt = ((c & 7u) << 18) | ((s[i + 1] & 0x3Fu) << 12) | ((s[i + 2] & 0x3Fu) << 6) | (s[i + 3] & 0x3Fu); n = 4; }
            else { cpnt = 0xFFFD; n = 1; }
            out += (wchar_t)cpnt;
            i += n;
        }
    } else {
        for (int i = 0; i < cb; ++i) out += (wchar_t)ansi_char_to_unicode((unsigned char)s[i]);
    }
    if (cch == 0) return (int)out.size();
    if ((int)out.size() > cch) { t_last_error = 122; return 0; }
    memcpy(w, out.data(), out.size() * sizeof(wchar_t));
    return (int)out.size();
}
int WINAPI WideCharToMultiByte(UINT cp, DWORD, LPCWSTR w, int cch, LPSTR s, int cb, LPCSTR def, LPBOOL used)
{
    if (!w) return 0;
    if (cch < 0) { cch = 0; while (w[cch]) ++cch; ++cch; }
    std::string out;
    bool lossy = false;
    for (int i = 0; i < cch; ++i) {
        unsigned c = (unsigned)w[i];
        if (cp == 65001) {
            if (c < 0x80) out += (char)c;
            else if (c < 0x800) { out += (char)(0xC0 | (c >> 6)); out += (char)(0x80 | (c & 0x3F)); }
            else if (c < 0x10000) { out += (char)(0xE0 | (c >> 12)); out += (char)(0x80 | ((c >> 6) & 0x3F)); out += (char)(0x80 | (c & 0x3F)); }
            else { out += (char)(0xF0 | (c >> 18)); out += (char)(0x80 | ((c >> 12) & 0x3F)); out += (char)(0x80 | ((c >> 6) & 0x3F)); out += (char)(0x80 | (c & 0x3F)); }
        } else {
            int found = -1;
            if (c < 0x80) found = (int)c;
            else for (unsigned b = 0x80; b < 0x100; ++b) if (ansi_char_to_unicode((unsigned char)b) == c) { found = (int)b; break; }
            if (found < 0) { out += def ? def[0] : '?'; lossy = true; }
            else out += (char)found;
        }
    }
    if (used) *used = lossy;
    if (cb == 0) return (int)out.size();
    if ((int)out.size() > cb) { t_last_error = 122; return 0; }
    memcpy(s, out.data(), out.size());
    return (int)out.size();
}

// ---- strings ----------------------------------------------------------------------------------------------------------
LPSTR WINAPI lstrcpyA(LPSTR a, LPCSTR b) { return strcpy(a, b); }
LPSTR WINAPI lstrcpynA(LPSTR a, LPCSTR b, int n)
{
    if (n <= 0) return a;
    size_t l = strnlen(b, (size_t)n - 1);
    memcpy(a, b, l);
    a[l] = 0;
    return a;
}
LPSTR WINAPI lstrcatA(LPSTR a, LPCSTR b) { return strcat(a, b); }
int WINAPI lstrlenA(LPCSTR s) { return s ? (int)strlen(s) : 0; }
int WINAPI lstrcmpA(LPCSTR a, LPCSTR b) { return strcmp(a, b); }
int WINAPI lstrcmpiA(LPCSTR a, LPCSTR b) { return strcasecmp(a, b); }
int WINAPI MulDiv(int n, int num, int den)
{
    if (den == 0) return -1;
    long long v = (long long)n * num;
    long long h = (den < 0 ? -(long long)den : den) / 2;
    v += ((v < 0) != (den < 0)) ? -h : h;
    long long r = v / den;
    if (r > 0x7FFFFFFFll || r < -0x7FFFFFFFll - 1) return -1;
    return (int)r;
}
// wsprintf: printf subset (c d i u x X s p %), the 'l' / 'h' size prefixes meaning the Win32 32-bit types, 1024-byte limit.
int WINAPIV wsprintfA(LPSTR out, LPCSTR fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    std::string res;
    for (const char* p = fmt; *p; ++p) {
        if (*p != '%') { res += *p; continue; }
        std::string spec = "%";
        ++p;
        while (*p && strchr("-+ #0", *p)) spec += *p++;
        if (*p == '*') { char b[16]; snprintf(b, sizeof b, "%d", va_arg(ap, int)); spec += b; ++p; }
        else while (*p && isdigit((unsigned char)*p)) spec += *p++;
        if (*p == '.') {
            spec += *p++;
            if (*p == '*') { char b[16]; snprintf(b, sizeof b, "%d", va_arg(ap, int)); spec += b; ++p; }
            else while (*p && isdigit((unsigned char)*p)) spec += *p++;
        }
        while (*p == 'l' || *p == 'h' || *p == 'I') ++p;
        char buf[1100];
        switch (*p) {
        case 'd': case 'i': spec += 'd'; snprintf(buf, sizeof buf, spec.c_str(), va_arg(ap, int)); res += buf; break;
        case 'u': case 'x': case 'X': spec += *p; snprintf(buf, sizeof buf, spec.c_str(), va_arg(ap, unsigned)); res += buf; break;
        case 'c': spec += 'c'; snprintf(buf, sizeof buf, spec.c_str(), va_arg(ap, int)); res += buf; break;
        case 's': { spec += 's'; const char* s = va_arg(ap, const char*); snprintf(buf, sizeof buf, spec.c_str(), s ? s : "(null)"); res += buf; break; }
        case 'p': spec += 'p'; snprintf(buf, sizeof buf, spec.c_str(), va_arg(ap, void*)); res += buf; break;
        case '%': res += '%'; break;
        case 0: --p; break;
        default: res += spec; res += *p; break;
        }
    }
    va_end(ap);
    if (res.size() > 1023) res.resize(1023);
    memcpy(out, res.c_str(), res.size() + 1);
    return (int)res.size();
}
}   // extern "C"

// ---- Windows-1252 <-> Unicode, UTF-8 helpers --------------------------------------------------------------------------
namespace mbp {
static const unsigned short k1252[32] = {0x20AC, 0x81, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x8D, 0x017D, 0x8F,
                                         0x90, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x9D, 0x017E, 0x0178};
unsigned ansi_char_to_unicode(unsigned char c)
{
    if (c >= 0x80 && c < 0xA0) return k1252[c - 0x80];
    return c;
}
static void put_utf8(std::string& o, unsigned c)
{
    if (c < 0x80) o += (char)c;
    else if (c < 0x800) { o += (char)(0xC0 | (c >> 6)); o += (char)(0x80 | (c & 0x3F)); }
    else if (c < 0x10000) { o += (char)(0xE0 | (c >> 12)); o += (char)(0x80 | ((c >> 6) & 0x3F)); o += (char)(0x80 | (c & 0x3F)); }
    else { o += (char)(0xF0 | (c >> 18)); o += (char)(0x80 | ((c >> 12) & 0x3F)); o += (char)(0x80 | ((c >> 6) & 0x3F)); o += (char)(0x80 | (c & 0x3F)); }
}
std::string ansi_to_utf8(const char* s)
{
    std::string o;
    for (; s && *s; ++s) put_utf8(o, ansi_char_to_unicode((unsigned char)*s));
    return o;
}
std::string wide_to_utf8(const wchar_t* w)
{
    std::string o;
    for (; w && *w; ++w) put_utf8(o, (unsigned)*w);
    return o;
}
std::string utf8_to_ansi(const char* s)
{
    std::string o;
    while (s && *s) {
        unsigned char c = (unsigned char)*s;
        unsigned cp; int n;
        if (c < 0x80) { cp = c; n = 1; }
        else if ((c >> 5) == 6 && s[1]) { cp = ((c & 0x1Fu) << 6) | (s[1] & 0x3Fu); n = 2; }
        else if ((c >> 4) == 14 && s[1] && s[2]) { cp = ((c & 0xFu) << 12) | ((s[1] & 0x3Fu) << 6) | (s[2] & 0x3Fu); n = 3; }
        else if ((c >> 3) == 30 && s[1] && s[2] && s[3]) { cp = ((c & 7u) << 18) | ((s[1] & 0x3Fu) << 12) | ((s[2] & 0x3Fu) << 6) | (s[3] & 0x3Fu); n = 4; }
        else { cp = '?'; n = 1; }
        char out = '?';
        if (cp < 0x80 || (cp >= 0xA0 && cp < 0x100)) out = (char)cp;
        else for (unsigned b = 0x80; b < 0xA0; ++b) if (k1252[b - 0x80] == cp) { out = (char)b; break; }
        o += out;
        s += n;
    }
    return o;
}
}   // namespace mbp
