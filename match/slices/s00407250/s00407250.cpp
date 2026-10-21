// Slice s00407250: D3DFMT name lookup, high-res timer, dynamic array.
#include <string.h>
#include <stdlib.h>
// windows.h does not compile here (sal.h missing); declare the few Win32 calls used.
typedef unsigned long DWORD;
typedef union _LARGE_INTEGER { struct { DWORD LowPart; long HighPart; }; __int64 QuadPart; } LARGE_INTEGER;
extern "C" {
__declspec(dllimport) int __stdcall QueryPerformanceFrequency(LARGE_INTEGER*);   // 0x54917c
__declspec(dllimport) int __stdcall QueryPerformanceCounter(LARGE_INTEGER*);     // 0x549178
__declspec(dllimport) int __stdcall lstrlenA(const char*);                       // 0x549160
__declspec(dllimport) DWORD __stdcall timeGetTime(void);                         // 0x5493e8
}
#include <mbstring.h>

// @ 0x407250
const unsigned char* d3dfmt_name(int fmt, char keep_prefix)
{
    const char* s;
    switch (fmt) {
    case 0: s = "D3DFMT_UNKNOWN"; break;
    case 0x14: s = "D3DFMT_R8G8B8"; break;
    case 0x15: s = "D3DFMT_A8R8G8B8"; break;
    case 0x16: s = "D3DFMT_X8R8G8B8"; break;
    case 0x17: s = "D3DFMT_R5G6B5"; break;
    case 0x18: s = "D3DFMT_X1R5G5B5"; break;
    case 0x19: s = "D3DFMT_A1R5G5B5"; break;
    case 0x1a: s = "D3DFMT_A4R4G4B4"; break;
    case 0x1b: s = "D3DFMT_R3G3B2"; break;
    case 0x1c: s = "D3DFMT_A8"; break;
    case 0x1d: s = "D3DFMT_A8R3G3B2"; break;
    case 0x1e: s = "D3DFMT_X4R4G4B4"; break;
    case 0x1f: s = "D3DFMT_A2B10G10R10"; break;
    case 0x20: s = "D3DFMT_A8B8G8R8"; break;
    case 0x21: s = "D3DFMT_X8B8G8R8"; break;
    case 0x22: s = "D3DFMT_G16R16"; break;
    case 0x23: s = "D3DFMT_A2R10G10B10"; break;
    case 0x24: s = "D3DFMT_A16B16G16R16"; break;
    case 0x28: s = "D3DFMT_A8P8"; break;
    case 0x29: s = "D3DFMT_P8"; break;
    case 0x32: s = "D3DFMT_L8"; break;
    case 0x33: s = "D3DFMT_A8L8"; break;
    case 0x34: s = "D3DFMT_A4L4"; break;
    case 0x3c: s = "D3DFMT_V8U8"; break;
    case 0x3d: s = "D3DFMT_L6V5U5"; break;
    case 0x3e: s = "D3DFMT_X8L8V8U8"; break;
    case 0x3f: s = "D3DFMT_Q8W8V8U8"; break;
    case 0x40: s = "D3DFMT_V16U16"; break;
    case 0x43: s = "D3DFMT_A2W10V10U10"; break;
    case 0x46: s = "D3DFMT_D16_LOCKABLE"; break;
    case 0x47: s = "D3DFMT_D32"; break;
    case 0x49: s = "D3DFMT_D15S1"; break;
    case 0x4b: s = "D3DFMT_D24S8"; break;
    case 0x4d: s = "D3DFMT_D24X8"; break;
    case 0x4f: s = "D3DFMT_D24X4S4"; break;
    case 0x50: s = "D3DFMT_D16"; break;
    case 0x51: s = "D3DFMT_L16"; break;
    case 100: s = "D3DFMT_VERTEXDATA"; break;
    case 0x65: s = "D3DFMT_INDEX16"; break;
    case 0x66: s = "D3DFMT_INDEX32"; break;
    case 0x6e: s = "D3DFMT_Q16W16V16U16"; break;
    case 0x6f: s = "D3DFMT_R16F"; break;
    case 0x70: s = "D3DFMT_G16R16F"; break;
    case 0x71: s = "D3DFMT_A16B16G16R16F"; break;
    case 0x72: s = "D3DFMT_R32F"; break;
    case 0x73: s = "D3DFMT_G32R32F"; break;
    case 0x74: s = "D3DFMT_A32B32G32R32F"; break;
    case 0x75: s = "D3DFMT_CxV8U8"; break;
    case 0x31545844: s = "D3DFMT_DXT1"; break;
    case 0x32545844: s = "D3DFMT_DXT2"; break;
    case 0x33545844: s = "D3DFMT_DXT3"; break;
    case 0x34545844: s = "D3DFMT_DXT4"; break;
    case 0x35545844: s = "D3DFMT_DXT5"; break;
    case 0x3154454d: s = "D3DFMT_MULTI2_ARGB8"; break;
    case 0x32595559: s = "D3DFMT_YUY2"; break;
    case 0x59565955: s = "D3DFMT_UYVY"; break;
    default: s = "Unknown format"; break;
    }
    if (keep_prefix == 0 && _mbsstr((const unsigned char*)s, (const unsigned char*)"D3DFMT_") != 0)
        return (const unsigned char*)s + lstrlenA("D3DFMT_");
    return (const unsigned char*)s;
}

// ---- timer ----
int g_qpc_inited;              // 0x5d6f94
int g_qpc_ok;                  // 0x5d6f90
__int64 g_qpc_freq;            // 0x5d6f88
__int64 g_pause;   // 0x5d6f80
__int64 g_last;    // 0x5d6f78
__int64 g_total;   // 0x5d6f70
int g_timer_running;           // 0x5a8100
double g_d_pause;              // 0x5d6f68
double g_d_last;               // 0x5d6f60
double g_d_total;              // 0x5d6f58

// @ 0x407660
double __stdcall timer_op(int op)
{
    if (!g_qpc_inited) {
        LARGE_INTEGER f;
        g_qpc_inited = 1;
        g_qpc_ok = QueryPerformanceFrequency(&f);
        if (g_qpc_ok == 0) goto fallback;
        g_qpc_freq = f.QuadPart;
    }
    if (g_qpc_ok != 0) {
        LARGE_INTEGER cur;
        if (g_pause == 0 || op == 1 || op == 4) {
            QueryPerformanceCounter(&cur);
        } else {
            cur.QuadPart = g_pause;
        }
        __int64 pause = g_pause;
        __int64 now = cur.QuadPart;
        if (op == 6) {
            cur.QuadPart = now - g_last;
            g_last = now;
            return (double)cur.QuadPart / (double)g_qpc_freq;
        }
        if (op == 5) {
            cur.QuadPart = now - g_total;
            return (double)cur.QuadPart / (double)g_qpc_freq;
        }
        if (op == 0) {
            g_pause = 0;
            g_timer_running = 0;
            g_total = now;
            g_last = now;
            return 0.0f;
        }
        if (op == 1) {
            if (g_timer_running != 0) g_total += now - pause;
            g_pause = 0;
            g_timer_running = 0;
            g_last = now;
            return 0.0f;
        }
        if (op == 2) {
            if (g_timer_running == 0) {
                g_timer_running = 1;
                g_last = now;
                g_pause = now;
            }
            return 0.0f;
        }
        if (op == 3) {
            g_pause = g_qpc_freq / 10 + pause;
            return 0.0f;
        }
        if (op == 4) return (double)cur.QuadPart / (double)g_qpc_freq;
        return -1.0f;
    }
fallback:
    double t;
    if (g_d_pause == 0.0 || op == 1 || op == 4) {
        DWORD ms = timeGetTime();
        t = (double)(int)ms;
        if ((int)ms < 0) t += 4294967296.0;
        t = t * 0.001;
    } else {
        t = g_d_pause;
    }
    if (op == 6) {
        double old = g_d_last;
        g_d_last = t;
        return t - old;
    }
    if (op == 5) return t - g_d_total;
    if (op == 0) {
        g_d_total = t;
        g_d_last = t;
        g_timer_running = 0;
        g_d_pause = 0.0;
        return 0.0f;
    }
    if (op == 1) {
        if (g_timer_running != 0) g_d_total = (t - g_d_pause) + g_d_total;
        g_d_pause = 0.0;
        g_timer_running = 0;
        g_d_last = t;
        return 0.0f;
    }
    if (op == 2) {
        if (g_timer_running == 0) {
            g_d_pause = t;
            g_d_last = t;
            g_timer_running = 1;
            return 0.0f;
        }
        return 0.0f;
    }
    if (op == 3) {
        g_d_pause = g_d_pause + 0.10000000149011612;
        return 0.0f;
    }
    if (op == 4) return t;
    return -1.0f;
}

// @ 0x407a30
int safe_strncpy(char* dst, const char* src, int n)
{
    if (dst != 0 && src != 0 && n >= 1) {
        strncpy(dst, src, n);
        dst[n - 1] = 0;
        return 0;
    }
    return 0x80070057;
}

// ---- dynamic array: mode 0 = by-value elements of elem_size, mode != 0 = pointers ----
struct dyn_array {
    int mode;        // 1 => pointer array
    char* data;
    int elem_size;
    unsigned count;
    unsigned cap;
    dyn_array(int m, int sz);          // 0x407a70
    ~dyn_array();                       // 0x407aa0
    int push_back(const void* e);      // 0x407ab0
    void* at(int i);                    // 0x407ba0
    bool contains(const void* e);       // 0x407bc0
};

// @ 0x407a70
dyn_array::dyn_array(int m, int sz)
{
    int s = 4;
    if (m != 1) s = sz;
    mode = m;
    data = 0;
    elem_size = s;
    count = 0;
    cap = 0;
}
// @ 0x407aa0
dyn_array::~dyn_array() { if (data) free(data); }
// @ 0x407ab0
int dyn_array::push_back(const void* e)
{
    if (elem_size == 0) return 0x80004005;
    if (data == 0 || count + 1 > cap) {
        int ncap;
        if (cap == 0) ncap = 16; else ncap = cap * 2;
        char* nd = (char*)operator new(elem_size * ncap);
        if (nd == 0) return 0x8007000e;
        if (data != 0) {
            memcpy(nd, data, count * elem_size);
            free(data);
        }
        data = nd;
        cap = ncap;
    }
    if (mode == 0) {
        memcpy(data + count * elem_size, e, elem_size);
        count = count + 1;
        return 0;
    }
    ((const void**)data)[count] = e;
    count = count + 1;
    return 0;
}
// @ 0x407ba0
void* dyn_array::at(int i)
{
    if (mode == 0) return data + elem_size * i;
    return ((void**)data)[i];
}
// @ 0x407bc0
bool dyn_array::contains(const void* e)
{
    for (unsigned i = 0; i < count; i++) {
        if (mode == 0) {
            if (memcmp(data + elem_size * i, e, elem_size) == 0) return true;
        } else if (((void**)data)[i] == e) return true;
    }
    return false;
}
