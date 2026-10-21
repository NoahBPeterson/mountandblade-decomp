// Slice s0041db20: the resource manager's file loader (names provisional). 0x41db20 reads a stream of
// "<type> <count> <records...> ... end" blocks and creates the textures, shaders(materials), skins, meshes,
// bodies and skeletons; 0x41e150 picks the text (.trf) or binary (.brf) resource file and loads it.
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <new>
#include "s0041db20.h"

// @ 0x41db20
stream_base* resmgr::load_stream(stream_base* s)
{
    tstring tok;
    tstring tok2;
    int n;
    s->r_tstr(&tok);
    while (tok.ne("end")) {
        if (tok.equals("texture")) {
            s->r_int(&n);
            if (n != 0) {
                render_res* t = new render_res[n];
                all.push_back(t);
                for (int i = 0; i < n; i++) {
                    s->r_tstr(&t[i].name);
                    s->r_int((int*)&t[i].flags);
                    add(&t[i]);
                }
            }
        } else if (tok.equals("shader")) {
            s->r_int(&n);
            if (n != 0) {
                material* m = new material[n];
                all.push_back(m);
                for (int i = 0; i < n; i++) {
                    m[i].load(s);
                    add(&m[i]);
                }
            }
        } else if (tok.equals("material")) {
            s->r_int(&n);
            if (n != 0) {
                skin* k = new skin[n];
                all.push_back(k);
                for (int i = 0; i < n; i++) {
                    k[i].load(s);
                    add(&k[i]);
                }
            }
        } else if (tok.equals("mesh")) {
            s->r_int(&n);
            if (n != 0) {
                rgl::mesh* m = new rgl::mesh[n];
                all.push_back((res_obj*)m);
                for (int i = 0; i < n; i++) {
                    read_mesh(s, &m[i]);
                    add((res_obj*)&m[i]);
                }
            }
        } else if (tok.equals("body")) {
            s->r_int(&n);
            if (n != 0) {
                body* b = new body[n];
                all.push_back(b);
                for (int i = 0; i < n; i++) {
                    read_body(s, &b[i]);
                    add(&b[i]);
                }
            }
        } else if (tok.equals("skeleton")) {
            s->r_int(&n);
            if (n != 0) {
                frame_set* f = new frame_set[n];
                all.push_back(f);
                for (int i = 0; i < n; i++) {
                    read_frame_set(s, &f[i]);
                    add(&f[i]);
                }
            }
        } else if (tok.equals("skeleton_anim")) {
            s->r_int(&n);
            if (n != 0) {
                skel_anim* a = new skel_anim[n];
                all.push_back(a);
                for (int i = 0; i < n; i++) {
                    read_skel_anim(s, &a[i]);
                    add(&a[i]);
                }
            }
        } else {
            mb_error("Unrecognized resource type %s", tok.c_str());
        }
        s->r_tstr(&tok);
    }
    return s;
}

static __forceinline tstring mkpath(const tstring& dir, const tstring& name, const char* ext)
{
    tstring e(ext);
    return dir + name + e;
}

// @ 0x41e150
void resmgr::load_pack(const tstring& name, int user)
{
    tstring dir;
    if (user) {
        dir = cur;
        tstring r("Resource\\");
        dir.append(r);
    } else {
        tstring r("CommonRes\\");
        dir.append(r);
    }
    tstring trf = mkpath(dir, name, ".trf");
    tstring brf = mkpath(dir, name, ".brf");
    loaded.resize(0, 0);
    bool done = false;
    if (g_use_text_res) {
        struct _stat st_t, st_b;
        if (_stat(trf.c_str(), &st_t) != -1) {
            if (_stat(brf.c_str(), &st_b) == -1 || difftime(st_t.st_mtime, st_b.st_mtime) > 0.0) {
                tstream_c ts;
                if (ts.open_read_text(trf.c_str())) {
                    load_stream(&ts);
                    ts.failed = 1;
                    done = true;
                    bin_writer w;
                    w.open_write(brf.c_str());
                    save_stream(&w);
                    w.close();
                }
            }
        }
    }
    if (!done) {
        bin_reader br;
        const char* p = brf.c_str();
        if (!br.open_read(p))
            mb_error("Unable to open file: %s\n", p);
        load_stream(&br);
        br.failed = 1;
    }
}
