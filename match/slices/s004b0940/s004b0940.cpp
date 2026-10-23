// Slice s004b0940: the script-line helpers of the mission script interpreter.
// 0x4b0940 (ScriptHolder::run_line) runs a single script line; 0x4b09e0 (ScriptHolder::emit) is the
// mission-level script runner: it steps over the statement list of the holder, dispatches each opcode
// through the per-statement callees (0x49dd30 / 0x487930 / 0x49d930 / 0x49d650 / 0x49d500 / 0x49d7f0),
// watches the input frames for the abort/return/continue keys and logs "At %s." when the script speaks.
// The callees are not decompiled, so 0x4b09e0 is faithful but not byte-exact. Names provisional.
#include <stdio.h>

// ---- tstring: the engine's 16-byte string with a read cursor (see s004608d0/tstring.h) ----
struct tstring {
    int pos;                                    // +0  read cursor
    char* buf;                                  // +4  (null while empty)
    int cap;                                    // +8
    int len;                                    // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    char equals(const tstring& o) const;        // 0x42bdd0
};

// message log object at 0x5d7080 (see s004333c0 / s004428c0); add takes its tstring by value
struct MsgLog {
    void add(tstring s, int color);             // 0x452c00 (thiscall, ret 0x14)
};
extern MsgLog g_msglog;                         // 0x5d7080

// text log stream at 0x5a9f60 (see s00418a70): slot 10 (+0x28) is w_cstr
struct stream_base {
    virtual void reset();
    virtual void rewind();
    virtual stream_base& w_stream(void* o) = 0;
    virtual stream_base& w_float(void* v) = 0;
    virtual stream_base& w_bool(void* v) = 0;
    virtual stream_base& w_u64(void* v) = 0;
    virtual stream_base& w_i64(void* v) = 0;
    virtual stream_base& w_uint(void* v) = 0;
    virtual stream_base& w_int(int* v) = 0;
    virtual stream_base& w_byte(void* v) = 0;
    virtual stream_base& w_cstr(const char* s) = 0;
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
};
extern log_stream g_log;                        // 0x5a9f60

// ---- interpreter globals ------------------------------------------------------------------------------
extern int   g_lineno;                          // 0x5e2598  script line number
extern int   g_script_out;                      // 0x5e259c  "script produced output" flag
extern int   g_cur_opcode;                      // 0x5e25a0  opcode of the statement being run
extern int   g_script_flag;                     // 0x5e25a4  "aborted/returned" latch cleared each run
extern char* g_mission;                         // 0x5e292c  current Mission (0 when none)

// keyboard state: per-scan-code ints, held state at 0xa17000 and this frame's press at 0xa17400 (see s0052cf60)
extern int g_key_a[256];                        // 0xa17000
extern int g_key_b[256];                        // 0xa17400

// the game data: the "current emitter audio" state at +0x1a068..+0x1a07c (see s00435490.h SoundCtl)
struct GameData {
    char pad0[0x1a068];
    int f1a068;                                 // +0x1a068  current emitter id (0 = none)
    tstring name;                               // +0x1a06c  current emitter name
    int f1a07c;                                 // +0x1a07c  current emitter tag
};
extern GameData* g_game;                        // 0x5e2930

// ---- the script block / interpreter ---------------------------------------------------------------------
struct ScriptHolder;
struct Elem {                                   // 0x98 bytes: one statement
    unsigned id;                                // +0
    int pad4;
    int n;                                      // +8   number of arguments
    int padc;
    __int64 vals[16];                           // +0x10
    int f90;                                    // +0x90 skip target (braces/else)
    int pad94;
    void set_value(int* a, long long* out);     // 0x49d500 (thiscall, ret 8)
    __int64 param(int* buf, int* which, int* out);    // 0x49d650 (thiscall on the statement, ret 0xc)
};
struct ScriptStmt {                             // statement pointer in ECX
    char run(int stmt, int which, int* out);    // 0x49fa10 (thiscall, ret 0xc)
};
struct ScriptHolder {
    Elem* items;                                // +0
    int count;                                  // +4
    char pad8[0x10 - 8];
    char* name;                                 // +0x10 (the script id; empty string is its own address)

    char run_line(int a, int* p, int which);                        // 0x4b0940 (thiscall, ret 0xc)
    char emit(int script, int depth, int* limit, __int64* vals, unsigned tag);   // 0x4b09e0 (ret 0x14)
    bool run_stmt(int* buf, int* buf2);                             // 0x49d7f0 (thiscall, ret 8)
    void format(tstring* out, int* buf, int line, int ok, int* lo, int* hi);   // 0x49d930
    void pop_party(int* a, int* t, int* out);                       // 0x49f7c0 (thiscall, ret 0xc)
    void reset_script_out();                                        // 0x49dd30 (thiscall): builds the statement list / clears the output
};

// the 0x34-byte emitter records at 0xa75eb0; the script sub-object is at +0x18 and the name at +4
struct Emitter {
    int pad0;                                   // +0
    tstring name;                               // +4
    char pad14[0x18 - 0x14];
    ScriptHolder script;                        // +0x18
    char pad34[0x34 - 0x18 - sizeof(ScriptHolder)];
};
extern Emitter* g_emitters;                     // 0xa75eb0
extern int g_emitter_count;                     // 0xa75eb4

// The goto/branch stack object (local_410 of emit): 0x400 bytes of entries, then the depth counter at +0x400
// (read by 0x488e60 / 0x49d7f0 / 0x49f7c0; the emitter only tests it against 0) and one more dword. Both
// trailing dwords are cleared in emit's prologue.
struct ScriptStack {
    int data[256];
    int sp;
    int pad;
    void goto_stmt(int line, int val);          // 0x488e60 (thiscall, ret 8)
};

// ---- per-statement interpreter callees (not decompiled; declared for shape) -----------------------------
struct KeyState { void frame(int a); };         // 0x487930 (thiscall, ret 4)
extern KeyState g_keyboard;                     // 0xa170c8
char __fastcall check_script_param(int* value, int* limit);   // 0x49ccc0
void pump_messages();                           // 0x4bb310
void set_debug_label();                         // 0x4bb390
void set_progress_range(int lo, int hi);        // 0x4bb3b0
void refresh_settings_list(const tstring* page, char detailed);   // 0x4bb3f0
void __cdecl mb_error(const char* fmt, ...);    // 0x42cc80

struct AgentTable { unsigned first_used(); };   // 0x43b1b0 lives at g_mission + 4
struct PartyMgr {
    char pad0[0x138dc - 0xf8];
    unsigned first_party_id();                  // 0x460d20 (thiscall, ret 4)
};

// @ 0x4b0940
char ScriptHolder::run_line(int a, int* p, int which)
{
    int out;
    char ok = ((ScriptStmt*)(items + *p))->run(a, which, &out);
    if (out > 0) {
        char buf[512];
        const char* q = name ? name : (char*)&name;
        _snprintf(buf, 0x200, "At %s.", q);
        g_msglog.add(tstring(buf), 0xffff5555);
        stream_base& lg = g_log;
        lg.w_cstr(buf);
    }
    return ok;
}

// @ 0x4b09e0
// The mission script runner. Steps over the statements `items[idx]` of this holder. Each opcode is run
// through one of the interpreter helpers, then run_stmt decides whether to fall through or jump to the
// statement's skip target (f90). While the script is "inside a key frame" (the tag matches the current
// line, or the frame was opened earlier) the input frames are polled for the abort/return/continue keys.
char ScriptHolder::emit(int script, int depth, int* limit, __int64* vals, unsigned tag)
{
    char last_ok = 1;                           // local_b05
    char done = 0;                              // local_b07
    char in_key = 0;                            // local_b06
    char brk = 0;                               // local_afa
    char first = 0;                             // local_af9
    char ok = 1;                                // local_af4
    int saved;                                  // local_af0
    int which;                                  // local_b04
    int idx = 0;                                // local_b00
    char advance = 1;                           // local_b08
    int pr_lo, pr_hi;                           // local_ae4 / local_ae0
    int out1;                                   // local_aec
    int out2;                                   // local_ad8
    int out3;                                   // local_ac4
    int range_lo, range_hi;                     // local_ad4 / local_adc
    int nsub;                                   // local_ae8
    long long value;                            // local_ad0 / local_acc
    ScriptHolder* self = this;                  // local_af8
    int buf[256];                               // local_a10
    ScriptStack buf2;                           // local_410
    char msg[512];                              // local_610
    __declspec(align(8)) __int64 sub[16];                            // auStack_a98 + 2
    char slot0[0x10];                           // local_ac0 (a tstring the callee constructs)
    char slot1[0x10];                           // local_ab0
    char slot2[0x10];                           // local_aa0

    g_script_out = 0;
    reset_script_out();
    if (g_script_out > 0) {
        const char* q = name ? name : (char*)&name;
        _snprintf(msg, 0x200, "At %s.", q);
        g_msglog.add(tstring(msg), 0xffff5555);
        stream_base& lg = g_log;
        lg.w_cstr(msg);
    }
    g_script_out = 0;
    buf2.sp = 0;
    buf2.pad = 0;
    last_ok = 1;
    g_script_flag = 0;
    idx = 0;
    in_key = 0;
    done = 0;
    advance = 1;

    for (;;) {
        if (idx >= count)
            break;
        brk = 0;
        if ((tag & 0xffff) == (unsigned)idx || in_key) {
            in_key = 1;
            first = 0;
            for (;;) {
                g_keyboard.frame(0);
                if (!first) {
                    first = 1;
                    format((tstring*)slot0, buf, idx, 1, &pr_lo, &pr_hi);
                    set_progress_range(pr_lo, pr_hi);
                    refresh_settings_list((tstring*)slot0, 1);
                    ((tstring*)slot0)->~tstring();
                }
                if ((g_key_a[0x3b] && !g_key_b[0x3b]) || g_game->f1a068 == 0) {
                    g_game->f1a068 = 0;
                    set_debug_label();
                    tag = 0xffffffff;
                    done = 1;
                } else if (g_key_a[0x3a] && !g_key_b[0x3a]) {
                    tag = 0xffffffff;
                    done = 1;
                } else if (g_key_a[0x39] && !g_key_b[0x39]) {
                    int e = (int)tag >> 16;
                    if (g_emitters[e].name.equals(g_game->name))
                        tag = ((unsigned)e << 16) | (unsigned)g_game->f1a07c;
                    done = 1;
                } else if (g_key_a[0x38] && !g_key_b[0x38]) {
                    brk = 1;
                } else if (!(g_key_a[0x37] && !g_key_b[0x37])) {
                    pump_messages();
                    continue;
                }
                break;
            }
            pump_messages();
        }

        Elem* stmt = items + idx;
        int off  = idx * 0x98;
        int op = stmt->id & 0xfffffff;
        (void)off;
        g_lineno = idx;
        saved = idx;
        ok = 1;
        g_cur_opcode = op;

        if (op < 0x15 && op != 1) {
            if (stmt->id == 3) {
                advance = 1;
                pop_party(buf, buf2.data, &idx);
            } else if (stmt->id == 4) {
                advance = 1;
                buf2.goto_stmt(idx, 0);
                idx++;
            } else if (stmt->id == 5) {
                if (advance) {
                    idx = stmt->f90;
                } else {
                    idx++;
                    advance = 1;
                }
            } else {
                int v = 0;
                advance = 1;
                if (stmt->id == 6) {
                    which = 1;
                    v = (int)stmt->param(buf, &which, &out2);
                } else if (stmt->id == 7) {
                    which = 2;
                    v = (int)stmt->param(buf, &which, &out2) - 1;
                } else if (stmt->id == 0xc) {
                    v = ((AgentTable*)(g_mission + 4))->first_used();
                } else if (stmt->id == 0xb) {
                    v = (int)((PartyMgr*)((char*)g_game + 0xf8))->first_party_id();
                } else if (stmt->id == 0xf) {
                    v = 0;
                } else {
                    mb_error("invalid opcode %d", stmt->id);
                }
                buf2.goto_stmt(idx, v);
                value = v;
                stmt->set_value(buf, &value);
                if (!run_stmt(buf, buf2.data)) {
                    idx = stmt->f90;
                    goto step_end;
                }
                idx++;
            }
        } else if (op < 0x1e) {
            if (op == 1) {
                which = 0;
                int id = (int)stmt->param(buf, &which, &out1);
                if (id < 0 || id >= g_emitter_count || depth >= 0x100) {
                    ok = 0;
                    advance = 0;
                    idx = stmt->f90;
                    goto step_end;
                }
                which = 1;
                for (int i = 1; i < stmt->n; i++) {
                    which = i;
                    sub[i - 1] = stmt->param(buf, &which, &out1);
                }
                unsigned t;
                if (brk)
                    t = 0;
                else if (g_game->f1a068 != 0 && g_emitters[id].name.equals(g_game->name))
                    t = ((unsigned)id << 16) | (unsigned)g_game->f1a07c;
                else
                    t = 0xffffffff;
                nsub = stmt->n - 1;
                ok = g_emitters[id].script.emit(script, depth + 1, &nsub, sub, t);
                if (!ok) {
                    advance = 0;
                    idx = stmt->f90;
                } else {
                    idx++;
                }
            } else if (op == 0x15) {
                which = 0;
                if (check_script_param(&which, limit))
                    stmt->set_value(buf, (long long*)vals);
                idx++;
            } else if (op == 0x16) {
                which = 1;
                if (check_script_param(&which, limit))
                    stmt->set_value(buf, (long long*)((char*)vals + 8));
                idx++;
            } else if (op == 0x17) {
                which = 1;
                int k = (int)stmt->param(buf, &which, &out3) - 1;
                which = k;
                if (check_script_param(&which, limit))
                    stmt->set_value(buf, &vals[k]);
                idx++;
            } else {
                idx++;
            }
        } else {
            ok = run_line((int)buf, &idx, (int)&script);
            if (ok) {
                idx++;
            } else {
                advance = 0;
                idx = stmt->f90;
            }
        }

    step_end:
        if (buf2.sp == 0)
            last_ok = ok;
        if (in_key) {
            if (done) {
                in_key = 0;
                done = 0;
            }
            format((tstring*)slot2, buf, saved, 1, &which, &saved);
            format((tstring*)slot1, buf, saved, ok, &range_lo, &range_hi);
            set_progress_range(range_lo, range_hi);
            refresh_settings_list((tstring*)slot1, 0);
            ((tstring*)slot1)->~tstring();
            ((tstring*)slot2)->~tstring();
        }
        if (g_script_out > 0) {
            const char* q = self->name ? self->name : (char*)&self->name;
            _snprintf(msg, 0x200, "At %s.", q);
            g_msglog.add(tstring(msg), 0xffff5555);
            stream_base& lg = g_log;
            lg.w_cstr(msg);
        }
        if (!last_ok)
            break;
    }

    return last_ok;
}
