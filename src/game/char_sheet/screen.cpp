#include "./screen.h"
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "../world_data/thiscall_free.h"   // CALL_STDCALL_ON
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/vec_table.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../key_names.h"
#include "../game_globals.h"      // canonical global declarations (g_module_name ...)
#include "../ui_globals.h"      // UI / core globals
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

// @ 0x4ebc10
// Saves the next free screenshot file, then logs "ui_screenshot_taken_to_value" with the file name, or
// "ui_screenshot_failed" when the save failed.
void __cdecl take_screenshot(void)
{
    char dir[260];
    char name[512];
    WIN32_FIND_DATAA fd;
    char msg[1024];
    tstring* s;
    HANDLE h;

    sprintf(dir, "", g_module_name.c_str());
    screenshots_path(dir);
    sprintf(name, "%s%s", dir, g_module_name.c_str());
    CreateDirectoryA(name, 0);
    for (;;) {
        g_script.screenshot_count++;
        mb_snprintf(name, 0x200, "%s%s\\mb%d.bmp", dir,
                    g_module_name.c_str(),
                    g_script.screenshot_count);
        h = FindFirstFileA(name, &fd);
        if (h == (HANDLE)-1)
            break;
        FindClose(h);
    }

    if (save_screenshot(name, D3DXIFF_BMP)) {
        {
            tstring key("ui_screenshot_taken_to_value");
            s = g_strtable.lookup(key);
            sprintf(msg, s->buf ? s->buf : (char*)&s->buf, name);
        }
        g_gui.add(tstring(msg), -1);
    } else {
        tstring key("ui_screenshot_failed");
        g_gui.add(*g_strtable.lookup(key), -1);
    }
}

// 0x4d10f0 (item_modifier_array_dtor) as the row destructor uses it: (first, model = this+0x10, last, this).

// The member at +0x10 is the row's model: [0]=first, [4]=buffer (freed), [8]=last.
// @ 0x4ebdc0
void __stdcall destroy(UiRow* param)
{
    UiRow* self = param;
    int* l = &self->pad10;
    if (l[1]) {
        // arg 4 is a distinct 4-arg instantiation ICF-folded with the 2-arg range destructor.
        ((void (__cdecl*)(void*, void*, void*, void*))item_modifier_array_dtor)((void*)l[1], (void*)l[2], l, param);   // 0x4d10f0 (reads two)
        ::operator delete((void*)l[1]);
    }
    l[1] = 0;
    l[2] = 0;
    l[3] = 0;
    self->name.~tstring();
}

// @ 0x4ebe10
// VC7.1 <memory> _Uninit_copy: copy-construct [first,last) into raw dest through the row allocator,
// destroying the constructed prefix and rethrowing if a copy ctor throws. The frame slot holding the
// original dest is what lets the compiler's cleanup funclet at 0x4ebe7a walk [next,dest).
UiRow* __cdecl append_rows(UiRow* first, UiRow* last, UiRow* dest, RowAlloc& al)
{
    UiRow* next = dest;
    try {
        for (; first != last; ++dest, ++first)
            ((std::allocator<RagModel>*)(void*)&al)->construct((RagModel*)dest, *(const RagModel*)first);   // 0x4e42f0
    } catch (...) {
        for (; next != dest; ++next)
            CALL_STDCALL_ON(destroy, &al, next);   // 0x4ebdc0 (the allocator in ecx)
        throw;
    }
    return dest;
}

// @ 0x4ebfd0
// One input pass: resets the top overlay, clamps the mouse delta against the screen edges, drains
// DirectInput (GetDeviceData, re-acquiring while the device is unacquired), maps the raw cursor into the
// 0..1 view rectangle, then dispatches every record to the key-flag path (0x4c52f0) or the active
// overlay (+0x8c), and finally fans the frame out to the screen vector, 0x4e6510 and 0x4c5330.
void gui_manager::process(int param)
{
    DiObjectData buf[16];
    unsigned count;
    unsigned i;
    int acc0, acc1;
    gui_timer* cr;

    objs.back()->clear_c8();

    dmx = 0.0f;
    dmy = 0.0f;
    if (this->mouse[0] <= 0.01f) {
        if (g_keyboard.axis[0] < 0.0f) {
            dmx = g_keyboard.axis[0];
            this->mouse_dx = this->mouse_dx + g_keyboard.axis[0];
        }
    }
    else if (this->mouse[0] >= 0.99f) {
        if (g_keyboard.axis[0] > 0.0f) {
            dmx = g_keyboard.axis[0];
            this->mouse_dx = this->mouse_dx + g_keyboard.axis[0];
        }
    }
    else {
        this->mouse_dx = 0.0f;
    }
    if (this->mouse[1] <= 0.01f) {
        if (g_keyboard.axis[1] > 0.0f) {
            dmy = -g_keyboard.axis[1];
            this->mouse_dy = this->mouse_dy - g_keyboard.axis[1];
        }
    }
    else if (this->mouse[1] >= 0.74f) {
        if (g_keyboard.axis[1] < 0.0f) {
            dmy = -g_keyboard.axis[1];
            this->mouse_dy = this->mouse_dy - g_keyboard.axis[1];
        }
    }
    else {
        this->mouse_dy = 0.0f;
    }

    count = 0x10;
    if (((DiDev*)g_keyboard.state)) {
        if (((DiDev*)g_keyboard.state)->vtbl->GetDeviceData(((DiDev*)g_keyboard.state), 0x14, buf, &count, 0)) {
            while (((DiDev*)g_keyboard.state)->vtbl->Acquire(((DiDev*)g_keyboard.state)) == (long)0x8007001e)
                ;
        }
    }

    this->warp_x = this->saved_x;
    this->warp_y = this->saved_y;
    GetCursorPos((POINT*)&this->saved_x);
    this->prev_mx = this->mouse[0];
    this->mouse[0] = (float)(this->saved_x - this->view_x) / (float)this->screen_w;
    this->prev_my = this->mouse[1];
    this->mouse[1] = (1.0f - (float)(this->saved_y - this->view_y) / (float)this->screen_h) * this->aspect;

    if (this->ui_active) {
        this->ui_active->key(1);
    }
    else if (objs.size()) {
        objs.back()->key(1);
    }

    acc0 = 0;
    acc1 = 0;
    this->wheel = 0;
    this->dirty = 0;

    {
        unsigned* rec = (unsigned*)buf + 1;
        for (i = 0; i < count; i++, rec += 5) {
            unsigned type;
            if (this->dirty)
                break;
            type = rec[-1];
            if (type == 0) {
                acc0 += (int)rec[0];
                continue;
            }
            if (type == 4) {
                acc1 += (int)rec[0];
                continue;
            }
            if (type == 8) {
                this->wheel += (int)rec[0];
                continue;
            }
            if (acc0 || acc1) {
                if (this->ui_active) {
                    this->ui_active->key(1);
                }
                else if (objs.size()) {
                    objs.back()->key(1);
                }
                acc0 = 0;
                acc1 = 0;
            }
            switch (type) {
            case 0xc:
                if ((char)rec[0] < 0) {
                    wheel_step(0x10);
                    cr = &this->t0;
                    if (cr->elapsed() < 0.4f) {
                        wheel_step(0x200);
                    }
                    else {
                        cr->start = g_ticks[cr->clock];
                    }
                }
                else {
                    wheel_step(8);
                }
                break;
            case 0xd:
                if ((char)rec[0] < 0) {
                    wheel_step(0x40);
                    cr = &this->t1;
                    if (cr->elapsed() < 0.4f) {
                        wheel_step(0x400);
                    }
                    else {
                        cr->start = g_ticks[cr->clock];
                    }
                }
                else {
                    wheel_step(0x20);
                }
                break;
            case 0xe:
                if ((char)rec[0] < 0) {
                    wheel_step(0x100);
                    cr = &this->t2;
                    if (cr->elapsed() < 0.4f) {
                        wheel_step(0x800);
                    }
                    else {
                        cr->start = g_ticks[cr->clock];
                    }
                }
                else {
                    wheel_step(0x80);
                }
                break;
            }
        }
    }

    if (!this->dirty && (acc0 || acc1)) {
        if (this->ui_active) {
            this->ui_active->key(1);
        }
        else if (objs.size()) {
            objs.back()->key(1);
        }
    }

    if (this->ui_active && this->f1718 > 0)
        this->ui_active->key(0x1000);

    for (i = 0; i < (unsigned)objs.size(); i++)
        objs[i]->animate(param);

    update(*(float*)&param);   // 0x4e6510
    if (this->f1709)
        clear_selection();   // 0x4c5330
}

// @ 0x4ec460
// The engine's end-of-frame callback. When a screenshot was requested it saves the frame; otherwise, with a
// camera present and a module-leave pending, it leaves the current module.
void __cdecl frame_hook(void)
{
    if ((g_keyboard.down[0x1d] || g_keyboard.down[0x9d]) && g_keyboard.down[0xd2] && !g_keyboard.prev[0xd2])
        take_screenshot();
    else if (g_cfg.enable_edit_mode && (g_keyboard.down[0x1d] || g_keyboard.down[0x9d]) && (g_keyboard.down[42] || g_keyboard.down[0x36]) && g_keyboard.down[47] && !g_keyboard.prev[47])
        menu_variables_loaded(0);   // 0x4ebb80
}
