// port/platform/keymap.cpp: one table relating SDL scancodes, DirectInput DIK codes and Win32 virtual-key codes.
#include "mbp_internal.h"

namespace {
struct K { SDL_Scancode sc; unsigned char dik; unsigned char vk; bool ext; };
const K kTable[] = {
    {SDL_SCANCODE_ESCAPE, 0x01, 0x1B, false},
    {SDL_SCANCODE_1, 0x02, '1', false}, {SDL_SCANCODE_2, 0x03, '2', false}, {SDL_SCANCODE_3, 0x04, '3', false},
    {SDL_SCANCODE_4, 0x05, '4', false}, {SDL_SCANCODE_5, 0x06, '5', false}, {SDL_SCANCODE_6, 0x07, '6', false},
    {SDL_SCANCODE_7, 0x08, '7', false}, {SDL_SCANCODE_8, 0x09, '8', false}, {SDL_SCANCODE_9, 0x0A, '9', false},
    {SDL_SCANCODE_0, 0x0B, '0', false},
    {SDL_SCANCODE_MINUS, 0x0C, 0xBD, false}, {SDL_SCANCODE_EQUALS, 0x0D, 0xBB, false},
    {SDL_SCANCODE_BACKSPACE, 0x0E, 0x08, false}, {SDL_SCANCODE_TAB, 0x0F, 0x09, false},
    {SDL_SCANCODE_Q, 0x10, 'Q', false}, {SDL_SCANCODE_W, 0x11, 'W', false}, {SDL_SCANCODE_E, 0x12, 'E', false},
    {SDL_SCANCODE_R, 0x13, 'R', false}, {SDL_SCANCODE_T, 0x14, 'T', false}, {SDL_SCANCODE_Y, 0x15, 'Y', false},
    {SDL_SCANCODE_U, 0x16, 'U', false}, {SDL_SCANCODE_I, 0x17, 'I', false}, {SDL_SCANCODE_O, 0x18, 'O', false},
    {SDL_SCANCODE_P, 0x19, 'P', false},
    {SDL_SCANCODE_LEFTBRACKET, 0x1A, 0xDB, false}, {SDL_SCANCODE_RIGHTBRACKET, 0x1B, 0xDD, false},
    {SDL_SCANCODE_RETURN, 0x1C, 0x0D, false},
    {SDL_SCANCODE_LCTRL, 0x1D, 0x11, false},
    {SDL_SCANCODE_A, 0x1E, 'A', false}, {SDL_SCANCODE_S, 0x1F, 'S', false}, {SDL_SCANCODE_D, 0x20, 'D', false},
    {SDL_SCANCODE_F, 0x21, 'F', false}, {SDL_SCANCODE_G, 0x22, 'G', false}, {SDL_SCANCODE_H, 0x23, 'H', false},
    {SDL_SCANCODE_J, 0x24, 'J', false}, {SDL_SCANCODE_K, 0x25, 'K', false}, {SDL_SCANCODE_L, 0x26, 'L', false},
    {SDL_SCANCODE_SEMICOLON, 0x27, 0xBA, false}, {SDL_SCANCODE_APOSTROPHE, 0x28, 0xDE, false},
    {SDL_SCANCODE_GRAVE, 0x29, 0xC0, false},
    {SDL_SCANCODE_LSHIFT, 0x2A, 0x10, false},
    {SDL_SCANCODE_BACKSLASH, 0x2B, 0xDC, false},
    {SDL_SCANCODE_NONUSBACKSLASH, 0x56, 0xE2, false},
    {SDL_SCANCODE_Z, 0x2C, 'Z', false}, {SDL_SCANCODE_X, 0x2D, 'X', false}, {SDL_SCANCODE_C, 0x2E, 'C', false},
    {SDL_SCANCODE_V, 0x2F, 'V', false}, {SDL_SCANCODE_B, 0x30, 'B', false}, {SDL_SCANCODE_N, 0x31, 'N', false},
    {SDL_SCANCODE_M, 0x32, 'M', false},
    {SDL_SCANCODE_COMMA, 0x33, 0xBC, false}, {SDL_SCANCODE_PERIOD, 0x34, 0xBE, false}, {SDL_SCANCODE_SLASH, 0x35, 0xBF, false},
    {SDL_SCANCODE_RSHIFT, 0x36, 0x10, false},
    {SDL_SCANCODE_KP_MULTIPLY, 0x37, 0x6A, false},
    {SDL_SCANCODE_LALT, 0x38, 0x12, false},
    {SDL_SCANCODE_SPACE, 0x39, 0x20, false},
    {SDL_SCANCODE_CAPSLOCK, 0x3A, 0x14, false},
    {SDL_SCANCODE_F1, 0x3B, 0x70, false}, {SDL_SCANCODE_F2, 0x3C, 0x71, false}, {SDL_SCANCODE_F3, 0x3D, 0x72, false},
    {SDL_SCANCODE_F4, 0x3E, 0x73, false}, {SDL_SCANCODE_F5, 0x3F, 0x74, false}, {SDL_SCANCODE_F6, 0x40, 0x75, false},
    {SDL_SCANCODE_F7, 0x41, 0x76, false}, {SDL_SCANCODE_F8, 0x42, 0x77, false}, {SDL_SCANCODE_F9, 0x43, 0x78, false},
    {SDL_SCANCODE_F10, 0x44, 0x79, false},
    {SDL_SCANCODE_NUMLOCKCLEAR, 0x45, 0x90, false}, {SDL_SCANCODE_SCROLLLOCK, 0x46, 0x91, false},
    {SDL_SCANCODE_KP_7, 0x47, 0x67, false}, {SDL_SCANCODE_KP_8, 0x48, 0x68, false}, {SDL_SCANCODE_KP_9, 0x49, 0x69, false},
    {SDL_SCANCODE_KP_MINUS, 0x4A, 0x6D, false},
    {SDL_SCANCODE_KP_4, 0x4B, 0x64, false}, {SDL_SCANCODE_KP_5, 0x4C, 0x65, false}, {SDL_SCANCODE_KP_6, 0x4D, 0x66, false},
    {SDL_SCANCODE_KP_PLUS, 0x4E, 0x6B, false},
    {SDL_SCANCODE_KP_1, 0x4F, 0x61, false}, {SDL_SCANCODE_KP_2, 0x50, 0x62, false}, {SDL_SCANCODE_KP_3, 0x51, 0x63, false},
    {SDL_SCANCODE_KP_0, 0x52, 0x60, false}, {SDL_SCANCODE_KP_PERIOD, 0x53, 0x6E, false},
    {SDL_SCANCODE_F11, 0x57, 0x7A, false}, {SDL_SCANCODE_F12, 0x58, 0x7B, false},
    {SDL_SCANCODE_F13, 0x64, 0x7C, false}, {SDL_SCANCODE_F14, 0x65, 0x7D, false}, {SDL_SCANCODE_F15, 0x66, 0x7E, false},
    {SDL_SCANCODE_KP_EQUALS, 0x8D, 0x92, false},
    {SDL_SCANCODE_KP_ENTER, 0x9C, 0x0D, true},
    {SDL_SCANCODE_RCTRL, 0x9D, 0x11, true},
    {SDL_SCANCODE_KP_DIVIDE, 0xB5, 0x6F, true},
    {SDL_SCANCODE_PRINTSCREEN, 0xB7, 0x2C, true},
    {SDL_SCANCODE_RALT, 0xB8, 0x12, true},
    {SDL_SCANCODE_PAUSE, 0xC5, 0x13, false},
    {SDL_SCANCODE_HOME, 0xC7, 0x24, true}, {SDL_SCANCODE_UP, 0xC8, 0x26, true}, {SDL_SCANCODE_PAGEUP, 0xC9, 0x21, true},
    {SDL_SCANCODE_LEFT, 0xCB, 0x25, true}, {SDL_SCANCODE_RIGHT, 0xCD, 0x27, true},
    {SDL_SCANCODE_END, 0xCF, 0x23, true}, {SDL_SCANCODE_DOWN, 0xD0, 0x28, true}, {SDL_SCANCODE_PAGEDOWN, 0xD1, 0x22, true},
    {SDL_SCANCODE_INSERT, 0xD2, 0x2D, true}, {SDL_SCANCODE_DELETE, 0xD3, 0x2E, true},
    {SDL_SCANCODE_LGUI, 0xDB, 0x5B, true}, {SDL_SCANCODE_RGUI, 0xDC, 0x5C, true}, {SDL_SCANCODE_APPLICATION, 0xDD, 0x5D, true},
};
const K* by_sc[SDL_SCANCODE_COUNT];
const K* by_dik[256];
bool built = false;
void build()
{
    if (built) return;
    for (const K& k : kTable) {
        by_sc[k.sc] = &k;
        by_dik[k.dik] = &k;
    }
    built = true;
}
}   // namespace

namespace mbp {
int keymap_vk_for_scancode(SDL_Scancode sc, bool* ext)
{
    build();
    if ((int)sc < 0 || (int)sc >= SDL_SCANCODE_COUNT || !by_sc[sc]) { if (ext) *ext = false; return 0; }
    if (ext) *ext = by_sc[sc]->ext;
    return by_sc[sc]->vk;
}
int keymap_dik_for_scancode(SDL_Scancode sc)
{
    build();
    return ((int)sc >= 0 && (int)sc < SDL_SCANCODE_COUNT && by_sc[sc]) ? by_sc[sc]->dik : 0;
}
SDL_Scancode keymap_scancode_for_dik(int dik)
{
    build();
    return (dik > 0 && dik < 256 && by_dik[dik]) ? by_dik[dik]->sc : SDL_SCANCODE_UNKNOWN;
}
// the left-hand key for the generic Shift / Control / Menu codes
SDL_Scancode keymap_scancode_for_vk(int vk)
{
    build();
    for (const K& k : kTable)
        if (k.vk == vk) return k.sc;
    return SDL_SCANCODE_UNKNOWN;
}
}   // namespace mbp
