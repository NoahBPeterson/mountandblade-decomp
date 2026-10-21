// Slice s00403120: D3D application framework (window proc, enumeration helpers).
// windows.h does not parse under cl 13.10 here: declare what we use.
typedef struct HWND__* HWND; typedef int BOOL; typedef unsigned int UINT; typedef unsigned long DWORD;
typedef long LONG; typedef long LRESULT;
struct POINT { LONG x, y; };
struct RECT { LONG left, top, right, bottom; };
#define WINAPI __stdcall
#define DLLI extern "C" __declspec(dllimport)
DLLI LRESULT WINAPI DefWindowProcA(HWND, UINT, UINT, LONG);
DLLI long WINAPI SendMessageA(HWND, UINT, UINT, long);
DLLI void* WINAPI GetMenu(HWND);
DLLI BOOL WINAPI DestroyMenu(void*);
DLLI BOOL WINAPI DestroyWindow(HWND);
DLLI void WINAPI PostQuitMessage(int);
DLLI LONG WINAPI GetWindowLongA(HWND, int);
DLLI BOOL WINAPI ClipCursor(const RECT*);
DLLI void* WINAPI SetCursor(void*);
DLLI BOOL WINAPI GetCursorPos(POINT*);
DLLI BOOL WINAPI ScreenToClient(HWND, POINT*);

// Direct3D 9 device / object, only the slots used.
struct IDev9 {
    virtual long __stdcall s0(); virtual long __stdcall s1(); virtual long __stdcall s2();
    virtual long __stdcall s3(); virtual long __stdcall s4(); virtual long __stdcall s5();
    virtual long __stdcall s6(); virtual long __stdcall s7(); virtual long __stdcall s8();
    virtual long __stdcall s9(); virtual long __stdcall s10();
    virtual void __stdcall SetCursorPosition(int x, int y, unsigned flags);   // slot 11 (+0x2c)
    virtual int  __stdcall ShowCursor(int show);                                // slot 12 (+0x30)
    virtual long __stdcall s13(); virtual long __stdcall s14(); virtual long __stdcall s15();
    virtual long __stdcall s16();
    virtual long __stdcall Present(const RECT*, const RECT*, HWND, void*);      // slot 17 (+0x44)
};
struct IUnk { virtual long __stdcall a(); virtual long __stdcall b(); virtual unsigned long __stdcall rel(); };

struct App {
    virtual bool v0(int, int);
    virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual void v5(); virtual void v6(); virtual void v7();
    virtual void v8();                              // +0x20
    virtual void v9(); virtual void v10();
    virtual void v11();                             // +0x2c
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(int);                          // +0x40
    char pad0[0xa4 - 4];
    bool windowed;                                  // +0xa4
    bool active;                                    // +0xa5
    bool f_a6;
    bool minimized;                                 // +0xa7
    bool maximized;                                 // +0xa8
    char pad1;
    bool f_aa, f_ab, f_ac, f_ad;
    char pad2[0xe8 - 0xae];
    HWND hwnd;                                      // +0xe8
    char pad3[0xf4 - 0xec];
    IUnk* d3d;                                      // +0xf4
    IDev9* dev;                                     // +0xf8
    char pad4[0x250 - 0xfc];
    LONG saved_style;                               // +0x250
    char pad5[0x344 - 0x254];
    bool f344, f345;

    int  reset_a();                                 // 0x4026e0
    void size_changed();                            // 0x402150
    int  fail(int hr, int flag);                    // 0x4014f0
    int  handle_size_change();                      // 0x402be0
    int  toggle_fullscreen();                       // 0x402d40
    LRESULT wndproc(HWND h, UINT msg, UINT wp, LONG lp);   // 0x403120
};

double __stdcall timer_op(int);                     // 0x407660

// @ 0x403120
LRESULT App::wndproc(HWND h, UINT msg, UINT wp, LONG lp)
{
    switch (msg) {
    case 0x10: { // WM_CLOSE
        v11();
        reset_a();
        if (d3d) {
            d3d->rel();
            d3d = 0;
        }
        void* menu = GetMenu(h);
        if (menu)
            DestroyMenu(menu);
        DestroyWindow(h);
        PostQuitMessage(0);
        hwnd = 0;
        return 0;
    }
    case 0xf: // WM_PAINT
        if (dev && !active && windowed && f_aa && f_ab) {
            v8();
            dev->Present(0, 0, 0, 0);
        }
        break;
    case 5:   // WM_SIZE
        if (windowed && hwnd)
            saved_style = GetWindowLongA(hwnd, -16);
        if (wp == 1) {
            if (f345 && !windowed)
                ClipCursor(0);
            v16(1);
            minimized = true;
            maximized = false;
        } else if (wp == 2) {
            if (minimized)
                v16(0);
            minimized = false;
            maximized = true;
            size_changed();
        } else if (wp == 0) {
            if (maximized) {
                maximized = false;
                size_changed();
            } else if (minimized) {
                v16(0);
                minimized = false;
                size_changed();
            }
        }
        break;
    case 0x20: // WM_SETCURSOR
        if (active && !windowed) {
            SetCursor(0);
            if (f344)
                dev->ShowCursor(1);
            return 1;
        }
        break;
    case 0x111: // WM_COMMAND
        switch ((unsigned short)wp) {
        case 0x9c44:
            {
                bool nb = !f_ac;
                f_ac = nb;
                timer_op(!nb + 1);
            }
            break;
        case 0x9c45:
            if (!f_ac)
                timer_op(3);
            else
                timer_op(2);
            f_ac = false;
            f_ad = true;
            break;
        case 0x9c42:
            v16(1);
            toggle_fullscreen();
            v16(0);
            return 0;
        case 0x9c43:
            v16(1);
            if (handle_size_change() < 0)
                fail((int)0x8200000c, 1);
            v16(0);
            return 0;
        case 0x9c46:
            SendMessageA(h, 0x10, 0, 0);
            return 0;
        }
        break;
    case 0x24: // WM_GETMINMAXINFO
        ((int*)lp)[6] = 100;
        ((int*)lp)[7] = 100;
        break;
    case 0x112: // WM_SYSCOMMAND
        switch (wp) {
        case 0xf030: case 0xf000: case 0xf010: case 0xf100: case 0xf170:
            if (!windowed)
                return 1;
        }
        break;
    case 0x84: // WM_NCHITTEST
        if (!windowed)
            return 1;
        break;
    case 0x211: case 0x231:
        v16(1);
        break;
    case 0x232:
        v16(0);
        size_changed();
        break;
    case 0x200: // WM_MOUSEMOVE
        if (active && dev) {
            POINT pt;
            GetCursorPos(&pt);
            if (!windowed)
                ScreenToClient(hwnd, &pt);
            dev->SetCursorPosition(pt.x, pt.y, 0);
        }
        break;
    case 0x212:
        v16(0);
        break;
    case 0x218: // WM_POWERBROADCAST
        if (wp == 0)
            return 1;
        if (wp == 7)
            return 1;
        break;
    }
    LRESULT r = DefWindowProcA(h, msg, wp, lp);
    return r;
}

// ---- enumeration helpers ----------------------------------------------------------------------
struct VecItem {
    ~VecItem();                                     // 0x407aa0
};
void __cdecl operator delete(void*);                // 0x535712

struct DeviceComboDtor {
    char pad[0x14];
    VecItem* v[6];                                  // +0x14..+0x28
    void destroy();                                 // 0x403640
};

// @ 0x403640
void DeviceComboDtor::destroy()
{
    delete v[0];
    delete v[1];
    delete v[2];
    delete v[3];
    delete v[4];
    delete v[5];
}

struct D3DEnumeration {
    int f0, f4, f8, fc, f10, f14, f18, f1c, f20;
    char b24, b25, b26, b27;
    int f28;
    D3DEnumeration();                                    // 0x4036e0 (constructor: returns this)
};
// @ 0x4036e0
D3DEnumeration::D3DEnumeration()
{
    f4 = 0;
    f28 = 0;
    fc = 0x280;
    f10 = 0x1e0;
    f14 = 5;
    f18 = 0;
    f1c = 0xf;
    f20 = 0;
    b24 = 0;
    b25 = 0;
    b26 = 0;
    b27 = 0;
}

// @ 0x403720
struct DisplayMode { unsigned Width, Height, RefreshRate; int Format; };   // D3DDISPLAYMODE
int SortModesCallback(const void* arg1, const void* arg2)       // cdecl qsort comparator
{
    DisplayMode* pdm1 = (DisplayMode*)arg1;
    DisplayMode* pdm2 = (DisplayMode*)arg2;
    if (pdm1->Width > pdm2->Width) return 1;
    if (pdm1->Width < pdm2->Width) return -1;
    if (pdm1->Height > pdm2->Height) return 1;
    if (pdm1->Height < pdm2->Height) return -1;
    if (pdm1->Format > pdm2->Format) return 1;
    if (pdm1->Format < pdm2->Format) return -1;
    if (pdm1->RefreshRate > pdm2->RefreshRate) return 1;
    if (pdm1->RefreshRate < pdm2->RefreshRate) return -1;
    return 0;
}

// @ 0x4035d0
// File-local in the original: cl passes the argument in EAX for such functions, which is what the binary
// shows. A caller is needed so the compiler keeps it (its real callers are in later slices).
static unsigned GetColorChannelBits(int fmt)
{
    switch (fmt) {
    case 0x14: return 8; case 0x15: return 8; case 0x16: return 8;
    case 0x17: return 5; case 0x18: return 5; case 0x19: return 5;
    case 0x1a: return 4; case 0x1e: return 4;
    case 0x1b: return 2; case 0x1d: return 2;
    case 0x1f: return 10; case 0x23: return 10;
    }
    return 0;
}
unsigned color_bits_pair(int a, int b) { return GetColorChannelBits(a) + GetColorChannelBits(b); }

// ---- Direct3D enumeration (D3DEnumeration::BuildDepthStencilFormatList / BuildMultiSampleTypeList / ...) ----
struct ID3D9 {
    virtual long __stdcall q0(); virtual long __stdcall q1(); virtual long __stdcall q2();
    virtual long __stdcall q3(); virtual long __stdcall q4(); virtual long __stdcall q5();
    virtual long __stdcall q6(); virtual long __stdcall q7(); virtual long __stdcall q8();
    virtual long __stdcall q9();
    virtual long __stdcall CheckDeviceFormat(unsigned adapter, unsigned devtype, unsigned adapterfmt,
                                             unsigned usage, unsigned rtype, unsigned fmt);          // slot 10 (+0x28)
    virtual long __stdcall CheckDeviceMultiSampleType(unsigned adapter, unsigned devtype, unsigned fmt,
                                             int windowed, unsigned mstype, unsigned* quality);      // slot 11 (+0x2c)
    virtual long __stdcall CheckDepthStencilMatch(unsigned adapter, unsigned devtype, unsigned adapterfmt,
                                             unsigned rtfmt, unsigned dsfmt);                        // slot 12 (+0x30)
};

struct UIntVec {
    void push_back(const unsigned&);                // 0x407ab0 (CArrayList::Add, thiscall ret 4)
};

struct FmtInfo {                                            // D3DDeviceCombo
    unsigned adapter, devtype, adapterfmt, backfmt;         // +0..+0xc
    unsigned char windowed; char p0[3];                     // +0x10
    UIntVec* depth_fmts;                                    // +0x14
    UIntVec* mstypes;                                       // +0x18
    UIntVec* msquality;                                     // +0x1c
};

struct DeviceEnum {                                         // D3DEnumeration
    ID3D9* pd3d;                                            // +0
    char p0[0x1c - 4];
    unsigned min_depth_bits;                                // +0x1c
    unsigned min_stencil_bits;                              // +0x20
    void enum_depth_formats(FmtInfo* info);                 // 0x403770
    void enum_multisample(FmtInfo* info);                   // 0x4038c0
};

// @ 0x403770
void DeviceEnum::enum_depth_formats(FmtInfo* info)
{
    unsigned fmts[6] = { 0x50, 0x49, 0x4d, 0x4b, 0x4f, 0x47 };
    for (unsigned i = 0; i < 6; i++) {
        unsigned fmt = fmts[i];
        unsigned depth_bits;
        switch (fmt) {
        case 0x50: depth_bits = 0x10; break;
        case 0x49: depth_bits = 0xf; break;
        case 0x4b: depth_bits = 0x18; break;
        case 0x4d: depth_bits = 0x18; break;
        case 0x4f: depth_bits = 0x18; break;
        case 0x47: depth_bits = 0x20; break;
        default: depth_bits = 0;
        }
        if (depth_bits < min_depth_bits)
            continue;
        unsigned stencil_bits;
        switch (fmt) {
        case 0x49: stencil_bits = 1; break;
        case 0x4b: stencil_bits = 8; break;
        case 0x4f: stencil_bits = 4; break;
        case 0x47: stencil_bits = 0; break;
        case 0x4d: stencil_bits = 0; break;
        case 0x50: stencil_bits = 0; break;
        default: stencil_bits = 0;
        }
        if (stencil_bits < min_stencil_bits)
            continue;
        if (0 <= pd3d->CheckDeviceFormat(info->adapter, info->devtype, info->adapterfmt, 2, 1, fmt) &&
            0 <= pd3d->CheckDepthStencilMatch(info->adapter, info->devtype, info->adapterfmt, info->backfmt, fmt))
            info->depth_fmts->push_back(fmt);
    }
}

// @ 0x4038c0
void DeviceEnum::enum_multisample(FmtInfo* info)
{
    unsigned types[17] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    for (unsigned i = 0; i < 17; i++) {
        unsigned type = types[i];
        unsigned quality;
        if (pd3d->CheckDeviceMultiSampleType(info->adapter, info->devtype, info->backfmt, info->windowed, type, &quality) >= 0) {
            info->mstypes->push_back(type);
            info->msquality->push_back(quality);
        }
    }
}

// ---- 0x4039b0 ---------------------------------------------------------------------------------
struct IntVec {
    void push_back(const int&);                     // 0x407ab0 (same function as UIntVec::push_back)
};
struct ModeRec {                                            // D3DDeviceInfo (provisional)
    char p0[8];
    char key[4];                                            // +8: D3DCAPS9, passed by address to the callback
    char p1[0x24 - 12];
    unsigned flags;                                         // +0x24: Caps.DevCaps
};
struct ModeKey { char p0[8]; int x; int y; char p1[0x24 - 0x10]; IntVec* out; };   // +8, +0xc, +0x24

struct ModeFilter {                                         // D3DEnumeration
    char p0[8];
    bool (*cb)(void* rec, int kind, int x, int y);          // +8 ConfirmDeviceCallback
    char p1[0x25 - 0xc];
    bool want1;                                             // +0x25 AppUsesMixedVP
    void collect(ModeRec* rec, ModeKey* key);               // 0x4039b0 BuildVertexProcessingTypeList
};

// @ 0x4039b0
void ModeFilter::collect(ModeRec* rec, ModeKey* key)
{
    int v;
    if (rec->flags & 0x10000) {
        if ((rec->flags & 0x100000) && (!cb || cb(rec->key, 3, key->x, key->y))) {
            v = 3;
            key->out->push_back(v);
        }
        if (!cb || cb(rec->key, 2, key->x, key->y)) {
            v = 2;
            key->out->push_back(v);
        }
        if (want1 && (!cb || cb(rec->key, 1, key->x, key->y))) {
            v = 1;
            key->out->push_back(v);
        }
    }
    if (!cb || cb(rec->key, 0, key->x, key->y)) {
        v = 0;
        key->out->push_back(v);
    }
}
