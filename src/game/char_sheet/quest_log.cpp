#include "../../include/image_addrs.h"
#include "./quest_log.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../party2/camera_class.h"      // Camera (the 0x4a8-byte list object built by 0x49deb0)
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../res_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_globals.h"      // UI / core globals
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)
struct GameData;
extern GameData* g_game;                                    // 0x5e2930

// @ 0x4ee840
void __fastcall Rec454::refresh()
{
    if (QUEST_COUNT > 0x80) {
        tstring s("ui_too_many_quests");     // 0x4ee861: tstring(const char*) 0x42bbe0
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
            int m = w04;
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
UiD* make_scroll_track(ContainerBase* parent, float x, float y, float h, float w)
{
    UiD* self = new UiD;
    float pos[2];
    pos[0] = x;
    pos[1] = y;
    self->move_a(pos);
    self->f178 = 1;
    // the size is the unscaled (h, w); the scaled pair is the meshes' colour (0x4ee965..0x4ee998: [esp+0x10] = h,
    // [esp+0x14] = w; h * g_590b70, w * g_592758 and 1.0 into the vector at [esp+0x20], whose w is never set)
    float size[2];
    size[0] = h;
    size[1] = w;
    vector4 col;
    col.x = h * 50.0f;
    col.y = w * 1.5576323f;
    col.z = 1.0f;
    self->set_size(size);
    self->f17c = col.y * 0.31f;
    // the original pushes 0x592748 = "scrollbar" (core_ui_meshes.brf has no "ui_" prefix) into a temporary
    // tstring(const char*) (0x42bbe0), destroyed right after the clone
    mesh* m1 = g_resmgr.get_object(&tstring("scrollbar"))->clone();
    m1->set_color(col);
    // 0x592734 = "scrollbar_handle", a temporary the same way
    mesh* m2 = g_resmgr.get_object(&tstring("scrollbar_handle"))->clone();
    m2->set_color(col);
    // the original calls vtable slot 0x80 = MissionUi::set_track(int i, mesh* p) with *both* arguments
    // (0x4eea5e: push edi/push 0/ecx=esi; 0x4eea6b: push ebx/push 2), which is why a one-argument call through a
    // cdecl cast handed the object over as the index (fault at [esi + edi*4 + 0x108]).
    self->set_track(0, m1);         // vtable +0x80
    self->set_track(2, m2);
    parent->add_child(self);
    return self;
}

// @ 0x4eea90   (parent, pos, size, d)
Camera* make_quest_list(ContainerBase* parent, const float* pos, const float* size, UiD* track)
{
    Camera* cam = new Camera;    // 0x49deb0
    cam->f104 |= 0x20;
    if (track)
        cam->follow[1] = track;
    cam->target = parent;      // the owner, also handed to add_child (vtable +0xc8)
    cam->add_child(parent);
    cam->move_a(pos);
    cam->set_xy(size);
    ((Camera*)cam)->draw();   // 0x488d50
    return cam;
}

// @ 0x4ee230
void Rec454::init_screen()
{
    g_gui.reset();   // 0x42d1b0
    // the window itself: 0x190 bytes, vtable 0x592848, byte +0x188 cleared
    NodeA* win = new NodeA;                 // 0x4931a0, vptr 0x592848 (0x4ee268)
    if (win)
        win->f188 = 0;                      // 0x4ee26e
    window = win;
    float pos[2];
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    win->move_a(pos);
    float size[2];
    size[0] = 1.0f;
    size[1] = 0.75f;
    win->set_xy(size);
    mesh* bgm;
    {
        tstring bg("quests_window");   // 0x598810 (no "ui_" prefix); destroyed right after the clone
        bgm = g_resmgr.get_object(&bg)->clone();
    }
    win->set_mesh(0, bgm);
    g_gui.dirty = 1;
    g_gui.objs.push_back((MissionUi* const&)win);   // 0x49b120

    // the two panels behind the tabs
    panel1 = new MissionUi;
    pos[0] = 0.58f;
    pos[1] = 0.21875f;
    panel1->move_a(pos);
    size[0] = 0.39f;
    size[1] = 0.3f;
    panel1->set_xy(size);
    int flags = 0x101;
    panel1->set148(&flags);
    float tint[2];
    tint[0] = 0.025f;
    tint[1] = 0.025f;
    panel1->set_size(tint);
    win->add_child(panel1);

    panel2 = new MissionUi;
    panel2->move_a(pos);
    panel2->set_xy(size);
    flags = 0x10;
    panel2->set148(&flags);
    panel2->set_size(tint);
    win->add_child(panel2);

    // the three texts
    {
        tstring key("ui_done");
        done = new_medium_button(win, g_strtable.lookup(key), 0.7f, 0.09f, 0.04f);
    }
    {
        tstring key("ui_active_quests");
        active_tab = new_medium_button(win, g_strtable.lookup(key), 0.1f, 0.04f, 0.04f);
    }
    {
        tstring key("ui_finished_quests");
        finished_tab = new_medium_button(win, g_strtable.lookup(key), 0.28f, 0.04f, 0.04f);
    }
    active_tab->set_flag();
    finished_tab->set_flag();
    if (w04 == 0)
        active_tab->set15c(1);     // 0x4ee532: push 1; call [vtbl+0x2c]
    else
        finished_tab->set15c(1);
    f250 = -1;
    refresh();

    // the quest list frame: 0x188 bytes, vtable 0x5924a0
    float h = (float)count * 0.04f + 0.09f;
    if (h < 0.56f)     // 0x4ee565: jne taken (0.56 <= h) keeps h; otherwise h = 0.56
        h = 0.56f;
    frame = new ContainerBase;                  // vptr 0x5924a0
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    frame->move_a(pos);
    float fsize[2];
    fsize[0] = 0.32f;
    fsize[1] = h;
    frame->set_xy(fsize);
    frame->set_mesh(0, make_background(frame->screen, (int)0xff000000));

    // one row per listed quest
    h -= 0.05f;     // 0x4ee623: fsub [0x54b4d0] (0.05) before the loop
    for (int i = 0; i < count; i++) {
        char* q = QUEST(indices[i]);
        tstring text(*(tstring*)(q + 0x40));
        char buf[512];
        if (q[0x50] & 1) {
            mb_snprintf(buf, 0x200, "%s (%d%%)", text.buf ? text.buf : (char*)&text, *(int*)(q + 0x64) & 0xff);
        } else {
            mb_snprintf(buf, 0x200, "%s", text.buf ? text.buf : (char*)&text);
        }
        UiA* row = new UiA;                     // vptr 0x591d58
        rows[i] = row;
        float rp[2];
        rp[0] = 0.01f;
        rp[1] = h - (float)i * 0.04f;
        row->move_a(rp);
        float rs[2];
        rs[0] = 0.3f;
        rs[1] = 0.04f;
        row->set_xy(rs);
        {
            // set_text(out, text): `out` is the hidden pointer of the 8-byte result (never initialised, 0x4ee750),
            // text is a temporary tstring built from the formatted buffer and destroyed right after the call
            char ret[8];
            tstring shown(buf);
            row->set_text((float*)ret, &shown);
        }
        unsigned a = 0xffffffff;
        row->set_color(&a);
        int b = -3941;     // 0xfffff09b
        row->set_target(&b);
        frame->add_child(row);
    }
    UiD* track = make_scroll_track(window, 0.48f, 0.1f, 0.02f, 0.6f);
    float lpos[2];
    lpos[0] = 0.1f;     // 0x4ee7ea..: pos = [esp+0x4c] (0.1, 0.11), size = [esp+0x44] (0.32, 0.58)
    lpos[1] = 0.11f;
    float lsize[2];
    lsize[0] = 0.32f;
    lsize[1] = 0.58f;
    list = make_quest_list(frame, lpos, lsize, track);
    window->add_child(list);
}

