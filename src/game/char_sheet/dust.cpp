#include "./dust.h"

// @ 0x4f3e10
void DustTerrain::update_particles(const matrix* view)
{
    for (int a = 0; a < n_outer; a++) {
        for (int b = 0; b < n_inner; b++) {
            cell_rec* rec = &cells[b][a];
            if (emitted[b][a] == 0) {
                float dx = rec->x - view->o.x;
                float dy = rec->y - view->o.y;
                if ((float)sqrt(dy * dy + dx * dx) < rec->reach + g_k_reach)
                    emit_cell(b, a);
            }
            cell_obj* o = grid[b][a];
            if (o->list && (o->list_end - o->list) != 0) {
                float dx = o->pos.x - view->o.x;
                float dy = o->pos.y - view->o.y;
                if ((float)sqrt(dy * dy + dx * dx) > o->length() + g_k_reach) {
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
                    m->s.x *= g_k_0_2;
                    m->s.y *= g_k_0_2;
                    m->s.z *= g_k_0_2;
                    m->f.x *= g_k_0_2;
                    m->f.y *= g_k_0_2;
                    m->f.z *= g_k_0_2;
                    m->u.x *= g_k_0_2;
                    m->u.y *= g_k_0_2;
                    m->u.z *= g_k_0_2;
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
void DustTerrain::emit_cell(int x, int y)
{
    emitted[x][y] = 1;
    int n = dust_count;
    unsigned nf = 0, nr = 0, no = 0;
    if (n > 0) {
        int i = 0;
        do {
            DustParticle* p = &dust[i];
            int ix = (int)(p->frame.o.x * g_k_1_20);
            int cx;
            if (ix < 0) {
                cx = 0;
            } else {
                cx = n_inner - 1;
                if (ix < n_inner)
                    cx = ix;
            }
            int cy = (int)(p->frame.o.y * g_k_1_20);
            if (cy < 0)
                cy = 0;
            else if (cy >= n_outer)
                cy = n_outer - 1;
            if (cx == x && cy == y) {
                dmesh* src = g_ptypes[p->type].boxes[p->anim].meshes[0];
                nf += src->get50()->size();
                nr += src->get70()->size();
                no += src->get90()->size();
            }
            i++;
        } while (--n);
    }
    vec32* v70 = &grid[x][y]->list[0]->f70;
    if (nr > v70->capacity())
        v70->resize(nr);
    vec96* v50 = &grid[x][y]->list[0]->f50;
    if (nf > v50->capacity())
        v50->resize(nf);
    vec32* v90 = &grid[x][y]->list[0]->f90;
    if (no > v90->capacity())
        v90->resize(no);
    for (int i = 0; i < dust_count; i++) {
        DustParticle* p = &dust[i];
        int cx = (int)(p->frame.o.x * g_k_1_20);
        if (cx < 0)
            cx = 0;
        else if (cx >= n_inner)
            cx = n_inner - 1;
        int cy = (int)(p->frame.o.y * g_k_1_20);
        if (cy < 0)
            cy = 0;
        else if (cy >= n_outer)
            cy = n_outer - 1;
        if (cx == x && cy == y) {
            vector4 col;
            ground_color(&col, &p->frame.o.x);
            unsigned color = pack_color(&col.x) | 0xff000000;
            mat3 m3;
            identity3(&m3);
            m3.s.x = p->dir.x;
            m3.f.y = p->dir.y;
            m3.u.z = p->dir.z;
            mxo mm;
            mm.r = ((pos*)&p->frame)->rot3(*(const matrix*)&m3);
            mm.o = p->frame.o;
            p->nmesh = 1;
            p->meshes[0] = grid[cx][cy]->list[0];
            p->first[0] = grid[cx][cy]->list[0]->f50.size();
            ptype* pt = &g_ptypes[p->type];
            p->anim = p->anim % pt->nframes;
            dmesh* src = pt->boxes[p->anim].meshes[0];
            if ((pt->flags & 0x20000i64) != 0)
                grid[cx][cy]->list[0]->append_shaded(src, (const matrix*)&mm, color);
            else
                grid[cx][cy]->list[0]->append_skinned(src, (const matrix*)&mm, -1, color, 0);
            p->last[0] = grid[cx][cy]->list[0]->f50.size();
        }
    }
    cell_obj* o = grid[x][y];
    if (o->list && (o->list_end - o->list) != 0) {
        dmesh* m = o->list[0];
        m->finish_load();
        grid[x][y]->refresh();
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
            vec2 t = { *(float*)v * g_k_0_2, *(float*)(v + 4) * g_k_0_2 };
            fr->xy = t;
        }
    }
    if (g_use_shaders == 0 || g_x5aa084 != 2) {
        for (int i = 0; i < dust_count; i++) {
            DustParticle* p = &dust[i];
            int ix = (int)(p->frame.o.x * g_k_1_20);
            int cx;
            if (ix < 0) {
                cx = 0;
            } else {
                cx = n_inner - 1;
                if (ix < n_inner)
                    cx = ix;
            }
            int cy = (int)(p->frame.o.y * g_k_1_20);
            if (cy < 0)
                cy = 0;
            else if (cy >= n_outer)
                cy = n_outer - 1;
            if (cx == x && cy == y)
                light_particle(&dust_count, i, false);
        }
    }
}
