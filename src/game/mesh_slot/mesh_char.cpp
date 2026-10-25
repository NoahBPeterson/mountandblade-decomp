// mesh_char.cpp: TU 63 "mesh", part 4 (0x47f650..0x47ff60): the character mesh collector and its animation-state wrapper,
// the scene-object loader and the shadow-map passes. See MIGRATION.md.
#include "./mesh_slot.h"
#include "../terrain_a/game_state.h"
#include "../terrain_b/scene_file.h"
#include "../../include/msg_log.h"
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)

using namespace rgl;
// src/game/terrain_b/poly_mesh.h (pulled in through mesh_slot.h) declares a global ::vat with the same signature
#define vat rgl::vat

// the resource manager as this TU calls it: `resmgr` (mesh_slot.h) for the lookups of core/resmgr.h, the resource_manager of src/rgl/resource.h
// for resolve_shader (0x418e90); 0x4175f0 is the free stdcall clear_f14 of rgl/mem_stream.cpp (the folded release(res), which ignores ecx)
#define g_rm (*(::resmgr*)&g_resmgr)
void __stdcall clear_f14(named_object* r);                                 // 0x4175f0

extern MsgLog g_gui;                          // 0x5d7080 (MsgLog::add 0x452c00, ret 0x14, is the message log's own member)
// 0x47e520 is RenderPass::render (mesh_pass.cpp), the pass driver of the scene registry object g_batch (0x5fe898)
struct RenderPass {
    bool render(int pass);   // 0x47e520
};
extern RenderPass g_batch;                   // 0x5fe898

// EChar::collect lives next to its caller (the original's tail call)
struct EChar_c : EChar {
    __declspec(noinline) void collect(lod_mesh** a, lod_mesh** b, lod_mesh** c,
                                      const float* weights, int item, int set, int material,
                                      int variant, float scale, float time);   // 0x47f650
};

// @ 0x47f650
NOINL void EChar_c::collect(lod_mesh** out_a, lod_mesh** out_b, lod_mesh** out_c, const float* weights,
                      int item, int set, int material, int variant, float scale, float time)
{
    int remap[65536];
    int color;
    int color2;
    int v;

    *out_a = 0;
    *out_b = 0;
    *out_c = 0;

    if (set_count == 0)
        set = -1;
    else if (set >= 0)
        set %= set_count;

    material %= mat_count;

    v = variant;
    if (name_count == 0)
        v = -1;
    if (variant_count == 0)
        variant = -1;

    *out_a = ((lod_mesh_ms*)ref->base)->clone();

    if (set < 0 || set >= set_count) {
        set = -1;
    } else {
        *out_b = ((lod_mesh_ms*)sets[set].mesh)->clone();
    }

    mesh* m = (*out_a)->lists[0].v[0];

    if (!m->local_a0) {
        m->v90 = m->parent->v90;
        m->local_a0 = 1;
    }
    m->ensure_local();

    blend(m, 0, weights);

    if (*out_b != 0) {
        blend_all(*out_b, &blend_a[set], weights);
        build_mirror((*out_b)->lists[0].v[0], remap, &color2, 0);
    }

    m->compute_normals();
    build_mirror(m, remap, &color2, 1);

    unsigned alpha = ((unsigned)(scale * scale * 255.0f)) << 24;

    for (unsigned i = 0; i < m->v50.size(); i++) {
        m->v50[i].color &= 0xffffff;
        m->v50[i].color |= alpha;
    }

    color = -1;
    color2 = alpha | 0xffffff;

    Ramp* mrecs = (Ramp*)mats;
    if (material >= 0) {
        m->res = mrecs[material].res0;
        if ((*out_a)->count > 1) {
            for (int i = 1; i < (*out_a)->count; i++)
                (*out_a)->lists[i].v[0]->res = mrecs[material].res1;
        }
        color = mrecs[material].eval(&time);
    }

    if (item >= 0 && item_count > 0 && variant_count > 0 && item < item_count && variant >= 0) {
        *out_c = ((lod_mesh_ms*)items[item])->clone();
        blend_all(*out_c, &blend_b[item], weights);
    }

    if (*out_c != 0) {
        if (variant < 0)
            variant = 0;
        else if (variant > variant_count - 1)
            variant = variant_count - 1;

        (*out_c)->lists[0].v[0]->res = g_resmgr.get_material(*(rgl::string*)&names_a[variant]);
        (*out_c)->lists[0].v[0]->color = color;
        (*out_c)->lists[0].v[0]->f40 = color2;
        if ((*out_c)->count > 1) {
            for (int i = 1; i < (*out_c)->count; i++) {
                (*out_c)->lists[i].v[0]->res = (*out_c)->lists[0].v[0]->res;
                (*out_c)->lists[i].v[0]->color = color;
                (*out_c)->lists[i].v[0]->f40 = color2;
            }
        }
    }

    if (*out_b != 0 && set >= 0 && v >= 0) {
        tstring name(names_b[v]);
        (*out_b)->lists[0].v[0]->res = g_resmgr.get_material(*(rgl::string*)&name);
        (*out_b)->lists[0].v[0]->color = color;
        (*out_b)->lists[0].v[0]->f40 = color2;
        if ((*out_b)->count > 1) {
            for (int i = 1; i < (*out_b)->count; i++) {
                (*out_b)->lists[i].v[0]->res = (*out_b)->lists[0].v[0]->res;
                (*out_b)->lists[i].v[0]->color = color;
                (*out_b)->lists[i].v[0]->f40 = color2;
            }
        }
    }
}

// @ 0x47fb10
void Animator::collect(char flag, lod_mesh** a, lod_mesh** b, lod_mesh** c)
{
    weights[chars[cur].bone_count - 1] = 1.0f;

    int v = variant;
    if (flag != 0) {
        int* vals = ((Ramp*)chars[cur].mats)[material].vals;
        unsigned n = vals ? ((Ramp*)chars[cur].mats)[material].vals_end - vals : 0;
        v = vals[(unsigned)variant % n];
    }

    ((EChar_c*)&chars[cur])->collect(a, b, c, weights, item, set, material, v, scale, time);
}

// @ 0x47fbd0
// The binary reader on the stack: its constructor and destructor are expanded inline in the original (vtable 0x54a740 stored
// first, 0x54ae48 last; 0x54ad68 in the destructor), so it is spelled out here; src/rgl/stream.h's bin_reader::bin_reader
// is the out-of-line 0x417c50.
struct btstr {
    int pos;                     // +0
    char* buf;                   // +4
    int cap;                     // +8
    int len;                     // +0xc
    void ctor() { ((tstring*)this)->tstring::tstring(); }      // 0x42bbd0 (tstring::tstring)
    void dtor() { ((tstring*)this)->~tstring(); }              // 0x42bc40 (tstring::~tstring)
};
struct bin_reader_o {
    void* vt;                    // 0x00
    btstr name;                  // 0x04
    int state;                   // 0x14
    char autoflush;              // 0x18
    char pad19[3];
    int flushed;                 // 0x1c
    btstr buf;                   // 0x20
    FILE* fp;                    // 0x30
    char failed;                 // 0x34
    bin_reader_o()
    {
        vt = (void*)0x54a740;
        name.ctor();
        buf.ctor();
        autoflush = 0;
        flushed = 0;
        failed = 1;
        fp = 0;
        state = 0;
        vt = (void*)0x54ae48;
    }
    ~bin_reader_o()
    {
        vt = (void*)0x54ad68;
        buf.dtor();
        name.dtor();
    }
    bool open_read(const char* path) { return ((text_stream*)this)->open_read(path); }     // 0x415290 (the folded text_stream::open_read)
};
void GameData::load_scene(int idx)
{
    tstring path = scene_path(idx);
    bin_reader_o s;
    if (!s.open_read(module_path(path.c_str()))) {
        g_gui.add("Unable to load scene objects!", 0xffff0000);
        g_gui.add(levels[idx].name10, 0xffff0000);
    } else {
        ((SceneFile*)&levels[idx])->load((stream_base*)&s, 0);
        s.failed = 1;
    }
}

// ---- the shadow-map passes -------------------------------------------------------------------------------------------
// The ShadowMap (src/game/shadow_map.h) embeds the render scene of its light pass at +0x18 (scene is 0x1d8 bytes: the
// object's fields +0x18..+0x1f0 are that scene's; shadow_map.h names the fields of the same bytes its own modules read).
#define SC (*(scene_ms2*)((char*)this + 0x18))
extern char g_use_hw_shadow;                     // 0xa17f24

// @ 0x47fce0
NOINL void ShadowMapP::build_hw()
{
    SC.reset();
    if ((*(int*)((char*)&g_options + 0x2c)) == 0)
        return;
    if (target2 == 0) {
        if (g_use_hw_shadow) {
            SC.f190 = 0xffffff;
            target2 = (ShSkin*)g_rm.get_render_target(w, h, 0x50, 1, 0);
        } else {
            target2 = (ShSkin*)g_rm.get_texture(w, h, 0x50);
            SC.f190 = -1;
        }
    }
    SC.f168 = 1;
    create_target(&target);
    if (target != 0 && target2 != 0 && ((texture*)target->refs[0])->loaded) {
        SC.target = (texture*)target->refs[0];
        SC.f15c = (int)target2;
        SC.active = 0;
        for (int i = 0; i < 4; i++) {
            SC.flags = i ? 1 : 0x301;
            ((rgl::world*)cam)->render_part(&SC, i, 4);      // 0x421110
            g_batch.render((int)&SC);
        }
    }
}

// @ 0x47fdd0
NOINL void ShadowMapP::build_sw()
{
    g_batch.render(0);
    SC.reset();
    if (g_use_shaders == 0 && (*(int*)((char*)&g_options + 0x2c)) != 0 && target == 0) {
        target = (ShSkin*)g_rm.get_shadow_skin(w, h, 0x1c, 0);
        if (target != 0) {
            target->file.assign("shadow_shader");
            target->shader = (int)g_resmgr.resolve_shader(*(rgl::string*)&target->file);      // 0x418e90
            target->id = 0x108;
            target->refs[0]->f24 = 0x300000;
        }
    }
    if (target2 == 0)
        target2 = (ShSkin*)g_rm.get_texture(w, h, 0x50);
    SC.flags |= 0x301;
    SC.f190 = 0;
    if (target != 0 && target->refs[0] != 0 && ((texture*)target->refs[0])->loaded) {
        SC.target = (texture*)target->refs[0];
        SC.f15c = (int)target2;
        SC.active = 0;
        ((rgl::world*)cam)->render_scene(&SC, 0);        // 0x429890
        g_batch.render((int)&SC);
    }
    if (target2 != 0) {
        clear_f14((named_object*)target2);          // 0x4175f0
        target2 = 0;
    }
}

// @ 0x47fef0
NOINL void ShadowMapP::render()
{
    setup();
    sc().mask = 0x10000;
    if (g_use_shaders != 0)
        build_hw();
    else
        build_sw();
    bind_target();
    if (g_use_shaders != 0) {
        if ((*(int*)((char*)&g_options + 0x7c)) != 1)
            return;
        if (!g_use_hw_shadow)
            return sync_readback();
    } else {
        if ((*(int*)((char*)&g_options + 0x7c)) != 1)
            return;
    }
    (*(int*)((char*)&g_options + 0x7c)) = 0;
}
