#include "./screen.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)

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

    _sprintf(dir, "", g_save_prefix ? g_save_prefix : (char*)&g_save_prefix);
    screenshots_path(dir);
    _sprintf(name, "%s%s", dir, g_save_prefix ? g_save_prefix : (char*)&g_save_prefix);
    CreateDirectoryA(name, 0);
    for (;;) {
        g_screenshots_counter++;
        mb_snprintf(name, 0x200, "%s%s\\mb%d.bmp", dir,
                    g_save_prefix ? g_save_prefix : (char*)&g_save_prefix,
                    g_screenshots_counter);
        h = FindFirstFileA(name, &fd);
        if (h == (HANDLE)-1)
            break;
        FindClose(h);
    }

    if (save_screenshot(name, 0)) {
        {
            tstring key("ui_screenshot_taken_to_value");
            s = g_strtable.lookup(key);
            _sprintf(msg, s->buf ? s->buf : (char*)&s->buf, name);
        }
        g_msglog.add(tstring(msg), -1);
    } else {
        tstring key("ui_screenshot_failed");
        g_msglog.add(*g_strtable.lookup(key), -1);
    }
}

// 0x4d10f0 as the row destructor uses it: (first, model = this+0x10, last, this).
void __cdecl destroy_row_range(void* first, void* model, void* last, void* self);

// The member at +0x10 is the row's model: [0]=first, [4]=buffer (freed), [8]=last.
// @ 0x4ebdc0
void __stdcall dtor_ui_row(UiRow* param)
{
    UiRow* self = param;
    int* l = &self->pad10;
    if (l[1]) {
        // arg 4 is a distinct 4-arg instantiation ICF-folded with the 2-arg range destructor.
        destroy_row_range((void*)l[1], (void*)l[2], l, *(UiRow* const volatile*)&param);
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
            al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next)
            al.destroy(next);
        throw;
    }
    return dest;
}

// @ 0x4ebea0
Screen::Screen()
{
    for (int i = 0; i < 5; i++)
        ((Hint*)((char*)this + 0x14))[i].Hint::Hint();
    ((Hint*)((char*)this + 0x30c))->Hint::Hint();
    *(int*)((char*)this + 0x3a8) = 0;
    *(int*)((char*)this + 0x3ac) = 0;
    *(int*)((char*)this + 0x3b0) = 0;
    ((vec_table*)((char*)this + 0x4183d8))->vec_table::vec_table();
    ((mesh*)((char*)this + 0x4183e0))->mesh::mesh();
    ((scene*)((char*)this + 0x418640))->scene::scene();
    *(xform**)((char*)this + 0x4183d4) = new xform[0x20];
    mat3 m;
    xform p;
    p.ex = vector4(0, 0, 0);
    for (int j = 0; j < 0x800; j += 0x40) {
        p.m = *identity3(&m);
        *MAT_XFORM(j) = p;
        xform* d = MAT_XFORM(j);
        d->m.s.w = 0;
        d->m.f.w = 0;
        d->m.u.w = 0;
        d->ex.w = 1.0f;
    }
    *(int*)((char*)this + 0x4183c4) = 0;
    *(int*)((char*)this + 0x4183c0) = 0;
    *(int*)((char*)this + 0x418828) = 0;
}

// @ 0x4ebfd0
// One input pass: resets the top overlay, clamps the mouse delta against the screen edges, drains
// DirectInput (GetDeviceData, re-acquiring while the device is unacquired), maps the raw cursor into the
// 0..1 view rectangle, then dispatches every record to the key-flag path (0x4c52f0) or the active
// overlay (+0x8c), and finally fans the frame out to the screen vector, 0x4e6510 and 0x4c5330.
void Screen::process(int param)
{
    DiObjectData buf[16];
    unsigned count;
    unsigned i;
    int acc0, acc1;
    ClickRec* cr;

    vec_back(this)->load();

    this->dmx = 0.0f;
    this->dmy = 0.0f;
    if (this->mx <= g_eps) {
        if (g_a178d4 < 0.0f) {
            this->dmx = g_a178d4;
            this->ax = this->ax + g_a178d4;
        }
    }
    else if (this->mx >= g_ninety_nine) {
        if (g_a178d4 > 0.0f) {
            this->dmx = g_a178d4;
            this->ax = this->ax + g_a178d4;
        }
    }
    else {
        this->ax = 0.0f;
    }
    if (this->my <= g_eps) {
        if (g_a178d8 > 0.0f) {
            this->dmy = -g_a178d8;
            this->ay = this->ay - g_a178d8;
        }
    }
    else if (this->my >= g_seventy_four) {
        if (g_a178d8 < 0.0f) {
            this->dmy = -g_a178d8;
            this->ay = this->ay - g_a178d8;
        }
    }
    else {
        this->ay = 0.0f;
    }

    count = 0x10;
    if (g_a170d0) {
        if (g_a170d0->vtbl->GetDeviceData(g_a170d0, 0x14, buf, &count, 0)) {
            while (g_a170d0->vtbl->Acquire(g_a170d0) == (long)0x8007001e)
                ;
        }
    }

    this->prev_cx = this->cur_mx;
    this->prev_cy = this->cur_my;
    GetCursorPos((POINT*)&this->cur_mx);
    this->prev_mx = this->mx;
    this->mx = (float)(this->cur_mx - this->view_w) / (float)this->view_x;
    this->prev_my = this->my;
    this->my = (g_one - (float)(this->cur_my - this->view_y) / (float)this->view_h) * this->f514;

    if (this->cur) {
        this->cur->key(1);
    }
    else if (vec_size(this)) {
        vec_back(this)->key(1);
    }

    acc0 = 0;
    acc1 = 0;
    this->f1710 = 0;
    this->f1708 = 0;

    {
        unsigned* rec = (unsigned*)buf + 1;
        for (i = 0; i < count; i++, rec += 5) {
            unsigned type;
            if (this->f1708)
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
                this->f1710 += (int)rec[0];
                continue;
            }
            if (acc0 || acc1) {
                if (this->cur) {
                    this->cur->key(1);
                }
                else if (vec_size(this)) {
                    vec_back(this)->key(1);
                }
                acc0 = 0;
                acc1 = 0;
            }
            switch (type) {
            case 0xc:
                if ((char)rec[0] < 0) {
                    this->key_flag(0x10);
                    cr = &this->click3;
                    if (cr->elapsed() < g_four_tenths) {
                        this->key_flag(0x200);
                    }
                    else {
                        cr->x = g_5e2948[cr->idx * 2];
                        cr->y = g_5e2948[cr->idx * 2 + 1];
                    }
                }
                else {
                    this->key_flag(8);
                }
                break;
            case 0xd:
                if ((char)rec[0] < 0) {
                    this->key_flag(0x40);
                    cr = &this->click2;
                    if (cr->elapsed() < g_four_tenths) {
                        this->key_flag(0x400);
                    }
                    else {
                        cr->x = g_5e2948[cr->idx * 2];
                        cr->y = g_5e2948[cr->idx * 2 + 1];
                    }
                }
                else {
                    this->key_flag(0x20);
                }
                break;
            case 0xe:
                if ((char)rec[0] < 0) {
                    this->key_flag(0x100);
                    cr = &this->click1;
                    if (cr->elapsed() < g_four_tenths) {
                        this->key_flag(0x800);
                    }
                    else {
                        cr->x = g_5e2948[cr->idx * 2];
                        cr->y = g_5e2948[cr->idx * 2 + 1];
                    }
                }
                else {
                    this->key_flag(0x80);
                }
                break;
            }
        }
    }

    if (!this->f1708 && (acc0 || acc1)) {
        if (this->cur) {
            this->cur->key(1);
        }
        else if (vec_size(this)) {
            vec_back(this)->key(1);
        }
    }

    if (this->cur && this->f1718 > 0)
        this->cur->key(0x1000);

    for (i = 0; i < (unsigned)vec_size(this); i++)
        ((ScreenVT*)this->obj_first[i])->update(param);

    this->post(param);
    if (this->f1709)
        this->finish();
}

// @ 0x4ec460
// The engine's end-of-frame callback. When a screenshot was requested it saves the frame; otherwise, with a
// camera present and a module-leave pending, it leaves the current module.
void __cdecl frame_hook(void)
{
    if ((g_a17148 || g_a17348) && g_a1741c && !g_a1781c)
        take_screenshot();
    else if (g_5a9fb0 && (g_a17148 || g_a17348) && (g_a1717c || g_a171ac) && g_a17190 && !g_a17590)
        leave_module(0);
}
