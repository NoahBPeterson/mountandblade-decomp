// port/platform/dinput.cpp: DirectInput 8 keyboard and mouse over SDL3.
//   keyboard: GetDeviceState = SDL's keyboard state re-indexed by DIK code (256 bytes, 0x80 = down);
//             GetDeviceData  = key transitions (ofs = DIK, data 0x80 / 0)
//   mouse:    GetDeviceState = relative X/Y/wheel deltas since the last call + live buttons (DIMOUSESTATE / DIMOUSESTATE2)
//             GetDeviceData  = buffered motion / button events (ofs 0 X, 4 Y, 8 Z, 12.. buttons)
// Events arrive from the window layer's SDL event pump (on_sdl_event_for_input); the device methods pump it themselves too, so
// they work without a message loop. A device returns zeros until Acquire()d.
#include "mbp_internal.h"
#include <dinput.h>
#include <algorithm>

using namespace mbp;

namespace {
const GUID kIID_IUnknown = {0x00000000, 0x0000, 0x0000, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
bool same_guid(const GUID& a, const GUID& b) { return memcmp(&a, &b, sizeof(GUID)) == 0; }

const HRESULT kNotImpl = (HRESULT)0x80004001L;             // E_NOTIMPL (DIERR_UNSUPPORTED)
const HRESULT kDevNotReg = (HRESULT)0x80040154L;           // DIERR_DEVICENOTREG
const HRESULT kNotBuffered = (HRESULT)0x80040207L;         // DIERR_NOTBUFFERED
const HRESULT kBadVersion = (HRESULT)0x80040110L;          // DIERR_OLDDIRECTINPUTVERSION

struct Rec { DWORD ofs, data, time, seq; };

class Device;
std::mutex g_devs_mu;
std::vector<Device*> g_devs;
DWORD g_seq = 1;
std::atomic<bool> g_keys[SDL_SCANCODE_COUNT];
std::atomic<unsigned> g_btn_mask{0};

class Device final : public IDirectInputDevice8A {
public:
    enum Kind { KB, MOUSE } kind;
    explicit Device(Kind k) : kind(k) {
        std::lock_guard<std::mutex> g(g_devs_mu);
        g_devs.push_back(this);
    }
    ~Device() {
        std::lock_guard<std::mutex> g(g_devs_mu);
        g_devs.erase(std::remove(g_devs.begin(), g_devs.end(), this), g_devs.end());
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override {
        if (!out) return E_POINTER;
        if (same_guid(riid, kIID_IUnknown)) { *out = this; AddRef(); return S_OK; }
        *out = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { ULONG r = --refs; if (!r) delete this; return r; }

    HRESULT STDMETHODCALLTYPE GetCapabilities(DIDEVCAPS*) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA, LPVOID, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE GetProperty(REFGUID g, LPDIPROPHEADER h) override {
        if (same_guid(g, DIPROP_BUFFERSIZE) && h && h->dwSize >= sizeof(DIPROPDWORD)) { ((DIPROPDWORD*)h)->dwData = bufsize; return DI_OK; }
        return E_INVALIDARG;
    }
    HRESULT STDMETHODCALLTYPE SetProperty(REFGUID g, LPCDIPROPHEADER h) override {
                if (same_guid(g, DIPROP_BUFFERSIZE) && h && h->dwSize >= sizeof(DIPROPDWORD)) {
            std::lock_guard<std::mutex> lk(mu);
            bufsize = ((const DIPROPDWORD*)h)->dwData;
            ring.clear();
            return DI_OK;
        }
        return E_INVALIDARG;
    }
    HRESULT STDMETHODCALLTYPE Acquire() override {
        mbp_sdl_init();
        std::lock_guard<std::mutex> lk(mu);
        if (acquired) return S_FALSE;                       // DI_NOEFFECT
        acquired = true; ax = ay = az = 0; ring.clear();
        return DI_OK;
    }
    HRESULT STDMETHODCALLTYPE Unacquire() override {
        std::lock_guard<std::mutex> lk(mu);
        bool was = acquired;
        acquired = false;
        return was ? DI_OK : S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE GetDeviceState(DWORD cb, LPVOID data) override {
        mbp_pump_events();
        std::lock_guard<std::mutex> lk(mu);
        if (!data) return E_POINTER;
        if (!fmt_set) return DIERR_NOTACQUIRED;             // no format yet: nothing can be read
        if (!acquired) return DIERR_NOTACQUIRED;
        if (kind == KB) {
            if (cb != 256) return E_INVALIDARG;
            dinput_poll_keyboard((uint8_t*)data);
            return DI_OK;
        }
        if (cb != 16 && cb != 20) return E_INVALIDARG;
        DIMOUSESTATE2 s;
        memset(&s, 0, sizeof s);
        s.lX = ax; s.lY = ay; s.lZ = az;
        ax = ay = az = 0;
        unsigned b = mouse_button_mask();
        s.rgbButtons[0] = (b & 1) ? 0x80 : 0;      // left
        s.rgbButtons[1] = (b & 4) ? 0x80 : 0;      // right (SDL button 3)
        s.rgbButtons[2] = (b & 2) ? 0x80 : 0;      // middle (SDL button 2)
        s.rgbButtons[3] = (b & 8) ? 0x80 : 0;
        s.rgbButtons[4] = (b & 16) ? 0x80 : 0;
        memcpy(data, &s, cb);
        return DI_OK;
    }
    HRESULT STDMETHODCALLTYPE GetDeviceData(DWORD cbObj, LPDIDEVICEOBJECTDATA out, LPDWORD inout, DWORD flags) override {
        mbp_pump_events();
        std::lock_guard<std::mutex> lk(mu);
        if (!inout) return E_POINTER;
        if (cbObj < 20) return E_INVALIDARG;
        if (!acquired) return DIERR_NOTACQUIRED;
        if (!bufsize) return kNotBuffered;
        DWORD want = *inout, n = 0;
        bool peek = (flags & 1) != 0;                       // DIGDD_PEEK
        if (!out) want = ~0u;                               // NULL buffer: flush
        while (n < want && n < ring.size()) {
            if (out) {
                unsigned char* p = (unsigned char*)out + (size_t)n * cbObj;
                memset(p, 0, cbObj);
                memcpy(p, &ring[n], sizeof(Rec));
            }
            ++n;
        }
        if (!peek) ring.erase(ring.begin(), ring.begin() + n);
        *inout = out ? n : 0;
        HRESULT r = overflow ? DI_BUFFEROVERFLOW : DI_OK;
        if (!peek) overflow = false;
        return r;
    }
    HRESULT STDMETHODCALLTYPE SetDataFormat(LPCDIDATAFORMAT f) override {
        if (!f) return E_POINTER;
        if (kind == KB && f->dwDataSize != 256) return E_INVALIDARG;
        if (kind == MOUSE && f->dwDataSize != 16 && f->dwDataSize != 20) return E_INVALIDARG;
        std::lock_guard<std::mutex> lk(mu);
        fmt_set = true;
        return DI_OK;
    }
    HRESULT STDMETHODCALLTYPE SetEventNotification(HANDLE h) override { std::lock_guard<std::mutex> lk(mu); event = h; return DI_OK; }
    HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND h, DWORD flags) override {
        if (!IsWindow(h)) return E_INVALIDARG;
        if (!(flags & (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE)) || !(flags & (DISCL_FOREGROUND | DISCL_BACKGROUND))) return E_INVALIDARG;
        std::lock_guard<std::mutex> lk(mu);
        hwnd = h; coop = flags;
        return DI_OK;
    }
    HRESULT STDMETHODCALLTYPE GetObjectInfo(DIDEVICEOBJECTINSTANCEA*, DWORD, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE GetDeviceInfo(DIDEVICEINSTANCEA*) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE RunControlPanel(HWND, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE Initialize(HINSTANCE, DWORD, REFGUID) override { return DI_OK; }
    HRESULT STDMETHODCALLTYPE CreateEffect(REFGUID, const DIEFFECT*, IDirectInputEffect**, LPUNKNOWN) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE EnumEffects(LPDIENUMEFFECTSCALLBACKA, LPVOID, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE GetEffectInfo(DIEFFECTINFOA*, REFGUID) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE GetForceFeedbackState(LPDWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE SendForceFeedbackCommand(DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK, LPVOID, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE Escape(DIEFFESCAPE*) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE Poll() override { return acquired ? S_FALSE : DIERR_NOTACQUIRED; }   // DI_NOEFFECT: these devices need no polling
    HRESULT STDMETHODCALLTYPE SendDeviceData(DWORD, LPCDIDEVICEOBJECTDATA, LPDWORD, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE EnumEffectsInFile(LPCSTR, LPDIENUMEFFECTSINFILECALLBACK, LPVOID, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE WriteEffectToFile(LPCSTR, DWORD, DIFILEEFFECT*, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE BuildActionMap(DIACTIONFORMATA*, LPCSTR, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE SetActionMap(DIACTIONFORMATA*, LPCSTR, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE GetImageInfo(DIDEVICEIMAGEINFOHEADERA*) override { return kNotImpl; }

    // ---- fed by the SDL event pump ----
    void push(DWORD ofs, DWORD data, bool coalesce_motion) {
        if (!bufsize) return;
        if (coalesce_motion && ring.size() >= 2) {
            Rec& a = ring[ring.size() - 2];
            Rec& b = ring[ring.size() - 1];
            if (a.ofs == 0 && b.ofs == 4 && data == 0) return;    // (never used: motion pushes go through push_motion)
        }
        if (ring.size() >= bufsize) { overflow = true; return; }
        ring.push_back(Rec{ofs, data, (DWORD)SDL_GetTicks(), g_seq++});
    }
    void push_motion(int dx, int dy) {
        if (dx == 0 && dy == 0) return;
        // fold into the previous pair when the newest two records are the unread X / Y of the last motion
        if (ring.size() >= 2 && ring[ring.size() - 2].ofs == 0 && ring[ring.size() - 1].ofs == 4) {
            ring[ring.size() - 2].data = (DWORD)((int)ring[ring.size() - 2].data + dx);
            ring[ring.size() - 1].data = (DWORD)((int)ring[ring.size() - 1].data + dy);
            return;
        }
        if (ring.size() + 2 > bufsize) { if (bufsize) overflow = true; return; }
        push(0, (DWORD)dx, false);
        push(4, (DWORD)dy, false);
    }
    void on_event(const SDL_Event& e) {
        std::lock_guard<std::mutex> lk(mu);
        if (!acquired) return;
        bool signal = false;
        if (kind == MOUSE) {
            if (e.type == SDL_EVENT_MOUSE_MOTION) {
                int dx = (int)e.motion.xrel, dy = (int)e.motion.yrel;
                if (dx || dy) { ax += dx; ay += dy; push_motion(dx, dy); signal = true; }
            } else if (e.type == SDL_EVENT_MOUSE_WHEEL) {
                int dz = (int)(e.wheel.y * (e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -120 : 120));
                if (dz) { az += dz; push(8, (DWORD)dz, false); signal = true; }
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                int i = e.button.button == SDL_BUTTON_LEFT ? 0 : e.button.button == SDL_BUTTON_RIGHT ? 1 : e.button.button == SDL_BUTTON_MIDDLE ? 2
                      : e.button.button == SDL_BUTTON_X1 ? 3 : e.button.button == SDL_BUTTON_X2 ? 4 : -1;
                if (i >= 0) { push(DIMOFS_BUTTON0 + i, e.button.down ? 0x80 : 0, false); signal = true; }
            }
        } else if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) {
            int dik = keymap_dik_for_scancode(e.key.scancode);
            if (dik && !e.key.repeat) { push((DWORD)dik, e.key.down ? 0x80 : 0, false); signal = true; }
        }
        if (signal && event) SetEvent(event);
    }

    std::atomic<ULONG> refs{1};
    std::mutex mu;
    bool acquired = false, fmt_set = false, overflow = false;
    DWORD bufsize = 0, coop = 0;
    HWND hwnd = nullptr;
    HANDLE event = nullptr;
    int ax = 0, ay = 0, az = 0;
    std::vector<Rec> ring;
};

class DirectInput final : public IDirectInput8A {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override {
        if (!out) return E_POINTER;
        if (same_guid(riid, kIID_IUnknown) || same_guid(riid, IID_IDirectInput8A)) { *out = this; AddRef(); return S_OK; }
        *out = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { ULONG r = --refs; if (!r) delete this; return r; }
    HRESULT STDMETHODCALLTYPE CreateDevice(REFGUID g, LPDIRECTINPUTDEVICE8A* out, LPUNKNOWN outer) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (outer) return (HRESULT)0x80040110;               // CLASS_E_NOAGGREGATION
        if (same_guid(g, GUID_SysKeyboard)) { *out = new Device(Device::KB); return DI_OK; }
        if (same_guid(g, GUID_SysMouse)) { *out = new Device(Device::MOUSE); return DI_OK; }
        return kDevNotReg;
    }
    HRESULT STDMETHODCALLTYPE EnumDevices(DWORD, LPDIENUMDEVICESCALLBACKA, LPVOID, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE GetDeviceStatus(REFGUID g) override {
        return (same_guid(g, GUID_SysKeyboard) || same_guid(g, GUID_SysMouse)) ? DI_OK : S_FALSE;   // S_FALSE: not attached
    }
    HRESULT STDMETHODCALLTYPE RunControlPanel(HWND, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE Initialize(HINSTANCE, DWORD) override { return DI_OK; }
    HRESULT STDMETHODCALLTYPE FindDevice(REFGUID, LPCSTR, LPGUID) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE EnumDevicesBySemantics(LPCSTR, DIACTIONFORMATA*, LPDIENUMDEVICESBYSEMANTICSCBA, LPVOID, DWORD) override { return kNotImpl; }
    HRESULT STDMETHODCALLTYPE ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK, DICONFIGUREDEVICESPARAMSA*, DWORD, LPVOID) override { return kNotImpl; }
    std::atomic<ULONG> refs{1};
};

// ---- the data formats ----------------------------------------------------------------------------------------------------------
const GUID kXAxis = {0xA36D02E0, 0xC9F3, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
const GUID kYAxis = {0xA36D02E1, 0xC9F3, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
const GUID kZAxis = {0xA36D02E2, 0xC9F3, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
const GUID kKey = {0x55728220, 0xD33C, 0x11CF, {0xBF, 0xC3, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
const DWORD DIDFT_AXIS_OPT = 0x80FFFF03, DIDFT_BTN_OPT = 0x80FFFF0C;
DIOBJECTDATAFORMAT g_kb_objs[256];
DIOBJECTDATAFORMAT g_mouse_objs[11] = {
    {&kXAxis, 0, DIDFT_AXIS_OPT, 0}, {&kYAxis, 4, DIDFT_AXIS_OPT, 0}, {&kZAxis, 8, DIDFT_AXIS_OPT, 0},
    {nullptr, 12, DIDFT_BTN_OPT, 0}, {nullptr, 13, DIDFT_BTN_OPT, 0}, {nullptr, 14, DIDFT_BTN_OPT, 0}, {nullptr, 15, DIDFT_BTN_OPT, 0},
    {nullptr, 16, DIDFT_BTN_OPT, 0}, {nullptr, 17, DIDFT_BTN_OPT, 0}, {nullptr, 18, DIDFT_BTN_OPT, 0}, {nullptr, 19, DIDFT_BTN_OPT, 0}};
struct InitKb {
    InitKb() { for (int i = 0; i < 256; ++i) g_kb_objs[i] = DIOBJECTDATAFORMAT{&kKey, (DWORD)i, 0x8000000Cu | ((DWORD)i << 8), 0}; }
} g_init_kb;
}   // namespace

extern "C" {
const GUID mb_DIPROP_BUFFERSIZE = {1, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
const GUID GUID_SysKeyboard = {0x6F1D2B61, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
const GUID GUID_SysMouse = {0x6F1D2B60, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
const GUID IID_IDirectInput8A = {0xBF798030, 0x483A, 0x4DA2, {0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00}};
const DIDATAFORMAT c_dfDIKeyboard = {sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT), 2 /*DIDF_RELAXIS*/, 256, 256, g_kb_objs};
const DIDATAFORMAT c_dfDIMouse = {sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT), 2, 16, 7, g_mouse_objs};
const DIDATAFORMAT c_dfDIMouse2 = {sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT), 2, 20, 11, g_mouse_objs};

HRESULT WINAPI DirectInput8Create(HINSTANCE, DWORD version, REFIID riid, LPVOID* out, LPUNKNOWN outer)
{
    if (!out) return E_POINTER;
    *out = nullptr;
    if (version != 0x0800) return kBadVersion;
    if (outer) return (HRESULT)0x80040110;
    if (!same_guid(riid, IID_IDirectInput8A) && !same_guid(riid, kIID_IUnknown)) return E_NOINTERFACE;
    if (!mbp_sdl_init()) return E_FAIL;
    *out = new DirectInput;
    return DI_OK;
}
}

namespace mbp {
void on_sdl_event_for_input(const SDL_Event& e)
{
    switch (e.type) {
    case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP:
        if ((int)e.key.scancode > 0 && (int)e.key.scancode < SDL_SCANCODE_COUNT) g_keys[e.key.scancode] = e.key.down;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP:
        if (e.button.button >= 1 && e.button.button <= 5) {
            unsigned bit = 1u << (e.button.button - 1);
            if (e.button.down) g_btn_mask |= bit; else g_btn_mask &= ~bit;
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        for (auto& k : g_keys) k = false;
        g_btn_mask = 0;
        break;
    }
    std::vector<Device*> snapshot;
    {
        std::lock_guard<std::mutex> g(g_devs_mu);
        snapshot = g_devs;
    }
    for (Device* d : snapshot) d->on_event(e);
}
void dinput_poll_keyboard(uint8_t state[256])
{
    memset(state, 0, 256);
    for (int sc = 1; sc < SDL_SCANCODE_COUNT; ++sc) {
        if (!g_keys[sc]) continue;
        int dik = keymap_dik_for_scancode((SDL_Scancode)sc);
        if (dik) state[dik] = 0x80;
    }
}
bool key_is_down(SDL_Scancode sc) { return (int)sc > 0 && (int)sc < SDL_SCANCODE_COUNT && g_keys[sc]; }
unsigned mouse_button_mask() { return g_btn_mask; }
}   // namespace mbp
