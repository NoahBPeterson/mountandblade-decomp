#include "./facegen_misc.h"
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/skin.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../game_globals.h"      // g_limit_hair_colors, g_module_name ... (canonical global declarations)
#include "../../rgl/stream.h"      // text_stream g_debug_log (0x5a9f30): its buf (+0x20 = 0x5a9f50) is the status text the save/load screen draws
#include "../strat/gui_manager.h"      // MsgLog::build_scene 0x4e6050 (listed directly: the resolution audit reads only the source's own includes)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
text_stream g_debug_log;   // 0x5a9f30 the debug text log (constructed by the compiler: the original's static initializer 0x547aa0; destroyed by 0x548170)
int g_5e28c8;   // 0x5e28c8
Animator g_anim;   // 0x8ed4f60 the skin table / character builder (0x130 bytes: set, item, cur, variant, material, the skin vector at +0x18, count at +0x28, 63 weights, scale, time); the one object behind g_anim.skin_vec.first 0x8ed4f7c, g_anim.char_count 0x8ed4f88


// @ 0x4eb270
void FaceGen::update_view(int unused)
{
    mat4 m;
    identity_pos((pos*)&m);
    float angle = 1.5707964f;
    ((rgl::matrix*)&m)->rotate_about_s(&angle);
    vector4 t = m.u * zoom;
    m.o.x = m.o.x + t.x;
    m.o.y = t.y + m.o.y;
    m.o.z = t.z + m.o.z;
    rgl::frustum* f = &fr;
    *(mat4*)((char*)f + 0xc8) = m;
    f->build_planes();
    body->scene3d.fr = *f;
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
    Skin* base = &skin_vec.first[cur];
    base->relax(-1, weights);
}

// @ 0x4eb3c0
void FaceGen::refresh()
{
    int n = g_anim.skin_vec.first[skin].n_face_keys - 1;
    for (int i = 0; i < n; i++)
        g_anim.weights[i] = deform_rows[i]->get168();
    f360 = sl_age->get168();
    f364 = sl_hair_color->get168();
    ent->clear();
    int flags = *(int*)(g_game->chars_c + w14 * 0x508 + 0x30) & 0xf;
    int zero = 0;
    g_anim.cur = clamp_index(&flags, &zero, &g_anim.char_count);
    g_anim.time = f364;
    g_anim.scale = f360;
    g_anim.material = f35c;
    g_anim.set = f350 - 1;
    g_anim.item = f354 - 1;
    int r = f358 % g_anim.skin_vec.first[g_anim.cur].n_hair_tex;
    g_anim.variant = r;
    g_anim.f10 = r;
    void *a, *b, *c;
    g_anim.collect(g_module_settings.limit_hair_colors != 0, (rgl::lod_mesh**)&a, (rgl::lod_mesh**)&b, (rgl::lod_mesh**)&c);
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
        cam_btn->set_text((float*)&pos, &(text));
    }
}

// ---- skin-record array helpers (0x4eb810 is ~SkinVec, include/skin.h; 0x4eb860 erase, used by the map editor) -------------------------------
struct BigArray {
    char pad0[4];
    Skin* begin;                                         // +4
    Skin* end;                                           // +8
    Skin* cap;                                           // +0xc
    void erase(Skin** out, Skin* first, Skin* last);       // 0x4eb860
};
// (array_copy_range 0x4da880 is copy_configs_forward, cs_common.h)

// @ 0x4eb810
SkinVec::~SkinVec()
{
    if (first) {
        Skin* p = first;
        Skin* stop = last;
        while (p != stop) {
            p->~Skin();
            ++p;
        }
        operator delete(first);
    }
    first = 0;
    last = 0;
    end = 0;
}

// @ 0x4eb860
void BigArray::erase(Skin** out, Skin* first, Skin* last)
{
    if (first != last) {
        Skin* end0 = this->end;
        Skin* p = ((Skin* (__cdecl*)(Skin*, Skin*, Skin*, Skin*))copy_configs_forward)(last, end0, first, end0);
        Skin* e = this->end;
        for (Skin* i = p; i != e; ++i)
            i->~Skin();
        this->end = p;
    }
    *out = first;
}

// ---- event forwarding -----------------------------------------------------------------------------------
struct Forwarder {
    void forward(int a, int b, int c);                  // 0x4eb8c0
};
// (func_4e4340 is std::_Uninit_copy<Skin>, cs_common.h)

// @ 0x4eb8c0
void Forwarder::forward(int a, int b, int c)
{
    std::_Uninitialized_copy(std::vector<Skin>::const_iterator((Skin*)a), std::vector<Skin>::const_iterator((Skin*)b), (Skin*)c, *(std::allocator<Skin>*)this);   // 0x4e4340 through _Ptr_cat
}

// @ 0x4eb8e0 save-load UI entry (whole body reconstructed at 0x4eb8e0; 0x4eb8ed is its interior
// continuation and is not separately callable)
#include "../game_globals.h"
#include "../../include/renderer.h"       // g_render.dev: the Direct3D device (0xa17f28)
#include "../window_mgr.h"                 // g_id_stack (0xa739f8, a73a04, a73a08), current_screen_id

struct Viewport { int x, y, w, h; float minz, maxz; };

HRESULT GameApp::Render()
{
    if (g_scene_dirty) {
        g_scene_dirty = 0;
        g_app_obj.GameApp::InvalidateDeviceObjects();
        g_app_obj.GameApp::RestoreDeviceObjects();
    }
    ++g_5e28c8;
    ++g_batch.frame;
    update_view_rect();   // 0x4bace0

    Viewport vp;
    vp.x = 0; vp.y = 0;
    vp.w = g_gui.screen_w; vp.h = g_gui.screen_h;
    vp.minz = 0.0f; vp.maxz = 1.0f;
    g_render.dev->SetViewport((D3DVIEWPORT9*)&vp);

    m_pd3dDevice->Clear(0, 0, 2, 0x302010, 1.0f, 0);

    g_gui.build_scene();
    g_batch.render(0);   // 0x47e520

    if (g_cfg.show_framerate != 0) {
        if (m_pd3dDevice->BeginScene() >= 0) {
            Viewport vp2;
            vp2.x = 0; vp2.y = 0;
            vp2.w = g_gui.screen_w; vp2.h = g_gui.screen_h;
            vp2.minz = 0.0f; vp2.maxz = 1.0f;
            g_render.dev->SetViewport((D3DVIEWPORT9*)&vp2);

            font->DrawText(2.0f, 0.0f, 0xffffff00, m_strFrameStats, 0);

            if (g_cfg.debug_mode != 0 || g_cfg.enable_edit_mode != 0) {
                Viewport vp3;
                vp3.x = 0; vp3.y = 0;
                vp3.w = g_gui.screen_w; vp3.h = g_gui.screen_h;
                vp3.minz = 0.0f; vp3.maxz = 1.0f;
                g_render.dev->SetViewport((D3DVIEWPORT9*)&vp3);

                tstring ts;
                int y = 40;
                for (int i = 0; i < g_debug_log.buf.len; ++i) {
                    char ch = g_debug_log.buf.buf[i];
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

            g_render.dev->SetTexture(0, 0);
            g_render.dev->SetTexture(1, 0);
            m_pd3dDevice->EndScene();
        }
    }

    g_debug_log.buf.assign("");
    return 0;
}

// 0x4eb8ed: interior continuation of GameApp::Render (the 645 bytes after the 13-byte prologue at
// 0x4eb8e0). It is not a separately callable function and has no source symbol of its own; the body is
// the one reconstructed above.

// @ 0x4eb660
void Animator::pack_state(PackedState* out)
{
    for (int k = 0; k < 4; k++)
        ((unsigned __int64*)out)[k] = 0;
    out->a |= (unsigned __int64)(item + 1);
    out->a |= (unsigned __int64)(set + 1) << 6;
    out->a |= (unsigned __int64)material << 12;
    out->a |= (unsigned __int64)variant << 18;
    out->a |= (unsigned __int64)(int)(time * 63.0f) << 24;
    out->a |= (unsigned __int64)(int)(scale * 63.0f) << 30;
    int i = 0;
    if (i < skin_vec.first[cur].n_face_keys) {
        do {
            int wi = 1;
            int sh = i * 3;
            if (sh >= 0x3f) {
                wi = 2;
                sh -= 0x3f;
            }
            float fv = weights[i] * 7.0f;
            int fi = grid_count(fv);
            ((unsigned __int64*)out)[wi] |= (unsigned __int64)fi << sh;
        } while (++i < skin_vec.first[cur].n_face_keys);
    }
}
// @ 0x4ebb80

#include "../window_mgr.h"
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)

void __cdecl menu_variables_loaded(char flag)
{
    if (!flag && !g_cfg.enable_edit_mode)
        return;
    ((void (__fastcall*)(void*))load_game_variables)(&g_app_obj);   // 0x4c0520 (a member of the application object in the original: ecx is loaded and ignored)
    if (flag)
        return;
    int value = g_windows.dq[g_windows.dq.size() - 1];   // [size - 1] rather than back(): the offset folds into `lea [off + size - 1]`
    g_windows.set_state(value);
    g_gui.add("Menu variables are loaded...", 0xffff0000);
}
