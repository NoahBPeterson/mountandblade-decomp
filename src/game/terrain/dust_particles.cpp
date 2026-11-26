#define DUST_RESERVE_INSTANTIATED_HERE   // dust_particles.h: the reserve instances of the mesh lists are this file's
#include "game/terrain/dust_particles.h"
#include "game/config/options.h"   // g_options
#include "engine/render/render_flags.h"

// the dust cells' scene objects (rgl::entity: meshes[0] is the cell's dust mesh, center / get_radius / visible its culling)
static inline rgl::entity* cell_at(Terrain* t, int x, int y) { return t->gridA[x][y]; }

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
            if (cell_at(this, b, a)->meshes.size() != 0) {
                float dx = cell_at(this, b, a)->center.x - view->o.x;
                float dy = cell_at(this, b, a)->center.y - view->o.y;
                if ((float)sqrt(dy * dy + dx * dx) > cell_at(this, b, a)->get_radius() + 36.0f) {
                    cell_at(this, b, a)->visible = 0;
                } else {
                    cell_at(this, b, a)->visible = 1;
                    matrix* m = &cell_at(this, b, a)->meshes[0]->tex_matrix;
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
    int i = 0;
    unsigned nf = 0, nr = 0, no = 0;
    if (n > 0) {
        do {
            TerrainParticle* p = &list_c.items[i];
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
                rgl::mesh* src = g_ptypes[p->type].boxes[p->anim].meshes[0];
                nf += src->get50_rec().size();
                nr += src->get70_rec().size();
                no += src->get90_rec().size();
            }
            i++;
        } while (i < n);
    }
    std::vector<rgl::point20>& v70 = cell_at(this, x, y)->meshes[0]->points;
    if (nr > v70.capacity())
        v70.reserve(nr);
    std::vector<rgl::vertex60>& v50 = cell_at(this, x, y)->meshes[0]->vertices;
    if (nf > v50.capacity())
        v50.reserve(nf);
    std::vector<rgl::face20>& v90 = cell_at(this, x, y)->meshes[0]->faces;
    if (no > v90.capacity())
        v90.reserve(no);
    for (int i = 0; i < list_c.count; i++) {
        TerrainParticle* p = &list_c.items[i];
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
            rgl::mesh* cm = cell_at(this, cx, cy)->meshes[0];
            p->meshes[0] = cm;
            p->first[0] = cm->vertices.size();
            ptype* pt = &g_ptypes[p->type];
            p->anim = p->anim % pt->nframes;
            rgl::mesh* src = pt->boxes[p->anim].meshes[0];
            if ((pt->flags & 0x20000i64) != 0)
                cm->transform(src, (const matrix*)&mm, color);
            else
                cm->append_skinned(src, (const rgl::mat4*)&mm, -1, color, 0);
            p->last[0] = cm->vertices.size();
        }
    }
    rgl::entity* o = cell_at(this, x, y);
    if (o->meshes.size() != 0) {
        rgl::mesh* m = o->meshes[0];
        m->compute_bounds();
        cell_at(this, x, y)->recalc();
        vector4 zv(0.0f, 0.0f, 0.0f);
        mat3 tmp;
        mxo mm;
        mm.r = *identity3(&tmp);
        mm.o = zv;
        m->tex_matrix = *(matrix*)&mm;
        m->tex_matrix.s.w = 0.0f;
        m->tex_matrix.f.w = 0.0f;
        m->tex_matrix.u.w = 0.0f;
        m->tex_matrix.o.w = 1.0f;
        for (unsigned i = 0; i < m->vertices.size(); i++) {
            rgl::vertex60& fr = m->vertices[i];       // the second uv pair: the vertex's point in the cell, scaled
            const rgl::point20& pt = m->points[fr.index];
            vec2 t = { pt.a.x * 0.0125f, pt.a.y * 0.0125f };
            fr.uv2 = t;
        }
    }
    if (g_use_shaders == 0 || g_options.i7c != 2) {
        for (int i = 0; i < list_c.count; i++) {
            TerrainParticle* p = &list_c.items[i];
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
namespace std {
template<class I, class T> static __forceinline T* _Uninitialized_copy_fi(I f, I l, T* d, allocator<T>& a)
{
    return _Uninit_copy(f, l, d, a, _Ptr_cat(f, d));   // std::_Uninitialized_copy
}
template<class I, class T> static __forceinline T* _Uninit_copy_fi(I f, I l, T* d, allocator<T>& a)
{
    return _Uninitialized_copy_fi(f, l, d, a);         // vector::_Ucopy, expanded in the library's reserve here
}
}
// ---- the mesh lists' std::vector<T>::reserve, out of line after emit_cell (its three grow calls) ---------------------------
// The library's own reserve (its _Ucopy is std::_Uninit_copy 0x4f47f0 / 0x4f4960); the original's linker folded face20's onto
// point20's (ICF, 0x4f4860).
// @ 0x4f46d0
namespace std {
template<> void vector<rgl::vertex60>::reserve(size_type _Count)
{
    if (max_size() < _Count)
        _Xlen();
    else if ((_Myfirst == 0 ? 0 : (size_type)(_Myend - _Myfirst)) < _Count) {
        pointer _Ptr = _Alval.allocate(_Count, (void*)0);
        try {
            _Uninit_copy_fi(iterator(_Myfirst), iterator(_Mylast), _Ptr, _Alval);
        } catch (...) {
            _Alval.deallocate(_Ptr, _Count);
            throw;
        }
        size_type _Size = _Myfirst == 0 ? 0 : (size_type)(_Mylast - _Myfirst);
        if (_Myfirst != 0)
            _Alval.deallocate(_Myfirst, _Myend - _Myfirst);
        _Myend = _Ptr + _Count;
        _Mylast = _Ptr + _Size;
        _Myfirst = _Ptr;
    }
}
}
// @ 0x4f4860
namespace std {
template<> void vector<rgl::point20>::reserve(size_type _Count)
{
    if (max_size() < _Count)
        _Xlen(), __assume(0);   // noreturn in the original's TU (n stays in edx)
    else if ((_Myfirst == 0 ? 0 : (size_type)(_Myend - _Myfirst)) < _Count) {
        pointer _Ptr = _Alval.allocate(_Count, (void*)0);
        try {
            _Uninit_copy_fi(iterator(_Myfirst), iterator(_Mylast), _Ptr, _Alval);
        } catch (...) {
            _Alval.deallocate(_Ptr, _Count);
            throw;
        }
        size_type _Size = _Myfirst == 0 ? 0 : (size_type)(_Mylast - _Myfirst);
        if (_Myfirst != 0)
            _Alval.deallocate(_Myfirst, _Myend - _Myfirst);
        _Myend = _Ptr + _Count;
        _Mylast = _Ptr + _Size;
        _Myfirst = _Ptr;
    }
}
}
namespace std {   // the same code as point20's: folded onto 0x4f4860 by the original's ICF
template<> void vector<rgl::face20>::reserve(size_type _Count)
{
    if (max_size() < _Count)
        _Xlen(), __assume(0);   // noreturn in the original's TU (n stays in edx)
    else if ((_Myfirst == 0 ? 0 : (size_type)(_Myend - _Myfirst)) < _Count) {
        pointer _Ptr = _Alval.allocate(_Count, (void*)0);
        try {
            _Uninit_copy_fi(iterator(_Myfirst), iterator(_Mylast), _Ptr, _Alval);
        } catch (...) {
            _Alval.deallocate(_Ptr, _Count);
            throw;
        }
        size_type _Size = _Myfirst == 0 ? 0 : (size_type)(_Mylast - _Myfirst);
        if (_Myfirst != 0)
            _Alval.deallocate(_Myfirst, _Myend - _Myfirst);
        _Myend = _Ptr + _Count;
        _Mylast = _Ptr + _Size;
        _Myfirst = _Ptr;
    }
}
}
