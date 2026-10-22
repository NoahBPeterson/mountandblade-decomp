// Slice s004f73c0: the conversation (dialogue) screen body 0x4f73c0 -- it picks or advances the current
// dialog line, writes the speaker's name and the (tag-stripped) line text into the two label widgets, advances
// all 0x400 choice buttons, then, when the dialog has more than one line, lays out one button per line
// (position/size from the "conv_sentence_*" settings, mesh "dlg_button"/"dlg_button_down" for each) and finally
// repositions the choice box -- plus the agent appearance refresh 0x4f7e00 (calls the appearance builders
// 0x482240 / 0x47cfe0 on the body, caches the equipment value, re-wields slots 0 and 1) and its caller
// 0x4f7d70 (pick equipment, refresh the appearance, reset the wielded items).
// Widget slots, tstring (0x42bbe0 ctor, 0x42bc40 dtor, 0x42c110 copy), the setting table (0xa44240, entries of
// 0x2c from 0xa4425c), the dialog manager accessors (s0045ff30) and the agent layout (s0045ff30) follow the
// neighbouring slices.  Names are provisional.  Neither function is byte-exact yet: the notes in the comments
// record what the original does (stack frame, call order, value flow) so the sources can be finished.
// clang-format off
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <new>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ---- strings / setting table ------------------------------------------------------------------------
struct tstring {                                         // rgl::string, 16 bytes (match/include/rgl.h)
    int pos; char* buf; int cap; int len;
    tstring();                                           // zero ctor used for compiler temporaries
    tstring(const char* s);                              // 0x42bbe0
    tstring(const tstring& o);                           // 0x42c110
    ~tstring();                                          // 0x42bc40
    bool equals(const tstring& o) const;                 // 0x42bdd0
    tstring strip_tags() const;                          // 0x42c5d0
};
struct setting { int f0; char* value; char pad[0x2c - 8]; };   // string pointer +4 (null: field is the text)
extern int g_n_settings;                                 // 0xa70240
extern setting g_settings;                               // 0xa44240 (entry i at 0xa4425c + 0x2c*i)
extern tstring g_empty_string;                           // 0x8ee30f0 (?g_empty_string@@3Utstring@@A)

// lookup of one setting, 0.0f when the key is missing (the original inlines this seven times)
static float setting_float(const char* name)
{
    tstring key(name);
    int idx = -1;
    int i = 0;
    do {
        if (i >= g_n_settings)
            break;
        if (key.equals(((tstring*)((char*)&g_settings + 0x1c))[i]))
            idx = i;
        i++;
    } while (idx == -1);
    float r = 0.0f;
    if (idx >= 0) {
        setting* e = (setting*)((char*)&g_settings + 0x1c) + idx;
        r = (float)atof(e->value ? e->value : (char*)&e->value);
    }
    return r;
}

// ---- game data --------------------------------------------------------------------------------------
struct TroopRec {
    tstring name;                                        // +0
    char pad10[0x10];
    tstring id;                                          // +0x20
    char pad30[0x508 - 0x30];
};
struct GameData {
    char pad0[0x13a00];
    TroopRec* chars;                                     // +0x13a00
};
struct StrTable {
    tstring* find(const tstring* key) throw();           // 0x4c6010
};
extern GameData* g_game;                                 // 0x5e2930
extern StrTable g_strtable;                              // 0x5e2a18

// the troop's translated id, falling back to its name (the original's inlined 0x4c6010 use)
static const char* troop_text(int id)
{
    TroopRec* t = &g_game->chars[id];
    tstring* r = g_strtable.find(&t->id);
    if (!r)
        r = &t->name;
    char* v = r->buf;
    return v ? v : (const char*)&r->buf;
}

// ---- dialog manager (layout verified in s0045ff30) --------------------------------------------------
struct DialogRec { char pad0[0x20]; unsigned flags; char pad24[0x40]; };
struct rawline {                                         // one dialog line, 0x1c bytes
    int dialog;
    int idx;
    unsigned flags;                                      // +8
    tstring text;                                        // +0xc
};
struct DialogMgr {
    int count;                                           // +0
    DialogRec* recs;                                     // +4
    int f8;                                              // +8
    int fc;                                              // +0xc
    char pad10[8];
    int cur;                                             // +0x18
    char pad1c[8];
    rawline list[0x400];                                 // +0x24
    void select(rawline* l);                             // 0x460d80
    int get_c8();                                        // 0x460620
    int speaker();                                       // 0x460640
    void advance();                                      // 0x460ff0
    void advance_all(int* n);                            // 0x461140
    tstring line_text(int i);                            // 0x4606a0 (ret 8)
    int line_flags(int i);                               // 0x4606c0
};
extern DialogMgr g_n_dialogs;                            // 0x8ed50c8 (?g_n_dialogs@@3UDialogMgr@@A)
extern rawline* g_dlg_lines;                             // 0x8ed50ec (?g_dlg_lines@@3PAUrawline@@A)

// ---- widget slots (see s004dfd50 / s004e05c0) -------------------------------------------------------
// used slots: 0x58/0x64/0x68/0x6c/0x70 pos & scale, 0x74 flag, 0x80 track, 0xa4 text, 0xb0 advance, 0xb4 clear
struct Widget {
    virtual void s00(void* p); virtual void s01(void* p); virtual void s02(void* p);
    virtual void s03(void* p); virtual void s04(void* p); virtual void s05(void* p);
    virtual void s06(void* p); virtual void s07(void* p); virtual void s08(void* p);
    virtual void s09(void* p); virtual void s10(void* p); virtual void s11(void* p);
    virtual void s12(void* p); virtual void s13(void* p); virtual void s14(void* p);
    virtual void s15(void* p); virtual void s16(void* p); virtual void s17(void* p);
    virtual void s18(void* p); virtual void s19(void* p); virtual void s20(void* p);
    virtual void s21(void* p);
    virtual void set_pos(const float* p);                // +0x58
    virtual void s23(void* p); virtual void s24(void* p);
    virtual void set_scale(const float* p);              // +0x64
    virtual void s26(void* p);
    virtual void set_size(const float* p);               // +0x6c
    virtual void set_pos2(const float* p);               // +0x70
    virtual void set_flag(const int* f);                 // +0x74
    virtual void s31(void* p); virtual void s32(void* p); virtual void s33(void* p);
    virtual void s34(void* p); virtual void s35(void* p); virtual void s36(void* p);
    virtual void s37(void* p); virtual void s38(void* p);
    virtual void set_track(int i, void* m);              // +0x80
    virtual void s40(void* p); virtual void s41(void* p); virtual void s42(void* p);
    virtual void s43(void* p); virtual void s44(void* p); virtual void s45(void* p);
    virtual void s46(void* p); virtual void s47(void* p); virtual void s48(void* p);
    virtual void s49(void* p);
    virtual void set_text(const tstring* in, tstring* out);   // +0xa4
    virtual void s51(void* p); virtual void s52(void* p); virtual void s53(void* p);
    virtual void s54(void* p); virtual void s55(void* p); virtual void s56(void* p);
    virtual void s57(void* p);
    virtual void advance();                              // +0xb0
    virtual void clear();                                // +0xb4
};

// the conversation screen (`this` of 0x4f73c0)
struct ConvScreen {
    char pad0[0x174];
    Widget* label_speaker;                               // +0x174
    Widget* label_text;                                  // +0x178
    Widget* body;                                        // +0x17c
    char pad180[8];
    Widget* choice;                                      // +0x188
    char pad18c[4];
    Widget* choice_buttons[0x400];                       // +0x190
    int n_lines;                                         // +0x1190
    char pad1194[0x1204 - 0x1194];
    int speaker_id;                                      // +0x1204
    void refresh(int sel, int* out);                     // 0x4f73c0
};

struct base_mesh;
struct resmgr { base_mesh* get_object(const tstring& name); };   // 0x419540
extern resmgr g_alloc;                                   // 0x5d6fa0
struct base_mesh {
    base_mesh* clone();                                  // 0x41c6d0
    void set_color(const float* c);                      // 0x47af30 (four floats)
};

float decode_utf8(const tstring* s, int* out_count, const void* font);   // 0x487b00 (pixel width)

// ---- conversation screen ----------------------------------------------------------------------------
// @ 0x4f73c0
void ConvScreen::refresh(int sel, int* out)
{
    if (n_lines > 1) {
        if (sel < 0)
            goto tail;
        g_n_dialogs.select(&g_dlg_lines[sel]);
        *out = g_n_dialogs.get_c8();
        g_n_dialogs.advance();
    } else {
        if (n_lines != 1) {
            if (sel != -2)
                goto tail;
        } else {
            g_n_dialogs.select(g_dlg_lines);
            goto tail;
        }
        *out = g_n_dialogs.get_c8();
        g_n_dialogs.advance();
    }
    {
        static float one = 1.0f;
        float y_a = 0.0f;                                // original: [esp+0x24] (int slot reused as float)
        float text_px = 0.0f;                            // original: [esp+0x20]
tail:
        speaker_id = g_n_dialogs.speaker();
        {
            tstring name(troop_text(speaker_id));        // original: snprintf("%s:") into [esp+0x140]
            tstring tmp;
            label_speaker->set_text(&name, &tmp);
        }
        {
            tstring text = g_empty_string;               // original: copy ctor from 0x8ee30f0
            tstring out2;
            label_text->set_text(&(text.strip_tags()), &out2);
        }
        for (int i = 0; i < 0x400; i++)
            choice_buttons[i]->advance();
        int n = n_lines;                                 // original: pre-advance count stays in [esp+0x10]
        g_n_dialogs.advance_all(&n_lines);
        if (n > 1) {
            body->advance();
            y_a = setting_float("conv_sentence_y_a");
            for (int i = 0; i < n_lines; i++) {
                choice_buttons[i]->clear();
                tstring text = g_n_dialogs.line_text(i).strip_tags();
                unsigned flags = g_n_dialogs.line_flags(i);
                float ratio = setting_float("conv_sentence_width_ratio");
                (void)setting_float("conv_sentence_button_height");
                float size_y = setting_float("conv_sentence_size_y");
                float h = size_y * 0.65f;
                int count = 0;
                text_px = decode_utf8(&text, &count, 0) * ratio * 0.39f;
                if (size_y * 23.5f - 0.02f < text_px && (flags & 0x80000)) {
                    flags = 0x100;
                    choice_buttons[i]->set_flag((const int*)&flags);
                    do {
                        text_px -= y_a;
                        text_px += ratio;
                    } while (y_a < text_px - ratio);
                    float v = 0.02f;
                    choice_buttons[i]->set_scale(&v);
                } else {
                    int flag = 0x10;
                    choice_buttons[i]->set_flag(&flag);
                    y_a = i * 0.5f - ratio * 0.3f;
                }
                float sz[2];
                sz[0] = i * ratio;                       // original: [esp+0xbc]
                sz[1] = text_px;                         // original: [esp+0xc0]
                choice_buttons[i]->set_scale(sz);
                float pos[2];
                pos[0] = y_a;                            // original: [esp+0xb4]
                pos[1] = 0.0f;                           // original: [esp+0xb8]
                choice_buttons[i]->set_pos(pos);
                float sc[2];
                sc[0] = (float)i;                        // original: [esp+0x5c]
                sc[1] = 0.0f;                            // original: [esp+0x60]
                choice_buttons[i]->set_scale(sc);
                float size[2];
                size[0] = text_px;                       // original: [esp+0xac]
                choice_buttons[i]->set_size(size);
                choice_buttons[i]->set_pos((const float*)&text_px);
                float mv[2];
                mv[0] = (float)i;                        // original: [esp+0x2c]
                mv[1] = -y_a;                            // original: [esp+0x30]
                choice_buttons[i]->set_pos2(mv);
                {
                    tstring name("dlg_button");
                    base_mesh* bm = g_alloc.get_object(name)->clone();
                    float col[4] = { one, text_px, 0.0f, one };   // original: four 1.0f at +0x8c..+0xa4
                    bm->set_color(col);
                    choice_buttons[i]->set_track(2, bm);
                }
                {
                    tstring name("dlg_button_down");
                    base_mesh* bm = g_alloc.get_object(name)->clone();
                    float col[4] = { one, text_px, 0.0f, one };
                    bm->set_color(col);
                    choice_buttons[i]->set_track(1, bm);
                }
            }
        } else {
            choice->clear();
        }
        setting_float("conv_choices_min_y");
        (void)n;
    }
}

// ---- agent appearance -------------------------------------------------------------------------------
struct AgentBody { char pad0[0x304]; int f304; };        // +0x304 (0x420ec0 resets it)
struct Agent {
    char pad0[8]; int f8;                                // +8
    char padc[4]; int f10;                               // +0x10
    char pad14[0x88 - 0x14]; int f88; int f8c; int f90; int f94;
    char pad98[0x130 - 0x98]; int f130; int f134; int f138; int f13c[22]; int f194;
    char pad198[0x1a4 - 0x198]; int f1a4; int f1a8;
    char pad1ac[0x1d0 - 0x1ac]; int f1d0;
    char pad1d4[0x22c - 0x1d4]; int f22c; int f230;
    char pad234[0x240 - 0x234]; int f240; float f24c; AgentBody* body;   // +0x250
    char pad254[0x5df0 - 0x254]; int f5df0;              // +0x5df0 (equipment kind)
    char pad5df4[0x6528 - 0x5df4]; unsigned char f6528;  // +0x6528 (bit 0: no mount?)
    char pad6529[0x400]; int f9a9;  // placeholder
    float equipment_value() const;                       // 0x436050
    int f5c;                                             // +0x5c (also reached through the layout above)
    void on_wield_reset();                               // 0x445a20
    void cycle_slot(int i);                              // 0x44bc30
    void f470(int i, int j);                             // 0x470d80
    void init();                                         // 0x4f7d70
    void refresh();                                      // 0x4f7e00 (stdcall: argument on the stack)
};

void AgentEq_pick_slots(Agent* a);                       // 0x46b2d0 (stdcall, one arg)
void FUN_00420ec0(int x);                                // 0x420ec0
// 0x482240: pushes f8, f1d0, f1a8, f1a4, f194, f240, f13c, f8c, f94, f88, f10, body -- in this order
// @ 0x482240
void build_appearance(AgentBody* b, int f10, int f88, int f94, int* f8c, int* f13c,
                      int* f240, int* f194, int* f1a4, int* f1a8, int* f1d0, int f8);
// 0x47cfe0: pushes f8, f1d0, f130, f13c, f88, f10, body
// @ 0x47cfe0
void build_equipment(AgentBody* b, int f10, int f88, int* f13c, int* f130, int* f1d0, int f8);

// @ 0x4f7e00
void Agent::refresh()
{
    // original: body = [esi+0x250]; ecx = [body+0x304]; reset; f13c[0..0x16] = 0 (ecx stays the zero register,
    // edx the store pointer); the two builders; f24c = equipment_value() (x87 fstp); f470(0,20); f470(1,20)
    FUN_00420ec0(body->f304);
    int zero = 0;
    for (int p = 0; p < 0x17; p++)
        *(int*)((char*)f13c + p * 4) = zero;
    build_appearance(body, f10, f88, f94, &f8c, f13c, &f240, &f194, &f1a4, &f1a8, &f1d0, f8);
    build_equipment(body, f10, f88, f13c, &f130, &f1d0, f8);
    f24c = equipment_value();
    f470(0, 20);
    f470(1, 20);
}

// @ 0x4f7d70
void Agent::init()
{
    AgentEq_pick_slots(this);
    refresh();
    f22c = -1;
    f230 = -1;
    on_wield_reset();
    if (((int*)this)[0x5c / 4] == ((int*)this)[8] && ((f6528 & 1) || (1 & (*(int*)((char*)0x5e2a00 + f5df0 * 0x58 + 0x20) >> 1)))) {
        cycle_slot(0);
        cycle_slot(1);
    }
    f470(0, 20);
    f470(1, 20);
}
