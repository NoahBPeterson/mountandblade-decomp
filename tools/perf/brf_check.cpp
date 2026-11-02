// brf_check.cpp: offline check of patches/perf (stream_read_slots.cpp, resmgr_add_index.cpp) against the faithful code.
// Models the faithful path (virtual slot -> tstring::read -> memcpy; r_tstr one push_back per char; O(n^2) name scan)
// and the patched path with the same code as the patches, walks every real .brf with the format reader of load_stream,
// and compares a running hash of every scalar, string and mesh-name check result. Not the game; plain C++.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>
#include <chrono>
#include <dirent.h>
#include <sys/stat.h>

// ---- tstring model (src/game/strat/tstring.cpp semantics) ----
struct TS {
    int pos = 0; char* buf = 0; int cap = 0; int len = 0;
    ~TS() { delete[] buf; }
    void grow(unsigned n) {
        if (n > 0) {
            unsigned need = n + 1;
            if ((unsigned)cap < need) {
                unsigned a = (unsigned)(cap * 1.5f), b = cap + 16;
                b = a > b ? a : b; b = b > need ? b : need;
                char* nb = new char[b];
                if (buf) { memcpy(nb, buf, len + 1); delete[] buf; }
                cap = b; buf = nb;
            }
        }
    }
    void assign_empty() { len = 0; pos = 0; grow(0); if (buf) buf[0] = 0; }
    void push_back(char c) { grow(len + 1); buf[len] = c; buf[len + 1] = 0; len++; }
    __attribute__((noinline)) void read(void* dst, int n) { memcpy(dst, buf + pos, n); pos += n; }
    void write(const void* src, int n) { int nl = len + n; grow(nl); memcpy(buf + len, src, n); len = nl; buf[nl] = 0; }
    bool equals(const TS& o) const {
        if (len == 0) return o.len == 0;
        if (o.len != 0) return strcmp(buf, o.buf) == 0;
        return false;
    }
};

struct Stream {
    TS buf;
    virtual Stream& r_int(int* v) = 0;
    virtual Stream& r_float(float* v) = 0;
    virtual Stream& r_tstr(TS* v) = 0;
    virtual ~Stream() {}
};
struct OldStream : Stream {     // mem_stream::r_int / r_tstr, faithful
    Stream& r_int(int* v) override { buf.read(v, 4); return *this; }
    Stream& r_float(float* v) override { return r_int((int*)v); }   // forwarder
    Stream& r_tstr(TS* v) override {
        int n; r_int(&n);
        v->assign_empty();
        for (int i = 0; i < n; i++) { char c; buf.read(&c, 1); v->push_back(c); }
        return *this;
    }
};
struct NewStream : Stream {     // patches/perf/stream_read_slots.cpp
    Stream& r_int(int* v) override { int p = buf.pos; memcpy(v, buf.buf + p, 4); buf.pos = p + 4; return *this; }
    Stream& r_float(float* v) override { return r_int((int*)v); }
    Stream& r_tstr(TS* v) override {
        int n; r_int(&n);
        v->assign_empty();
        if (n > 0) { int p = buf.pos; v->write(buf.buf + p, n); buf.pos = p + n; }
        return *this;
    }
};

// ---- hash sink ----
static uint64_t H = 1469598103934665603ull;
static inline void mix(uint64_t x) { H = (H ^ x) * 1099511628211ull; }
static int ri(Stream& s) { int v; s.r_int(&v); mix((uint32_t)v); return v; }
static void rf(Stream& s, int n) { float f; for (int i = 0; i < n; i++) { s.r_float(&f); uint32_t u; memcpy(&u, &f, 4); mix(u); } }
static void rt(Stream& s, TS& t) { s.r_tstr(&t); mix(t.len); for (int i = 0; i < t.len; i++) mix((unsigned char)t.buf[i]); }

static void shape(Stream& s, const std::string& k) {
    TS t;
    if (k == "sphere") { rf(s, 4); ri(s); }
    else if (k == "capsule") { rf(s, 7); ri(s); }
    else if (k == "face") { int n = ri(s); rf(s, 3 * n); ri(s); }
    else if (k == "composite") { int n = ri(s); for (int i = 0; i < n; i++) { rt(s, t); shape(s, t.buf ? t.buf : ""); } }
    else if (k == "manifold") { int n = ri(s); rf(s, 3 * n); int m = ri(s); for (int i = 0; i < m; i++) { ri(s); ri(s); int k2 = ri(s); for (int j = 0; j < k2; j++) ri(s); } }
    else { fprintf(stderr, "bad shape %s\n", k.c_str()); exit(2); }
}

typedef std::vector<TS*> Names;
// walks one file like load_stream + the record readers; mesh names are appended to `names`
static void walk(Stream& s, std::vector<std::string>& mesh_names) {
    TS tok, t;
    rt(s, tok);
    while (strcmp(tok.buf ? tok.buf : "", "end") != 0) {
        std::string k = tok.buf ? tok.buf : "";
        int n = ri(s);
        for (int r = 0; r < n; r++) {
            if (k == "texture") { rt(s, t); ri(s); }
            else if (k == "shader") { rt(s, t); ri(s); ri(s); rt(s, t); int na = ri(s); for (int i = 0; i < na; i++) rt(s, t); int ns = ri(s); for (int i = 0; i < 4 * ns; i++) ri(s); }
            else if (k == "material") { rt(s, t); ri(s); rt(s, t); for (int i = 0; i < 5; i++) rt(s, t); rf(s, 4); }
            else if (k == "mesh") {
                rt(s, t); mesh_names.push_back(t.buf ? t.buf : ""); ri(s); rt(s, t);
                int np = ri(s); rf(s, 3 * np);
                int nb = ri(s); for (int i = 0; i < nb; i++) { ri(s); int m = ri(s); for (int j = 0; j < m; j++) { ri(s); rf(s, 1); } }
                int kc = ri(s); for (int i = 0; i < kc; i++) { ri(s); int n1 = ri(s); rf(s, 3 * n1); int n2 = ri(s); rf(s, 3 * n2); }
                int nv = ri(s); for (int i = 0; i < nv; i++) { ri(s); ri(s); rf(s, 3); rf(s, 4); }
                int nf = ri(s); for (int i = 0; i < 3 * nf; i++) ri(s);
            }
            else if (k == "body") { rt(s, t); TS kind; rt(s, kind); shape(s, kind.buf ? kind.buf : ""); }
            else if (k == "skeleton") { rt(s, t); int nb = ri(s); for (int i = 0; i < nb; i++) { ri(s); rt(s, t); ri(s); rf(s, 12); } }
            else if (k == "skeleton_anim") { rt(s, t); int ns = ri(s); for (int i = 0; i < ns; i++) { int m = ri(s); for (int j = 0; j < m; j++) { ri(s); rf(s, 4); } } int m = ri(s); for (int j = 0; j < m; j++) { ri(s); rf(s, 3); } }
            else { fprintf(stderr, "bad token %s\n", k.c_str()); exit(2); }
        }
        rt(s, tok);
    }
}

// ---- duplicate-name check: faithful O(n^2) vs hashed (patches/perf/resmgr_add_index.cpp) ----
static unsigned name_hash(const TS& s) {
    if (s.len == 0) return 0;
    unsigned h = 2166136261u;
    for (const unsigned char* p = (const unsigned char*)s.buf; *p; ++p) h = (h ^ *p) * 16777619u;
    return h | 1u;
}
struct Index {
    std::vector<TS*> items; std::vector<unsigned> hash; std::vector<int> next, head;
    Index() { head.assign(1024, -1); }
    void insert(TS* m) {
        if (items.size() * 2 >= head.size()) {
            std::vector<int> nh(head.size() * 2, -1);
            for (size_t i = 0; i < items.size(); i++) { unsigned b = hash[i] & (nh.size() - 1); next[i] = nh[b]; nh[b] = (int)i; }
            head.swap(nh);
        }
        unsigned h = name_hash(*m), b = h & (head.size() - 1);
        items.push_back(m); hash.push_back(h); next.push_back(head[b]); head[b] = (int)items.size() - 1;
    }
    TS* find(const TS& n) const {
        unsigned h = name_hash(n);
        for (int i = head[h & (head.size() - 1)]; i >= 0; i = next[i]) if (hash[i] == h && items[i]->equals(n)) return items[i];
        return 0;
    }
};
static TS* mk(const std::string& s) { TS* t = new TS; if (!s.empty()) t->write(s.data(), (int)s.size()); return t; }

static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

int main(int argc, char** argv) {
    std::vector<std::string> files;
    for (int a = 1; a < argc; a++) {
        std::string d = argv[a];
        if (d.size() > 4 && d.substr(d.size() - 4) == ".brf") { files.push_back(d); continue; }
        DIR* dir = opendir(d.c_str()); if (!dir) continue;
        while (dirent* e = readdir(dir)) { std::string n = e->d_name; if (n.size() > 4 && n.substr(n.size() - 4) == ".brf") files.push_back(d + "/" + n); }
        closedir(dir);
    }
    std::vector<std::vector<char>> data;
    size_t total = 0;
    for (auto& f : files) { FILE* fp = fopen(f.c_str(), "rb"); fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, 0, SEEK_SET); data.emplace_back(n + 1); fread(data.back().data(), 1, n, fp); fclose(fp); total += n; }
    printf("%zu files, %.1f MB\n", files.size(), total / 1048576.0);

    uint64_t hashes[2]; double secs[2]; std::vector<std::string> names[2];
    for (int mode = 0; mode < 2; mode++) {
        H = 1469598103934665603ull;
        double t0 = now();
        for (size_t i = 0; i < files.size(); i++) {
            Stream* s = mode == 0 ? (Stream*)new OldStream : (Stream*)new NewStream;
            s->buf.buf = data[i].data(); s->buf.len = (int)data[i].size() - 1; s->buf.pos = 0;
            walk(*s, names[mode]);
            s->buf.buf = 0;       // not owned
            if (s->buf.pos != s->buf.len) { printf("%s: consumed %d of %d\n", files[i].c_str(), s->buf.pos, s->buf.len); return 1; }
            delete s;
        }
        secs[mode] = now() - t0; hashes[mode] = H;
        printf("%s reader: %.3f s, hash %016llx\n", mode ? "patched " : "faithful", secs[mode], (unsigned long long)H);
    }
    printf("readers %s, speedup %.2fx\n", hashes[0] == hashes[1] && names[0] == names[1] ? "IDENTICAL" : "DIFFER", secs[0] / secs[1]);

    // duplicate check over the real mesh names (in registration order), then with injected duplicates
    std::vector<TS*> objs; for (auto& n : names[0]) objs.push_back(mk(n));
    for (int variant = 0; variant < 2; variant++) {
        std::vector<TS*> seq = objs;
        if (variant) { seq.push_back(mk("")); seq.push_back(mk("")); for (size_t i = 0; i < objs.size(); i += 97) seq.push_back(mk(names[0][i])); }
        std::vector<int> r0, r1;
        double t0 = now();
        { std::vector<TS*> reg; for (TS* r : seq) { int hit = -1; for (size_t i = 0; i < reg.size(); i++) if (reg[i]->equals(*r)) { hit = (int)i; break; } r0.push_back(hit >= 0); reg.push_back(r); } }
        double t1 = now();
        { Index ix; std::vector<TS*> reg; for (TS* r : seq) { TS* h = ix.find(*r); r1.push_back(h != 0); ix.insert(r); } }
        double t2 = now();
        int dups = 0; for (int x : r0) dups += x;
        printf("name check, %zu meshes%s: O(n^2) %.3f s, hashed %.4f s, %d duplicates, results %s\n", seq.size(), variant ? " (+injected dups, empty names)" : "", t1 - t0, t2 - t1, dups, r0 == r1 ? "IDENTICAL" : "DIFFER");
    }
    return hashes[0] == hashes[1] ? 0 : 1;
}
