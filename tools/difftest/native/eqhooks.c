/*
 * eqhooks.c -- native bulk tracing for tools/difftest/equiv.py (portable C99, no dependency on unicorn's headers).
 *
 * Why: the equivalence harness runs two hot per-instruction / per-write hooks in Python (the uninit-copy taint pass,
 * Runner.uninit_copies, and the --fp-tol reference run's write/read tracking, FpTol.ref_run). Each fires millions of
 * times per input and the unicorn Python binding makes every call cost microseconds. This library registers NATIVE
 * unicorn callbacks (uc_hook_add on the same uc_engine handle, function pointers handed in from the loaded libunicorn,
 * so there is no link-time dependency) and keeps the bookkeeping in C. Python keeps every decision: instruction
 * decoding (capstone) stays in Python and is asked once per distinct (address, bytes) through a callback; the verdict
 * logic reads the summary at the end of the run.
 *
 * The semantics are a line-for-line port of the Python hooks (equiv.py: Runner.uninit_copies on_code / on_write /
 * src_taint / stale_w, FpTol.ref_run on_write / on_read); the Python code remains the reference and the fallback
 * (EQUIV_NATIVE=0 forces it). Built by tools/difftest/native/build.py into work/difftest/native/.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define EQAPI __declspec(dllexport)
#else
#define EQAPI __attribute__((visibility("default")))
#endif

/* ---- unicorn entry points, passed in by Python (the variadic uc_hook_add must be called through a variadic type) */
typedef int (*fn_reg_read)(void *uc, int regid, void *value);
typedef int (*fn_mem_read)(void *uc, uint64_t addr, void *bytes, size_t size);
typedef int (*fn_hook_add)(void *uc, size_t *hh, int type, void *cb, void *ud, uint64_t begin, uint64_t end, ...);
typedef int (*fn_hook_del)(void *uc, size_t hh);

#define UC_HOOK_CODE_T 4
#define UC_HOOK_MEM_READ_T 1024
#define UC_HOOK_MEM_WRITE_T 2048
/* unicorn x86 register ids (unicorn/x86.h) */
#define UC_REG_ESP 30
#define UC_REG_EIP 26
#define UC_REG_ESI 29
#define UC_REG_EDI 23

typedef struct {
    fn_reg_read reg_read; fn_mem_read mem_read; fn_hook_add hook_add; fn_hook_del hook_del;
} UcApi;

static uint32_t rd32(const UcApi *api, void *uc, int reg) {
    uint32_t v = 0;
    api->reg_read(uc, reg, &v);
    return v;
}

/* a taint origin value: ok == 0 is "False" (untainted); w == -1 is None (no recorded writer) */
typedef struct { int ok; uint32_t a; int64_t w; uint32_t r; } Org;
static const Org NOORG = {0, 0, 0, 0};

static uint64_t hash64(uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33;
    return x;
}

/* ================================================================ decoded-instruction cache */
enum { K_SKIP = 0, K_ERR, K_NOP, K_MOV, K_MOVS, K_PUSH, K_POP, K_PENDMEM, K_LEA, K_FLD, K_FADD, K_FST, K_GEN };
enum { S_NONE = 0, S_MEM, S_REG };
#define MAXCLR 64
typedef struct {
    int32_t kind;
    int32_t src;                    /* S_MEM / S_REG / S_NONE (mov, push source) */
    int32_t dst_reg;                /* capstone register id (mov / pop / lea destination); 0: mov to memory (pend) */
    int32_t src_reg;                /* capstone register id of a register source (mov, push, lea base) */
    int32_t ea_base, ea_index, ea_scale;   /* unicorn register ids, 0 = none */
    int64_t ea_disp;
    int32_t esize;                  /* fadd operand size */
    int32_t fpop;                   /* fstp */
    int32_t fclear;                 /* generic: the mnemonic starts with 'f' */
    int32_t nclr;
    int32_t clr[MAXCLR];            /* capstone ids dropped from regt (each written register and its full form) */
} Desc;

typedef struct { uint64_t addr; uint8_t raw[16]; int rawlen; int used; Desc d; } CEnt;
typedef struct { CEnt *t; size_t cap, n; } ICache;
/* decode callback: fill *d from the instruction at addr with these bytes; return 0 on a decode failure (K_SKIP) */
typedef int (*fn_decode)(uint64_t addr, const uint8_t *raw, int len, Desc *d);

static CEnt *icache_find(ICache *c, uint64_t addr, const uint8_t *raw, int len) {
    if (!c->cap) return NULL;
    size_t i = hash64(addr) & (c->cap - 1);
    while (c->t[i].used) {
        if (c->t[i].addr == addr && c->t[i].rawlen == len && memcmp(c->t[i].raw, raw, len) == 0) return &c->t[i];
        i = (i + 1) & (c->cap - 1);
    }
    return NULL;
}
static CEnt *icache_add(ICache *c, uint64_t addr, const uint8_t *raw, int len) {
    if ((c->n + 1) * 2 > c->cap) {
        size_t nc = c->cap ? c->cap * 2 : 4096, j;
        CEnt *nt = (CEnt *)calloc(nc, sizeof(CEnt));
        for (j = 0; j < c->cap; j++) if (c->t[j].used) {
            size_t i = hash64(c->t[j].addr) & (nc - 1);
            while (nt[i].used) i = (i + 1) & (nc - 1);
            nt[i] = c->t[j];
        }
        free(c->t); c->t = nt; c->cap = nc;
    }
    size_t i = hash64(addr) & (c->cap - 1);
    while (c->t[i].used) i = (i + 1) & (c->cap - 1);
    CEnt *e = &c->t[i];
    memset(e, 0, sizeof *e);
    e->used = 1; e->addr = addr; e->rawlen = len; memcpy(e->raw, raw, len);
    c->n++;
    return e;
}

EQAPI void *eq_icache_new(void) { return calloc(1, sizeof(ICache)); }
EQAPI void eq_icache_free(void *c) { if (c) { free(((ICache *)c)->t); free(c); } }
EQAPI int eq_desc_size(void) { return (int)sizeof(Desc); }

/* ================================================================ taint context (Runner.uninit_copies) */
typedef struct { uint64_t key; Org o; uint64_t seq; int used; } TEnt;
typedef struct { uint32_t seq; uint32_t pc; uint8_t inside; } LastW;
#define NREG 1024

typedef struct {
    UcApi api; void *uc; ICache *ic; fn_decode decode;
    uint64_t lo_s, entry, span_lo, span_hi, max_steps, steps;
    uint64_t nstack;                /* tracked stack bytes: lo_s .. entry + 0x1000 */
    uint8_t *written, *calleew; LastW *lastw;
    uint32_t *cw; size_t ncw, capcw;               /* callee-written list (entries may be stale: check calleew[]) */
    TEnt *tt; size_t tcap, tn; uint64_t tseq;      /* tainted: byte -> origin, with Python-dict insertion order */
    Org regt[NREG];
    Org *fpu; size_t nfpu, capfpu;
    Org pend; int64_t movs;                        /* movs < 0: none (else esi - edi) */
    uint64_t last_esp; int away; uint32_t seq, cur_pc;
    uint64_t *stale; size_t nstale, capstale;      /* stale_w slots hit, first-seen order */
    size_t hh_code, hh_write;
} TC;

static size_t tfind(TC *t, uint64_t key) {          /* slot of key, or the empty slot that ends its probe */
    size_t i = hash64(key) & (t->tcap - 1);
    while (t->tt[i].used && t->tt[i].key != key) i = (i + 1) & (t->tcap - 1);
    return i;
}
static void tset(TC *t, uint64_t key, Org o) {
    if (!t->tcap || (t->tn + 1) * 2 > t->tcap) {
        size_t nc = t->tcap ? t->tcap * 2 : 1024, j;
        TEnt *nt = (TEnt *)calloc(nc, sizeof(TEnt));
        TEnt *ot = t->tt; size_t oc = t->tcap;
        t->tt = nt; t->tcap = nc;
        for (j = 0; j < oc; j++) if (ot[j].used) nt[tfind(t, ot[j].key)] = ot[j];
        free(ot);
    }
    size_t i = tfind(t, key);
    if (t->tt[i].used) { t->tt[i].o = o; return; }          /* an update keeps the dict position */
    t->tt[i].used = 1; t->tt[i].key = key; t->tt[i].o = o; t->tt[i].seq = ++t->tseq; t->tn++;
}
static void tpop(TC *t, uint64_t key) {
    if (!t->tn) return;
    size_t i = tfind(t, key);
    if (!t->tt[i].used) return;
    t->tt[i].used = 0; t->tn--;
    size_t j = i;                                           /* backward-shift deletion */
    for (;;) {
        j = (j + 1) & (t->tcap - 1);
        if (!t->tt[j].used) break;
        size_t k = hash64(t->tt[j].key) & (t->tcap - 1);
        if ((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j))) { t->tt[i] = t->tt[j]; t->tt[j].used = 0; i = j; }
    }
}
static const Org *tget(TC *t, uint64_t key) {
    if (!t->tn) return NULL;
    size_t i = tfind(t, key);
    return t->tt[i].used ? &t->tt[i].o : NULL;
}

static int instack(const TC *t, uint64_t b) { return b >= t->lo_s && b < t->lo_s + t->nstack; }

static int stale_w(const TC *t, uint64_t a) {
    if ((a & 3) || a < 12 || !(t->lo_s <= a - 12 && a + 4 <= t->entry)) return 0;
    const LastW *w = &t->lastw[a - t->lo_s];
    if (!w->seq || w->inside) return 0;
    int k;
    uint64_t b;
    for (k = 4; k <= 12; k += 4)
        for (b = a - k; b < a - k + 4; b++) {
            const LastW *x = &t->lastw[b - t->lo_s];
            if (!x->seq || !x->inside || x->seq < w->seq) return 0;
        }
    for (b = a + 1; b < a + 4; b++) {
        const LastW *x = &t->lastw[b - t->lo_s];
        if (!x->seq || x->inside) return 0;
    }
    return 1;
}

static Org src_taint(TC *t, uint64_t a, int n) {
    Org o = NOORG;
    uint64_t b;
    if (n == 4 && stale_w(t, a)) {
        size_t i;
        for (i = 0; i < t->nstale; i++) if (t->stale[i] == a) break;
        if (i == t->nstale) {
            if (t->nstale == t->capstale) { t->capstale = t->capstale ? t->capstale * 2 : 64; t->stale = (uint64_t *)realloc(t->stale, t->capstale * 8); }
            t->stale[t->nstale++] = a;
        }
        o.ok = 1; o.a = (uint32_t)a; o.w = t->lastw[a - t->lo_s].pc; o.r = t->cur_pc;
        return o;
    }
    for (b = a; b < a + (uint64_t)n; b++) {
        if (tget(t, b)) continue;
        if (b >= t->lo_s && b < t->entry && !t->written[b - t->lo_s]) continue;
        return NOORG;
    }
    const Org *e = tget(t, a);
    if (e) return *e;
    o.ok = 1; o.a = (uint32_t)a;
    o.w = (instack(t, a) && t->lastw[a - t->lo_s].seq) ? (int64_t)t->lastw[a - t->lo_s].pc : -1;
    o.r = t->cur_pc;
    return o;
}

static uint64_t tc_ea(TC *t, const Desc *d) {
    uint64_t a = (uint64_t)d->ea_disp;
    if (d->ea_base) a += rd32(&t->api, t->uc, d->ea_base);
    if (d->ea_index) a += (uint64_t)rd32(&t->api, t->uc, d->ea_index) * (uint64_t)d->ea_scale;
    return a & 0xFFFFFFFFULL;
}

static void fpu_push(TC *t, Org o) {
    if (t->nfpu == t->capfpu) { t->capfpu = t->capfpu ? t->capfpu * 2 : 16; t->fpu = (Org *)realloc(t->fpu, t->capfpu * sizeof(Org)); }
    t->fpu[t->nfpu++] = o;
}

static void tc_code(void *uc, uint64_t address, uint32_t size, void *ud) {
    TC *t = (TC *)ud;
    t->steps++;
    if (t->steps > t->max_steps) return;
    t->cur_pc = (uint32_t)address;
    uint8_t raw[16];
    if (size > 15 || t->api.mem_read(uc, address, raw, size)) return;
    CEnt *e = icache_find(t->ic, address, raw, (int)size);
    if (!e) {
        Desc d;
        memset(&d, 0, sizeof d);
        if (!t->decode(address, raw, (int)size, &d)) d.kind = K_SKIP;
        e = icache_add(t->ic, address, raw, (int)size);
        e->d = d;
    }
    const Desc *d = &e->d;
    if (d->kind == K_SKIP) return;
    t->pend = NOORG; t->movs = -1;
    uint64_t esp_now = rd32(&t->api, uc, UC_REG_ESP);
    if (esp_now > t->last_esp) {
        /* the bytes esp just rose past belong to a frame that has returned: dead again */
        uint64_t hi = t->last_esp + 0x10000, b;
        if (esp_now < hi) hi = esp_now;
        for (b = t->last_esp; b < hi; b++) if (instack(t, b)) t->written[b - t->lo_s] = 0;
    }
    t->last_esp = esp_now;
    if (t->span_lo <= address && address < t->span_hi) {
        if (t->away) {
            size_t i;
            for (i = 0; i < t->ncw; i++) {
                uint32_t k = t->cw[i];
                if (t->calleew[k]) { t->written[k] = 0; t->calleew[k] = 0; }
            }
            t->ncw = 0; t->away = 0;
        }
    } else t->away = 1;

    switch (d->kind) {
    case K_ERR: case K_NOP: return;
    case K_MOV: {
        Org o = NOORG;
        if (d->src == S_MEM) o = src_taint(t, tc_ea(t, d), 4);
        else if (d->src == S_REG) o = t->regt[d->src_reg];
        if (d->dst_reg) t->regt[d->dst_reg] = o; else t->pend = o;
        return;
    }
    case K_MOVS:
        t->movs = (int64_t)((rd32(&t->api, uc, UC_REG_ESI) - rd32(&t->api, uc, UC_REG_EDI)) & 0xFFFFFFFFu);
        return;
    case K_PUSH:
        if (d->src == S_REG) t->pend = t->regt[d->src_reg];
        else if (d->src == S_MEM) t->pend = src_taint(t, tc_ea(t, d), 4);
        return;
    case K_POP: {
        uint64_t esp = rd32(&t->api, uc, UC_REG_ESP);
        Org o = src_taint(t, esp, 4);
        if (o.ok) { const Org *g = tget(t, esp); o = g ? *g : NOORG; }
        t->regt[d->dst_reg] = o;
        return;
    }
    case K_PENDMEM: t->pend = src_taint(t, tc_ea(t, d), 4); return;
    case K_LEA: t->regt[d->dst_reg] = t->regt[d->src_reg]; return;
    case K_FLD: fpu_push(t, src_taint(t, tc_ea(t, d), 4)); return;
    case K_FADD:
        /* st0 +- a memory operand; the operand is not examined when st0 is already tainted */
        if (t->nfpu && !t->fpu[t->nfpu - 1].ok) {
            Org o = src_taint(t, tc_ea(t, d), d->esize);
            t->fpu[t->nfpu - 1] = o;
        }
        return;
    case K_FST:
        t->pend = t->nfpu ? t->fpu[t->nfpu - 1] : NOORG;
        if (d->fpop && t->nfpu) t->nfpu--;
        return;
    case K_GEN: {
        int i;
        for (i = 0; i < d->nclr; i++) t->regt[d->clr[i]] = NOORG;
        if (d->fclear) t->nfpu = 0;
        return;
    }
    }
}

static void tc_write(void *uc, int type, uint64_t address, int size, int64_t value, void *ud) {
    TC *t = (TC *)ud;
    int i;
    (void)uc; (void)type; (void)value;
    t->seq++;
    int inside = t->span_lo <= t->cur_pc && t->cur_pc < t->span_hi;
    for (i = 0; i < size; i++) {
        uint64_t b = address + (uint64_t)i;
        Org o = t->pend;
        if (t->movs >= 0) o = src_taint(t, (b + (uint64_t)t->movs) & 0xFFFFFFFFULL, 1);
        if (instack(t, b)) {
            uint64_t k = b - t->lo_s;
            t->written[k] = 1;
            t->lastw[k].seq = t->seq; t->lastw[k].inside = (uint8_t)inside; t->lastw[k].pc = t->cur_pc;
            if (inside) t->calleew[k] = 0;
            else if (b < t->entry && !t->calleew[k]) {
                t->calleew[k] = 1;
                if (t->ncw == t->capcw) { t->capcw = t->capcw ? t->capcw * 2 : 1024; t->cw = (uint32_t *)realloc(t->cw, t->capcw * 4); }
                t->cw[t->ncw++] = (uint32_t)k;
            }
        }
        if (o.ok) tset(t, b, o); else tpop(t, b);
    }
}

EQAPI void *eq_taint_new(void *uc, void *reg_read, void *mem_read, void *hook_add, void *hook_del, void *icache, void *decode,
                         uint64_t lo_s, uint64_t entry, uint64_t span_lo, uint64_t span_hi, uint64_t max_steps) {
    TC *t = (TC *)calloc(1, sizeof(TC));
    if (!t) return NULL;
    t->api.reg_read = (fn_reg_read)reg_read; t->api.mem_read = (fn_mem_read)mem_read;
    t->api.hook_add = (fn_hook_add)hook_add; t->api.hook_del = (fn_hook_del)hook_del;
    t->uc = uc; t->ic = (ICache *)icache; t->decode = (fn_decode)decode;
    t->lo_s = lo_s; t->entry = entry; t->span_lo = span_lo; t->span_hi = span_hi; t->max_steps = max_steps;
    t->nstack = entry + 0x1000 - lo_s;
    t->written = (uint8_t *)calloc(t->nstack, 1); t->calleew = (uint8_t *)calloc(t->nstack, 1);
    t->lastw = (LastW *)calloc(t->nstack, sizeof(LastW));
    if (!t->written || !t->calleew || !t->lastw) { free(t->written); free(t->calleew); free(t->lastw); free(t); return NULL; }
    t->movs = -1; t->last_esp = entry;
    return t;
}
/* register both native hooks, in the order the Python path did (code, then write); 0 on success */
EQAPI int eq_taint_start(void *tp) {
    TC *t = (TC *)tp;
    int e = t->api.hook_add(t->uc, &t->hh_code, UC_HOOK_CODE_T, (void *)tc_code, t, 1, 0);
    if (e) return e;
    return t->api.hook_add(t->uc, &t->hh_write, UC_HOOK_MEM_WRITE_T, (void *)tc_write, t, 1, 0);
}
EQAPI void eq_taint_stop(void *tp) {
    TC *t = (TC *)tp;
    if (t->hh_code) t->api.hook_del(t->uc, t->hh_code);
    if (t->hh_write) t->api.hook_del(t->uc, t->hh_write);
    t->hh_code = t->hh_write = 0;
}
/* bit k set: stack-argument dword k (of 8) at esp is a plain copy of a never-written slot; evaluates all 8, in order */
EQAPI uint32_t eq_taint_args(void *tp, uint64_t esp) {
    TC *t = (TC *)tp;
    uint32_t m = 0;
    int k;
    for (k = 0; k < 8; k++) if (src_taint(t, esp + 4 * (uint64_t)k, 4).ok) m |= 1u << k;
    return m;
}
EQAPI uint64_t eq_taint_steps(void *tp) { return ((TC *)tp)->steps; }
EQAPI uint64_t eq_taint_nstale(void *tp) { return ((TC *)tp)->nstale; }
EQAPI void eq_taint_stale(void *tp, uint64_t *out) { TC *t = (TC *)tp; size_t i; for (i = 0; i < t->nstale; i++) out[i] = t->stale[i]; }

static int cmp_seq(const void *x, const void *y) {
    uint64_t a = (*(const TEnt *const *)x)->seq, b = (*(const TEnt *const *)y)->seq;
    return a < b ? -1 : a > b;
}
static int kept(const TC *t, const TEnt *e) { return e->used && !(e->key >= t->lo_s && e->key < t->entry); }
/* number of tainted bytes outside [lo_s, entry) -- what Python keeps in uninit_origin */
EQAPI uint64_t eq_taint_count(void *tp) {
    TC *t = (TC *)tp; uint64_t n = 0; size_t i;
    for (i = 0; i < t->tcap; i++) if (kept(t, &t->tt[i])) n++;
    return n;
}
/* fill the rows (address, origin a, origin writer pc (-1: None), origin read pc) in dict-insertion order */
EQAPI void eq_taint_dump(void *tp, uint64_t *addr, uint32_t *oa, int64_t *ow, uint32_t *orr) {
    TC *t = (TC *)tp; size_t n = 0, i;
    const TEnt **v = (const TEnt **)malloc((t->tn + 1) * sizeof(TEnt *));
    for (i = 0; i < t->tcap; i++) if (kept(t, &t->tt[i])) v[n++] = &t->tt[i];
    qsort(v, n, sizeof(TEnt *), cmp_seq);
    for (i = 0; i < n; i++) { addr[i] = v[i]->key; oa[i] = v[i]->o.a; ow[i] = v[i]->o.w; orr[i] = v[i]->o.r; }
    free(v);
}
EQAPI void eq_taint_free(void *tp) {
    TC *t = (TC *)tp;
    if (!t) return;
    free(t->written); free(t->calleew); free(t->lastw); free(t->cw); free(t->tt); free(t->fpu); free(t->stale);
    free(t);
}

/* ================================================================ ref_run tracking (FpTol.ref_run) */
typedef struct { uint64_t key; uint8_t val; int used; } FEnt;
typedef struct { uint64_t key; int used; } VEnt;
typedef struct { uint64_t addr; uint8_t raw[16]; int rawlen; int used; int f; } PEnt;
typedef int (*fn_fclass)(uint64_t pc, const uint8_t *raw, int len);

typedef struct {
    UcApi api; void *uc; fn_fclass classify;
    PEnt *pc; size_t pcap, pn;                     /* persistent (pc, bytes) -> is-float-store, shared across runs */
    FEnt *fs; size_t fcap, fn;                     /* fstore: dword address -> last write was a float store */
    VEnt *fv; size_t vcap, vn;                     /* fvals */
    int huge_in, denorm_in;
    size_t hh_w, hh_r;
} RC;

EQAPI void *eq_pcache_new(void) { return calloc(1, sizeof(RC)); }

static PEnt *pc_find(RC *r, uint64_t pc, const uint8_t *raw, int len) {
    if (!r->pcap) return NULL;
    size_t i = hash64(pc) & (r->pcap - 1);
    while (r->pc[i].used) {
        if (r->pc[i].addr == pc && r->pc[i].rawlen == len && memcmp(r->pc[i].raw, raw, len) == 0) return &r->pc[i];
        i = (i + 1) & (r->pcap - 1);
    }
    return NULL;
}
static PEnt *pc_add(RC *r, uint64_t pc, const uint8_t *raw, int len, int f) {
    if ((r->pn + 1) * 2 > r->pcap) {
        size_t nc = r->pcap ? r->pcap * 2 : 4096, j;
        PEnt *nt = (PEnt *)calloc(nc, sizeof(PEnt));
        for (j = 0; j < r->pcap; j++) if (r->pc[j].used) {
            size_t i = hash64(r->pc[j].addr) & (nc - 1);
            while (nt[i].used) i = (i + 1) & (nc - 1);
            nt[i] = r->pc[j];
        }
        free(r->pc); r->pc = nt; r->pcap = nc;
    }
    size_t i = hash64(pc) & (r->pcap - 1);
    while (r->pc[i].used) i = (i + 1) & (r->pcap - 1);
    PEnt *e = &r->pc[i];
    e->used = 1; e->addr = pc; e->rawlen = len; memcpy(e->raw, raw, len); e->f = f; r->pn++;
    return e;
}

static int fvals_has(const RC *r, uint64_t v) {
    if (!r->vcap) return 0;
    size_t i = hash64(v) & (r->vcap - 1);
    while (r->fv[i].used) { if (r->fv[i].key == v) return 1; i = (i + 1) & (r->vcap - 1); }
    return 0;
}
static void fvals_add(RC *r, uint64_t v) {
    if (fvals_has(r, v)) return;
    if ((r->vn + 1) * 2 > r->vcap) {
        size_t nc = r->vcap ? r->vcap * 2 : 1024, j;
        VEnt *nt = (VEnt *)calloc(nc, sizeof(VEnt));
        for (j = 0; j < r->vcap; j++) if (r->fv[j].used) {
            size_t i = hash64(r->fv[j].key) & (nc - 1);
            while (nt[i].used) i = (i + 1) & (nc - 1);
            nt[i] = r->fv[j];
        }
        free(r->fv); r->fv = nt; r->vcap = nc;
    }
    size_t i = hash64(v) & (r->vcap - 1);
    while (r->fv[i].used) i = (i + 1) & (r->vcap - 1);
    r->fv[i].used = 1; r->fv[i].key = v; r->vn++;
}
static void fstore_set(RC *r, uint64_t k, uint8_t v) {
    if ((r->fn + 1) * 2 > r->fcap) {
        size_t nc = r->fcap ? r->fcap * 2 : 1024, j;
        FEnt *nt = (FEnt *)calloc(nc, sizeof(FEnt));
        for (j = 0; j < r->fcap; j++) if (r->fs[j].used) {
            size_t i = hash64(r->fs[j].key) & (nc - 1);
            while (nt[i].used) i = (i + 1) & (nc - 1);
            nt[i] = r->fs[j];
        }
        free(r->fs); r->fs = nt; r->fcap = nc;
    }
    size_t i = hash64(k) & (r->fcap - 1);
    while (r->fs[i].used && r->fs[i].key != k) i = (i + 1) & (r->fcap - 1);
    if (!r->fs[i].used) { r->fs[i].used = 1; r->fs[i].key = k; r->fn++; }
    r->fs[i].val = v;
}

static void rc_write(void *uc, int type, uint64_t address, int size, int64_t value, void *ud) {
    RC *r = (RC *)ud;
    (void)type;
    uint64_t pc = rd32(&r->api, uc, UC_REG_EIP);
    uint8_t raw[16];
    int len = r->api.mem_read(uc, pc, raw, 15) ? 0 : 15;
    PEnt *e = pc_find(r, pc, raw, len);
    if (!e) e = pc_add(r, pc, raw, len, r->classify(pc, raw, len) ? 1 : 0);
    if (size >= 4) {
        /* an integer store of a value an x87/SSE store wrote earlier is a copied float */
        uint64_t v = (uint64_t)value & 0xFFFFFFFFULL;
        if (e->f) fvals_add(r, v);
        fstore_set(r, address & ~(uint64_t)3, (uint8_t)(e->f || fvals_has(r, v)));
    }
}
static void rc_read(void *uc, int type, uint64_t address, int size, int64_t value, void *ud) {
    RC *r = (RC *)ud;
    (void)type; (void)value;
    if (size == 4 && !(r->huge_in && r->denorm_in)) {
        uint32_t v, e;
        if (r->api.mem_read(uc, address, &v, 4)) return;
        e = (v >> 23) & 0xFF;
        if (e >= 127 + 100) r->huge_in = 1;
        else if (e == 0 && (v & 0x7FFFFF)) r->denorm_in = 1;
    }
}
/* start a run: fresh fstore / fvals / flags (the pc cache stays); the write hook first, then the read hook */
EQAPI int eq_ref_start(void *rp, void *uc, void *reg_read, void *mem_read, void *hook_add, void *hook_del, void *classify) {
    RC *r = (RC *)rp;
    free(r->fs); free(r->fv);
    r->fs = NULL; r->fv = NULL; r->fcap = r->fn = r->vcap = r->vn = 0; r->huge_in = r->denorm_in = 0;
    r->api.reg_read = (fn_reg_read)reg_read; r->api.mem_read = (fn_mem_read)mem_read;
    r->api.hook_add = (fn_hook_add)hook_add; r->api.hook_del = (fn_hook_del)hook_del;
    r->uc = uc; r->classify = (fn_fclass)classify;
    int e = r->api.hook_add(uc, &r->hh_w, UC_HOOK_MEM_WRITE_T, (void *)rc_write, r, 1, 0);
    if (e) return e;
    return r->api.hook_add(uc, &r->hh_r, UC_HOOK_MEM_READ_T, (void *)rc_read, r, 1, 0);
}
EQAPI void eq_ref_stop(void *rp) {
    RC *r = (RC *)rp;
    if (r->hh_w) r->api.hook_del(r->uc, r->hh_w);
    if (r->hh_r) r->api.hook_del(r->uc, r->hh_r);
    r->hh_w = r->hh_r = 0;
}
EQAPI int eq_ref_flags(void *rp) { RC *r = (RC *)rp; return r->huge_in | (r->denorm_in << 1); }
EQAPI uint64_t eq_ref_nstore(void *rp) { return ((RC *)rp)->fn; }
EQAPI void eq_ref_dump(void *rp, uint64_t *addr, uint8_t *val) {
    RC *r = (RC *)rp; size_t n = 0, i;
    for (i = 0; i < r->fcap; i++) if (r->fs[i].used) { addr[n] = r->fs[i].key; val[n] = r->fs[i].val; n++; }
}
EQAPI void eq_pcache_free(void *rp) {
    RC *r = (RC *)rp;
    if (!r) return;
    free(r->pc); free(r->fs); free(r->fv); free(r);
}
