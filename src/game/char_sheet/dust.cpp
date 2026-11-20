#include "./dust.h"
#include "../../rgl/options.h"   // g_options
#include "../../include/render_flags.h"

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
// ---- the mesh lists' std::vector<T>::reserve, out of line after emit_cell (its three grow calls) ---------------------------
// The library's body as this build of <vector> had it: no `else` after the length check, the elements copied with _Ucopy
// inside try / catch(...) (std::_Uninit_copy 0x4f47f0 / 0x4f4960 follow each instance), the old block freed without a
// _Destroy (the elements are trivial). _Xlen is the one folded instance 0x43e2a0 (include/vec_xlen.h): an ordinary call
// in the vertex60 instance, a known noreturn call in the point20 / face20 ones (n stays in edx). The face20 instance is
// the point20 code, which the original's linker folded into 0x4f4860 (ICF).
#include "../../include/vec_xlen.h"
// vector::_Ucopy and std::uninitialized_copy as the library writes them, forced inline (the original expands both into reserve
// and calls only std::_Uninit_copy)
template<class I, class T> static __forceinline T* uninitialized_copy_(I f, I l, T* d, std::allocator<T>& a)
{
    return std::_Uninit_copy(f, l, d, a, std::_Ptr_cat(f, d));
}
template<class I, class T> static __forceinline T* ucopy(I f, I l, T* d, std::allocator<T>& a)
{
    return uninitialized_copy_(f, l, d, a);
}

// @ 0x4f46d0
template<> __declspec(noinline) void std::vector<rgl::vertex60>::reserve(size_type n)
{
    if ((size_type)-1 / sizeof(rgl::vertex60) < n)
        VEC_XLEN_CALL(this);
    if ((_Myfirst == 0 ? 0 : (size_type)(_Myend - _Myfirst)) < n) {      // capacity(), inline (this TU's capacity() is the out-of-line 0x40f030 / 0x40f090 instance)
        pointer p = this->_Alval.allocate(n);
        try {
            ucopy(begin(), end(), p, this->_Alval);
        } catch (...) {
            this->_Alval.deallocate(p, n);
            throw;
        }
        size_type sz = size();
        if (_Myfirst != 0)
            this->_Alval.deallocate(_Myfirst, _Myend - _Myfirst);
        _Myend = p + n;
        _Mylast = p + sz;
        _Myfirst = p;
    }
}

// @ 0x4f4860
template<> __declspec(noinline) void std::vector<rgl::point20>::reserve(size_type n)
{
    if ((size_type)-1 / sizeof(rgl::point20) < n)
        VEC_XLEN(this);
    if ((_Myfirst == 0 ? 0 : (size_type)(_Myend - _Myfirst)) < n) {      // capacity(), inline (this TU's capacity() is the out-of-line 0x40f030 / 0x40f090 instance)
        pointer p = this->_Alval.allocate(n);
        try {
            ucopy(begin(), end(), p, this->_Alval);
        } catch (...) {
            this->_Alval.deallocate(p, n);
            throw;
        }
        size_type sz = size();
        if (_Myfirst != 0)
            this->_Alval.deallocate(_Myfirst, _Myend - _Myfirst);
        _Myend = p + n;
        _Mylast = p + sz;
        _Myfirst = p;
    }
}

// face20's instance: the point20 code (one function in the original, 0x4f4860, folded by the linker's ICF)
template<> __declspec(noinline) void std::vector<rgl::face20>::reserve(size_type n)
{
    if ((size_type)-1 / sizeof(rgl::face20) < n)
        VEC_XLEN(this);
    if ((_Myfirst == 0 ? 0 : (size_type)(_Myend - _Myfirst)) < n) {      // capacity(), inline (this TU's capacity() is the out-of-line 0x40f030 / 0x40f090 instance)
        pointer p = this->_Alval.allocate(n);
        try {
            ucopy(begin(), end(), p, this->_Alval);
        } catch (...) {
            this->_Alval.deallocate(p, n);
            throw;
        }
        size_type sz = size();
        if (_Myfirst != 0)
            this->_Alval.deallocate(_Myfirst, _Myend - _Myfirst);
        _Myend = p + n;
        _Mylast = p + sz;
        _Myfirst = p;
    }
}
