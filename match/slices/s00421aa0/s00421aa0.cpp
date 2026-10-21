// Slice s00421aa0: the strategic_entity update path (0x421aa0-0x422130): emitter list management, the per-frame
// update, and the effect definition's pose / envelope update. Names and layouts provisional (see fx.h).
#include "fx.h"
#include <math.h>

// @ 0x421aa0
void StrategicEntity::add_emitter(Emitter* e)
{
    emitters.push_back(e);
}

// @ 0x421b00
void StrategicEntity::clear_emitters()
{
    for (unsigned i = 0; i < emitters.size(); ++i) {
        Emitter* e = emitters[i];
        delete e;
    }
    emitters.resize(0);
}

// @ 0x421b70
void StrategicEntity::update(float dt)
{
    radius = fbc;
    if (p98) {
        p98->set_frame((const rgl::matrix*)&pos);
        vac = p98->get_b();
        v9c = p98->get_a();
    }
    for (std::vector<Emitter*>::iterator it = emitters.begin(); it != emitters.end(); ++it) {
        (*it)->update(&dt, pos);
        if (radius < (*it)->radius) radius = (*it)->radius;
    }
    float dx = last_pos.x - pos.o.x;
    float dy = last_pos.y - pos.o.y;
    float dz = last_pos.z - pos.o.z;
    float len = (float)sqrt(dx * dx + dy * dy + dz * dz);
    if (len > thresh) {
        if (!f2ec) f2c0 = 0;
        last_pos = pos.o;
    }
    if (fx) fx->update(dt, &pos);
    prev = pos;
    f142 = f141;
    f141 = false;
    for (std::vector<Psys*>::iterator it = psys.begin(); it < psys.end(); ++it)
        (*it)->update_emitters(&dt, pos);
    if (wants_items()) {
        for (unsigned i = 0; i < items.size(); ++i) {
            rgl::matrix m = *(const rgl::matrix*)&pos;
            m.orthonormalize();
            items[i]->set_frame(&m);
            ctx->attach(items[i]);
        }
    }
    if (p288 && f31c && !f170) {
        g_mgr.release(p288);
        p288 = 0;
    }
    f31c = 0;
    f170 = 0;
}

// @ 0x421df0
void FxDef::update(float dt, const matrix64* m)
{
    if (b58) return;
    if (!p64) update_envelopes(dt);
    f54 = dt + f54;
    b50 = false;
    if (!p64) update_poses2();
    else update_poses();
    for (int i = 0; i < count; ++i) {
        for (std::vector<Psys*>::iterator it = elems[i].psys.begin(); it < elems[i].psys.end(); ++it) {
            if (!(*it)->emitters.empty())
                (*it)->update_emitters(&dt, *m * elems[i].m84);
        }
    }
}

// @ 0x421ef0
void FxDef::update_envelopes(float dt)
{
    float v = 1.0f;
    for (int i = 0; i < nenv; ++i) {
        if (env[i].target > 0.5f) v = env[i].attack;
        if (env[i].target > env[i].cur) {
            env[i].cur = dt / env[i].attack + env[i].cur;
            if (env[i].cur > env[i].target) env[i].cur = env[i].target;
        } else if (env[i].target < env[i].cur) {
            env[i].cur = env[i].cur - dt / env[i].release;
            if (env[i].cur < env[i].target) env[i].cur = env[i].target;
        }
    }
    for (int i = 0; i < count; ++i) {
        float w = 1.0f / (v + 0.5f);
        float s = (1.0f > w) ? 1.0f : w;
        elems[i].f1d0 = (1.0f - elems[i].f1d0) * elems[i].f1d4 * s * dt + elems[i].f1d0;
        if (elems[i].f1d0 > 1.0f) elems[i].f1d0 = 1.0f;
        else if (elems[i].f1d0 < 0.0f) elems[i].f1d0 = 0.0f;
    }
}

// @ 0x422080
void FxDef::update_poses()
{
    for (int i = 0; i < count; ++i) {
        if (elems[i].parent == -1)
            elems[i].m84 = elems[i].m44;
        else
            elems[i].m84 = elems[elems[i].parent].m84 * elems[i].m44;
    }
    f54 = 0.0f;
    b50 = true;
}
