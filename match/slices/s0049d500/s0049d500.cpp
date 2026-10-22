// Slice s0049d500: script-interpreter operand accessors / lvalue assignment and the relational comparison
// operator (0x49d500..0x49d930). Opcodes (module system `header_operations.py` numbering + 30 offset):
// 6 = assign, 7 = eq, 0xb = lt, 0xc = gt, 0xf = le, 0x11 = local temporary variable. The interpreter walks its
// instruction array with a 0x98-byte stride; 0x49dd30 is the try/else_try/try_end linker.
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "rgl.h"
extern "C" void _ReadWriteBarrier(void); // compiler barrier (emits no code)

// tstring: the engine's 16-byte string with a read cursor (declaration verified in s0042c0f0).
struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4  (null while empty)
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
    void assign(const char* s);                 // 0x42bcb0
    void push_back(char c);                     // 0x42be70
    void erase(int start, int n);               // 0x42bea0
    int  find(const char& c, int start) const;  // 0x42c080
    void append(const tstring& o);              // 0x42c1e0
    int  find(const tstring& s, int start) const;   // 0x42c440
    tstring substr(int start, int end) const;   // 0x42c4a0
    void trim();                                // 0x42c7a0
    void insert(int at, const tstring& s);      // 0x42c900
};

// 0xa70490/0xa70494: register pair table.  Records are 8 bytes apart, so index p*2 into each 32-bit array.
extern unsigned g_reg_lo[];                         // 0xa70490
extern int g_reg_hi[];                              // 0xa70494
struct VarTable {
    int get(int id);                                // 0x48d690
    void set(int id, int v);                        // 0x493db0
};
extern VarTable g_vartab;                           // 0xa70440
extern int g_cur_opcode;                            // 0x5e25a0
extern int g_cur_param;                             // 0x5e2590
extern char* g_game;                                // 0x5e2930 (reads +0x139d4; tstring at +0x1a080)
// the script text tstring lives inside the game object at g_game + 0x1a080
extern int* g_party;                                // 0x5e292c (reads +4)
extern int g_script_output;                         // 0x5a9fb0
// mb_error(const char* msg): the message string reaches it in ecx (__fastcall), so it builds its own
// tstring and prints; call sites load the address directly into ecx.
void __fastcall report(const char* msg);             // 0x49cc60
struct PartyMgr {
    void* get(int id);                              // 0x51ee50
    int index_of(int id);                           // 0x48ad00
};
int __fastcall container_size(void* p);             // 0x40f010 (count = (last-first)>>2)

// 0x49d160 is a file-static function in the original translation unit, so its pointer argument arrives in
// EAX.  A local static re-implementation reproduces that convention exactly (and hence the original
// behaviour); the party-table probe mirrors 0x49d160: max-id guard, index_of, page/base lookup, live flag.
static __declspec(noinline) bool check_party(int* id)
{
    char buf[0x200];
    int v = *id;
    PartyMgr* pm = (PartyMgr*)(g_game + 0xf8);
    if ((unsigned)v < *(unsigned*)((char*)pm + 0x138dc)) {
        int r = pm->index_of(v);
        if (r >= 0) {
            void** page = *(void***)((char*)pm + 0x138d0);
            void* base = page[r >> 4];
            if (*(int*)((char*)base + (r & 0xf) * 0x5ac8 + 4) == 1)
                return true;
        }
    }
    sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Party ID: %d", g_cur_opcode, v);
    report(buf);
    return false;
}

// 0x54ba40 "MyButton", 0x593164 ", ", 0x593160 " = ", 0x593158 " = %d"
// 0x593168 "Warning: Unmatched try_end", 0x593184 "Warning: Unmatched else_try."

struct Interp {
    int* instrs;                                    // +0    instruction array (0x98-byte stride)
    int count;                                      // +4    number of instructions
    char linked;                                    // +8    instruction offsets linked
    char pad9[3];
    int f0c;
    int p0;                                         // +0x10 operand 0
    int p1;                                         // +0x14 operand 1 (7-bit kind in bits 24..30)
    int f18;
    int flag;                                       // +0x400
    __int64 get(int local, int* idxp, int* kindp);  // 0x49d650
    bool less(int local, int* cur);                 // 0x49d7f0
    void assign(int local, int* v);                 // 0x49d500
    tstring renumber(int local, int index, int use_var, int* pvar, int* pend);  // 0x49d930
    void link();                                    // 0x49dd30
};

// @ 0x49d500
void Interp::assign(int local, int* v)
{
    char buf[512];
    int k = p1;
    int idx = p0;
    if (k < 0 || (k == 0 && (unsigned)idx >= 0x80000000u)) {
        sprintf(buf, "Illegal lvalue in statement; Opcode = %d", g_cur_opcode, g_cur_param);
        report(buf);
        return;
    }
    int bad = 0;
    int kind = ((unsigned)k >> 24) & 0x7f;
    if (kind == 1) {
        if (bad != 0) {
        } else if (idx < 0 || idx >= 0x80) {
            report("illegal register no.");
            return;
        } else {
            g_reg_lo[idx*2] = v[0];
            g_reg_hi[idx*2] = v[1];
            return;
        }
    } else if (kind == 2) {
        if (bad != 0) {
        } else if (idx < 0 || idx >= 0x10000) {
            report("illegal variable no.");
            return;
        } else {
            g_vartab.set(idx, *v);
            return;
        }
    } else if (kind == 0x11) {
        if (bad != 0) {
        } else if (idx < 0 || idx >= 0x80) {
            report("illegal local variable no.");
            return;
        } else {
            *(int*)(local + idx * 8) = *v;
            *(int*)(local + idx * 8 + 4) = v[1];
            return;
        }
    }
    sprintf(buf, "Illegal lvalue in statement; Opcode = %d", g_cur_opcode, g_cur_param);
    report(buf);
}

// @ 0x49d650
__int64 Interp::get(int local, int* idxp, int* kindp)
{
    char buf[512];
    int idx = *idxp;
    int p = *(int*)((char*)this + idx * 8 + 0x10);
    int k = *(int*)((char*)this + idx * 8 + 0x14);
    if (k < 0 || (k == 0 && (unsigned)p < 0x80000000u)) {
        *kindp = 0;
        return ((__int64)(unsigned)k << 32) | (unsigned)p;
    }
    int kind = ((unsigned)k >> 24) & 0x7f;
    *kindp = kind;
    int r = k & 0xffffff;
    if (kind == 0)
        return ((__int64)(unsigned)k << 32) | (unsigned)p;
    if (kind == 1) {
        if (p >= 0 && p < 0x80) {
            unsigned lo = g_reg_lo[p*2];
            _ReadWriteBarrier();
            int hi = g_reg_hi[p*2];
            return (__int64)lo | ((__int64)hi << 32);
        }
        sprintf(buf, "Invalid register no in operation = %d, param0 = %d param1 = %d", (int)this->instrs, p0, f18);
        report(buf);
        return 0;
    }
    if (kind == 2) {
        if (r == 0 && (unsigned)p < 0x10000u) {
            int v = g_vartab.get(p);
            return (__int64)v;
        }
        sprintf(buf, "Invalid variable no in operation = %d, param0 = %d param1 = %d", (int)this->instrs, p0, f18);
        report(buf);
        return 0;
    }
    if (kind == 0x11) {
        if (r == 0 && (unsigned)p < 0x80u)
            return *(__int64*)(local + p * 8);
        sprintf(buf, "Invalid local variable no in operation = %d, param0 = %d param1 = %d", (int)this->instrs, p0, f18);
        report(buf);
        return 0;
    }
    return ((__int64)(unsigned)r << 32) | (unsigned)p;
}

// @ 0x49d7f0
bool Interp::less(int local, int* cur)
{
    int n = *(int*)((char*)cur + 0x400);
    if (n == 0)
        return false;
    int left, idx;
    if (n >= 1 && n < 0x80) {
        idx = *(int*)((char*)cur + n * 8 - 8);
        left = *(int*)((char*)cur + n * 8 - 4);
    } else {
        idx = local;
        left = local;
    }
    int op = *(int*)((char*)instrs + idx * 0x98);
    Interp* ins = (Interp*)((char*)instrs + idx * 0x98);
    int kind;
    if (op == 6) {
        int two = 2;
        int v = (int)ins->get(local, &two, &kind);
        return left < v;
    }
    if (op == 7) {
        int one = 1;
        int v = (int)ins->get(local, &one, &kind);
        return left >= v;
    }
    if (op == 0xc)
        return left < *(int*)((char*)g_party + 4);
    if (op == 0xb)
        return left < *(int*)(g_game + 0x139d4);
    if (op == 0xf) {
        int one = 1;
        int v = (int)ins->get(local, &one, &kind);
        if (check_party(&v)) {
            PartyMgr* pm = (PartyMgr*)(g_game + 0xf8);
            void* party = pm->get(v);
            return left < container_size((char*)party + 0x8b8);
        }
        return left < -1;
    }
    int one = 1;
    int v = (int)ins->get(local, &one, &kind);
    return left < v;
}

// @ 0x49d930
tstring Interp::renumber(int local, int index, int use_var, int* pvar, int* pend)
{
    tstring* src = (tstring*)((char*)this + 0xc);   // the interpreter's script-text tstring
    tstring* gs = (tstring*)(g_game + 0x1a080);     // the whole script text
    tstring result;
    tstring key("\"");
    key.append(src->substr(src->find(' ', 0) + 1, src->len));
    {
        tstring q("\"");
        key.append(q);
    }
    int pos = gs->find(key, 0);
    tstring value;
    if (pos > 0) {
        for (int i = index; i > 0; i--) {
            if (pos >= 2) {
                tstring sep("),");
                pos = gs->find(sep, pos) + 2;
            }
        }
        if (pos >= 2)
            pos = gs->find('(', pos) + 1;
        if (pos >= 1) {
            int end;
            {
                tstring sep("),");
                end = gs->find(sep, pos);
            }
            if (end >= 0) {
                *pvar = pos;
                *pend = end;
                tstring body = gs->substr(pos, end);
                int eq;
                {
                    tstring eqs(" = ");
                    eq = body.find(eqs, 0);
                }
                while (eq >= 0) {
                    int c = body.find(',', eq);
                    if (c < 0)
                        c = body.len;
                    body.erase(eq, c - eq);
                    tstring eqs(" = ");
                    eq = body.find(eqs, 0);
                }
                value = body;
                int c = body.find(',', 0);
                if (c >= 0) {
                    int base = c + 1;
                    body = body.substr(base, body.len);
                    int idx = 0;
                    for (;;) {
                        int n = body.find(',', 0);
                        if (n <= 0 && body.len <= 0)
                            break;
                        int endpos = (n <= 0) ? body.len : n;
                        tstring item = body.substr(0, endpos);
                        if (endpos + 1 < body.len)
                            body = body.substr(endpos + 1, body.len);
                        else
                            body.assign("");
                        item.trim();
                        bool named = false;
                        for (int k = 0; k < item.len; k++) {
                            char ch = item.buf[k];
                            if (ch != '-' && (ch < '0' || ch > '9')) {
                                named = true;
                                break;
                            }
                        }
                        if (named) {
                            int kind;
                            int v = (int)((Interp*)((char*)instrs + index * 0x98))->get(local, &idx, &kind);
                            char tmp[132];
                            sprintf(tmp, " = %d", v);
                            tstring pre(tmp);
                            tstring num(tmp);
                            value.insert(base + endpos, num);
                            base += pre.len;
                        }
                        idx++;
                        base = base + 1 + endpos;
                    }
                }
            }
        }
    }
    result.append(value);
    return result;
}

// @ 0x49dd30
void Interp::link()
{
    int stack[0x100];
    int try_end[0x100];
    if (linked) return;
    int depth = 0;
    for (int i = 0; i < count; i++) {
        unsigned op = *(unsigned*)((char*)instrs + i * 0x98);
        if ((op & 0xfffffff) < 30) {
            if (op == 4 || op == 6 || op == 7 || op == 0xb || op == 0xf || op == 0xc) {
                try_end[depth] = i;
                stack[depth++] = i;
            } else if (op == 5) {
                if (depth <= 0) {
                    report("Warning: Unmatched else_try.");
                } else {
                    for (int j = stack[depth - 1] + 1; j < i; j++)
                        if (*(int*)((char*)instrs + j * 0x98 + 0x90) < 0)
                            *(int*)((char*)instrs + j * 0x98 + 0x90) = i;
                    stack[depth - 1] = i;
                }
            } else if (op == 3) {
                if (depth <= 0) {
                    if (g_script_output) report("Warning: Unmatched try_end");
                } else {
                    for (int j = try_end[depth - 1]; j < i; j++)
                        if (*(int*)((char*)instrs + j * 0x98 + 0x90) < 0)
                            *(int*)((char*)instrs + j * 0x98 + 0x90) = i;
                    depth--;
                }
            }
        }
    }
    for (int i = 0; i < count; i++)
        if (*(int*)((char*)instrs + i * 0x98 + 0x90) < 0)
            *(int*)((char*)instrs + i * 0x98 + 0x90) = count;
    linked = 1;
}
