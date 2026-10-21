// Slice s0046d060: GeomBuffer::upload (0x46d060): refills a Direct3D vertex or index buffer from the meshes
// registered with it. Static buffers (flag +0x30 == 0) are patched incrementally and rebuilt when full; dynamic
// buffers are rebuilt every call. Growth requests are logged and reported with 0xfff3ebc3.
// ALL NAMES PROVISIONAL. Flags: default /O2 /MT /Gy /TP.
#include <windows.h>
#include <d3d9.h>
#include <string.h>
#include <vector>
#include "rgl.h"
#include "mesh.h"

using rgl::mesh;

void mb_error(const char* fmt, ...);                // 0x42cc80

struct tstring { int pos; char* buf; int cap, len; };

// Text log stream (global object at 0x5a9f60, see s00418a70); only slots 8 (w_int), 10 (w_cstr) and 11 (w_tstr)
// are used here.
struct stream_base {
    virtual void reset();
    virtual void rewind();
    virtual stream_base& w_stream(void* o) = 0;
    virtual stream_base& w_float(void* v) = 0;
    virtual stream_base& w_bool(void* v) = 0;
    virtual stream_base& w_u64(void* v) = 0;
    virtual stream_base& w_i64(void* v) = 0;
    virtual stream_base& w_uint(void* v) = 0;
    virtual stream_base& w_int(int* v) = 0;                 // 8  (+0x20)
    virtual stream_base& w_byte(void* v) = 0;
    virtual stream_base& w_cstr(const char* s) = 0;         // 10 (+0x28)
    virtual stream_base& w_tstr(const tstring* s) = 0;      // 11 (+0x2c)
};
struct log_stream : stream_base {
    virtual stream_base& w_stream(void* o);
    virtual stream_base& w_float(void* v);
    virtual stream_base& w_bool(void* v);
    virtual stream_base& w_u64(void* v);
    virtual stream_base& w_i64(void* v);
    virtual stream_base& w_uint(void* v);
    virtual stream_base& w_int(int* v);
    virtual stream_base& w_byte(void* v);
    virtual stream_base& w_cstr(const char* s);
    virtual stream_base& w_tstr(const tstring* s);
};
extern log_stream g_log;                            // 0x5a9f60
extern char g_buffers_dirty;                        // 0x5e2938 (set when a buffer must be recreated larger)

struct GeomBuffer {
    int kind;                                       // 0x00: 0 = vertex buffer, 1 = index buffer
    int stride;                                     // 0x04: bytes per element (2 for indices)
    IDirect3DVertexBuffer9* vb;                     // 0x08
    unsigned fmt;                                   // 0x0c: vertex field flags (see mesh::fill_vertices)
    int cursor;                                     // 0x10: elements already used
    int gen;                                        // 0x14: rebuild generation
    tstring name;                                   // 0x18
    int capacity;                                   // 0x28: elements
    int pad2c;
    int is_dynamic;                                 // 0x30
    std::vector<mesh*> meshes;                      // 0x34
    IDirect3DIndexBuffer9* ib;                      // 0x44
    int upload(void* scratch, void* tag);           // 0x46d060
};
typedef char chk_cursor[offsetof(GeomBuffer, cursor) == 0x10 ? 1 : -1];
typedef char chk_capacity[offsetof(GeomBuffer, capacity) == 0x28 ? 1 : -1];
typedef char chk_meshes[offsetof(GeomBuffer, meshes) == 0x34 ? 1 : -1];
typedef char chk_ib[offsetof(GeomBuffer, ib) == 0x44 ? 1 : -1];

// Body of mesh::count (0x468620, byte-exact in s004684d0), visible here so that cl sees it has no side effects
// (the original TU contains it, which lets cl hoist loads across the calls below).
int mesh::count(int k)
{
    if (k == 1) {
        std::vector<rgl::face20>& f = local_a0 ? v90 : parent->get90_();
        return f.size() * 3;
    }
    std::vector<rgl::vertex60>& v = local_a1 ? v50 : parent->get50_();
    return v.size();
}

#define LOG_GROW(KIND_WORD, KIND_V_WORD)                                                           \
    do {                                                                                           \
        stream_base& lg = g_log;                                                                   \
        stream_base* ls;                                                                           \
        if (kind == 1)                                                                             \
            ls = &lg.w_cstr("Out of " KIND_WORD " index buffer memory. Needed = ");                \
        else                                                                                       \
            ls = &lg.w_tstr(&name).w_cstr(KIND_V_WORD);                                            \
        int needed = total;\
        ls = &ls->w_int(&needed);                                                                   \
        ls = &ls->w_cstr("Available:");                                                            \
        ls = &ls->w_int(pcap);                                                                \
        ls->w_cstr("\n");                                                                          \
        int grow = total / 4;                                                                      \
        if (grow <= 0x20)                                                                          \
            grow = 0x20;                                                                           \
        *pcap = grow + total;                                                                   \
        ls = &lg.w_cstr("Expanding Buffer Capacity to ");                                          \
        ls = &ls->w_int(pcap);                                                                \
        ls->w_cstr("\n");                                                                          \
        *pcap = *pcap / 32 * 32;                                                             \
        g_buffers_dirty = 1;                                                                       \
        return 0xfff3ebc3;                                                                         \
    } while (0)

// @ 0x46d060
int GeomBuffer::upload(void* scratch, void* tag)
{
    int n = 0;                          // elements staged in scratch
    int base = 0;                       // elements assigned so far in this upload
    int written = 0;                    // bytes already copied into the locked buffer
    char* locked;
    if (kind == 1)
        stride = 2;
    std::vector<mesh*>::iterator it;
    for (it = meshes.begin(); it != meshes.end(); ++it) {
        rgl::slot& s = (*it)->slots[kind];
        if (s.f24 != tag) {
            s.f24 = tag;
            (&s.p.a)[s.f20] = s.f28;
            s.f20 = (s.f20 + 1) % 2;
        }
    }
    it = meshes.begin();
    if (!is_dynamic) {
        int need = 0;
        int off = kind * sizeof(rgl::slot);
        for (; it != meshes.end(); ++it) {
            rgl::slot& s = *(rgl::slot*)((char*)&(*it)->slots[0] + off);
            if (gen != s.fc || s.f28 != 0 || s.f14 != s.f4)
                need += (*it)->count(kind);
        }
        if (need == 0)
            return 0;
        int* pcap = &capacity;
        if (cursor + need < *pcap) {
            int tries = 0;
            if (kind == 1) {
                do {
                    if (ib->Lock(cursor * 2, need * 2, (void**)&locked, D3DLOCK_NOOVERWRITE) >= 0)
                        break;
                    tries++;
                    Sleep(10);
                    if (tries > 0x5a)
                        mb_error("unable to lock index buffer");
                } while (tries < 100);
            } else {
                do {
                    if (vb->Lock(cursor * stride, stride * need, (void**)&locked, D3DLOCK_NOOVERWRITE) >= 0)
                        break;
                    tries++;
                    Sleep(10);
                    if (tries > 0x5a)
                        mb_error("unable to lock vertex buffer");
                } while (tries < 100);
            }
            for (it = meshes.begin(); it != meshes.end(); ++it) {
                rgl::slot& s = (*it)->slots[kind];
                if (s.fc != gen || s.f28 != 0 || s.f14 != s.f4) {
                    if (((*it)->count(kind) + n) * stride >= 0x418000) {
                        memcpy(locked + written, scratch, stride * n);
                        written += stride * n;
                        n = 0;
                    }
                    char* dst = (char*)scratch + stride * n;
                    if (kind == 1)
                        (*it)->fill_indices((unsigned short*)dst);
                    else
                        (*it)->fill_vertices((int*)dst, &fmt);
                    s.f8 = cursor + base;
                    s.fc = gen;
                    s.f14 = s.f4;
                    s.f28 = 0;
                    int k = kind;
                    mesh* tm = *it;
                    n += tm->count(k);
                    base += tm->count(k);
                }
            }
            memcpy(locked + written, scratch, stride * n);
            if (kind == 1) {
                ib->Unlock();
                cursor += base;
                return 0;
            }
            vb->Unlock();
            cursor += base;
            return 0;
        }
        gen++;
        base = 0;
        int total = 0;
        for (it = meshes.begin(); it != meshes.end(); ++it)
            total += (*it)->count(kind);
        if (total > *pcap)
            LOG_GROW("Static", ":Out of Static vertex buffer memory. Needed = ");
        if (kind == 1) {
            if (ib->Lock(0, 0, (void**)&locked, D3DLOCK_DISCARD) < 0)
                mb_error("unable to lock index buffer");
        } else {
            if (vb->Lock(0, 0, (void**)&locked, D3DLOCK_DISCARD) < 0)
                mb_error("unable to lock vertex buffer");
        }
        for (it = meshes.begin(); it != meshes.end(); ++it) {
            rgl::slot& s = (*it)->slots[kind];
            if (((*it)->count(kind) + n) * stride >= 0x418000) {
                memcpy(locked + written, scratch, stride * n);
                written += stride * n;
                n = 0;
            }
            char* dst = (char*)scratch + stride * n;
            if (kind == 1)
                (*it)->fill_indices((unsigned short*)dst);
            else
                (*it)->fill_vertices((int*)dst, &fmt);
            s.f8 = base;
            s.fc = gen;
            s.f14 = s.f4;
            s.f28 = 0;
            int k = kind;
            mesh* tm = *it;
            base += tm->count(k);
            n += tm->count(k);
        }
        memcpy(locked + written, scratch, stride * n);
        if (kind == 1) {
            ib->Unlock();
            cursor = base;
            return 0;
        }
        vb->Unlock();
        cursor = base;
        return 0;
    }
    int total = 0;
    for (; it != meshes.end(); ++it)
        total += (*it)->count(kind);
    int* pcap = &capacity;
    if (total > capacity)
        LOG_GROW("dynamic", ":Out of dynamic vertex buffer memory. Needed = ");
    if (kind == 1) {
        if (ib->Lock(0, 0, (void**)&locked, D3DLOCK_DISCARD) < 0)
            mb_error("unable to lock index buffer");
    } else if (capacity != 0) {
        int tries = 0;
        do {
            if (vb->Lock(0, 0, (void**)&locked, D3DLOCK_DISCARD) >= 0)
                break;
            tries++;
            Sleep(10);
            if (tries > 0x5a)
                mb_error("unable to lock vertex buffer");
        } while (tries < 100);
    }
    for (it = meshes.begin(); it != meshes.end(); ++it) {
        rgl::slot& s = (*it)->slots[kind];
        if (((*it)->count(kind) + n) * stride >= 0x418000) {
            memcpy(locked + written, scratch, stride * n);
            written += stride * n;
            n = 0;
        }
        char* dst = (char*)scratch + stride * n;
        if (kind == 1)
            (*it)->fill_indices((unsigned short*)dst);
        else
            (*it)->fill_vertices((int*)dst, &fmt);
        s.f8 = base;
        s.f14 = s.f4;
        int k = kind;
        mesh* tm = *it;
        base += tm->count(k);
        n += tm->count(k);
    }
    memcpy(locked + written, scratch, stride * n);
    if (kind == 1) {
        ib->Unlock();
        return 0;
    }
    if (capacity != 0)
        vb->Unlock();
    return 0;
}
