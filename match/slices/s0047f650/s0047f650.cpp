// Slice s0047f650: character mesh collection, the animation-state wrapper, the scene-object loader and the
// shadow-map passes. Render-layer types are in mesh.h, game types in s0047f650.h. Names are provisional.
#include "s0047f650.h"

using rgl::g_resmgr;

// @ 0x47f650
__declspec(noinline)
void EChar::collect(lod_mesh** out_a, lod_mesh** out_b, lod_mesh** out_c, const float* weights,
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

    *out_a = base->clone();

    if (set < 0 || set >= set_count) {
        set = -1;
    } else {
        *out_b = sets[set].mesh->clone();
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

    if (material >= 0) {
        m->res = mats[material].res0;
        if ((*out_a)->count > 1) {
            for (int i = 1; i < (*out_a)->count; i++)
                (*out_a)->lists[i].v[0]->res = mats[material].res1;
        }
        color = mats[material].eval(&time);
    }

    if (item >= 0 && item_count > 0 && variant_count > 0 && item < item_count && variant >= 0) {
        *out_c = items[item]->clone();
        blend_all(*out_c, &blend_b[item], weights);
    }

    if (*out_c != 0) {
        if (variant < 0)
            variant = 0;
        else if (variant > variant_count - 1)
            variant = variant_count - 1;

        (*out_c)->lists[0].v[0]->res = g_resmgr.get_material(names_a[variant]);
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
        (*out_b)->lists[0].v[0]->res = g_resmgr.get_material(name);
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
        int* vals = chars[cur].mats[material].vals;
        unsigned n = vals ? chars[cur].mats[material].vals_end - vals : 0;
        v = vals[(unsigned)variant % n];
    }

    chars[cur].collect(a, b, c, weights, item, set, material, v, scale, time);
}

// @ 0x47fbd0
void GameState::load_scene(int idx)
{
    tstring path = scene_path(idx);
    bin_reader s;
    if (!s.open_read(module_path(path.c_str()))) {
        g_msglog.add("Unable to load scene objects!", 0xffff0000);
        g_msglog.add(sceneobjs[idx].name, 0xffff0000);
    } else {
        sceneobjs[idx].load(&s, 0);
        s.failed = 1;
    }
}

// @ 0x47fce0
__declspec(noinline)
void ShadowMap::build_hw()
{
    sc.reset();
    if (g_x5aa034 == 0)
        return;
    if (target2 == 0) {
        if (g_use_hw_shadow) {
            sc.f190 = 0xffffff;
            target2 = (Skin*)g_resmgr.get_render_target(w, h, 0x50, 1, 0);
        } else {
            target2 = (Skin*)g_resmgr.get_texture(w, h, 0x50);
            sc.f190 = -1;
        }
    }
    sc.f168 = 1;
    create_target(&target);
    if (target != 0 && target2 != 0 && target->refs[0]->loaded) {
        sc.target = target->refs[0];
        sc.rt = target2;
        sc.f1ac = 0;
        for (int i = 0; i < 4; i++) {
            sc.f18c = i ? 1 : 0x301;
            cam->attach(&sc, i, 4);
            g_scene_reg.select(&sc);
        }
    }
}

// @ 0x47fdd0
__declspec(noinline)
void ShadowMap::build_sw()
{
    g_scene_reg.select(0);
    sc.reset();
    if (g_dinput_enabled == 0 && g_x5aa034 != 0 && target == 0) {
        target = (Skin*)g_resmgr.get_shadow_skin(w, h, 0x1c, 0);
        if (target != 0) {
            target->file.assign("shadow_shader");
            target->shader = g_resmgr.resolve_shader(&target->file);
            target->id = 0x108;
            target->refs[0]->f24 = 0x300000;
        }
    }
    if (target2 == 0)
        target2 = (Skin*)g_resmgr.get_texture(w, h, 0x50);
    sc.f18c |= 0x301;
    sc.f190 = 0;
    if (target != 0 && target->refs[0] != 0 && target->refs[0]->loaded) {
        sc.target = target->refs[0];
        sc.rt = target2;
        sc.f1ac = 0;
        cam->render_scene(&sc, 0);
        g_scene_reg.select(&sc);
    }
    if (target2 != 0) {
        g_resmgr.release(target2);
        target2 = 0;
    }
}

// @ 0x47fef0
__declspec(noinline)
void ShadowMap::render()
{
    setup();
    sc.mask = 0x10000;
    if (g_dinput_enabled != 0)
        build_hw();
    else
        build_sw();
    bind_target();
    if (g_dinput_enabled != 0) {
        if (g_water_mode != 1)
            return;
        if (!g_use_hw_shadow)
            return sync_readback();
    } else {
        if (g_water_mode != 1)
            return;
    }
    g_water_mode = 0;
}
