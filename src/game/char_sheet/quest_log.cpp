#include "./quest_log.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4ee840
void __fastcall quest_window::refresh()
{
    if (QUEST_COUNT > 0x80) {
        tstring s;
        s.assign("ui_too_many_quests");
        mb_error(g_strtable.lookup(&s));
    }
    count = 0;
    char* q = QUESTS;
    int i = 0;
    int off = 0;
    while (i < QUEST_COUNT) {
        if (off >= 0x9c00)
            break;
        unsigned f = *(unsigned*)(q + off + 0x64);
        if (f & 0x100) {
            int m = mode;
            if (m == 0 && !(f & 0x200)) {
                indices[count] = i;
                count++;
            } else if (m == 1 && (f & 0x200)) {
                indices[count] = i;
                count++;
            }
        }
        i++;
        off += 0x138;
    }
}

// @ 0x4ee910   (parent, x, y, h, w)
widget* make_scrollbar(widget* parent, float x, float y, float h, float w)
{
    widget* mem = (widget*)operator new(0x188);
    widget* self = mem ? ctor_scrollbar(mem) : 0;
    float pos[2];
    pos[0] = x;
    pos[1] = y;
    self->set_pos(pos);
    *(int*)((char*)self + 0x178) = 1;
    float size[2];
    size[0] = w * g_590b70;
    size[1] = h * g_592758;
    self->set_size(size);
    *(float*)((char*)self + 0x17c) = w * g_592754;
    tstring s1;
    s1.assign("ui_scrollbar");
    mesh* m1 = g_resmgr.get_object(&s1)->clone();
    vector4 col;
    col.x = x;
    col.y = y;
    col.z = h;
    col.w = w;
    m1->set_color(col);
    tstring s2;
    s2.assign("ui_scrollbar_handle");
    mesh* m2 = g_resmgr.get_object(&s2)->clone();
    m2->set_color(col);
    VCALL1(self, 0x80, 0);
    VCALL1(self, 0x80, 2);
    parent->add_child(self);
    return self;
}

// @ 0x4eea90   (parent, pos, size, d)
widget* make_quest_list(widget* parent, const float* pos, const float* size, int d)
{
    widget* mem = (widget*)operator new(0x4a8);
    widget* self = mem ? ctor_list(mem) : 0;
    *(int*)((char*)self + 0x104) |= 0x20;
    if (d)
        *(int*)((char*)self + 0x484) = d;
    *(widget**)((char*)self + 0x490) = parent;      // the owner, also handed to add_child (vtable +0xc8)
    self->add_child(parent);
    self->set_pos(pos);
    self->set_size(size);
    list_setup(self);
    return self;
}

// @ 0x4ee230
void __fastcall quest_window::build()
{
    gui_reset();
    // the window itself: 0x190 bytes, vtable 0x592848, byte +0x188 cleared
    widget* win = (widget*)operator new(0x190);
    win = win ? ctor_window(win) : 0;
    window = win;
    float pos[2];
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    win->set_pos(pos);
    float size[2];
    size[0] = 1.0f;
    size[1] = 0.75f;
    win->set_size(size);
    tstring bg("ui_quests_window");
    win->add_item(0, g_resmgr.get_object(&bg)->clone());
    g_frame_flag = 1;
    gui_set_window(win);

    // the two panels behind the tabs
    widget* p1 = (widget*)operator new(0x178);
    panel1 = p1 ? ctor_panel(p1) : 0;
    pos[0] = 0.58f;
    pos[1] = 0.21875f;
    panel1->set_pos(pos);
    size[0] = 0.39f;
    size[1] = 0.3f;
    panel1->set_size(size);
    int flags = 0x101;
    panel1->set_flags(&flags);
    float tint[2];
    tint[0] = 0.025f;
    tint[1] = 0.025f;
    panel1->set_color(tint);
    win->add_child(panel1);

    widget* p2 = (widget*)operator new(0x178);
    panel2 = p2 ? ctor_panel(p2) : 0;
    panel2->set_pos(pos);
    panel2->set_size(size);
    flags = 0x10;
    panel2->set_flags(&flags);
    panel2->set_color(tint);
    win->add_child(panel2);

    // the three texts
    {
        tstring key("ui_done");
        done = make_text(win, g_strtable.lookup(&key), 0.7f, 0.075f, 0.075f, 0.36f);
    }
    {
        tstring key("ui_active_quests");
        active_tab = make_text(win, g_strtable.lookup(&key), 0.1f, 0.075f, 0.075f, 0.075f);
    }
    {
        tstring key("ui_finished_quests");
        finished_tab = make_text(win, g_strtable.lookup(&key), 0.28f, 0.075f, 0.075f, 0.075f);
    }
    *(int*)((char*)active_tab + 0x184) = 1;
    *(int*)((char*)finished_tab + 0x184) = 1;
    if (mode == 0)
        VCALL0(active_tab, 0x2c);
    else
        VCALL0(finished_tab, 0x2c);
    f250 = -1;
    refresh();

    // the quest list frame: 0x188 bytes, vtable 0x5924a0
    float h = (float)count * g_54ab1c + g_591d54;
    if (h > g_5987e4)
        h = 0.56f;
    widget* fr = (widget*)operator new(0x188);
    if (fr) {
        fr = ctor_panel(fr);
        *(void**)fr = (void*)0x5924a0;
        *(int*)((char*)fr + 0x17c) = 0;
        *(int*)((char*)fr + 0x180) = 0;
        *(int*)((char*)fr + 0x184) = 0;
    }
    frame = fr;
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    frame->set_pos(pos);
    float fsize[2];
    fsize[0] = 0.32f;
    fsize[1] = h;
    frame->set_size(fsize);
    frame->add_item(0, make_color((char*)frame + 0xf0, 0xff000000));

    // one row per listed quest
    for (int i = 0; i < count; i++) {
        char* q = QUEST(indices[i]);
        tstring text(*(tstring*)(q + 0x40));
        if (q[0x50] & 1) {
            char buf[512];
            mb_snprintf(buf, 0x200, "(%d)", *(int*)(q + 0x64) & 0xff);
            text.assign(buf);
        } else {
            char buf2[512];
            mb_snprintf(buf2, 0x200, "%s", text.buf ? text.buf : (char*)&text);
            text.assign(buf2);
        }
        widget* row = (widget*)operator new(0x188);
        if (row) {
            row = ctor_panel(row);
            *(int*)((char*)row + 0x178) = 0;
            *((char*)row + 0x121) = 0;
            *(void**)row = (void*)0x591d58;
            *((char*)row + 0x180) = 0;
            *(int*)((char*)row + 0x184) = 0;
        }
        rows[i] = row;
        float rp[2];
        rp[0] = 0.01f;
        rp[1] = h - (float)i * g_54ab1c;
        row->set_pos(rp);
        float rs[2];
        rs[0] = 0.3f;
        rs[1] = 0.04f;
        row->set_size(rs);
        float tp[2];
        tp[0] = 1.0f;
        tp[1] = 0.0f;
        VCALL2(row, 0xa4, &text, tp);
        int a = -1;
        VCALL1(row, 0x98, (int)&a);
        int b = -1000;
        VCALL1(row, 0x9c, (int)&b);
        frame->add_child(row);
    }
    make_scrollbar(window, 0.48f, 0.1f, 0.02f, 0.6f);
    float lpos[2];
    lpos[0] = -0.02f;
    lpos[1] = 0.58f;
    float lsize[2];
    lsize[0] = 0.1f;
    lsize[1] = 0.11f;
    list = make_quest_list(frame, lpos, lsize, 0x3f800000);
    window->add_child(list);
}

