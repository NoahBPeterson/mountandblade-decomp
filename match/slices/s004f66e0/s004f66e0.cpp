// clang-format off
// Slice s004f66e0: the conversation (dialogue) screen's constructor 0x4f66e0 -- opens the screen: builds the
// talk overlay (+0x2c), the board/bar (+0x180), the window (+0x30), the speaker and line labels, the question
// (+0x188) and choice (+0x18c) boxes, the press label (+0x17c), 1024 body buttons (+0x190), the message
// window (+0x11e0), the relation bar/pointer (+0x11e8/+0x11e4) and five sentence boxes (+0x11ec); then resets
// the camera and re-targets the talk. Widget layouts/vcall slots, the tstring idioms, the config-table lookup
// and the ConvScreen offsets follow the neighbouring slice s004f73c0 (same `this`). The widget classes were
// identified by disassembling each constructor this function calls and dumping its vtable. Names provisional.
//
// The widget vcall slots in the original are set_pos=+0x58, set_size=+0x64, set_y=+0x68, set_scale2=+0x6c,
// set_flag=+0x74, set_mesh=+0x7c, set_track=+0x80, set_text=+0xa4, advance=+0xb0, add_child=+0xc8.
// The `Widget` declaration below orders the filler slots so those land on the right offsets.
//
// Identified widget classes (all constructed through ::operator new + a ctor that stores the vtable):
//   base Widget   0x178 bytes, ctor 0x48ae20, vtable 0x591c80
//   Panel         0x190 bytes, ctor 0x4931a0, vtable 0x592848, extra byte field +0x188
//   TextBox       0x188 bytes, ctor 0x4931a0, vtable 0x597050, vector<Widget*> at +0x178
//   Choices       0x188 bytes, ctor 0x48ae20, vtable 0x5924a0, three ints +0x17c/+0x180/+0x184
//   ViewWindow    0x358 bytes, ctor 0x4ed630, vtable 0x598698
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <new>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ---- string / tables -----------------------------------------------------------------------------------
struct tstring {                                         // rgl::string, 16 bytes (match/include/rgl.h)
    int pos; char* buf; int cap; int len;
    tstring();                                           // 0x42bbd0
    tstring(const char* s);                              // 0x42bbe0
    tstring(const tstring& o);                           // 0x42c110
    ~tstring();                                          // 0x42bc40
    bool equals(const tstring& o) const;                 // 0x42bdd0
};
struct entry { int f0; char* value; char pad[0x2c - 8]; };  // config-table entry (0xa4425c + 0x2c*i)
extern int g_n_strings;                                  // 0xa70240
extern entry g_strings;                                  // 0xa44240 (indexed from 0xa4425c)
extern tstring g_empty_string;                           // 0x8ee30f0
struct ConfigTable {
    int find(const tstring& key);                        // 0x4bd5a0 (ordered search)
};
extern ConfigTable g_config;                             // 0xa44240
extern unsigned char g_ui_dirty;                         // 0x5d8788

// lookup of one setting (0x4bd5a0 + atof), 0.0f when the key is missing
static float config_float(const char* name)
{
    tstring key(name);
    int idx = g_config.find(key);
    float r = 0.0f;
    if (idx >= 0) {
        entry* e = (entry*)((char*)&g_strings + 0x1c) + idx;
        r = (float)atof(e->value ? e->value : (char*)&e->value);
    }
    return r;
}

// the fourth..seventh lookups at the end of 0x4f66e0 use an explicit linear `equals` scan instead of `find`
static float scan_float(const char* name)
{
    tstring key(name);
    int idx = -1;
    int i = 0;
    do {
        if (i >= g_n_strings)
            break;
        if (key.equals(((tstring*)((char*)&g_strings + 0x1c))[i]))
            idx = i;
        i++;
    } while (idx == -1);
    float r = 0.0f;
    if (idx >= 0) {
        entry* e = (entry*)((char*)&g_strings + 0x1c) + idx;
        r = (float)atof(e->value ? e->value : (char*)&e->value);
    }
    return r;
}

struct base_mesh;
struct resmgr {
    base_mesh* get_object(const tstring& name);          // 0x419540
};
extern resmgr g_alloc;                                   // 0x5d6fa0
struct base_mesh {
    base_mesh* clone();                                  // 0x41c6d0
};

extern rgl::matrix identity3();                          // 0x40cd60 (0x40 bytes by value)
void build_camera_planes(void* cam);                     // 0x41e4f0 (frustum::build_planes)
// the engine's UI manager singleton (0x5d7080); 0x42d1b0/0x49b120 are its methods.
struct gui_manager_s {
    void reset();                                        // 0x42d1b0
    void add(void* o);                                   // 0x49b120
};
extern gui_manager_s* g_gui;                             // 0x5d7080
void choice_finalize(void* box);                         // 0x488d50
struct StrTable {
    tstring* find(const tstring* key);                   // 0x43a1e0
};
extern StrTable g_strtable;                              // 0x5e2a18

// ---- widget slots used here (s004f73c0 / s0042d360 gui.h) ----------------------------------------------
struct Widget {
    virtual void w00(); virtual void w01(); virtual void w02(); virtual void w03();
    virtual void w04(); virtual void w05(); virtual void w06(); virtual void w07();
    virtual void w08(); virtual void w09(); virtual void w10(); virtual void w11();
    virtual void w12(); virtual void w13(); virtual void w14(); virtual void w15();
    virtual void w16(); virtual void w17(); virtual void w18(); virtual void w19();
    virtual void w20(); virtual void w21();
    virtual void set_pos(const float* p);                // +0x58 (idx 22)
    virtual void w23(); virtual void w24();
    virtual void set_size(const float* p);               // +0x64 (idx 25)
    virtual void set_y(const float* p);                  // +0x68 (idx 26)
    virtual void set_scale2(const float* p);             // +0x6c (idx 27)
    virtual void w28();                                  // +0x70 (idx 28)
    virtual void set_flag(const int* f);                 // +0x74 (idx 29)
    virtual void w30();                                  // +0x78 (idx 30)
    virtual void set_mesh(void* m, int zero);            // +0x7c (idx 31)
    virtual void set_track(void* m, int zero);           // +0x80 (idx 32)
    virtual void w33(); virtual void w34(); virtual void w35(); virtual void w36();
    virtual void w37(); virtual void w38(); virtual void w39(); virtual void w40();
    virtual void set_text(const tstring* in, tstring* out);  // +0xa4 (idx 41)
    virtual void w42(); virtual void w43();
    virtual void advance();                              // +0xb0 (idx 44)
    virtual void w45(); virtual void w46(); virtual void w47(); virtual void w48();
    virtual void w49();
    virtual void add_child(Widget* c);                   // +0xc8 (idx 50)
    void configure(const tstring& s);                    // 0x4e4c70 (out of line in the original)
};

// ---- widget classes -------------------------------------------------------------------------------------
struct Panel;
struct TextBox : Widget {                                // vtable 0x597050, ctor 0x4931a0, 0x188 bytes
    char pad178[0x188 - 0x178];
};
struct Panel : Widget {                                  // vtable 0x592848, ctor 0x4931a0, 0x190 bytes
    std::vector<Widget*> children;                       // +0x178
    unsigned char f188;                                  // +0x188
};
struct Choices : Widget {                                // vtable 0x5924a0, ctor 0x48ae20, 0x188 bytes
    int f17c, f180, f184;                                // +0x17c/+0x180/+0x184
};
struct ViewWindow : Widget {                             // +0x30, ctor 0x4ed630, vtable 0x598698, 0x358 bytes
    char pad178[0x358 - 0x178];
    void init();                                         // 0x4ed630
};

// factory helpers (parent is the last argument)
Widget* make_bar(Widget* talk, const tstring& name);                         // 0x4f6160
Widget* make_question(Widget* choice, const tstring& name, Widget* owner);   // 0x4f62f0
Widget* make_body_button(Widget* parent, const tstring& a, const tstring& b, const tstring& c,
                         float x, float y, float w, float h, float f5, float f6,
                         float f7, float f8);                                // 0x4dcc50

// ---- the conversation screen (`this` of 0x4f66e0, 0x4f73c0, 0x4f7d70) --------------------------------
struct ConvScreen {
    char pad0[0x2c];
    Panel* talk;                                         // +0x2c
    ViewWindow* window;                                  // +0x30
    char pad34[0x174 - 0x34];
    Widget* label_speaker;                               // +0x174
    Widget* label_text;                                  // +0x178
    Widget* input_label;                                 // +0x17c
    TextBox* bar_reln;                                   // +0x180
    char pad184[0x188 - 0x184];
    Widget* question;                                    // +0x188
    Choices* choice;                                     // +0x18c
    Widget* buttons[0x400];                              // +0x190
    int n_lines;                                         // +0x1190
    rgl::matrix world;                                   // +0x1194 (camera world transform, 0x40)
    int quest_mode;                                      // +0x11d4
    char started;                                        // +0x11d8 (byte flag)
    char pad11d9[0x11e0 - 0x11d9];
    TextBox* msg_window;                                 // +0x11e0
    Widget* reln_pointer;                                // +0x11e4
    Widget* relation_bar;                                // +0x11e8
    Widget* lines[5];                                    // +0x11ec
    int target;                                          // +0x1200
    void open(int dialog, int* out);
    void refresh(int sel, int* out);                     // 0x4f73c0
};

// ---- mission / agent list helpers ---------------------------------------------------------------------
struct Agent {
    char pad[0x88];
    int agent_id;                                        // +0x88
};
struct AgentTable {
    char pad0[4];
    int count;                                           // +4
    Agent* get(int i);                                   // 0x43b1f0
};
struct Mission {
    char pad0[4];
    AgentTable* agents;                                  // +4
    char pad8[0x1c - 8];
    Agent** pages;                                       // +0x1c
};
extern Mission* g_mission;                               // 0x5e292c
void agent_reveal(Agent* a);                             // 0x4f7d70
void screen_refresh(int mode, int* out);                 // 0x4f73c0 (ecx = screen)

#define PI_OVER_EIGHT 0.39269909262657166   // 0x599200 (read as a double constant by fptan)

// the dialogue camera (0x358 bytes, ctor 0x4ed630, vtable 0x598698). Field offsets other than the ones
// written by 0x4f66e0 are unknown, so this is a flat provisional layout, not a widget-derived one.
struct Camera {
    char data[0x358];
    void init();                                         // 0x4ed630
    void configure(const tstring& s);                    // 0x4e4c70
    void set_pos(const float* p);                        // 0x488970 (slot +0x58 of vtable 0x598698)
    void set_size(const float* p);                       // 0x4889b0 (slot +0x64 of vtable 0x598698)
};

// memcpy of n dwords (the original inlines this as `rep movsd`)
static void dword_copy(int n, void* dst, const void* src)
{
    int* d = (int*)dst;
    const int* s = (const int*)src;
    for (int i = 0; i < n; i++)
        d[i] = s[i];
}

// the vector<Widget*>::push_back used at the two call sites (0x4927d0 is the out-of-line _Insert_n)

// ========================================================================================================
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
        talk->set_size(sz);
    }
    {
        tstring name("conversation_window");
        talk->set_mesh(g_alloc.get_object(name)->clone(), 0);
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
        bar->configure(name);
    }
    talk->children.push_back(bar);

    Widget* line = new Widget();
    label_text = line;
    {
        tstring name("conv_t_partner");
        line->configure(name);
    }
    bar->add_child(line);

    Widget* speaker = new Widget();
    label_speaker = speaker;
    {
        tstring name("conv_t_utterer_name");
        speaker->configure(name);
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
        ch->configure(name);
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

    Widget* input = new Widget();
    input_label = input;
    {
        tstring name("conv_t_press_mouse");
        input->configure(name);
    }
    {
        tstring key("ui_press_left_mouse_button_to_continue");
        tstring out2;
        input->set_text(g_strtable.find(&key), &out2);
    }
    talk->add_child(input);

    // the message window (+0x11e0): a TextBox at 0.03/0.3, 0.25/0.21, mesh "message_window"
    TextBox* msg = new TextBox();
    msg_window = msg;
    {
        float pos[2] = { 0.03f, 0.3f };
        msg->set_pos(pos);
        float sz[2] = { 0.25f, 0.21f };
        msg->set_size(sz);
    }
    {
        tstring name("message_window");
        msg->set_mesh(g_alloc.get_object(name)->clone(), 0);
    }
    msg->advance();

    Widget* rbar = new Widget();
    relation_bar = rbar;
    {
        float pos[2] = { 0.01f, 0.02f };
        rbar->set_pos(pos);
        float sz[2] = { 0.23f, 0.02f };
        rbar->set_size(sz);
    }
    {
        tstring name("talk_relation_bar");
        rbar->set_mesh(g_alloc.get_object(name)->clone(), 0);
    }
    msg->add_child(rbar);

    Widget* rptr = new Widget();
    reln_pointer = rptr;
    {
        float pos[2] = { 0.1f, 0.02f };
        rptr->set_pos(pos);
        float sz[2] = { 0.03f, 0.03f };
        rptr->set_size(sz);
    }
    {
        tstring name("talk_reln_pointer");
        rptr->set_track(g_alloc.get_object(name)->clone(), 0);
    }
    msg->add_child(rptr);

    for (int i = 0; i < 5; i++) {
        Widget* ln = new Widget();
        lines[i] = ln;
        float pos[2] = { 0.01f, 0.17f - (float)i * 0.03f };
        ln->set_pos(pos);
        float sz[2] = { 0.23f, 0.03f };
        ln->set_size(sz);
        int flag = 4;
        ln->set_flag(&flag);
        float sc[2] = { 0.021f, 0.021f };
        ln->set_scale2(sc);
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
        vw->init();
    else
        vw = 0;
    window = vw;
    {
        *(void**)((char*)vw + 0x178) = *(void**)g_mission;
        dword_copy(0x48, (char*)vw + 0x1ac, (char*)this + 0x34);
        vw->set_pos(pos);
        vw->set_size(sz);
    }

    // the camera/frustum embedded in the screen at +0x34 (its +0xf8/+0x120 float4s and the projection fields)
    {
        Camera* cam = (Camera*)((char*)this + 0x34);
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
        while (i < g_mission->agents->count && g_mission->agents->get(i)->agent_id == 0)
            i++;
        while (i < g_mission->agents->count) {
            Agent* a = g_mission->agents->get(i);
            if (a->agent_id == target)
                agent_reveal(a);
            i++;
            while (i < g_mission->agents->count && g_mission->agents->get(i)->agent_id == 0)
                i++;
        }
        target = -1;
    }
}
