// port/gfx/asm.cpp: D3DXAssembleShaderFromFileA (pixel shader assembly -> Direct3D 9 shader bytecode tokens) and the
// bytecode translator CreatePixelShader runs (tokens -> HLSL over the shared varying interface -> SDL_GPUShader).
// Covers ps_1_1..ps_1_4 arithmetic and texture instructions, modifiers, masks, swizzles, co-issue, def, and the ps_2_0
// subset (dcl, texld, arithmetic) a game asm file can use.
#include "gfx_internal.h"
#include "gfx_shader_sources.h"
#include <cctype>

namespace mbg {

ID3DXBuffer* make_buffer(const void* p, size_t n);

enum Op : uint32_t {
    OP_NOP = 0, OP_MOV = 1, OP_ADD = 2, OP_SUB = 3, OP_MAD = 4, OP_MUL = 5, OP_RCP = 6, OP_RSQ = 7, OP_DP3 = 8, OP_DP4 = 9,
    OP_MIN = 10, OP_MAX = 11, OP_SLT = 12, OP_SGE = 13, OP_EXP = 14, OP_LOG = 15, OP_LRP = 18, OP_FRC = 19, OP_POW = 32,
    OP_ABS = 35, OP_NRM = 36, OP_DCL = 31, OP_TEXCOORD = 64, OP_TEXKILL = 65, OP_TEX = 66, OP_CND = 80, OP_DEF = 81,
    OP_CMP = 88, OP_DP2ADD = 90, OP_PHASE = 0xFFFD, OP_COMMENT = 0xFFFE, OP_END = 0xFFFF
};
enum RegType : uint32_t { R_TEMP = 0, R_INPUT = 1, R_CONST = 2, R_TEXTURE = 3, R_SAMPLER = 10, R_COLOROUT = 8, R_DEPTHOUT = 9 };

struct OpInfo { const char* name; uint32_t op; int nsrc; bool has_dst; };
static const OpInfo k_ops[] = {
    {"nop", OP_NOP, 0, false}, {"mov", OP_MOV, 1, true}, {"add", OP_ADD, 2, true}, {"sub", OP_SUB, 2, true},
    {"mad", OP_MAD, 3, true}, {"mul", OP_MUL, 2, true}, {"rcp", OP_RCP, 1, true}, {"rsq", OP_RSQ, 1, true},
    {"dp3", OP_DP3, 2, true}, {"dp4", OP_DP4, 2, true}, {"min", OP_MIN, 2, true}, {"max", OP_MAX, 2, true},
    {"slt", OP_SLT, 2, true}, {"sge", OP_SGE, 2, true}, {"exp", OP_EXP, 1, true}, {"log", OP_LOG, 1, true},
    {"lrp", OP_LRP, 3, true}, {"frc", OP_FRC, 1, true}, {"pow", OP_POW, 2, true}, {"abs", OP_ABS, 1, true},
    {"nrm", OP_NRM, 1, true}, {"texcoord", OP_TEXCOORD, 0, true}, {"texcrd", OP_TEXCOORD, 1, true},
    {"texkill", OP_TEXKILL, 0, true}, {"tex", OP_TEX, 0, true}, {"texld", OP_TEX, 2, true}, {"cnd", OP_CND, 3, true},
    {"cmp", OP_CMP, 3, true}, {"dp2add", OP_DP2ADD, 3, true}, {"phase", OP_PHASE, 0, false},
};
static const OpInfo* op_by_name(const std::string& n)
{
    for (const OpInfo& o : k_ops) if (n == o.name) return &o;
    return nullptr;
}
static const OpInfo* op_by_code(uint32_t c, bool ps2)
{
    if (c == OP_TEX) return ps2 ? op_by_name("texld") : op_by_name("tex");
    if (c == OP_TEXCOORD) return op_by_name("texcoord");
    for (const OpInfo& o : k_ops) if (o.op == c) return &o;
    return nullptr;
}

static inline uint32_t regtok(uint32_t type, uint32_t num) { return 0x80000000u | (num & 0x7ff) | ((type & 7) << 28) | ((type & 0x18) << 8); }

// ---- assembler ----
static bool parse_reg(const std::string& s, uint32_t& type, uint32_t& num)
{
    if (s.size() < 2) return false;
    std::string p = s.substr(0, 1);
    size_t i = 1;
    if (s.rfind("oC", 0) == 0) { type = R_COLOROUT; i = 2; }
    else if (s.rfind("oDepth", 0) == 0) { type = R_DEPTHOUT; num = 0; return true; }
    else if (s[0] == 'r') type = R_TEMP;
    else if (s[0] == 'v') type = R_INPUT;
    else if (s[0] == 'c') type = R_CONST;
    else if (s[0] == 't') type = R_TEXTURE;
    else if (s[0] == 's') type = R_SAMPLER;
    else return false;
    if (i >= s.size() || !isdigit((unsigned char)s[i])) return false;
    num = (uint32_t)atoi(s.c_str() + i);
    return true;
}

static uint32_t parse_mask(const std::string& m)
{
    uint32_t w = 0;
    for (char c : m) {
        if (c == 'x' || c == 'r') w |= 1;
        else if (c == 'y' || c == 'g') w |= 2;
        else if (c == 'z' || c == 'b') w |= 4;
        else if (c == 'w' || c == 'a') w |= 8;
    }
    return w;
}

static uint32_t parse_swizzle(const std::string& m)
{
    if (m.empty()) return 0xE4;
    int c[4];
    for (int i = 0; i < 4; i++) {
        char ch = m[std::min<size_t>(i, m.size() - 1)];
        c[i] = (ch == 'x' || ch == 'r') ? 0 : (ch == 'y' || ch == 'g') ? 1 : (ch == 'z' || ch == 'b') ? 2 : 3;
    }
    return (uint32_t)(c[0] | (c[1] << 2) | (c[2] << 4) | (c[3] << 6));
}

static std::string trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

bool assemble_shader(const std::string& text_in, std::vector<DWORD>& tok, std::string& err)
{
    // strip comments
    std::string text;
    for (size_t i = 0; i < text_in.size(); i++) {
        if (text_in[i] == '/' && i + 1 < text_in.size() && text_in[i + 1] == '/') { while (i < text_in.size() && text_in[i] != '\n') i++; text += '\n'; continue; }
        if (text_in[i] == '/' && i + 1 < text_in.size() && text_in[i + 1] == '*') { i += 2; while (i + 1 < text_in.size() && !(text_in[i] == '*' && text_in[i + 1] == '/')) i++; i++; continue; }
        if (text_in[i] == ';') { while (i < text_in.size() && text_in[i] != '\n') i++; text += '\n'; continue; }
        text += text_in[i];
    }
    // statements: a version token may share a line with nothing else; instructions end at newlines
    std::vector<std::string> lines;
    size_t p = 0;
    while (p <= text.size()) {
        size_t e = text.find('\n', p);
        if (e == std::string::npos) e = text.size();
        std::string l = trim(text.substr(p, e - p));
        if (!l.empty()) lines.push_back(l);
        p = e + 1;
    }
    tok.clear();
    bool have_version = false, ps2 = false;
    for (std::string l : lines) {
        std::string low = l;
        for (char& c : low) c = (char)tolower((unsigned char)c);
        if (!have_version) {
            // ps.1.1 / ps_1_1 / ps_2_0
            if (low.size() >= 6 && low[0] == 'p' && low[1] == 's' && (low[2] == '.' || low[2] == '_')) {
                int maj = low[3] - '0', min = low[5] - '0';
                tok.push_back(0xFFFF0000u | (uint32_t)(maj << 8) | (uint32_t)min);
                ps2 = maj >= 2;
                have_version = true;
                l = trim(l.substr(6));
                low = trim(low.substr(6));
                if (l.empty()) continue;
            } else {
                err = "missing ps version: " + l;
                return false;
            }
        }
        bool coissue = false;
        if (low[0] == '+') { coissue = true; low = trim(low.substr(1)); }
        size_t sp = low.find_first_of(" \t");
        std::string mn = sp == std::string::npos ? low : low.substr(0, sp);
        std::string rest = sp == std::string::npos ? "" : trim(low.substr(sp));
        // instruction modifiers: _x2 _x4 _x8 _d2 _d4 _d8 _sat _pp; dcl_*
        uint32_t shift = 0, rmod = 0;
        std::string base = mn;
        std::string dcl_kind;
        if (mn.rfind("dcl", 0) == 0) { base = "dcl"; dcl_kind = mn.size() > 4 ? mn.substr(4) : ""; }
        else {
            size_t us;
            while ((us = base.rfind('_')) != std::string::npos && us > 0) {
                std::string m = base.substr(us + 1);
                if (m == "x2") shift = 1; else if (m == "x4") shift = 2; else if (m == "x8") shift = 3;
                else if (m == "d2") shift = 15; else if (m == "d4") shift = 14; else if (m == "d8") shift = 13;
                else if (m == "sat") rmod |= 1; else if (m == "pp") rmod |= 2;
                else break;
                base = base.substr(0, us);
            }
        }
        // operands
        std::vector<std::string> ops;
        {
            std::string cur;
            for (char c : rest) {
                if (c == ',') { ops.push_back(trim(cur)); cur.clear(); }
                else cur += c;
            }
            if (!trim(cur).empty()) ops.push_back(trim(cur));
        }
        if (base == "def") {
            if (ops.size() != 5) { err = "def needs 5 operands: " + l; return false; }
            uint32_t t, n;
            if (!parse_reg(ops[0], t, n)) { err = "bad register " + ops[0]; return false; }
            tok.push_back(OP_DEF | (ps2 ? (5u << 24) : 0));
            tok.push_back(regtok(t, n) | (0xFu << 16));
            for (int i = 1; i < 5; i++) { float f = (float)atof(ops[i].c_str()); uint32_t u; memcpy(&u, &f, 4); tok.push_back(u); }
            continue;
        }
        if (base == "dcl") {
            if (ops.size() != 1) { err = "dcl operand: " + l; return false; }
            std::string r = ops[0];
            size_t dot = r.find('.');
            std::string rn = dot == std::string::npos ? r : r.substr(0, dot);
            uint32_t t, n;
            if (!parse_reg(rn, t, n)) { err = "bad register " + rn; return false; }
            uint32_t usage = 0x80000000u;
            if (t == R_SAMPLER) usage |= (dcl_kind == "cube" ? 3u : dcl_kind == "volume" ? 4u : 2u) << 27;
            tok.push_back(OP_DCL | (2u << 24));
            tok.push_back(usage);
            tok.push_back(regtok(t, n) | ((dot == std::string::npos ? 0xFu : parse_mask(r.substr(dot + 1))) << 16));
            continue;
        }
        const OpInfo* oi = op_by_name(base);
        if (!oi) { err = "unknown instruction " + mn; return false; }
        if (base == "texld" || (base == "tex" && ps2)) oi = op_by_name("texld");
        size_t want = (oi->has_dst ? 1 : 0) + (size_t)oi->nsrc;
        if (ops.size() != want) { err = "operand count: " + l; return false; }
        size_t len_at = tok.size();
        tok.push_back(oi->op | (coissue ? 0x40000000u : 0));
        size_t o = 0;
        if (oi->has_dst) {
            std::string d = ops[o++];
            size_t dot = d.find('.');
            uint32_t t, n;
            if (!parse_reg(dot == std::string::npos ? d : d.substr(0, dot), t, n)) { err = "bad register " + d; return false; }
            uint32_t mask = dot == std::string::npos ? 0xF : parse_mask(d.substr(dot + 1));
            tok.push_back(regtok(t, n) | (mask << 16) | (rmod << 20) | (shift << 24));
        }
        for (int s = 0; s < oi->nsrc; s++) {
            std::string x = ops[o++];
            uint32_t smod = 0;
            bool neg = false, comp = false;
            if (x.rfind("1-", 0) == 0) { comp = true; x = trim(x.substr(2)); }
            if (!x.empty() && x[0] == '-') { neg = true; x = trim(x.substr(1)); }
            std::string sw;
            size_t dot = x.find('.');
            std::string rn = dot == std::string::npos ? x : x.substr(0, dot);
            if (dot != std::string::npos) sw = x.substr(dot + 1);
            std::string sfx;
            size_t us = rn.find('_');
            if (us != std::string::npos) { sfx = rn.substr(us + 1); rn = rn.substr(0, us); }
            size_t us2 = sw.find('_');
            if (us2 != std::string::npos) { sfx = sw.substr(us2 + 1); sw = sw.substr(0, us2); }
            if (comp) smod = 6;
            else if (sfx == "bias") smod = neg ? 3 : 2;
            else if (sfx == "bx2") smod = neg ? 5 : 4;
            else if (sfx == "x2") smod = neg ? 8 : 7;
            else if (sfx == "dz" || sfx == "db") smod = 9;
            else if (sfx == "dw" || sfx == "da") smod = 10;
            else if (sfx == "abs") smod = neg ? 12 : 11;
            else if (neg) smod = 1;
            uint32_t t, n;
            if (!parse_reg(rn, t, n)) { err = "bad register " + rn; return false; }
            tok.push_back(regtok(t, n) | (parse_swizzle(sw) << 16) | (smod << 24));
        }
        if (ps2) tok[len_at] |= (uint32_t)(tok.size() - len_at - 1) << 24;
    }
    if (!have_version) { err = "empty shader"; return false; }
    tok.push_back(OP_END);
    return true;
}

// ---- translator ----
static std::string reg_name(uint32_t type, uint32_t num, bool ps2)
{
    switch (type) {
    case R_TEMP: return "r" + std::to_string(num);
    case R_INPUT: return "v" + std::to_string(num);
    case R_CONST: return "c[" + std::to_string(num) + "]";
    case R_TEXTURE: return (ps2 ? "tc" : "t") + std::to_string(num);
    case R_COLOROUT: return "oC" + std::to_string(num);
    case R_DEPTHOUT: return "oDepth";
    default: return "r0";
    }
}

bool translate_ps_tokens(const DWORD* t, std::string& out, std::string& err, uint32_t* inputs)
{
    uint32_t ver = t[0];
    if ((ver & 0xFFFF0000u) != 0xFFFF0000u) { err = "not a pixel shader"; return false; }
    int major = (ver >> 8) & 0xff;
    bool ps2 = major >= 2;
    std::string body;
    std::vector<std::string> defs;
    uint32_t samplers = 0;
    std::vector<std::string> pending;   // writes held back while a co-issued pair executes
    const DWORD* p = t + 1;
    int tmpn = 0;
    while (*p != OP_END) {
        uint32_t ins = *p;
        uint32_t op = ins & 0xffff;
        if (op == OP_COMMENT) { p += 1 + ((ins >> 16) & 0x7fff); continue; }
        if (op == OP_PHASE) { p++; continue; }
        if (op == OP_DEF) {
            uint32_t d = p[1];
            uint32_t num = d & 0x7ff;
            float f[4];
            memcpy(f, p + 2, 16);
            char buf[160];
            snprintf(buf, sizeof buf, "    c[%u] = float4(%.9g, %.9g, %.9g, %.9g);\n", num, f[0], f[1], f[2], f[3]);
            defs.push_back(buf);
            p += 6;
            continue;
        }
        if (op == OP_DCL) {
            uint32_t d = p[2];
            uint32_t type = ((d >> 28) & 7) | ((d >> 8) & 0x18);
            if (type == R_SAMPLER) samplers |= 1u << (d & 0x7ff);
            p += 3;
            continue;
        }
        const OpInfo* oi = op_by_code(op, ps2);
        if (!oi) { char b[64]; snprintf(b, sizeof b, "unsupported opcode %u", op); err = b; return false; }
        bool coissue = (ins & 0x40000000u) != 0;
        const DWORD* a = p + 1;
        uint32_t dst = oi->has_dst ? *a++ : 0;
        uint32_t dtype = ((dst >> 28) & 7) | ((dst >> 8) & 0x18), dnum = dst & 0x7ff;
        uint32_t mask = (dst >> 16) & 0xf, rmod = (dst >> 20) & 0xf, shift = (dst >> 24) & 0xf;
        std::string src[3];
        for (int s = 0; s < oi->nsrc; s++) {
            uint32_t x = *a++;
            uint32_t type = ((x >> 28) & 7) | ((x >> 8) & 0x18), num = x & 0x7ff;
            uint32_t sw = (x >> 16) & 0xff, mod = (x >> 24) & 0xf;
            if (type == R_SAMPLER) { src[s] = "s" + std::to_string(num); samplers |= 1u << num; continue; }
            if (type == R_INPUT && inputs) *inputs |= 1u << (num == 0 ? VAR_COLOR0 : VAR_COLOR1);
            if (type == R_TEXTURE && ps2 && inputs) *inputs |= 1u << (VAR_TEX0 + num);
            std::string r = reg_name(type, num, ps2);
            if (sw != 0xE4) {
                static const char c[4] = {'x', 'y', 'z', 'w'};
                r += '.';
                for (int i = 0; i < 4; i++) r += c[(sw >> (2 * i)) & 3];
            }
            switch (mod) {
            case 1: r = "(-" + r + ")"; break;
            case 2: r = "(" + r + " - 0.5)"; break;
            case 3: r = "(0.5 - " + r + ")"; break;
            case 4: r = "(2.0 * " + r + " - 1.0)"; break;
            case 5: r = "(1.0 - 2.0 * " + r + ")"; break;
            case 6: r = "(1.0 - " + r + ")"; break;
            case 7: r = "(2.0 * " + r + ")"; break;
            case 8: r = "(-2.0 * " + r + ")"; break;
            case 9: r = "(" + r + " / " + r + ".zzzz)"; break;
            case 10: r = "(" + r + " / " + r + ".wwww)"; break;
            case 11: r = "abs(" + r + ")"; break;
            case 12: r = "(-abs(" + r + "))"; break;
            }
            src[s] = r;
        }
        std::string e;
        std::string dn = reg_name(dtype, dnum, ps2);
        switch (oi->op) {
        case OP_NOP: e.clear(); break;
        case OP_MOV: e = src[0]; break;
        case OP_ADD: e = src[0] + " + " + src[1]; break;
        case OP_SUB: e = src[0] + " - " + src[1]; break;
        case OP_MUL: e = src[0] + " * " + src[1]; break;
        case OP_MAD: e = src[0] + " * " + src[1] + " + " + src[2]; break;
        case OP_LRP: e = "lerp(" + src[2] + ", " + src[1] + ", " + src[0] + ")"; break;
        case OP_DP3: e = "dot((" + src[0] + ").xyz, (" + src[1] + ").xyz).xxxx"; break;
        case OP_DP4: e = "dot(" + src[0] + ", " + src[1] + ").xxxx"; break;
        case OP_DP2ADD: e = "(dot((" + src[0] + ").xy, (" + src[1] + ").xy) + (" + src[2] + ").x).xxxx"; break;
        case OP_MIN: e = "min(" + src[0] + ", " + src[1] + ")"; break;
        case OP_MAX: e = "max(" + src[0] + ", " + src[1] + ")"; break;
        case OP_SLT: e = "float4((" + src[0] + ") < (" + src[1] + "))"; break;
        case OP_SGE: e = "float4((" + src[0] + ") >= (" + src[1] + "))"; break;
        case OP_RCP: e = "(1.0 / (" + src[0] + ").w).xxxx"; break;
        case OP_RSQ: e = "rsqrt(abs((" + src[0] + ").w)).xxxx"; break;
        case OP_EXP: e = "exp2((" + src[0] + ").w).xxxx"; break;
        case OP_LOG: e = "log2(abs((" + src[0] + ").w)).xxxx"; break;
        case OP_FRC: e = "frac(" + src[0] + ")"; break;
        case OP_POW: e = "pow(abs((" + src[0] + ").w), (" + src[1] + ").w).xxxx"; break;
        case OP_ABS: e = "abs(" + src[0] + ")"; break;
        case OP_NRM: e = "float4(normalize((" + src[0] + ").xyz), 1)"; break;
        case OP_CND:
            e = ps2 || major == 1 && (ver & 0xff) >= 4 ? "((" + src[0] + ") > 0.5 ? (" + src[1] + ") : (" + src[2] + "))"
                                                         : "((" + src[0] + ").wwww > 0.5 ? (" + src[1] + ") : (" + src[2] + "))";
            break;
        case OP_CMP: e = "((" + src[0] + ") >= 0.0 ? (" + src[1] + ") : (" + src[2] + "))"; break;
        case OP_TEXCOORD:
            if (oi->nsrc == 0) {
                e = "saturate(i.t" + std::to_string(dnum) + ")";
                if (inputs) *inputs |= 1u << (VAR_TEX0 + dnum);
            } else e = src[0];
            break;
        case OP_TEXKILL:
            body += "    if (any((" + dn + ").xyz < 0.0)) discard;\n";
            p = a;
            continue;
        case OP_TEX:
            if (oi->nsrc == 0) {      // ps_1_x: tex tN samples stage N at texture coordinate N
                samplers |= 1u << dnum;
                if (inputs) *inputs |= 1u << (VAR_TEX0 + dnum);
                e = "tex2D(s" + std::to_string(dnum) + ", i.t" + std::to_string(dnum) + ".xy)";
            } else {
                e = "tex2D(" + src[1] + ", (" + src[0] + ").xy)";
            }
            break;
        }
        if (!e.empty()) {
            if (shift == 1) e = "(" + e + ") * 2.0";
            else if (shift == 2) e = "(" + e + ") * 4.0";
            else if (shift == 3) e = "(" + e + ") * 8.0";
            else if (shift == 15) e = "(" + e + ") * 0.5";
            else if (shift == 14) e = "(" + e + ") * 0.25";
            else if (shift == 13) e = "(" + e + ") * 0.125";
            if (rmod & 1) e = "saturate(" + e + ")";
            else if (!ps2) e = "clamp(" + e + ", -8.0, 8.0)";
            std::string tn = "_x" + std::to_string(tmpn++);
            body += "    float4 " + tn + " = " + e + ";\n";
            std::string w;
            if (mask == 0xf) w = "    " + dn + " = " + tn + ";\n";
            else {
                static const char c[4] = {'x', 'y', 'z', 'w'};
                std::string m;
                for (int i = 0; i < 4; i++) if (mask & (1u << i)) m += c[i];
                w = "    " + dn + "." + m + " = " + tn + "." + m + ";\n";
            }
            // a co-issued instruction reads the sources before its partner's write lands
            bool next_coissue = (a[0] & 0xffff) != OP_END && (a[0] & 0x40000000u) != 0;
            pending.push_back(w);
            if (!next_coissue || coissue) {
                if (!next_coissue) {
                    for (const std::string& x : pending) body += x;
                    pending.clear();
                }
            }
        }
        p = a;
    }
    for (const std::string& x : pending) body += x;
    std::string s = "#define MBP_PIXEL 1\n";
    s += k_src_mbp_common;
    s += "[[vk::binding(0, 3)]] cbuffer AsmPsConsts { float4 c_in[32]; };\n";
    for (int i = 0; i < 16; i++)
        if (samplers & (1u << i)) s += "[[vk::binding(" + std::to_string(i) + ", 2)]] sampler2D s" + std::to_string(i) + ";\n";
    s += "float4 mbp_ps(MBP_VSOUT i) : SV_Target0\n{\n";
    s += "    float4 c[32];\n    [unroll] for (int k = 0; k < 32; k++) c[k] = c_in[k];\n";
    for (const std::string& d : defs) s += d;
    s += "    float4 r0 = float4(0, 0, 0, 0), r1 = r0, r2 = r0, r3 = r0, r4 = r0, r5 = r0, r6 = r0, r7 = r0;\n";
    s += "    float4 v0 = i.v0, v1 = i.v1;\n";
    if (ps2) s += "    float4 tc0 = i.t0, tc1 = i.t1, tc2 = i.t2, tc3 = i.t3, tc4 = i.t4, tc5 = i.t5, tc6 = i.t6, tc7 = i.t7;\n    float4 oC0 = float4(0, 0, 0, 0);\n";
    else s += "    float4 t0 = float4(0, 0, 0, 0), t1 = t0, t2 = t0, t3 = t0, t4 = t0, t5 = t0;\n";
    s += body;
    s += ps2 ? "    return mbp_ps_finish(oC0, i);\n}\n" : "    return mbp_ps_finish(r0, i);\n}\n";
    out = s;
    return true;
}

} // namespace mbg

using namespace mbg;

extern "C" HRESULT WINAPI D3DXAssembleShaderFromFileA(LPCSTR file, CONST D3DXMACRO*, LPD3DXINCLUDE, DWORD, LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrors)
{
    if (ppShader) *ppShader = nullptr;
    if (ppErrors) *ppErrors = nullptr;
    std::vector<uint8_t> data;
    if (!file || !read_file(file, data)) {
        if (ppErrors) { const char m[] = "cannot open shader file"; *ppErrors = make_buffer(m, sizeof m); }
        return D3DERR_INVALIDCALL;
    }
    std::vector<DWORD> tok;
    std::string err;
    if (!assemble_shader(std::string(data.begin(), data.end()), tok, err)) {
        log("D3DXAssembleShaderFromFileA %s: %s", file, err.c_str());
        if (ppErrors) *ppErrors = make_buffer(err.c_str(), err.size() + 1);
        return D3DERR_INVALIDCALL;
    }
    if (ppShader) *ppShader = make_buffer(tok.data(), tok.size() * 4);
    return D3D_OK;
}
