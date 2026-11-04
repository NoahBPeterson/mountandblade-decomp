#include "../../include/image_addrs.h"
#include "./conv_open.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ini_file.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui/container.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../conv_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_globals.h"      // UI / core globals

// @ 0x4f66e0
void Rec1208::slot_empty_2()
{
    ((gui_manager*)g_gui)->reset();   // 0x42d1b0 (g_gui is the gui_manager)

    NodeA* panel = (NodeA*)new_container(0x190, vtbl_592848);
    if (panel)
        panel->f188 = 0;
    talk = panel;
    {
        float pos[2] = { 0.0f, 0.0f };
        talk->move_a(pos);
        float sz[2] = { 1.0f, 0.75f };
        talk->set_xy(sz);
    }
    {
        tstring name("conversation_window");
        talk->set_mesh(0, ((mesh_x*)res_manager().get_object((const rgl::string*)&name))->clone());
    }
    {
        NodeA* v = (NodeA*)talk;
        g_gui_dirty = 1;
        ((NodeVec*)(void*)g_gui)->push_back(v);   // 0x49b120
    }

    float sentence_y_a = config_float("conv_sentence_y_a");

    text_box* bar = (text_box*)new_container(0x188, vtbl_597050);
    bar_reln = bar;
    {
        tstring name("conv_bo_board");
        label_refresh((MissionUi*)bar, name);
    }
    push_back_inline(talk->kids, bar);

    MissionUi* line = (MissionUi*)new_widget_base(0x178);
    label_text = line;
    {
        tstring name("conv_t_partner");
        label_refresh((MissionUi*)line, name);
    }
    bar->add_child(line);

    MissionUi* speaker = (MissionUi*)new_widget_base(0x178);
    label_speaker = speaker;
    {
        tstring name("conv_t_utterer_name");
        label_refresh((MissionUi*)speaker, name);
    }
    bar->add_child(speaker);

    // sentence_y_a = max(sentence_y_a * 1024.0f, conv_choices_min_y)
    float scaled = sentence_y_a * 1024.0f;
    float min_y = config_float("conv_choices_min_y");
    sentence_y_a = (scaled > min_y) ? scaled : min_y;

    Choices* ch = (Choices*)new_container(0x188, 0);   // 0x4931a0 (vtable 0x5924a0, the three ints cleared)
    choice = ch;
    {
        tstring name("conv_bo_choices");
        label_refresh((MissionUi*)ch, name);
    }
    {
        float zero = 0.0f;
        float v[2] = { zero, -sentence_y_a };
        ch->move_f8(v);
    }

    tstring scroll_name("conv_s_choices");
    MissionUi* reln = (MissionUi*)make_scrollbar(talk, &scroll_name);
    tstring list_name("conv_sc_choices");
    question = (MissionUi*)make_editor_list(ch, &list_name, (int)reln);
    ((MissionPanel*)question)->draw();   // 0x488d50
    talk->add_child(question);

    MissionUi* input = (MissionUi*)new_widget_base(0x178);
    input_label = input;
    {
        tstring name("conv_t_press_mouse");
        label_refresh((MissionUi*)input, name);
    }
    {
        tstring key("ui_press_left_mouse_button_to_continue");
        struct { int a, b; } ret;                        // set_text's 8-byte result (original [esp+0x10]); the looked-up string is the text
        input->set_text((float*)&ret, &(*g_strtable.lookup(key)));
    }
    talk->add_child(input);

    // the message window (+0x11e0): a text_box at 0.03/0.3, 0.25/0.21, mesh "message_window"
    text_box* msg = (text_box*)new_container(0x188, vtbl_597050);
    msg_window = msg;
    {
        float pos[2] = { 0.03f, 0.3f };
        msg->move_a(pos);
        float sz[2] = { 0.25f, 0.21f };
        msg->set_xy(sz);
    }
    {
        tstring name("message_window");
        msg->set_mesh(0, ((mesh_x*)res_manager().get_object((const rgl::string*)&name))->clone());
    }
    msg->hide();

    MissionUi* rbar = (MissionUi*)new_widget_base(0x178);
    relation_bar = rbar;
    {
        float pos[2] = { 0.01f, 0.02f };
        rbar->move_a((float*)pos);
        float sz[2] = { 0.23f, 0.02f };
        rbar->set_xy(sz);
    }
    {
        tstring name("talk_relation_bar");
        rbar->set_mesh(0, ((mesh_x*)res_manager().get_object((const rgl::string*)&name))->clone());
    }
    msg->add_child(rbar);

    MissionUi* rptr = (MissionUi*)new_widget_base(0x178);
    reln_pointer = rptr;
    {
        float pos[2] = { 0.1f, 0.02f };
        rptr->move_a((float*)pos);
        float sz[2] = { 0.03f, 0.03f };
        rptr->set_xy(sz);
    }
    {
        tstring name("talk_reln_pointer");
        rptr->set_track(0, ((mesh_x*)res_manager().get_object((const rgl::string*)&name))->clone());
    }
    msg->add_child(rptr);

    for (int i = 0; i < 5; i++) {
        MissionUi* ln = (MissionUi*)new_widget_base(0x178);
        lines[i] = ln;
        float pos[2] = { 0.01f, 0.17f - (float)i * 0.03f };
        ln->move_a((float*)pos);
        float sz[2] = { 0.23f, 0.03f };
        ln->set_xy(sz);
        int flag = 4;
        ln->set148(&flag);
        float sc[2] = { 0.021f, 0.021f };
        ln->set_size(sc);
        msg->add_child(ln);
    }

    talk->add_child((MissionUi*)msg);

    // the four trailing geometry settings use the explicit linear `equals` scan
    float sentence_x = scan_float("conv_sentence_x");
    float sentence_size_y = scan_float("conv_sentence_size_y");
    float width_ratio = scan_float("conv_sentence_width_ratio");
    float button_height = scan_float("conv_sentence_button_height");

    for (int i = 0; i < 0x400; i++) {
        buttons[i] = make_body_button(ch, tstring("dlg_button"), tstring("dlg_button_down"),
                                      tstring("hello"), -((float)i * sentence_y_a),
                                      width_ratio, sentence_size_y, button_height,
                                      sentence_x, 1.0f, 0.085f, 0.085f);
    }

    // +0x30: a 0x358-byte view-window object (ctor 0x4ed630); its +0x1ac receives a copy of the screen's
    // embedded camera (at this+0x34) and its +0x178 points at the current mission
    float pos[2] = { 0.500085f, 0.290000f };
    float sz[2] = { 0.289000f, 0.452998f };
    ViewWindow* vw = (ViewWindow*)::operator new(0x358);
    if (vw)
        ctor_4ed630((MapWidget*)vw);   // MapWidget::MapWidget is declared at 0x4ed630; the definition is this fastcall constructor
    else
        vw = 0;
    window = vw;
    {
        *(void**)((char*)vw + 0x178) = *(void**)g_mission;
        dword_copy(0x48, (char*)vw + 0x1ac, (char*)this + 0x34);
        vw->move_a((float*)pos);
        vw->set_xy(sz);
    }

    // the camera/frustum embedded in the screen at +0x34 (its +0xf8/+0x120 float4s and the projection fields)
    {
        ConvCamera* cam = (ConvCamera*)((char*)this + 0x34);
        float ratio = sz[1] / sz[0];
        *(unsigned char*)((char*)cam + 0) = 1;
        float* f = (float*)((char*)cam + 0xf8);      // this+0x12c
        f[0] = f[1] = 0.0f;
        f[2] = 10.0f;
        f[3] = 0.0f;
        *(float*)((char*)cam + 0x118) = 0.1f;
        *(float*)((char*)cam + 0x11c) = 500.0f;
        float a = (float)tan(PI_OVER_EIGHT) * 0.1f;
        *(float*)((char*)cam + 0x108) = -a;
        *(float*)((char*)cam + 0x10c) = a;
        *(float*)((char*)cam + 0x114) = -a * ratio;
        *(float*)((char*)cam + 0x110) = a * ratio;
        float* q = (float*)((char*)cam + 0x120);     // this+0x154
        q[0] = q[1] = q[2] = q[3] = 0.0f;
        *(int*)((char*)cam + 0x130) = 0;             // this+0x164
        *(int*)((char*)cam + 0x134) = 0;             // this+0x168
        *(float*)((char*)cam + 0x138) = 1.0f;        // this+0x16c
        ((rgl::frustum*)cam)->build_planes();   // 0x41e4f0 (the camera block is a frustum)
    }

    quest_mode = 0;
    {
        rgl::matrix id;
        identity3((rgl::mat3*)&id);
        dword_copy(16, &world, &id);          // +0x1194: the screen's world transform
    }

    // the talk panel's child vector gets the view window at +0x30 pushed directly (again via _Insert_n)
    push_back_inline(talk->kids, (MissionUi*)vw);
    started = 1;

    n_lines = 0;

    refresh(-1, 0);

    if (target >= 0) {
        int i = 0;
        while (i < g_mission->agents.count && g_mission->agents.get(i)->troop == 0)
            i++;
        while (i < g_mission->agents.count) {
            Agent* a = g_mission->agents.get(i);
            if (a->troop == target)
                g_mission->init_agent(a);
            i++;
            while (i < g_mission->agents.count && g_mission->agents.get(i)->troop == 0)
                i++;
        }
        target = -1;
    }
}
