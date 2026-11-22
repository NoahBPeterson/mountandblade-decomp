#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>                     // IDirectInputDevice8A::GetDeviceData / Acquire, DIDEVICEOBJECTDATA (the mouse device g_keyboard.state)
#include "game/ui/screens/screen_misc.h"
#include "game/ui/widgets/container.h"      // NodeA: the UI root's windows (objs)
#include "game/data/head_rec.h"
#include "game/config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "engine/core/tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "engine/core/vec_table.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "game/input/key_names.h"
#include "game/common/game_globals.h"      // canonical global declarations (g_module_name ...)
#include "game/ui/ui_globals.h"      // UI / core globals
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object

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

#include "engine/physics/rag_model.h"
namespace std {
template<> void _Destroy_range(RagdollBone*, RagdollBone*, allocator<RagdollBone>&, _Nonscalar_ptr_iterator_tag);   // 0x4d10f0 (module_load: the bone list's destroy)

// @ 0x4ebdc0
template void allocator<RagModel>::destroy(RagModel*);

// @ 0x4ebe10
// VC7.1 <memory> _Uninit_copy: copy-construct [first,last) into raw dest through the allocator, destroying the
// constructed prefix and rethrowing if a copy ctor throws (the cleanup funclet is 0x4ebe7a).
template<> RagModel* _Uninit_copy(RagModel* _First, RagModel* _Last, RagModel* _Dest, allocator<RagModel>& _Al,
                                  _Nonscalar_ptr_iterator_tag)
{
    RagModel* _Next = _Dest;
    _TRY_BEGIN
    for (; _First != _Last; ++_Dest, ++_First)
        _Al.construct(_Dest, *_First);   // 0x4e42f0
    _CATCH_ALL
    for (; _Next != _Dest; ++_Next)
        _Al.destroy(_Next);              // 0x4ebdc0
    _RERAISE;
    _CATCH_END
    return _Dest;
}
}

// @ 0x4ebfd0
// One input pass: resets the top overlay, clamps the mouse delta against the screen edges, drains
// DirectInput (GetDeviceData, re-acquiring while the device is unacquired), maps the raw cursor into the
// 0..1 view rectangle, then dispatches every record to the key-flag path (0x4c52f0) or the active
// overlay (+0x8c), and finally fans the frame out to the screen vector, 0x4e6510 and 0x4c5330.
void gui_manager::process(int param)
{
    DWORD count;
    unsigned i;
    int acc0, acc1;

    objs[objs.size() - 1]->clear_c8();

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
    DIDEVICEOBJECTDATA buf[16];
    if (g_keyboard.state) {
        if (g_keyboard.state->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), buf, &count, 0)) {
            while (g_keyboard.state->Acquire() == DIERR_INPUTLOST)
                ;
        }
    }

    this->warp_x = this->saved_x;
    this->warp_y = this->saved_y;
    POINT cur;
    GetCursorPos(&cur);
    this->saved_x = cur.x;
    this->saved_y = cur.y;
    this->prev_mx = this->mouse[0];
    this->mouse[0] = (float)(cur.x - this->view_x) / (float)this->screen_w;
    this->prev_my = this->mouse[1];
    this->mouse[1] = (1.0f - (float)(cur.y - this->view_y) / (float)this->screen_h) * this->aspect;

    if (this->ui_active) {
        this->ui_active->key(1);
    }
    else if (objs.size()) {
        objs[objs.size() - 1]->key(1);
    }

    acc0 = 0;
    this->wheel = 0;
    acc1 = 0;
    this->dirty = 0;

    {
        const DIDEVICEOBJECTDATA* rec = buf;
        for (i = 0; i < count; i++, rec++) {
            unsigned type;
            if (this->dirty)
                break;
            type = rec->dwOfs;
            if (type == 0) {
                acc0 += (int)rec->dwData;
                continue;
            }
            if (type == 4) {
                acc1 += (int)rec->dwData;
                continue;
            }
            if (type == 8) {
                this->wheel += (int)rec->dwData;
                continue;
            }
            if (acc0 || acc1) {
                if (this->ui_active) {
                    this->ui_active->key(1);
                }
                else if (objs.size()) {
                    objs[objs.size() - 1]->key(1);
                }
                acc0 = 0;
                acc1 = 0;
            }
            switch (type) {
            case 0xc:
                if ((char)rec->dwData < 0) {
                    wheel_step(0x10);
                    if (t0.elapsed() < 0.4f) {
                        wheel_step(0x200);
                    }
                    else {
                        gui_timer* cr = &this->t0;
                        cr->start = g_ticks[cr->clock];
                    }
                }
                else {
                    wheel_step(8);
                }
                break;
            case 0xd:
                if ((char)rec->dwData < 0) {
                    wheel_step(0x40);
                    if (t1.elapsed() < 0.4f) {
                        wheel_step(0x400);
                    }
                    else {
                        gui_timer* cr = &this->t1;
                        cr->start = g_ticks[cr->clock];
                    }
                }
                else {
                    wheel_step(0x20);
                }
                break;
            case 0xe:
                if ((char)rec->dwData < 0) {
                    wheel_step(0x100);
                    if (t2.elapsed() < 0.4f) {
                        wheel_step(0x800);
                    }
                    else {
                        gui_timer* cr = &this->t2;
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
            objs[objs.size() - 1]->key(1);
        }
    }

    if (this->ui_active && this->last_key > 0)
        this->ui_active->key(0x1000);

    for (i = 0; i < (unsigned)objs.size(); i++)
        objs[i]->animate(param);

    update(*(float*)&param);   // 0x4e6510
    if (this->close_pending)
        clear_selection();   // 0x4c5330
}

// @ 0x4ec460
// The engine's end-of-frame callback. When a screenshot was requested it saves the frame; otherwise, with a
// camera present and a module-leave pending, it leaves the current module.
void head_rec::frame_hook()
{
    if ((g_keyboard.down[0x1d] || g_keyboard.down[0x9d]) && g_keyboard.down[0xd2] && !g_keyboard.prev[0xd2])
        take_screenshot();
    else if (g_cfg.enable_edit_mode && (g_keyboard.down[0x1d] || g_keyboard.down[0x9d]) && (g_keyboard.down[42] || g_keyboard.down[0x36]) && g_keyboard.down[47] && !g_keyboard.prev[47])
        menu_variables_loaded(0);   // 0x4ebb80
}
