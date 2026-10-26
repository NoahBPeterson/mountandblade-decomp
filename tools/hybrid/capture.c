/* capture.c: the snapshot runtime that tools/hybrid/capture.py links into a runnable copy of the original exe.

   At entry (before the CRT): resolve the kernel32 functions it needs through the game's own GetProcAddress /
   GetModuleHandleA imports, install a vectored exception handler, plant int3 at every target function entry
   (cap_cfg.bps, sorted), then jump to the original entry point.

   On a target's int3: if the function has hits left (cap_cfg.max_hits), write a snapshot file
   <dir>\cap_<seq>_<va>.bin: the thread's CONTEXT (registers, x87, SSE) and every writable page of the process
   that changed since the previous snapshot (the exe's own .data and private allocations; other modules' images
   and mapped files are left out). A page's hash is kept between snapshots, so snapshot k holds only the pages
   that differ from snapshot k-1: tools/difftest/snapreplay.py rebuilds the memory of snapshot k by applying
   snapshots 0..k in order. A page that became all zero is recorded without data. Then the original byte is put
   back; while hits remain, a single step re-plants the int3 after the first instruction ran.

   No CRT (compiled /Zl /GS-), no globals initialised by code before cap_entry runs. */

typedef unsigned long DWORD;
typedef unsigned char BYTE;
typedef void* HANDLE;
typedef int BOOL;

typedef struct {
    void* BaseAddress; void* AllocationBase; DWORD AllocationProtect; DWORD RegionSize; DWORD State; DWORD Protect;
    DWORD Type;
} MBI;

typedef struct { DWORD ExceptionCode; DWORD ExceptionFlags; void* ExceptionRecord; void* ExceptionAddress; } EXREC;
typedef struct { EXREC* ExceptionRecord; BYTE* ContextRecord; } EXPTRS;
#define CTX_SIZE 0x2cc
#define CTX_EIP 0xb8
#define CTX_EFLAGS 0xc0

typedef void* (__stdcall* GetProcAddress_t)(void*, const char*);
typedef void* (__stdcall* GetModuleHandleA_t)(const char*);
typedef void* (__stdcall* AddVEH_t)(DWORD, long (__stdcall*)(EXPTRS*));
typedef DWORD (__stdcall* VirtualQuery_t)(const void*, MBI*, DWORD);
typedef void* (__stdcall* VirtualAlloc_t)(void*, DWORD, DWORD, DWORD);
typedef HANDLE (__stdcall* CreateFileA_t)(const char*, DWORD, DWORD, void*, DWORD, DWORD, HANDLE);
typedef BOOL (__stdcall* WriteFile_t)(HANDLE, const void*, DWORD, DWORD*, void*);
typedef BOOL (__stdcall* CloseHandle_t)(HANDLE);
typedef void (__stdcall* EnterCS_t)(void*);
typedef void (__stdcall* InitCS_t)(void*);
typedef void (__stdcall* FlushIC_t)(HANDLE, const void*, DWORD);
typedef HANDLE (__stdcall* GetCurrentProcess_t)(void);

/* cl 13.10 emits stack-cookie checks here (it has no /GS-); the runtime has no CRT, so it supplies them */
DWORD __security_cookie = 0xbb40e64e;
void __fastcall __security_check_cookie(DWORD cookie) { (void)cookie; }

/* filled by capture.py */
struct cap_cfg {
    DWORD magic;            /* 'MBCF' */
    DWORD oep;
    DWORD iat_gpa;          /* address of the IAT slot of kernel32!GetProcAddress */
    DWORD iat_gmh;          /* ... of kernel32!GetModuleHandleA */
    DWORD n;                /* number of targets */
    DWORD bps;              /* -> DWORD[n], sorted entry addresses */
    DWORD orig;             /* -> BYTE[n], the bytes the int3s replace */
    DWORD hits;             /* -> BYTE[n], snapshots taken per target */
    DWORD max_hits;
    DWORD skip_lo, skip_hi; /* never dumped: the exe's code (made writable for the int3s) */
    char dir[128];          /* where the snapshots go (with a trailing backslash), "" = cwd */
    DWORD arm;              /* 0, or: only this address gets an int3 at start; its first hit plants the targets */
    DWORD arm_orig;         /* the byte the arm int3 replaces */
};
struct cap_cfg cap_cfg = { 0x4643424d };

static struct {
    GetProcAddress_t gpa;
    VirtualQuery_t vq;
    VirtualAlloc_t va;
    CreateFileA_t cf;
    WriteFile_t wf;
    CloseHandle_t ch;
    EnterCS_t enter, leave;
    FlushIC_t flush;
    GetCurrentProcess_t curproc;
    DWORD* page_hash;       /* one hash per 4 KB page of the 4 GB space (0 = never seen) */
    DWORD seq;
    DWORD rearm;            /* the int3 to re-plant after the single step (0 = none) */
    DWORD bytes;
    BYTE cs[64];            /* CRITICAL_SECTION */
    char buf[0x10000];      /* write buffer */
    DWORD fill;
    HANDLE file;
} g;

static void put(const void* p, DWORD n)
{
    const BYTE* s = (const BYTE*)p;
    DWORD w;
    while (n) {
        DWORD k = sizeof g.buf - g.fill;
        DWORD i;
        if (k > n) k = n;
        for (i = 0; i < k; i++) g.buf[g.fill + i] = s[i];
        g.fill += k; s += k; n -= k;
        if (g.fill == sizeof g.buf) {
            g.wf(g.file, g.buf, g.fill, &w, 0);
            g.bytes += g.fill;
            g.fill = 0;
        }
    }
}

static void flush(void)
{
    DWORD w;
    if (g.fill) {
        g.wf(g.file, g.buf, g.fill, &w, 0);
        g.bytes += g.fill;
    }
    g.fill = 0;
}

static char* hex8(char* o, DWORD v)
{
    int i;
    for (i = 7; i >= 0; i--) {
        DWORD d = (v >> (4 * i)) & 15;
        *o++ = (char)(d < 10 ? '0' + d : 'a' + d - 10);
    }
    return o;
}

static DWORD page_hash(const DWORD* p, int* zero)
{
    DWORD h = 2166136261u, i, nz = 0;
    for (i = 0; i < 1024; i++) {
        h = (h ^ p[i]) * 16777619u;
        nz |= p[i];
    }
    *zero = nz == 0;
    return h | 1;           /* never 0: 0 marks a page not seen yet */
}

static int writable(DWORD prot)
{
    if (prot & 0x100) return 0;                     /* PAGE_GUARD */
    prot &= 0xff;
    return prot == 0x04 || prot == 0x08 || prot == 0x40 || prot == 0x80;
}

static void snapshot(DWORD va, BYTE* ctx)
{
    char name[256];
    char* o = name;
    const char* d = cap_cfg.dir;
    DWORD a = 0x10000, hdr[4];
    MBI mbi;
    while (*d) *o++ = *d++;
    *o++ = 'c'; *o++ = 'a'; *o++ = 'p'; *o++ = '_';
    o = hex8(o, g.seq);
    *o++ = '_';
    o = hex8(o, va);
    *o++ = '.'; *o++ = 'b'; *o++ = 'i'; *o++ = 'n'; *o = 0;
    g.file = g.cf(name, 0x40000000 /* GENERIC_WRITE */, 0, 0, 2 /* CREATE_ALWAYS */, 0x80, 0);
    if (g.file == (HANDLE)-1)
        return;
    hdr[0] = 0x50434d4d;    /* 'MMCP' */
    hdr[1] = g.seq;
    hdr[2] = va;
    hdr[3] = CTX_SIZE;
    put(hdr, sizeof hdr);
    put(ctx, CTX_SIZE);
    /* the pages: [addr | 1 if all zero] then 4096 bytes unless zero; terminated by 0xffffffff */
    while (a < 0xfffe0000 && g.vq((void*)a, &mbi, sizeof mbi)) {
        DWORD base = (DWORD)mbi.BaseAddress, end = base + mbi.RegionSize, p;
        if (end <= a)
            break;
        if (mbi.State == 0x1000 /* MEM_COMMIT */ && writable(mbi.Protect) &&
                (mbi.Type == 0x20000 /* MEM_PRIVATE */ ||
                 (mbi.Type == 0x1000000 /* MEM_IMAGE */ && (DWORD)mbi.AllocationBase == 0x400000))) {
            for (p = base; p < end; p += 0x1000) {
                int zero;
                DWORD h, rec;
                if (p >= cap_cfg.skip_lo && p < cap_cfg.skip_hi)
                    continue;
                h = page_hash((const DWORD*)p, &zero);
                if (g.page_hash[p >> 12] == h)
                    continue;
                if (g.page_hash[p >> 12] == 0 && zero) {
                    g.page_hash[p >> 12] = h;   /* first sight and zero: the replay starts from zero */
                    continue;
                }
                g.page_hash[p >> 12] = h;
                rec = p | (zero ? 1 : 0);
                put(&rec, 4);
                if (!zero) {                        /* the page itself straight from memory (no copy) */
                    DWORD w;
                    flush();
                    g.wf(g.file, (const void*)p, 0x1000, &w, 0);
                    g.bytes += 0x1000;
                }
            }
        }
        a = end;
    }
    hdr[0] = 0xffffffff;
    put(hdr, 4);
    flush();
    g.ch(g.file);
    g.seq++;
}

static int find(DWORD addr)
{
    const DWORD* bps = (const DWORD*)cap_cfg.bps;
    int lo = 0, hi = (int)cap_cfg.n - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (bps[mid] == addr) return mid;
        if (bps[mid] < addr) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}

static void plant_all(void)
{
    DWORD i;
    for (i = 0; i < cap_cfg.n; i++)
        *(BYTE*)((DWORD*)cap_cfg.bps)[i] = 0xcc;
    g.flush(g.curproc(), 0, 0);
}

static long __stdcall handler(EXPTRS* ep)
{
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    BYTE* ctx = ep->ContextRecord;
    if (code == 0x80000003) {                       /* EXCEPTION_BREAKPOINT */
        DWORD addr = (DWORD)ep->ExceptionRecord->ExceptionAddress;
        int i;
        BYTE* hits = (BYTE*)cap_cfg.hits;
        if (cap_cfg.arm && addr == cap_cfg.arm) {   /* the trigger: from now on every target snapshots */
            *(BYTE*)addr = (BYTE)cap_cfg.arm_orig;
            cap_cfg.arm = 0;
            *(DWORD*)(ctx + CTX_EIP) = addr;
            plant_all();
            if (find(addr) < 0)
                return -1;
            *(BYTE*)addr = 0xcc;                    /* the trigger is a target too: take it now */
        }
        i = find(addr);
        if (i < 0)
            return 0;                               /* EXCEPTION_CONTINUE_SEARCH */
        g.enter(g.cs);
        *(DWORD*)(ctx + CTX_EIP) = addr;
        if (hits[i] < cap_cfg.max_hits) {
            hits[i]++;
            snapshot(addr, ctx);
        }
        *(BYTE*)addr = ((BYTE*)cap_cfg.orig)[i];
        g.flush(g.curproc(), (void*)addr, 1);
        if (hits[i] < cap_cfg.max_hits) {
            g.rearm = addr;
            *(DWORD*)(ctx + CTX_EFLAGS) |= 0x100;    /* trap after the first instruction */
        } else {
            g.leave(g.cs);
        }
        return -1;                                  /* EXCEPTION_CONTINUE_EXECUTION */
    }
    if (code == 0x80000004 && g.rearm) {            /* EXCEPTION_SINGLE_STEP */
        *(BYTE*)g.rearm = 0xcc;
        g.flush(g.curproc(), (void*)g.rearm, 1);
        g.rearm = 0;
        *(DWORD*)(ctx + CTX_EFLAGS) &= ~0x100u;
        g.leave(g.cs);
        return -1;
    }
    return 0;
}

void __cdecl cap_init(void)
{
    GetModuleHandleA_t gmh = *(GetModuleHandleA_t*)cap_cfg.iat_gmh;
    void* k32;
    AddVEH_t addveh;
    InitCS_t initcs;
    DWORD i;
    g.gpa = *(GetProcAddress_t*)cap_cfg.iat_gpa;
    k32 = gmh("kernel32.dll");
    addveh = (AddVEH_t)g.gpa(k32, "AddVectoredExceptionHandler");
    g.vq = (VirtualQuery_t)g.gpa(k32, "VirtualQuery");
    g.va = (VirtualAlloc_t)g.gpa(k32, "VirtualAlloc");
    g.cf = (CreateFileA_t)g.gpa(k32, "CreateFileA");
    g.wf = (WriteFile_t)g.gpa(k32, "WriteFile");
    g.ch = (CloseHandle_t)g.gpa(k32, "CloseHandle");
    g.enter = (EnterCS_t)g.gpa(k32, "EnterCriticalSection");
    g.leave = (EnterCS_t)g.gpa(k32, "LeaveCriticalSection");
    initcs = (InitCS_t)g.gpa(k32, "InitializeCriticalSection");
    g.flush = (FlushIC_t)g.gpa(k32, "FlushInstructionCache");
    g.curproc = (GetCurrentProcess_t)g.gpa(k32, "GetCurrentProcess");
    initcs(g.cs);
    /* 4 GB / 4 KB pages x 4 bytes: reserved and committed lazily by the system's zero pages */
    g.page_hash = (DWORD*)g.va(0, 0x400000, 0x3000 /* MEM_COMMIT | MEM_RESERVE */, 0x04);
    addveh(1, handler);
    if (cap_cfg.arm) {
        *(BYTE*)cap_cfg.arm = 0xcc;
        g.flush(g.curproc(), 0, 0);
    } else {
        plant_all();
    }
    (void)i;
}

__declspec(naked) void cap_entry(void)
{
    __asm {
        call cap_init
        jmp dword ptr [cap_cfg + 4]
    }
}
