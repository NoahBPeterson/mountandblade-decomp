#include "./facegen_misc.h"
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/skin.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../game_globals.h"      // g_limit_hair_colors, g_module_name ... (canonical global declarations)
#include "../../rgl/stream.h"      // text_stream g_debug_log (0x5a9f30): its buf (+0x20 = 0x5a9f50) is the status text the save/load screen draws
#include "../strat/gui_manager.h"      // MsgLog::build_scene 0x4e6050 (listed directly: the resolution audit reads only the source's own includes)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/ptr_tag.h"      // ptr_tag (std::_Nonscalar_ptr_iterator_tag)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
text_stream g_debug_log;   // 0x5a9f30 the debug text log (constructed by the compiler: the original's static initializer 0x547aa0; destroyed by 0x548170)
int g_5e28c8;   // 0x5e28c8
Animator g_anim;   // 0x8ed4f60 the skin table / character builder (0x130 bytes: set, item, cur, variant, material, the skin vector at +0x18, count at +0x28, 63 weights, scale, time); the one object behind g_anim.skin_vec (first pointer 0x8ed4f7c), g_anim.char_count 0x8ed4f88


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
    f->m = m;
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
    // skin_vec is a std::vector<Skin>: operator[] goes through the iterator
    // class, whose temporary holds `first` as a value of its own (callee-saved esi, `add ecx,esi` before the pushes)
    skin_vec[cur].relax(-1, weights);
}

// 0x432510 clamp_index: the canonical definition is in rgl/xmlcfg/game_util.cpp; this unit saw the body too (refresh
// packs flags / zero into slots shared with later locals, which cl only does when it knows the callee keeps no address)
__declspec(noinline) inline int __cdecl clamp_index(const int* v, const int* lo, const int* hi)
{
    if (*v < *lo) return *lo;
    if (*v >= *hi) return *hi - 1;
    return *v;
}

// the face parameters as one inline setter with by-value arguments: every slider value is read before the first store
// (both floats travel on the x87 stack), and the skin row is looked up after the stores
static inline void anim_set_face(Animator& a, int set, int item, int material, int beard, float scale, float time)
{
    a.item = item;
    a.scale = scale;
    a.set = set;
    a.time = time;
    a.material = material;
    int r = beard % a.skin_vec[a.cur].n_hair_tex;
    a.variant = r;
    a.f10 = r;
}

// @ 0x4eb3c0
void FaceGen::refresh()
{
    int n = g_anim.skin_vec[skin].n_face_keys - 1;
    for (int i = 0; i < n; i++)
        g_anim.weights[i] = deform_rows[i]->get168();
    f360 = sl_age->get168();
    f364 = sl_hair_color->get168();
    ent->clear();
    int flags = g_game->troops[w14].flags32 & 0xf;
    int zero = 0;
    g_anim.cur = clamp_index(&flags, &zero, &g_anim.char_count);
    anim_set_face(g_anim, f350 - 1, f354 - 1, f35c, f358, f360, f364);
    rgl::lod_mesh *a, *b, *c;
    g_anim.collect(g_module_settings.limit_hair_colors != 0, &a, &b, &c);
    ent->lods.push_back(a);
    if (b)
        ent->lods.push_back(b);
    if (c)
        ent->lods.push_back(c);
    g_anim.pack_state(&state);
    char buf[2];                                         // one hex digit and its terminator
    for (int i = 0; i < 4; i++) {
        unsigned __int64 v = state.word[i];
        for (int j = 15; j >= 0; j--) {
            _itoa((int)(v & 0xf), buf, 16);
            keys[i][j] = buf[0];
            v >>= 4;
        }
        keys[i][16] = 0;
    }
    if (cam_btn) {
        char line[512];
        sprintf(line, "k=%s%s%s%s", keys[0], keys[1], keys[2], keys[3]);
        tstring text(line);
        cam_btn->set_text(text);
    }
}

// ---- std::vector<Skin> members (0x4eb810 is ~SkinVec, include/skin.h) ----------------------------------------------------
// 0x4eb860 is vector<Skin>::erase (copy down, destroy the tail; used by the map editor and vec_model::resize), 0x4eb8c0 its
// protected _Ucopy (std::_Uninitialized_copy through _Ptr_cat): explicit instantiations of just those two members.
template std::vector<Skin>::iterator std::vector<Skin>::erase(std::vector<Skin>::iterator, std::vector<Skin>::iterator);   // @ 0x4eb860
template Skin* std::vector<Skin>::_Ucopy(std::vector<Skin>::const_iterator, std::vector<Skin>::const_iterator, Skin*);    // @ 0x4eb8c0

// @ 0x4eb8e0 GameApp::Render (whole body reconstructed at the 13-byte symbol; 0x4eb8ed is its interior
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

    if (g_cfg.show_framerate) {
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

                int y = 40;
                tstring ts;
                for (int i = 0; i < g_debug_log.buf.len; ++i) {
                    char* b = g_debug_log.buf.buf;
                    if (b[i] == ';') {
                        char* s = ts.buf ? ts.buf : (char*)&ts.buf;
                        font->DrawText(2.0f, (float)y, 0xff64ff64, s, 0);
                        y += 12;
                        ts.assign("");
                    } else {
                        ts.push_back(b[i]);
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
        out->word[k] = 0;
    out->header |= (unsigned __int64)(item + 1);
    out->header |= (unsigned __int64)(set + 1) << 6;
    out->header |= (unsigned __int64)material << 12;
    out->header |= (unsigned __int64)variant << 18;
    out->header |= (unsigned __int64)(int)(time * 63.0f) << 24;
    out->header |= (unsigned __int64)(int)(scale * 63.0f) << 30;
    int i = 0;
    if (i < skin_vec[cur].n_face_keys) {
        do {
            int wi = 1;
            int sh = i * 3;
            if (sh >= 0x3f) {
                wi = 2;
                sh -= 0x3f;
            }
            float fv = weights[i] * 7.0f;
            int fi = grid_count(fv);
            out->word[wi] |= (unsigned __int64)fi << sh;
        } while (++i < skin_vec[cur].n_face_keys);
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
