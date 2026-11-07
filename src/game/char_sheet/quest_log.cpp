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
    for (int i = 0; i < QUEST_COUNT; i++) {
        int off = i * 0x138;
        if (off >= 0x9c00)
            break;
        unsigned f = *(unsigned*)(QUESTS + off + 0x64);
        if (f & 0x100) {
            if (w04 == 0) {
                if (!(f & 0x200))
                    goto add;
            }
            if (w04 == 1) {
                if (f & 0x200)
                    goto add;
            }
            goto next;
        add:
            indices[count] = i;
            count++;
        next:;
        }
    }
}

// @ 0x4ee910   (parent, x, y, h, w)
UiD* make_scroll_track(ContainerBase* parent, float x, float y, float h, float w)
{
    UiD* self = new UiD;
    {
        float pos[2];
        pos[0] = x;
        pos[1] = y;
        self->f178 = 1;
        self->move_a(pos);
    }
    // the size is the unscaled (h, w); the scaled pair is the meshes' colour (0x4ee965..0x4ee998: [esp+0x10] = h,
    // [esp+0x14] = w; h * g_590b70, w * g_592758 and 1.0 into the vector at [esp+0x20], whose w is never set)
    vector4 col;
    col.x = h * 50.0f;
    float sy = w * 1.5576323f;
    col.y = sy;
    col.z = 1.0f;
    {
        float size[2];
        size[0] = h;
        size[1] = w;
        self->set_xy(size);             // vtable +0x64 (0x4ee99c), not set_size +0x6c
    }
    self->f17c = sy * 0.31f;
    // the original pushes 0x592748 = "scrollbar" (core_ui_meshes.brf has no "ui_" prefix) into a tstring
    // (0x42bbe0) that lives in a scope and is destroyed right after the clone
    mesh* m1;
    {
        tstring name("scrollbar");
        m1 = g_resmgr.get_object(&name)->clone();
    }
    m1->set_color(col);
    // 0x592734 = "scrollbar_handle", a scoped tstring the same way
    mesh* m2;
    {
        tstring name("scrollbar_handle");
        m2 = g_resmgr.get_object(&name)->clone();
    }
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

static inline const float* float2_arg(const float2& v) { return &v.x; }
static inline float quest_list_height(float h) { return 0.56f > h ? 0.56f : h; }

// the list frame expands the container constructor (0x4ee590); NodeA calls 0x4931a0

// @ 0x4ee230
void Rec454::init_screen()
{
    g_gui.reset();   // 0x42d1b0
    // the window itself: 0x190 bytes, vtable 0x592848, byte +0x188 cleared
    window = new NodeA(0);                  // 0x4931a0, vptr 0x592848 (0x4ee268)
    {
        float v[2] = { 0.0f, 0.0f };
        window->move_a(v);
    }
    {
        float v[2] = { 1.0f, 0.75f };
        window->set_xy(v);
    }
    rgl::mesh* bgm;
    {
        tstring bg("quests_window");   // 0x598810 (no "ui_" prefix); destroyed right after the clone
        bgm = g_resmgr.get_object(&bg)->clone();
    }
    window->set_mesh(0, bgm);
    { MissionUi* root_ = (MissionUi*)window; g_gui.dirty = 1; g_gui.objs.push_back(root_); }   // 0x49b120

    // the two panels behind the tabs
    panel1 = new MissionUi;
    {
        float v[2] = { 0.58f, 0.21875f };
        panel1->move_a(v);
    }
    {
        float v[2] = { 0.39f, 0.3f };
        panel1->set_xy(v);
    }
    {
        int flags = 0x101;
        panel1->set148(&flags);
    }
    {
        float v[2] = { 0.025f, 0.025f };
        panel1->set_size(v);
    }
    window->add_child(panel1);

    panel2 = new MissionUi;
    {
        float v[2] = { 0.58f, 0.21875f };
        panel2->move_a(v);
    }
    {
        float v[2] = { 0.39f, 0.3f };
        panel2->set_xy(v);
    }
    {
        int flags = 0x10;
        panel2->set148(&flags);
    }
    {
        float v[2] = { 0.025f, 0.025f };
        panel2->set_size(v);
    }
    window->add_child(panel2);

    // the three texts
    {
        tstring key("ui_done");
        done = new_medium_button(window, g_strtable.lookup(key), 0.7f, 0.09f, 0.04f);
    }
    {
        tstring key("ui_active_quests");
        active_tab = new_medium_button(window, g_strtable.lookup(key), 0.1f, 0.04f, 0.04f);
    }
    {
        tstring key("ui_finished_quests");
        finished_tab = new_medium_button(window, g_strtable.lookup(key), 0.28f, 0.04f, 0.04f);
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
    float h = quest_list_height((float)count * 0.04f + 0.09f);     // 0x4ee565: jne taken (0.56 <= h) keeps h; otherwise h = 0.56
    frame = new ContainerBase(ContainerBase::inl_tag());                  // vptr 0x5924a0
    {
        float v[2] = { 0.0f, 0.0f };
        frame->move_a(v);
    }
    {
        float v[2] = { 0.32f, h };
        frame->set_xy(v);
    }
    frame->set_mesh(0, make_background(frame->screen, (int)0xff000000));

    // one row per listed quest
    for (int i = 0; i < count; i++) {
        tstring text(*(tstring*)(QUEST(indices[i]) + 0x40));
        char* q = QUEST(indices[i]);
        char buf[512];
        if (q[0x50] & 1) {
            mb_snprintf(buf, 0x200, "%s (%d%%)", text.buf ? text.buf : (char*)&text, *(int*)(q + 0x64) & 0xff);
        } else {
            mb_snprintf(buf, 0x200, "%s", text.buf ? text.buf : (char*)&text);
        }
        rows[i] = new UiA;                     // vptr 0x591d58
        {
            float v[2] = { 0.01f, h - 0.05f - (float)i * 0.04f };
            rows[i]->move_a(v);
        }
        {
            float v[2] = { 0.3f, 0.04f };
            rows[i]->set_xy(v);
        }
        {
            // set_text(out, text): `out` is the hidden pointer of the 8-byte result (never initialised, 0x4ee750),
            // text is a temporary tstring built from the formatted buffer and destroyed right after the call
            char ret[8];
            tstring shown(buf);
            rows[i]->set_text((float*)ret, &shown);
        }
        {
            unsigned a = 0xffffffff;
            rows[i]->set_color(&a);
        }
        {
            int b = -3941;     // 0xfffff09b
            rows[i]->set_target(&b);
        }
        frame->add_child(rows[i]);
    }
    // 0x4ee7ea..: pos = [esp+0x4c] (0.1, 0.11), size = [esp+0x44] (0.32, 0.58): argument temporaries, stored after the pushes
    list = make_quest_list(frame, float2_arg(float2(0.1f, 0.11f)), float2_arg(float2(0.32f, 0.58f)), make_scroll_track(window, 0.48f, 0.1f, 0.02f, 0.6f));
    window->add_child(list);
}

