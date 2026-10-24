#include "./conv_open.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../config_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../overlay.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../conv_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4f66e0
void ConvScreen::open(int dialog, int* out)
{
    g_gui->reset();

    Panel* panel = new Panel();
    talk = panel;
    {
        float pos[2] = { 0.0f, 0.0f };
        talk->set_pos(pos);
        float sz[2] = { 1.0f, 0.75f };
        talk->set_scale(sz);
    }
    {
        tstring name("conversation_window");
        talk->set_mesh(0, g_alloc.get_object(name)->clone());
    }
    {
        int v = (int)talk;
        g_ui_dirty = 1;
        g_gui->add(&v);
    }

    float sentence_y_a = config_float("conv_sentence_y_a");

    TextBox* bar = new TextBox();
    bar_reln = bar;
    {
        tstring name("conv_bo_board");
        label_refresh(bar, name);
    }
    talk->children.push_back(bar);

    Widget* line = new WidgetObj();
    label_text = line;
    {
        tstring name("conv_t_partner");
        label_refresh(line, name);
    }
    bar->add_child(line);

    Widget* speaker = new WidgetObj();
    label_speaker = speaker;
    {
        tstring name("conv_t_utterer_name");
        label_refresh(speaker, name);
    }
    bar->add_child(speaker);

    // sentence_y_a = max(sentence_y_a * 1024.0f, conv_choices_min_y)
    float scaled = sentence_y_a * 1024.0f;
    float min_y = config_float("conv_choices_min_y");
    sentence_y_a = (scaled > min_y) ? scaled : min_y;

    Choices* ch = new Choices();
    choice = ch;
    {
        tstring name("conv_bo_choices");
        label_refresh(ch, name);
    }
    {
        float zero = 0.0f;
        float v[2] = { zero, -sentence_y_a };
        ch->set_y(v);
    }

    Widget* reln = make_bar(talk, tstring("conv_s_choices"));
    question = make_question(ch, tstring("conv_sc_choices"), reln);
    choice_finalize(question);
    talk->add_child(question);

    Widget* input = new WidgetObj();
    input_label = input;
    {
        tstring name("conv_t_press_mouse");
        label_refresh(input, name);
    }
    {
        tstring key("ui_press_left_mouse_button_to_continue");
        tstring out2;
        input->set_text(g_strtable.lookup(&key), out2);
    }
    talk->add_child(input);

    // the message window (+0x11e0): a TextBox at 0.03/0.3, 0.25/0.21, mesh "message_window"
    TextBox* msg = new TextBox();
    msg_window = msg;
    {
        float pos[2] = { 0.03f, 0.3f };
        msg->set_pos(pos);
        float sz[2] = { 0.25f, 0.21f };
        msg->set_scale(sz);
    }
    {
        tstring name("message_window");
        msg->set_mesh(0, g_alloc.get_object(name)->clone());
    }
    msg->hide();

    Widget* rbar = new WidgetObj();
    relation_bar = rbar;
    {
        float pos[2] = { 0.01f, 0.02f };
        rbar->set_pos(pos);
        float sz[2] = { 0.23f, 0.02f };
        rbar->set_scale(sz);
    }
    {
        tstring name("talk_relation_bar");
        rbar->set_mesh(0, g_alloc.get_object(name)->clone());
    }
    msg->add_child(rbar);

    Widget* rptr = new WidgetObj();
    reln_pointer = rptr;
    {
        float pos[2] = { 0.1f, 0.02f };
        rptr->set_pos(pos);
        float sz[2] = { 0.03f, 0.03f };
        rptr->set_scale(sz);
    }
    {
        tstring name("talk_reln_pointer");
        rptr->set_track(0, g_alloc.get_object(name)->clone());
    }
    msg->add_child(rptr);

    for (int i = 0; i < 5; i++) {
        Widget* ln = new WidgetObj();
        lines[i] = ln;
        float pos[2] = { 0.01f, 0.17f - (float)i * 0.03f };
        ln->set_pos(pos);
        float sz[2] = { 0.23f, 0.03f };
        ln->set_scale(sz);
        int flag = 4;
        ln->set_flag(&flag);
        float sc[2] = { 0.021f, 0.021f };
        ln->set_size(sc);
        msg->add_child(ln);
    }

    talk->add_child((Widget*)msg);

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
        new (vw) MapWidget();
    else
        vw = 0;
    window = vw;
    {
        *(void**)((char*)vw + 0x178) = *(void**)g_mission;
        dword_copy(0x48, (char*)vw + 0x1ac, (char*)this + 0x34);
        vw->set_pos(pos);
        vw->set_scale(sz);
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
        build_camera_planes(cam);
    }

    quest_mode = 0;
    {
        rgl::matrix id = identity3();
        dword_copy(16, &world, &id);          // +0x1194: the screen's world transform
    }

    // the talk panel's child vector gets the view window at +0x30 pushed directly (again via _Insert_n)
    talk->children.push_back((Widget*)vw);
    started = 1;

    n_lines = 0;

    screen_refresh(-1, out);

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
