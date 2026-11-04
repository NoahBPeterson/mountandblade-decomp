#include "./dust.h"
#include "../../rgl/options.h"   // g_options
#include "../../include/render_flags.h"

// the dust cells' scene objects and particles as this pass sees them (cell_obj / DustParticle: dust.h)
static inline cell_obj* cell_at(Terrain* t, int x, int y) { return (cell_obj*)t->gridA[x][y]; }
static inline DustParticle* dust_at(Terrain* t, int i) { return (DustParticle*)&t->list_c.items[i]; }

// @ 0x4f3e10
void Terrain::update_particles(const matrix* view)
{
    for (int a = 0; a < n_outer; a++) {
        for (int b = 0; b < n_inner; b++) {
            rgl::aabb* rec = &boxes[b][a];
            if (emitted[b][a] == 0) {
                float dx = rec->center.x - view->o.x;
                float dy = rec->center.y - view->o.y;
                if ((float)sqrt(dy * dy + dx * dx) < rec->radius + 36.0f)
                    emit_cell(b, a);
            }
            cell_obj* o = cell_at(this, b, a);
            if (o->list && (o->list_end - o->list) != 0) {
                float dx = o->pos.x - view->o.x;
                float dy = o->pos.y - view->o.y;
                if ((float)sqrt(dy * dy + dx * dx) > o->length() + 36.0f) {
                    o->active = 0;
                } else {
                    o->active = 1;
                    matrix* m = &o->list[0]->frame;
                    *(mat3*)m = *(const mat3*)view;
                    m->o.w = 1.0f;
                    m->s.w = 0.0f;
                    m->f.w = 0.0f;
                    m->u.w = 0.0f;
                    *(mat3*)m = *(const mat3*)view;
                    m->s.x *= 0.0125f;
                    m->s.y *= 0.0125f;
                    m->s.z *= 0.0125f;
                    m->f.x *= 0.0125f;
                    m->f.y *= 0.0125f;
                    m->f.z *= 0.0125f;
                    m->u.x *= 0.0125f;
                    m->u.y *= 0.0125f;
                    m->u.z *= 0.0125f;
                    vector4 tint(0.5f, 0.5f, 0.5f);
                    m->o = tint;
                }
            }
        }
    }
}

static inline int cell_clamp(int v, int n)
{
    if (v < 0)
        return 0;
    return v < n ? v : n - 1;
}

// @ 0x4f4060
void Terrain::emit_cell(int x, int y)
{
    emitted[x][y] = 1;
    int n = list_c.count;
    unsigned nf = 0, nr = 0, no = 0;
    if (n > 0) {
        int i = 0;
        do {
            DustParticle* p = dust_at(this, i);
            int ix = (int)(p->frame.o.x * 0.05f);
            int cx;
            if (ix < 0) {
                cx = 0;
            } else {
                cx = n_inner - 1;
                if (ix < n_inner)
                    cx = ix;
            }
            int cy = (int)(p->frame.o.y * 0.05f);
            if (cy < 0)
                cy = 0;
            else if (cy >= n_outer)
                cy = n_outer - 1;
            if (cx == x && cy == y) {
                dmesh* src = (dmesh*)g_ptypes[p->type].boxes[p->anim].meshes[0];
                nf += src->get50()->size();
                nr += src->get70()->size();
                no += src->get90()->size();
            }
            i++;
        } while (--n);
    }
    vec32* v70 = &cell_at(this, x, y)->list[0]->f70;
    if (nr > v70->capacity_pt())
        v70->reserve_nr(nr);
    vec96* v50 = &cell_at(this, x, y)->list[0]->f50;
    if (nf > v50->capacity())
        v50->reserve(nf);
    vec32* v90 = &cell_at(this, x, y)->list[0]->f90;
    if (no > v90->capacity_face())
        v90->reserve_nr(no);
    for (int i = 0; i < list_c.count; i++) {
        DustParticle* p = dust_at(this, i);
        int cx = (int)(p->frame.o.x * 0.05f);
        if (cx < 0)
            cx = 0;
        else if (cx >= n_inner)
            cx = n_inner - 1;
        int cy = (int)(p->frame.o.y * 0.05f);
        if (cy < 0)
            cy = 0;
        else if (cy >= n_outer)
            cy = n_outer - 1;
        if (cx == x && cy == y) {
            vector4 col;
            ((Terrain*)this)->ground_color(&col, &p->frame.o.x);
            unsigned color = rgl::pack_color(&col) | 0xff000000;
            mat3 m3;
            identity3(&m3);
            m3.s.x = p->dir.x;
            m3.f.y = p->dir.y;
            m3.u.z = p->dir.z;
            mxo mm;
            mm.r = ((pos*)&p->frame)->rot3(*(const matrix*)&m3);
            mm.o = p->frame.o;
            p->nmesh = 1;
            p->meshes[0] = cell_at(this, cx, cy)->list[0];
            p->first[0] = cell_at(this, cx, cy)->list[0]->f50.size();
            ptype* pt = &g_ptypes[p->type];
            p->anim = p->anim % pt->nframes;
            dmesh* src = (dmesh*)pt->boxes[p->anim].meshes[0];
            if ((pt->flags & 0x20000i64) != 0)
                ((rgl::mesh*)cell_at(this, cx, cy)->list[0])->transform((rgl::mesh*)src, (const matrix*)&mm, color);
            else
                ((rgl::mesh*)cell_at(this, cx, cy)->list[0])->append_skinned((rgl::mesh*)src, (const rgl::mat4*)&mm, -1, color, 0);
            p->last[0] = cell_at(this, cx, cy)->list[0]->f50.size();
        }
    }
    cell_obj* o = cell_at(this, x, y);
    if (o->list && (o->list_end - o->list) != 0) {
        dmesh* m = o->list[0];
        ((rgl::mesh*)m)->compute_bounds();
        cell_at(this, x, y)->refresh();
        vector4 zv(0.0f, 0.0f, 0.0f);
        mat3 tmp;
        mxo mm;
        mm.r = *identity3(&tmp);
        mm.o = zv;
        m->frame = *(matrix*)&mm;
        m->frame.s.w = 0.0f;
        m->frame.f.w = 0.0f;
        m->frame.u.w = 0.0f;
        m->frame.o.w = 1.0f;
        for (unsigned i = 0; m->f50.first != 0 && i < m->f50.size(); i++) {
            frame60* fr = (frame60*)m->f50.first + i;
            char* v = m->f70.first + (fr->type_index << 5);
            vec2 t = { *(float*)v * 0.0125f, *(float*)(v + 4) * 0.0125f };
            fr->xy = t;
        }
    }
    if (g_use_shaders == 0 || g_options.i7c != 2) {
        for (int i = 0; i < list_c.count; i++) {
            DustParticle* p = dust_at(this, i);
            int ix = (int)(p->frame.o.x * 0.05f);
            int cx;
            if (ix < 0) {
                cx = 0;
            } else {
                cx = n_inner - 1;
                if (ix < n_inner)
                    cx = ix;
            }
            int cy = (int)(p->frame.o.y * 0.05f);
            if (cy < 0)
                cy = 0;
            else if (cy >= n_outer)
                cy = n_outer - 1;
            if (cx == x && cy == y)
                light_particle(&list_c, i, false);
        }
    }
}
