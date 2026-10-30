#include "./quest_log.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../res_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

// @ 0x4ee840
void __fastcall quest_window::refresh()
{
    if (QUEST_COUNT > 0x80) {
        tstring s;
        s.assign("ui_too_many_quests");
        tstring* msg = g_strtable.lookup(s);
        mb_error(msg->buf ? msg->buf : (char*)&msg->buf);
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
widget* make_scroll_track(widget* parent, float x, float y, float h, float w)
{
    widget* mem = (widget*)operator new(0x188);
    widget* self = mem ? (widget*)new (mem, raw_place()) UiD : 0;
    float pos[2];
    pos[0] = x;
    pos[1] = y;
    self->set_pos(pos);
    *(int*)((char*)self + 0x178) = 1;
    // the size is the unscaled (h, w); the scaled pair is the meshes' colour (0x4ee965..0x4ee998: [esp+0x10] = h,
    // [esp+0x14] = w; h * g_590b70, w * g_592758 and 1.0 into the vector at [esp+0x20], whose w is never set)
    float size[2];
    size[0] = h;
    size[1] = w;
    vector4 col;
    col.x = h * g_590b70;
    col.y = w * g_592758;
    col.z = 1.0f;
    self->set_size(size);
    *(float*)((char*)self + 0x17c) = col.y * g_592754;
    // the original pushes 0x592748 = "scrollbar" (core_ui_meshes.brf has no "ui_" prefix) into a temporary
    // tstring(const char*) (0x42bbe0), destroyed right after the clone
    mesh* m1 = g_resmgr.get_object((const rgl::string*)&tstring("scrollbar"))->clone();
    m1->set_color(col);
    // 0x592734 = "scrollbar_handle", a temporary the same way
    mesh* m2 = g_resmgr.get_object((const rgl::string*)&tstring("scrollbar_handle"))->clone();
    m2->set_color(col);
    // the original calls vtable slot 0x80 = MissionUi::set_track(int i, mesh* p) with *both* arguments
    // (0x4eea5e: push edi/push 0/ecx=esi; 0x4eea6b: push ebx/push 2), which is why a one-argument call through a
    // cdecl cast handed the object over as the index (fault at [esi + edi*4 + 0x108]).
    self->add_item2(0, m1);         // vtable +0x80 (set_track), called through the vtable as the original does
    self->add_item2(2, m2);
    parent->add_child(self);
    return self;
}

// @ 0x4eea90   (parent, pos, size, d)
widget* make_quest_list(widget* parent, const float* pos, const float* size, int d)
{
    widget* mem = (widget*)operator new(0x4a8);
    widget* self = mem ? (widget*)new (mem, raw_place()) c49deb0::Camera : 0;
    *(int*)((char*)self + 0x104) |= 0x20;
    if (d)
        *(int*)((char*)self + 0x484) = d;
    *(widget**)((char*)self + 0x490) = parent;      // the owner, also handed to add_child (vtable +0xc8)
    self->add_child(parent);
    self->set_pos(pos);
    self->set_size(size);
    ((MissionPanel*)self)->draw();   // 0x488d50
    return self;
}

// @ 0x4ee230
void __fastcall quest_window::build()
{
    ((gui_manager*)0x5d7080)->reset();   // 0x42d1b0
    // the window itself: 0x190 bytes, vtable 0x592848, byte +0x188 cleared
    widget* win = (widget*)operator new(0x190);
    if (win) {
        ((ContainerW*)win)->construct();     // 0x4931a0
        *(void**)win = (void*)0x592848;      // 0x4ee268
        *((char*)win + 0x188) = 0;           // 0x4ee26e
    }
    window = win;
    float pos[2];
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    win->set_pos(pos);
    float size[2];
    size[0] = 1.0f;
    size[1] = 0.75f;
    win->set_size(size);
    mesh* bgm;
    {
        tstring bg("quests_window");   // 0x598810 (no "ui_" prefix); destroyed right after the clone
        bgm = g_resmgr.get_object((const rgl::string*)&bg)->clone();
    }
    win->add_item(0, bgm);
    g_frame_flag = 1;
    ((NodeVec*)0x5d7080)->push_back((NodeA* const&)win);   // 0x49b120

    // the two panels behind the tabs
    widget* p1 = (widget*)operator new(0x178);
    panel1 = p1 ? (widget*)new (p1, raw_place()) MissionUi : 0;
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
    panel2 = p2 ? (widget*)new (p2, raw_place()) MissionUi : 0;
    panel2->set_pos(pos);
    panel2->set_size(size);
    flags = 0x10;
    panel2->set_flags(&flags);
    panel2->set_color(tint);
    win->add_child(panel2);

    // the three texts
    {
        tstring key("ui_done");
        done = (widget*)new_medium_button((MissionUi*)win, g_strtable.lookup(key), 0.7f, 0.09f, 0.04f);
    }
    {
        tstring key("ui_active_quests");
        active_tab = (widget*)new_medium_button((MissionUi*)win, g_strtable.lookup(key), 0.1f, 0.04f, 0.04f);
    }
    {
        tstring key("ui_finished_quests");
        finished_tab = (widget*)new_medium_button((MissionUi*)win, g_strtable.lookup(key), 0.28f, 0.04f, 0.04f);
    }
    *(int*)((char*)active_tab + 0x184) = 1;
    *(int*)((char*)finished_tab + 0x184) = 1;
    if (mode == 0)
        active_tab->set_focus(1);     // 0x4ee532: push 1; call [vtbl+0x2c]
    else
        finished_tab->set_focus(1);
    f250 = -1;
    refresh();

    // the quest list frame: 0x188 bytes, vtable 0x5924a0
    float h = (float)count * g_54ab1c + g_591d54;
    if (h < g_5987e4)     // 0x4ee565: jne taken (0.56 <= h) keeps h; otherwise h = 0.56
        h = 0.56f;
    widget* fr = (widget*)operator new(0x188);
    if (fr) {
        fr = (widget*)new (fr, raw_place()) MissionUi;
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
    frame->add_item(0, make_background((float*)((char*)frame + 0xf0), (int)0xff000000));

    // one row per listed quest
    h -= g_54b4d0;     // 0x4ee623: fsub [0x54b4d0] (0.05) before the loop
    for (int i = 0; i < count; i++) {
        char* q = QUEST(indices[i]);
        tstring text(*(tstring*)(q + 0x40));
        char buf[512];
        if (q[0x50] & 1) {
            mb_snprintf(buf, 0x200, "%s (%d%%)", text.buf ? text.buf : (char*)&text, *(int*)(q + 0x64) & 0xff);
        } else {
            mb_snprintf(buf, 0x200, "%s", text.buf ? text.buf : (char*)&text);
        }
        widget* row = (widget*)operator new(0x188);
        if (row) {
            row = (widget*)new (row, raw_place()) MissionUi;
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
        {
            // set_text(out, text): `out` is the hidden pointer of the 8-byte result (never initialised, 0x4ee750),
            // text is a temporary tstring built from the formatted buffer and destroyed right after the call
            char ret[8];
            tstring shown(buf);
            row->set_text((tstring*)ret, &shown);
        }
        int a = -1;
        row->set_value_a(&a);
        int b = -3941;     // 0xfffff09b
        row->set_value_b(&b);
        frame->add_child(row);
    }
    widget* track = make_scroll_track(window, 0.48f, 0.1f, 0.02f, 0.6f);
    float lpos[2];
    lpos[0] = 0.1f;     // 0x4ee7ea..: pos = [esp+0x4c] (0.1, 0.11), size = [esp+0x44] (0.32, 0.58)
    lpos[1] = 0.11f;
    float lsize[2];
    lsize[0] = 0.32f;
    lsize[1] = 0.58f;
    list = make_quest_list(frame, lpos, lsize, (int)track);
    window->add_child(list);
}

