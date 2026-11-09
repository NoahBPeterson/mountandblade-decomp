#include "../../include/image_addrs.h"
#include "./conv_open.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ini_file.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui/container.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../party2/camera_class.h"      // Camera (draw 0x488d50)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../conv_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_globals.h"      // UI / core globals
#include "../../include/vec.h"      // vector2
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)

// the mesh lookup by name that the original inlines at the talk panel, the relation bar and the pointer
template<class W> static __forceinline void mesh_by_name(W* w, const char* nm)
{
    tstring name(nm);
    w->set_mesh(0, ((rgl::mesh*)g_resmgr.get_object(&name))->clone());
}
template<class W> static __forceinline void track_by_name(W* w, const char* nm)
{
    tstring name(nm);
    w->set_track(0, ((rgl::mesh*)g_resmgr.get_object(&name))->clone());
}

// push_back as the original inlines it: size first (0 when empty), then the room check, then a direct store
static inline void push_back_direct(std::vector<MissionUi*>& c, MissionUi* const& v)
{
    unsigned n = c._Myfirst == 0 ? 0 : (unsigned)(c._Mylast - c._Myfirst);
    if (c._Myfirst != 0 && n < (unsigned)(c._Myend - c._Myfirst)) {
        MissionUi** p = c._Mylast;
        *p = v;
        c._Mylast = p + 1;
    } else {
        ((SceneObjVec*)&c)->_Insert_n(std::vector<SceneObject*>::iterator((SceneObject**)c._Mylast), 1, (SceneObject* const&)v);
    }
}

// the 0x178-byte base widget: new + the inline MissionUi ctor, the null check merged in the result register
static __forceinline MissionUi* widget_base()
{
    void* p = ::operator new(0x178);
    return p ? new (p, raw_place()) MissionUi : 0;
}

// @ 0x4f66e0
void Rec1208::init_screen()
{
    g_gui.reset();   // 0x42d1b0 (g_gui is the gui_manager)

    talk = new NodeA(0);
    talk->move_a((float*)&vector2(0.0f, 0.0f));
    talk->set_xy((float*)&vector2(1.0f, 0.75f));
    mesh_by_name(talk, "conversation_window");
    {
        MissionUi* w = talk;
        g_gui.dirty = 1;
        g_gui.objs.push_back((MissionUi* const&)w);   // 0x49b120
    }

    float sentence_y_a = config_float("conv_sentence_y_a");

    bar_reln = new text_box;
    label_refresh((MissionUi*)bar_reln, tstring("conv_bo_board"));
    push_back_direct(talk->kids, (MissionUi*)bar_reln);

    label_text = widget_base();
    label_refresh(label_text, tstring("conv_t_partner"));
    bar_reln->add_child(label_text);

    label_speaker = widget_base();
    label_refresh(label_speaker, tstring("conv_t_utterer_name"));
    bar_reln->add_child(label_speaker);

    // sentence_y_a = max(sentence_y_a * 1024.0f, conv_choices_min_y)
    sentence_y_a = sentence_y_a * 1024.0f;
    sentence_y_a = config_max("conv_choices_min_y", sentence_y_a);

    choice = new ContainerBase(ContainerBase::inl_tag());
    label_refresh((MissionUi*)choice, tstring("conv_bo_choices"));
    choice->move_f8((float*)&vector2(0.0f, -sentence_y_a));

    MissionUi* reln;
    {
        tstring scroll_name("conv_s_choices");
        reln = (MissionUi*)make_scrollbar(talk, &scroll_name);
    }
    {
        tstring list_name("conv_sc_choices");
        question = (MissionUi*)make_editor_list(choice, &list_name, (int)reln);
    }
    ((Camera*)question)->draw();   // 0x488d50
    talk->add_child(question);

    input_label = widget_base();
    label_refresh(input_label, tstring("conv_t_press_mouse"));
    {
        tstring key("ui_press_left_mouse_button_to_continue");
        int ret[2];                        // set_text's 8-byte result (original [esp+0x10]); the looked-up string is the text
        input_label->set_text((float*)ret, &(*g_strtable.lookup(key)));
    }
    talk->add_child(input_label);

    // the message window (+0x11e0): a text_box at 0.03/0.3, 0.25/0.21, mesh "message_window"
    msg_window = new text_box;
    msg_window->move_a((float*)&vector2(0.03f, 0.3f));
    msg_window->set_xy((float*)&vector2(0.25f, 0.21f));
    {
        tstring name("message_window");
        rgl::mesh* m = ((rgl::mesh*)g_resmgr.get_object(&name))->clone();
        msg_window->set_mesh(0, m);
    }
    msg_window->hide();

    relation_bar = widget_base();
    relation_bar->move_a((float*)&vector2(0.01f, 0.02f));
    relation_bar->set_xy((float*)&vector2(0.23f, 0.02f));
    mesh_by_name(relation_bar, "talk_relation_bar");
    msg_window->add_child(relation_bar);

    reln_pointer = widget_base();
    reln_pointer->move_a((float*)&vector2(0.1f, 0.02f));
    reln_pointer->set_xy((float*)&vector2(0.03f, 0.03f));
    track_by_name(reln_pointer, "talk_reln_pointer");
    msg_window->add_child(reln_pointer);

    for (int i = 0; i < 5; i++) {
        lines[i] = widget_base();
        lines[i]->move_a((float*)&vector2(0.01f, 0.17f - (float)i * 0.03f));
        lines[i]->set_xy((float*)&vector2(0.23f, 0.03f));
        int flag = 4;
        lines[i]->set148(&flag);
        lines[i]->set_size((float*)&vector2(0.021f, 0.021f));
        msg_window->add_child(lines[i]);
    }

    talk->add_child((MissionUi*)msg_window);

    // the four trailing geometry settings use the explicit linear `equals` scan
    float sentence_x = scan_float("conv_sentence_x");
    float sentence_size_y = scan_float("conv_sentence_size_y");
    float width_ratio = scan_float("conv_sentence_width_ratio");
    float button_height = scan_float("conv_sentence_button_height");

    // button i sits (i + 1) slots below the choices box origin: the original's counter is incremented before the offset is taken
    for (int i = 1; i <= 0x400; i++) {
        tstring hello("hello");
        tstring dlg("dlg_button");
        tstring dlg_down("dlg_button_down");
        buttons[i - 1] = (MissionUi*)new_sized_button((MissionUi*)choice, hello, sentence_x, -((float)i * sentence_y_a), sentence_size_y,
                                              dlg, dlg_down, width_ratio, button_height, 0.085f);   // 0x4dcc50
    }

    // +0x30: a 0x358-byte view-window object (ctor 0x4ed630); its +0x1ac receives a copy of the screen's
    // embedded camera (at this+0x34) and its +0x178 points at the current mission
    window = new MapWidget;
    *(void**)((char*)window + 0x178) = *(void**)g_mission;
    rgl::frustum* cam = &fr;
    memcpy(&window->scene3d.fr, cam, 0x120);
    window->move_a((float*)&vector2(0.008f, 0.29f));
    float sz[2] = { 0.289f, 0.453f };
    window->set_xy(sz);

    // the camera/frustum embedded in the screen at +0x34 (its projection fields; the frame origin and the float4 at +0x154 are set below)
    {
        float ratio = sz[1] / sz[0];
        cam->perspective = 1;
        cam->nearz = 0.1f;
        cam->farz = 500.0f;
        float a = 0.1f * (float)tan(PI_OVER_EIGHT);
        cam->left = -a;
        float b = (float)tan(PI_OVER_EIGHT) * 0.1f;
        cam->right = b;
        cam->bottom = -a * ratio;
        cam->top = b * ratio;
        cam->build_planes();   // 0x41e4f0
    }

    quest_mode = 0;
    {
        rgl::vector4 _pos;
        _pos.x = 0.0f;
        _pos.y = 0.0f;
        _pos.z = 0.0f;
        rgl::mat3 _t;
        rgl::mat3 _rot = *rgl::identity3(&_t);
        rgl::matrix _m;
        *(rgl::mat3*)&_m = _rot;
        _m.o = _pos;
        world = _m;                           // +0x1194: the screen's world transform
    }

    // the talk panel's child vector gets the view window at +0x30 pushed directly (again via _Insert_n)
    started = 1;
    push_back_direct(talk->kids, (MissionUi*)window);
    fr.m.o = rgl::vector4(0.0f, 0.0f, 10.0f);
    *(rgl::vector4*)(pad154) = rgl::vector4(0.0f, 0.0f, 0.0f);    // +0x154 (w never written)
    *(int*)(pad154 + 0x10) = 0;                                   // +0x164
    *(int*)(pad154 + 0x14) = 0;                                   // +0x168
    *(float*)(pad154 + 0x18) = 1.0f;                              // +0x16c

    n_lines = 0;

    refresh(-1, 0);

    if (target >= 0) {
        // the first_used / next_used scans (0x43b1b0 / 0x43b160) as the original inlines them
        AgentTable* t = &g_mission->agents;
        int n = t->count;
        unsigned i = 0;
        for (; (int)i < n; i++)
            if (t->get(i)->used)
                break;
        while (i < (unsigned)t->count) {
            Agent* a = &g_mission->agents.pages[i >> 4][i & 0xf];
            if (a->troop == target)
                g_mission->init_agent(a);
            t = &g_mission->agents;
            n = t->count;
            i++;
            for (; (int)i < n; i++)
                if (t->get(i)->used)
                    break;
        }
        target = -1;
    }
}
