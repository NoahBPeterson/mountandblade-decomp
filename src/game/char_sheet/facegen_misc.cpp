#include "./facegen_misc.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/skin_set.h"      // (listed directly: the resolution audit scans only the source's own includes)

// @ 0x4eb270
void FaceGen::update_view(int unused)
{
    mat4 m;
    identity_pos((pos*)&m);
    float angle = 1.5707964f;
    ((rgl::matrix*)&m)->rotate_about_s(&angle);
    vector4 t;
    t.x = m.u.x * f16c;
    t.y = m.u.y * f16c;
    t.z = m.u.z * f16c;
    m.o.x = t.x + m.o.x;
    m.o.y = t.y + m.o.y;
    m.o.z = t.z + m.o.z;
    rgl::frustum* f = &fr;
    *(mat4*)((char*)f + 0xc8) = m;
    f->build_planes();
    body->view = *f;
}

// @ 0x4eb320
void Animator::randomise()
{
    int n = 0x3f;
    float* out = weights;
    do {
        int r1 = rand() % 0x3dc9;
        float first = (float)r1 * 6.322312e-05f;
        int r2 = rand() % 0x3dc9;
        *out = ((float)r2 * 6.322312e-05f + first) * 0.5f;
        ++out;
        --n;
    } while (n != 0);
    int r = rand() % 0x3dc9;
    time = (float)r * 6.322312e-05f;
    SkinSet* base = (SkinSet*)((char*)skins + cur * 0x1f8);
    ((FaceRace*)base)->relax(-1, weights);
}

// @ 0x4eb3c0
void FaceGen::refresh()
{
    int n = g_skins_begin[skin].n_elems - 1;
    for (int i = 0; i < n; i++)
        (&g_dummy_mat)[i] = deform_rows[i]->get_float();
    f360 = sl_age->get_float();
    f364 = sl_hair_color->get_float();
    ((UiVt*)ent)->refresh_anim();
    int flags = *(int*)(g_game->chars_c + troop * 0x508 + 0x30) & 0xf;
    int zero = 0;
    g_cur = clamp_index(&flags, &zero, &g_char_count);
    g_dummy1 = f364;
    g_dummy0 = f360;
    g_anim.material = f35c;
    g_anim.set = f350 - 1;
    g_anim.item = f354 - 1;
    int r = f358 % g_skins_begin[g_cur].div;
    g_anim.variant = r;
    g_anim.f10 = r;
    void *a, *b, *c;
    g_anim.collect(g_xa181ac != 0, (rgl::lod_mesh**)&a, (rgl::lod_mesh**)&b, (rgl::lod_mesh**)&c);
    void* p;
    p = a;
    ((std::vector<named_object*>*)&ent->lods)->push_back((named_object* const&)p);
    p = b;
    if (p)
        ((std::vector<named_object*>*)&ent->lods)->push_back((named_object* const&)p);
    p = c;
    if (p)
        ((std::vector<named_object*>*)&ent->lods)->push_back((named_object* const&)p);
    g_anim.pack_state(&state);
    char* kp = keys[0] + 16;
    for (int i = 0; i < 4; i++) {
        unsigned __int64 v = ((unsigned __int64*)&state)[i];
        for (int j = 15; j >= 0; j--) {
            char buf[4];
            _itoa((int)(v & 0xf), buf, 16);
            kp[j - 16] = buf[0];
            v >>= 4;
        }
        *kp = 0;
        kp += 0x28;
    }
    if (cam_btn) {
        char buf[512];
        sprintf(buf, "k=%s%s%s%s", keys[0], keys[1], keys[2], keys[3]);
        tstring text(buf);
        vector2 pos;
        cam_btn->set_text(&pos, text);
    }
}

// ---- skin-record array helpers (0x4eb810/0x4eb860, used by the map editor) -------------------------------
struct BigArray {
    char pad0[4];
    SkinSet* begin;                                         // +4
    SkinSet* end;                                           // +8
    SkinSet* cap;                                           // +0xc
    void destroy();
    void erase(SkinSet** out, SkinSet* first, SkinSet* last);       // 0x4eb860
};
// (array_copy_range 0x4da880 is copy_configs_forward, cs_common.h)

// @ 0x4eb810
void BigArray::destroy()
{
    if (begin) {
        SkinSet* p = begin;
        SkinSet* stop = end;
        while (p != stop) {
            p->destroy();
            p = (SkinSet*)((char*)p + 0x1f8);
        }
        operator delete(begin);
    }
    begin = 0;
    end = 0;
    cap = 0;
}

// @ 0x4eb860
void BigArray::erase(SkinSet** out, SkinSet* first, SkinSet* last)
{
    SkinSet** fp = &first;
    if (*fp != last) {
        SkinSet* p = (SkinSet*)copy_configs_forward((loading_config*)last, (loading_config*)this->end, (loading_config*)*fp);
        SkinSet* e = this->end;
        while (p != e) {
            p->destroy();
            p = (SkinSet*)((char*)p + 0x1f8);
        }
        this->end = p;
    }
    *out = *fp;
}

// ---- event forwarding -----------------------------------------------------------------------------------
struct Forwarder {
    void forward(int a, int b, int c);                  // 0x4eb8c0
};
// (func_4e4340 is std::_Uninit_copy<loading_config>, cs_common.h)

// @ 0x4eb8c0
void Forwarder::forward(int a, int b, int c)
{
    std::_Uninit_copy(std::vector<loading_config>::const_iterator((loading_config*)a), std::vector<loading_config>::const_iterator((loading_config*)b), (loading_config*)c, *(std::allocator<loading_config>*)this, std::_Nonscalar_ptr_iterator_tag());   // 0x4e4340
}

// @ 0x4eb8e0 save-load UI entry (whole body reconstructed at 0x4eb8e0; 0x4eb8ed is its interior
// continuation and is not separately callable)
#include "../game_globals.h"
extern int g_5e28c8;                                    // 0x5e28c8
extern int g_a16c58;                                    // 0xa16c58
extern int g_5d75c8;                                    // 0x5d75c8 (view width)
extern int g_5d75cc;                                    // 0x5d75cc (view height)
extern void* g_a17f28;                                  // 0xa17f28 (IDirect3DDevice9*)
extern char* g_5a9f54;                                  // 0x5a9f54 (save/load text)
extern int g_5a9f5c;                                    // 0x5a9f5c (text length)
extern int g_5a9fa0;                                    // 0x5a9fa0
extern int g_5a9fb8;                                    // 0x5a9fb8
extern int g_5a9fb0;                                    // 0x5a9fb0
extern tstring g_5a9f50;                                // 0x5a9f50
extern const char g_54a480[];                           // 0x54a480 ("")

struct Viewport { int x, y, w, h; float minz, maxz; };
typedef void (__stdcall* DevCall2)(void*, void*);
typedef void (__stdcall* DevCall3i)(void*, int, int);
typedef void (__stdcall* PanelCall7)(void*, int, int, int, int, float, int);
typedef int  (__stdcall* PanelCall1i)(void*);
typedef void (__stdcall* PanelCall1)(void*);

struct SaveLoadUi {
    int run();                                          // 0x4eb8e0
    // (update_view_rect 0x4bace0 is Core's, cs_common.h)
};

int SaveLoadUi::run()
{
    if (g_scene_dirty) {
        g_scene_dirty = 0;
        g_editor.InvalidateDeviceObjects();
        g_editor.RestoreDeviceObjects();
    }
    ++g_5e28c8;
    ++g_a16c58;
    ((Core*)(void*)this)->update_view_rect();   // 0x4bace0 (the Core object is g_app_obj 0x8ee3c48)

    Viewport vp;
    vp.x = 0; vp.y = 0;
    vp.w = g_5d75c8; vp.h = g_5d75cc;
    vp.minz = 0.0f; vp.maxz = 1.0f;
    ((DevCall2)(*(void***)g_a17f28)[0xbc / 4])(g_a17f28, &vp);

    void* panel = *(void**)((char*)this + 0xf8);
    ((PanelCall7)(*(void***)panel)[0xac / 4])(panel, 0, 0, 2, 0x302010, 1.0f, 0);

    g_msglog.build_scene();
    ((RenderPass*)&g_renderer)->render(0);   // 0x47e520

    if (g_5a9fa0 != 0) {
        void* w = *(void**)((char*)this + 0xf8);
        if (((PanelCall1i)(*(void***)w)[0xa4 / 4])(w) >= 0) {
            Viewport vp2;
            vp2.x = 0; vp2.y = 0;
            vp2.w = g_5d75c8; vp2.h = g_5d75cc;
            vp2.minz = 0.0f; vp2.maxz = 1.0f;
            ((DevCall2)(*(void***)g_a17f28)[0xbc / 4])(g_a17f28, &vp2);

            CD3DFont* font = *(CD3DFont**)((char*)this + 0x348);
            font->DrawText(2.0f, 0.0f, 0xffffff00, (const char*)this + 0x2da, 0);

            if (g_5a9fb8 != 0 || g_5a9fb0 != 0) {
                Viewport vp3;
                vp3.x = 0; vp3.y = 0;
                vp3.w = g_5d75c8; vp3.h = g_5d75cc;
                vp3.minz = 0.0f; vp3.maxz = 1.0f;
                ((DevCall2)(*(void***)g_a17f28)[0xbc / 4])(g_a17f28, &vp3);

                tstring ts;
                int y = 40;
                for (int i = 0; i < g_5a9f5c; ++i) {
                    char ch = g_5a9f54[i];
                    if (ch == ';') {
                        char* s = ts.buf ? ts.buf : (char*)&ts.buf;
                        font->DrawText(2.0f, (float)y, 0xff64ff64, s, 0);
                        y += 12;
                        ts.assign("");
                    } else {
                        ts.push_back(ch);
                    }
                }
                if (ts.len != 0) {
                    char* s = ts.buf ? ts.buf : (char*)&ts.buf;
                    font->DrawText(2.0f, (float)y, 0xff64ff64, s, 0);
                }
            }

            ((DevCall3i)(*(void***)g_a17f28)[0x104 / 4])(g_a17f28, 0, 0);
            ((DevCall3i)(*(void***)g_a17f28)[0x104 / 4])(g_a17f28, 1, 0);
            void* w2 = *(void**)((char*)this + 0xf8);
            ((PanelCall1)(*(void***)w2)[0xa8 / 4])(w2);
        }
    }

    g_5a9f50.assign("");
    return 0;
}

// 0x4eb8ed: interior continuation of SaveLoadUi::run (the 645 bytes after the 13-byte prologue at
// 0x4eb8e0). It is not a separately callable function and has no source symbol of its own; the body is
// the one reconstructed above.

// @ 0x4eb660
void Animator::pack_state(PackedState* out)
{
    out->a = out->b = out->c = out->d = 0;
    out->a = (unsigned __int64)(item + 1);
    out->a |= (unsigned __int64)(set + 1) << 6;
    out->a |= (unsigned __int64)material << 12;
    out->a |= (unsigned __int64)variant << 18;
    out->a |= (unsigned __int64)(int)(time * 63.0f) << 24;
    out->a |= (unsigned __int64)(int)(scale * 63.0f) << 30;
    unsigned __int64* w = (unsigned __int64*)out;
    int bit = 0;
    float* v = weights;
    for (int i = 0; i < ((SkinSet*)((char*)skins + cur * 0x1f8))->n_elems; i++) {
        int sh = bit;
        int wi = 1;
        if (sh >= 0x3f) {
            sh -= 0x3f;
            wi = 2;
        }
        float fv = *v * 7.0f;
        int fi = grid_count(fv);
        w[wi] |= (unsigned __int64)fi << sh;
        bit += 3;
        ++v;
    }
}

// @ 0x4ebb80
extern int g_no_cam;                                    // 0x5a9fb0
#include "../main_menu.h"
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_editor_obj.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
extern main_menu g_main_menu;                           // 0xa73980
struct MenuVars {
    char pad0[4];
    int** rows;                                         // +4
    unsigned count;                                     // +8
};
extern MenuVars g_menu_vars;                            // 0xa739f8
extern unsigned g_a73a04;                               // 0xa73a04
extern unsigned g_a73a08;                               // 0xa73a08

void __cdecl menu_variables_loaded(char flag)
{
    if (!flag && !g_no_cam)
        return;
    load_game_variables();   // 0x4c0520
    if (flag)
        return;
    unsigned u = g_a73a08 + g_a73a04 - 1;
    unsigned q = u >> 2;
    unsigned r = u - q * 4;
    MenuVars* vars = &g_menu_vars;
    unsigned count = vars->count;
    int** rows = vars->rows;
    if (count <= q)
        q -= count;
    int value = rows[q][r];
    g_main_menu.set_state(value);
    g_msglog.add("Menu variables are loaded...", 0xffff0000);
}
