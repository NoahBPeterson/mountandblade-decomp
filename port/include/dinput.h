// port/include/dinput.h: portable-build shim (ticket 004) for the DirectInput 8 surface src/ uses (keyboard + buffered
// mouse, src/game/config/config.cpp). Interfaces in the API's vtable order; the SDL3 platform layer (ticket 007) replaces
// the devices with an SDL keyboard/mouse poll into the same DIK-indexed state arrays.
#pragma once
#ifndef MB_PORT_DINPUT_H
#define MB_PORT_DINPUT_H
#include <windows.h>

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#define DI_OK S_OK
#define DIERR_INPUTLOST ((HRESULT)0x8007001EL)
#define DIERR_NOTACQUIRED ((HRESULT)0x8007000CL)
#define DIERR_OTHERAPPHASPRIO ((HRESULT)0x80070005L)
#define DI_BUFFEROVERFLOW S_FALSE
#define DISCL_EXCLUSIVE 0x00000001
#define DISCL_NONEXCLUSIVE 0x00000002
#define DISCL_FOREGROUND 0x00000004
#define DISCL_BACKGROUND 0x00000008
#define DISCL_NOWINKEY 0x00000010
#define DIPH_DEVICE 0
#define DIPH_BYOFFSET 1
// Microsoft's MAKEDIPROP(1) is a reference to the address 1, which clang treats as misaligned (UB); the property ids are
// real GUID objects here ({prop, 0, 0, {0}}) that the devices compare by value.
extern "C" const GUID mb_DIPROP_BUFFERSIZE;
#define DIPROP_BUFFERSIZE mb_DIPROP_BUFFERSIZE
#define DIMOFS_X 0
#define DIMOFS_Y 4
#define DIMOFS_Z 8
#define DIMOFS_BUTTON0 12
#define DIMOFS_BUTTON1 13
#define DIMOFS_BUTTON2 14

typedef struct _DIOBJECTDATAFORMAT { const GUID* pguid; DWORD dwOfs; DWORD dwType; DWORD dwFlags; } DIOBJECTDATAFORMAT, *LPDIOBJECTDATAFORMAT;
typedef struct _DIDATAFORMAT { DWORD dwSize, dwObjSize, dwFlags, dwDataSize, dwNumObjs; LPDIOBJECTDATAFORMAT rgodf; } DIDATAFORMAT, *LPDIDATAFORMAT;
typedef const DIDATAFORMAT* LPCDIDATAFORMAT;
typedef struct DIPROPHEADER { DWORD dwSize, dwHeaderSize, dwObj, dwHow; } DIPROPHEADER, *LPDIPROPHEADER;
typedef const DIPROPHEADER* LPCDIPROPHEADER;
typedef struct DIPROPDWORD { DIPROPHEADER diph; DWORD dwData; } DIPROPDWORD, *LPDIPROPDWORD;
typedef struct DIDEVICEOBJECTDATA { DWORD dwOfs, dwData, dwTimeStamp, dwSequence; UINT_PTR uAppData; } DIDEVICEOBJECTDATA, *LPDIDEVICEOBJECTDATA;
typedef const DIDEVICEOBJECTDATA* LPCDIDEVICEOBJECTDATA;
typedef struct _DIMOUSESTATE { LONG lX, lY, lZ; BYTE rgbButtons[4]; } DIMOUSESTATE, *LPDIMOUSESTATE;
typedef struct _DIMOUSESTATE2 { LONG lX, lY, lZ; BYTE rgbButtons[8]; } DIMOUSESTATE2, *LPDIMOUSESTATE2;

extern "C" {
extern const DIDATAFORMAT c_dfDIKeyboard;
extern const DIDATAFORMAT c_dfDIMouse;
extern const DIDATAFORMAT c_dfDIMouse2;
extern const GUID GUID_SysKeyboard;
extern const GUID GUID_SysMouse;
extern const GUID IID_IDirectInput8A;
}

struct DIDEVCAPS; struct DIDEVICEOBJECTINSTANCEA; struct DIDEVICEINSTANCEA; struct DIEFFECT; struct DIEFFECTINFOA; struct DIEFFESCAPE;
struct DIFILEEFFECT; struct DIACTIONFORMATA; struct DIDEVICEIMAGEINFOHEADERA; struct DICONFIGUREDEVICESPARAMSA; struct IDirectInputEffect;
typedef BOOL (CALLBACK* LPDIENUMDEVICEOBJECTSCALLBACKA)(const DIDEVICEOBJECTINSTANCEA*, LPVOID);
typedef BOOL (CALLBACK* LPDIENUMEFFECTSCALLBACKA)(const DIEFFECTINFOA*, LPVOID);
typedef BOOL (CALLBACK* LPDIENUMCREATEDEFFECTOBJECTSCALLBACK)(IDirectInputEffect*, LPVOID);
typedef BOOL (CALLBACK* LPDIENUMEFFECTSINFILECALLBACK)(const DIFILEEFFECT*, LPVOID);
typedef BOOL (CALLBACK* LPDIENUMDEVICESCALLBACKA)(const DIDEVICEINSTANCEA*, LPVOID);
typedef BOOL (CALLBACK* LPDIENUMDEVICESBYSEMANTICSCBA)(const DIDEVICEINSTANCEA*, struct IDirectInputDevice8A*, DWORD, DWORD, LPVOID);
typedef BOOL (CALLBACK* LPDICONFIGUREDEVICESCALLBACK)(IUnknown*, LPVOID);

#define MB_DIM(ret, name) virtual ret STDMETHODCALLTYPE name
struct IDirectInputDevice8A : public IUnknown {
    MB_DIM(HRESULT, GetCapabilities)(DIDEVCAPS*) = 0;
    MB_DIM(HRESULT, EnumObjects)(LPDIENUMDEVICEOBJECTSCALLBACKA, LPVOID, DWORD) = 0;
    MB_DIM(HRESULT, GetProperty)(REFGUID, LPDIPROPHEADER) = 0;
    MB_DIM(HRESULT, SetProperty)(REFGUID, LPCDIPROPHEADER) = 0;
    MB_DIM(HRESULT, Acquire)() = 0;
    MB_DIM(HRESULT, Unacquire)() = 0;
    MB_DIM(HRESULT, GetDeviceState)(DWORD, LPVOID) = 0;
    MB_DIM(HRESULT, GetDeviceData)(DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD) = 0;
    MB_DIM(HRESULT, SetDataFormat)(LPCDIDATAFORMAT) = 0;
    MB_DIM(HRESULT, SetEventNotification)(HANDLE) = 0;
    MB_DIM(HRESULT, SetCooperativeLevel)(HWND, DWORD) = 0;
    MB_DIM(HRESULT, GetObjectInfo)(DIDEVICEOBJECTINSTANCEA*, DWORD, DWORD) = 0;
    MB_DIM(HRESULT, GetDeviceInfo)(DIDEVICEINSTANCEA*) = 0;
    MB_DIM(HRESULT, RunControlPanel)(HWND, DWORD) = 0;
    MB_DIM(HRESULT, Initialize)(HINSTANCE, DWORD, REFGUID) = 0;
    MB_DIM(HRESULT, CreateEffect)(REFGUID, const DIEFFECT*, IDirectInputEffect**, LPUNKNOWN) = 0;
    MB_DIM(HRESULT, EnumEffects)(LPDIENUMEFFECTSCALLBACKA, LPVOID, DWORD) = 0;
    MB_DIM(HRESULT, GetEffectInfo)(DIEFFECTINFOA*, REFGUID) = 0;
    MB_DIM(HRESULT, GetForceFeedbackState)(LPDWORD) = 0;
    MB_DIM(HRESULT, SendForceFeedbackCommand)(DWORD) = 0;
    MB_DIM(HRESULT, EnumCreatedEffectObjects)(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK, LPVOID, DWORD) = 0;
    MB_DIM(HRESULT, Escape)(DIEFFESCAPE*) = 0;
    MB_DIM(HRESULT, Poll)() = 0;
    MB_DIM(HRESULT, SendDeviceData)(DWORD, LPCDIDEVICEOBJECTDATA, LPDWORD, DWORD) = 0;
    MB_DIM(HRESULT, EnumEffectsInFile)(LPCSTR, LPDIENUMEFFECTSINFILECALLBACK, LPVOID, DWORD) = 0;
    MB_DIM(HRESULT, WriteEffectToFile)(LPCSTR, DWORD, DIFILEEFFECT*, DWORD) = 0;
    MB_DIM(HRESULT, BuildActionMap)(DIACTIONFORMATA*, LPCSTR, DWORD) = 0;
    MB_DIM(HRESULT, SetActionMap)(DIACTIONFORMATA*, LPCSTR, DWORD) = 0;
    MB_DIM(HRESULT, GetImageInfo)(DIDEVICEIMAGEINFOHEADERA*) = 0;
};
typedef IDirectInputDevice8A* LPDIRECTINPUTDEVICE8A;
typedef IDirectInputDevice8A* LPDIRECTINPUTDEVICE8;
struct IDirectInput8A : public IUnknown {
    MB_DIM(HRESULT, CreateDevice)(REFGUID, LPDIRECTINPUTDEVICE8A*, LPUNKNOWN) = 0;
    MB_DIM(HRESULT, EnumDevices)(DWORD, LPDIENUMDEVICESCALLBACKA, LPVOID, DWORD) = 0;
    MB_DIM(HRESULT, GetDeviceStatus)(REFGUID) = 0;
    MB_DIM(HRESULT, RunControlPanel)(HWND, DWORD) = 0;
    MB_DIM(HRESULT, Initialize)(HINSTANCE, DWORD) = 0;
    MB_DIM(HRESULT, FindDevice)(REFGUID, LPCSTR, LPGUID) = 0;
    MB_DIM(HRESULT, EnumDevicesBySemantics)(LPCSTR, DIACTIONFORMATA*, LPDIENUMDEVICESBYSEMANTICSCBA, LPVOID, DWORD) = 0;
    MB_DIM(HRESULT, ConfigureDevices)(LPDICONFIGUREDEVICESCALLBACK, DICONFIGUREDEVICESPARAMSA*, DWORD, LPVOID) = 0;
};
#undef MB_DIM
typedef IDirectInput8A* LPDIRECTINPUT8A;
typedef IDirectInput8A* LPDIRECTINPUT8;

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);

// DIK scan codes (the keyboard state array is indexed by them)
#define DIK_ESCAPE 0x01
#define DIK_1 0x02
#define DIK_2 0x03
#define DIK_3 0x04
#define DIK_4 0x05
#define DIK_5 0x06
#define DIK_6 0x07
#define DIK_7 0x08
#define DIK_8 0x09
#define DIK_9 0x0A
#define DIK_0 0x0B
#define DIK_RETURN 0x1C
#define DIK_LCONTROL 0x1D
#define DIK_A 0x1E
#define DIK_S 0x1F
#define DIK_D 0x20
#define DIK_F 0x21
#define DIK_LSHIFT 0x2A
#define DIK_RSHIFT 0x36
#define DIK_LMENU 0x38
#define DIK_SPACE 0x39
#define DIK_F1 0x3B
#define DIK_F9 0x43
#define DIK_RCONTROL 0x9D
#define DIK_RMENU 0xB8

#endif // MB_PORT_DINPUT_H
