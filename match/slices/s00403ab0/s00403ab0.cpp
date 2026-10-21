// Slice s00403ab0: D3D device enumeration (CD3DEnumeration from the DirectX 9 SDK sample framework:
// BuildPresentIntervalList, BuildDSMSConflictList, EnumerateDeviceCombos, EnumerateDevices, destructors).
// Layouts follow the SDK's d3denumeration.h; every offset below is confirmed by the disassembly.

void __cdecl operator delete(void*);                // 0x535712

// Direct3D 9 interface, only the slots used.
struct ID3D9 {
    virtual long __stdcall q0(); virtual long __stdcall q1(); virtual long __stdcall q2();
    virtual long __stdcall q3(); virtual long __stdcall q4(); virtual long __stdcall q5();
    virtual long __stdcall q6(); virtual long __stdcall q7(); virtual long __stdcall q8();
    virtual long __stdcall CheckDeviceType(unsigned adapter, unsigned devtype, unsigned adapterfmt,
                                           unsigned backfmt, int windowed);                          // slot 9 (+0x24)
    virtual long __stdcall q10();
    virtual long __stdcall CheckDeviceMultiSampleType(unsigned adapter, unsigned devtype, unsigned fmt,
                                           int windowed, unsigned mstype, unsigned* quality);        // slot 11 (+0x2c)
    virtual long __stdcall q12(); virtual long __stdcall q13();
    virtual long __stdcall GetDeviceCaps(unsigned adapter, unsigned devtype, void* caps);            // slot 14 (+0x38)
};

// CArrayList (0x14 bytes): kind, data, bytes-per-entry, count, capacity.
struct CArrayList {
    int kind; void* data; int entry_size; unsigned count; int capacity;
    CArrayList(int kind_, int bytes) throw();       // 0x407a70
    ~CArrayList();                                  // 0x407aa0
    long Add(void* entry);                          // 0x407ab0
    void* GetPtr(unsigned i);                       // 0x407ba0
};

struct DeviceCombo {                                // 0x2c bytes
    unsigned adapter, devtype, adapter_fmt, backbuffer_fmt;   // +0..+0xc
    unsigned char windowed; char p0[3];             // +0x10
    CArrayList* depth_stencil_fmts;                 // +0x14
    CArrayList* multisample_types;                  // +0x18
    CArrayList* multisample_quality;                // +0x1c
    CArrayList* ds_ms_conflicts;                    // +0x20
    CArrayList* vertex_processing;                  // +0x24
    CArrayList* present_intervals;                  // +0x28
    ~DeviceCombo();                                 // 0x403640
};

struct DeviceInfo {                                 // 0x13c bytes
    unsigned adapter, devtype;
    unsigned caps[0x130 / 4];                       // +8: D3DCAPS9 (PresentationIntervals at +0x14 = DeviceInfo+0x1c)
    CArrayList* combos;                             // +0x138
    ~DeviceInfo();                                  // 0x403bf0
};

struct AdapterInfo {
    char p0[0x450];
    CArrayList* display_modes;                      // +0x450
    CArrayList* devices;                            // +0x454
    ~AdapterInfo();                                 // 0x4040c0
};

struct DSMSConflict { unsigned ds_fmt, ms_type; };

struct D3DEnum {
    ID3D9* d3d;                                     // +0
    CArrayList* adapters;                           // +4
    bool (*confirm)(void*, unsigned, unsigned, unsigned);   // +8
    unsigned min_fs_width, min_fs_height;           // +0xc, +0x10
    unsigned min_color_bits, min_alpha_bits;        // +0x14, +0x18
    unsigned min_depth_bits, min_stencil_bits;      // +0x1c, +0x20
    bool uses_depth_buffer, uses_mixed_vp;          // +0x24, +0x25
    bool requires_windowed, requires_fullscreen;    // +0x26, +0x27
    CArrayList* allowed_adapter_fmts;               // +0x28

    void BuildDepthStencilFormatList(DeviceCombo*);        // 0x403770
    void BuildMultiSampleTypeList(DeviceCombo*);           // 0x4038c0
    void BuildVertexProcessingTypeList(DeviceInfo*, DeviceCombo*);   // 0x4039b0
    void BuildPresentIntervalList(DeviceInfo*, DeviceCombo*);   // 0x403ab0 (stdcall in the binary: see below)
    void BuildConflictList(DeviceCombo*);                  // 0x403b40
    long EnumerateDeviceCombos(DeviceInfo*, CArrayList*);  // 0x403c50
    long EnumerateDevices(AdapterInfo*, CArrayList*);      // 0x403f90
};

// @ 0x403ab0
void __stdcall BuildPresentIntervalList(DeviceInfo* dev, DeviceCombo* combo)
{
    unsigned intervals[6] = { 0x80000000, 0, 1, 2, 4, 8 };
    for (unsigned i = 0; i < 6; i++) {
        unsigned pi = intervals[i];
        if (combo->windowed && (pi == 2 || pi == 4 || pi == 8))
            continue;
        if (pi == 0 || (dev->caps[5] & pi))
            combo->present_intervals->Add(&pi);
    }
}

// @ 0x403b40
void D3DEnum::BuildConflictList(DeviceCombo* combo)
{
    for (unsigned i = 0; i < combo->depth_stencil_fmts->count; i++) {
        unsigned ds = *(unsigned*)combo->depth_stencil_fmts->GetPtr(i);
        for (unsigned j = 0; j < combo->multisample_types->count; j++) {
            unsigned ms = *(unsigned*)combo->multisample_types->GetPtr(j);
            if (d3d->CheckDeviceMultiSampleType(combo->adapter, combo->devtype, ds, combo->windowed, ms, 0) < 0) {
                DSMSConflict c;
                c.ds_fmt = ds;
                c.ms_type = ms;
                combo->ds_ms_conflicts->Add(&c);
            }
        }
    }
}

// @ 0x403bf0
DeviceInfo::~DeviceInfo()
{
    if (combos) {
        for (unsigned i = 0; i < combos->count; i++) {
            DeviceCombo* c = (DeviceCombo*)combos->GetPtr(i);
            delete c;
        }
        delete combos;
    }
}

// @ 0x4040c0
AdapterInfo::~AdapterInfo()
{
    delete display_modes;
    if (devices) {
        for (unsigned i = 0; i < devices->count; i++) {
            DeviceInfo* d = (DeviceInfo*)devices->GetPtr(i);
            delete d;
        }
        delete devices;
    }
}

// @ 0x403c50
long D3DEnum::EnumerateDeviceCombos(DeviceInfo* dev, CArrayList* adapter_fmts)
{
    unsigned bb_fmts[6] = { 0x15, 0x16, 0x23, 0x17, 0x19, 0x18 };
    bool windowed_arr[2] = { false, true };
    for (unsigned ifmt = 0; ifmt < adapter_fmts->count; ifmt++) {
        unsigned adapter_fmt = *(unsigned*)adapter_fmts->GetPtr(ifmt);
        for (unsigned ibb = 0; ibb < 6; ibb++) {
            unsigned bb_fmt = bb_fmts[ibb];
            unsigned alpha;
            switch (bb_fmt) {
            case 0x15: alpha = 8; break;
            case 0x19: alpha = 1; break;
            case 0x1a: alpha = 4; break;
            case 0x1d: alpha = 8; break;
            case 0x1f: alpha = 2; break;
            case 0x23: alpha = 2; break;
            case 0x14: alpha = 0; break;
            case 0x16: alpha = 0; break;
            case 0x17: alpha = 0; break;
            case 0x18: alpha = 0; break;
            default: alpha = 0;
            }
            if (alpha < min_alpha_bits)
                continue;
            for (unsigned iw = 0; iw < 2; iw++) {
                bool windowed = windowed_arr[iw];
                if (!windowed && requires_windowed)
                    continue;
                if (windowed && requires_fullscreen)
                    continue;
                if (d3d->CheckDeviceType(dev->adapter, dev->devtype, adapter_fmt, bb_fmt, windowed) < 0)
                    continue;
                DeviceCombo* combo = new DeviceCombo;
                if (!combo)
                    return (long)0x8007000e;
                combo->depth_stencil_fmts = new CArrayList(0, 4);
                combo->multisample_types = new CArrayList(0, 4);
                combo->multisample_quality = new CArrayList(0, 4);
                combo->ds_ms_conflicts = new CArrayList(0, 8);
                combo->vertex_processing = new CArrayList(0, 4);
                combo->present_intervals = new CArrayList(0, 4);
                if (!combo->depth_stencil_fmts || !combo->multisample_types || !combo->multisample_quality ||
                    !combo->ds_ms_conflicts || !combo->vertex_processing || !combo->present_intervals) {
                    delete combo;
                    return (long)0x8007000e;
                }
                combo->adapter = dev->adapter;
                combo->devtype = dev->devtype;
                combo->adapter_fmt = adapter_fmt;
                combo->backbuffer_fmt = bb_fmt;
                combo->windowed = windowed;
                if (uses_depth_buffer) {
                    BuildDepthStencilFormatList(combo);
                    if (combo->depth_stencil_fmts->count == 0) {
                        delete combo;
                        continue;
                    }
                }
                BuildMultiSampleTypeList(combo);
                if (combo->multisample_types->count == 0) {
                    delete combo;
                    continue;
                }
                BuildConflictList(combo);
                BuildVertexProcessingTypeList(dev, combo);
                if (combo->vertex_processing->count == 0) {
                    delete combo;
                    continue;
                }
                BuildPresentIntervalList(dev, combo);
                dev->combos->Add(combo);
            }
        }
    }
    return 0;
}

// @ 0x403f90
long D3DEnum::EnumerateDevices(AdapterInfo* adapter, CArrayList* adapter_fmts)
{
    unsigned dev_types[3] = { 1, 3, 2 };
    for (unsigned i = 0; i < 3; i++) {
        DeviceInfo* dev = new DeviceInfo;
        if (!dev)
            return (long)0x8007000e;
        dev->combos = new CArrayList(1, 0);
        if (!dev->combos) {
            delete dev;
            return (long)0x8007000e;
        }
        dev->adapter = *(unsigned*)adapter;
        dev->devtype = dev_types[i];
        if (d3d->GetDeviceCaps(*(unsigned*)adapter, dev_types[i], dev->caps) < 0) {
            delete dev;
            continue;
        }
        long hr = EnumerateDeviceCombos(dev, adapter_fmts);
        if (hr < 0) {
            delete dev;
            return hr;
        }
        if (dev->combos->count == 0) {
            delete dev;
            continue;
        }
        adapter->devices->Add(dev);
    }
    return 0;
}
