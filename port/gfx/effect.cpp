// port/gfx/effect.cpp: D3DXCreateEffectFromFileA and ID3DXEffect over SDL_GPU.
//
// The .fx file (SM2 HLSL with techniques / passes / sampler_state blocks) is parsed here: techniques, texture
// parameters and sampler_state blocks are taken out (the effect framework implements them), the remaining HLSL is
// compiled per pass by glslang with an entry point generated around the pass's `compile vs_2_0 f(args)` function:
//   - vertex inputs are matched to the vertex declaration by semantic (one compiled variant per declaration);
//   - outputs go to the shared varying block (shaders/mbp_common.hlsl), so a pass's pixel shader reads them by semantic;
//   - the pixel entry adds what D3D9 does after a pixel shader: user clip plane, alpha test, fog blend.
// Parameters keep D3DX's packed values; on BeginPass / CommitChanges they are written into the $Global uniform block
// at the offsets SPIRV-Cross reflects (array strides, matrix layout), and textures / sampler states go to the device
// stages bound to the pixel shader's samplers.
#include "gfx_internal.h"
#include "gfx_shader_sources.h"
#include <cctype>
#include <deque>

namespace mbg {

ID3DXBuffer* make_buffer(const void* p, size_t n);
std::string decl_hlsl_type(BYTE t);
std::string decl_to_float4(BYTE t, const std::string& v);

// ---------------------------------------------------------------------------------------------------------------------
// text helpers
static std::string trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
static bool is_ident(char c) { return isalnum((unsigned char)c) || c == '_'; }
static std::string lower(std::string s) { for (char& c : s) c = (char)tolower((unsigned char)c); return s; }
static std::string upper(std::string s) { for (char& c : s) c = (char)toupper((unsigned char)c); return s; }

static std::string strip_comments(const std::string& in)
{
    std::string o;
    o.reserve(in.size());
    for (size_t i = 0; i < in.size(); i++) {
        if (in[i] == '/' && i + 1 < in.size() && in[i + 1] == '/') {
            while (i < in.size() && in[i] != '\n') i++;
            if (i < in.size()) o += '\n';
            continue;
        }
        if (in[i] == '/' && i + 1 < in.size() && in[i + 1] == '*') {
            i += 2;
            while (i + 1 < in.size() && !(in[i] == '*' && in[i + 1] == '/')) { if (in[i] == '\n') o += '\n'; i++; }
            i++;
            continue;
        }
        o += in[i];
    }
    return o;
}

// split at top-level commas
static std::vector<std::string> split_top(const std::string& s, char sep = ',')
{
    std::vector<std::string> out;
    int d = 0;
    std::string cur;
    for (char c : s) {
        if (c == '(' || c == '{' || c == '[' || c == '<') d++;
        if (c == ')' || c == '}' || c == ']' || c == '>') d--;
        if (c == sep && d == 0) { out.push_back(trim(cur)); cur.clear(); continue; }
        cur += c;
    }
    if (!trim(cur).empty()) out.push_back(trim(cur));
    return out;
}

// ---------------------------------------------------------------------------------------------------------------------
// constant expressions (parameter defaults, array sizes)
struct ConstEval {
    const std::map<std::string, std::string>* macros;
    std::string s;
    size_t p = 0;
    int depth = 0;
    void ws() { while (p < s.size() && isspace((unsigned char)s[p])) p++; }
    double primary()
    {
        ws();
        if (p >= s.size()) return 0;
        if (s[p] == '(') { p++; double v = expr(); ws(); if (p < s.size() && s[p] == ')') p++; return v; }
        if (s[p] == '-') { p++; return -primary(); }
        if (s[p] == '+') { p++; return primary(); }
        if (isdigit((unsigned char)s[p]) || s[p] == '.') {
            size_t q = p;
            double v = strtod(s.c_str() + p, nullptr);
            while (p < s.size() && (isalnum((unsigned char)s[p]) || s[p] == '.' || ((s[p] == '-' || s[p] == '+') && (s[p - 1] == 'e' || s[p - 1] == 'E')))) p++;
            (void)q;
            return v;
        }
        if (is_ident(s[p])) {
            size_t q = p;
            while (p < s.size() && is_ident(s[p])) p++;
            std::string id = s.substr(q, p - q);
            if (id == "true") return 1;
            if (id == "false") return 0;
            auto it = macros ? macros->find(id) : macros->end();
            if (macros && it != macros->end() && depth < 16) {
                ConstEval sub{macros, it->second};
                sub.depth = depth + 1;
                return sub.expr();
            }
            return 0;
        }
        p++;
        return 0;
    }
    double term()
    {
        double v = primary();
        for (;;) {
            ws();
            if (p < s.size() && s[p] == '*') { p++; v *= primary(); }
            else if (p < s.size() && s[p] == '/') { p++; double d = primary(); v = d != 0 ? v / d : 0; }
            else return v;
        }
    }
    double expr()
    {
        double v = term();
        for (;;) {
            ws();
            if (p < s.size() && s[p] == '+') { p++; v += term(); }
            else if (p < s.size() && s[p] == '-') { p++; v -= term(); }
            else return v;
        }
    }
};

static bool is_type_name(const std::string& id)
{
    static const char* base[] = {"float", "half", "int", "uint", "bool", "double", "min16float", "matrix", "vector"};
    for (const char* b : base)
        if (id.rfind(b, 0) == 0) return true;
    return false;
}

// flatten an initializer (`float4(a, b, c, d)`, `{a, b}`, `x / y`) into scalars
static void flatten_init(const std::string& s, const std::map<std::string, std::string>& macros, std::vector<double>& out)
{
    std::string t = trim(s);
    if (t.empty()) return;
    if (t[0] == '{' && t.back() == '}') {
        for (const std::string& e : split_top(t.substr(1, t.size() - 2))) flatten_init(e, macros, out);
        return;
    }
    size_t q = 0;
    while (q < t.size() && is_ident(t[q])) q++;
    if (q > 0 && q < t.size() && is_type_name(t.substr(0, q))) {
        size_t r = q;
        while (r < t.size() && isspace((unsigned char)t[r])) r++;
        if (r < t.size() && t[r] == '(' && t.back() == ')') {
            for (const std::string& e : split_top(t.substr(r + 1, t.size() - r - 2))) flatten_init(e, macros, out);
            return;
        }
    }
    ConstEval ev{&macros, t};
    out.push_back(ev.expr());
}

// ---------------------------------------------------------------------------------------------------------------------
// parsed effect
struct HType {          // a parsed HLSL type
    std::string name;   // as written (without qualifiers)
    int base = 0;       // 0 float, 1 int, 2 bool, 3 uint, -1 other
    int rows = 1, cols = 1;
    bool matrix = false, row_major = false;
};
static HType parse_type(const std::string& tn_in)
{
    HType t;
    std::string tn = tn_in;
    t.name = tn;
    if (tn == "matrix") tn = "float4x4";
    if (tn == "vector") tn = "float4";
    std::string b;
    size_t i = 0;
    while (i < tn.size() && isalpha((unsigned char)tn[i])) b += tn[i++];
    if (b == "float" || b == "half" || b == "double") t.base = 0;
    else if (b == "int") t.base = 1;
    else if (b == "bool") t.base = 2;
    else if (b == "uint" || b == "dword") t.base = 3;
    else { t.base = -1; return t; }
    std::string rest = tn.substr(i);
    if (rest.empty()) return t;
    if (rest.size() == 1 && isdigit((unsigned char)rest[0])) { t.cols = rest[0] - '0'; return t; }
    if (rest.size() == 3 && rest[1] == 'x') { t.rows = rest[0] - '0'; t.cols = rest[2] - '0'; t.matrix = true; }
    return t;
}
static int type_components(const std::string& tn)
{
    HType t = parse_type(tn);
    return t.rows * t.cols;
}

struct Decl {           // a variable / parameter / member declaration
    std::vector<std::string> quals;
    std::string type;
    std::string name;
    std::string array;  // text inside [] or empty
    std::string semantic;
    std::string init;
    bool has(const char* q) const { for (const std::string& s : quals) if (s == q) return true; return false; }
};

static bool parse_decl(const std::string& text, Decl& d)
{
    std::string s = trim(text);
    // initializer
    int depth = 0;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == '(' || c == '{' || c == '[' || c == '<') depth++;
        if (c == ')' || c == '}' || c == ']' || c == '>') depth--;
        if (c == '=' && depth == 0) { d.init = trim(s.substr(i + 1)); s = trim(s.substr(0, i)); break; }
    }
    // annotations < ... >
    size_t lt = s.find('<');
    if (lt != std::string::npos) {
        size_t gt = s.rfind('>');
        if (gt != std::string::npos && gt > lt) s = trim(s.substr(0, lt) + s.substr(gt + 1));
    }
    // semantic
    size_t colon = std::string::npos;
    depth = 0;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '(' || s[i] == '[') depth++;
        if (s[i] == ')' || s[i] == ']') depth--;
        if (s[i] == ':' && depth == 0) { colon = i; break; }
    }
    if (colon != std::string::npos) {
        d.semantic = trim(s.substr(colon + 1));
        s = trim(s.substr(0, colon));
    }
    // array
    size_t lb = s.find('[');
    if (lb != std::string::npos) {
        size_t rb = s.find(']', lb);
        d.array = trim(s.substr(lb + 1, rb - lb - 1));
        s = trim(s.substr(0, lb));
    }
    // tokens: quals... type name
    std::vector<std::string> toks;
    std::string cur;
    for (char c : s) {
        if (isspace((unsigned char)c)) { if (!cur.empty()) toks.push_back(cur); cur.clear(); }
        else cur += c;
    }
    if (!cur.empty()) toks.push_back(cur);
    if (toks.size() < 2) return false;
    d.name = toks.back();
    d.type = toks[toks.size() - 2];
    for (size_t i = 0; i + 2 < toks.size(); i++) d.quals.push_back(toks[i]);
    return true;
}

struct FxStruct { std::string name; std::vector<Decl> members; };
struct FxFunc { std::string name, ret, ret_sem; std::vector<Decl> params; };

struct FxParam {
    std::string name, semantic;
    enum Kind { Value, Texture, Sampler, String } kind = Value;
    HType t;
    int elems = 0;                       // array size (0 = scalar)
    std::vector<uint32_t> v;             // D3DX packed: max(1,elems) * rows * cols 4-byte values (float bits / int)
    IDirect3DBaseTexture9* tex = nullptr;
    bool dirty = true;
    std::string handle;                  // the D3DXHANDLE points at handle.c_str()
    size_t count() const { return (size_t)std::max(1, elems) * t.rows * t.cols; }
};
struct FxSampler {
    std::string name, texture;
    std::vector<std::pair<int, DWORD>> states;
};
struct FxPass;
struct FxTechnique;
class Effect;

struct UboImage {
    std::vector<UboMember> layout;
    std::unordered_map<std::string, int> idx;
    std::vector<uint8_t> data;
    UniformSource src{};
    uint64_t sig = 0;
};

// one compiled shader's view of a uniform image: same bytes, its own version (bumped only for the members it reads)
struct ShaderUbo {
    UboImage* img = nullptr;
    UniformSource src{};
    std::vector<uint8_t> reads;          // by parameter index
};

struct FxVS final : public VShader {
    Effect* fx = nullptr;
    FxPass* pass = nullptr;
    VsVariant* variant(const VertexDecl* decl) override;
};

struct FxPass {
    std::string name, handle;
    std::string vs_fn, ps_fn;
    std::vector<std::string> vs_args, ps_args;
    std::vector<std::pair<int, DWORD>> rstates;
    FxVS* vs = nullptr;
    PShader* ps = nullptr;
    bool ps_tried = false;
    std::vector<FxSampler*> samplers;    // by PS sampler binding
};
struct FxTechnique {
    std::string name, handle;
    std::deque<FxPass> passes;
};

class Effect final : public Com<ID3DXEffect> {
public:
    Device* dev = nullptr;
    std::string source;                  // the HLSL handed to glslang (techniques / textures / sampler states removed)
    std::map<std::string, std::string> macros;
    std::deque<FxParam> params;
    std::unordered_map<std::string, FxParam*> by_name;
    std::unordered_map<const void*, FxParam*> by_handle;
    std::deque<FxSampler> samplers;
    std::unordered_map<std::string, FxSampler*> sampler_by_name;
    std::map<std::string, FxStruct> structs;
    std::map<std::string, FxFunc> funcs;
    std::deque<FxTechnique> techs;
    std::unordered_map<const void*, FxTechnique*> tech_by_handle;
    std::unordered_map<const void*, FxPass*> pass_by_handle;
    std::vector<std::unique_ptr<UboImage>> images;
    std::deque<ShaderUbo> subs;
    FxTechnique* cur = nullptr;
    int cur_pass = -1;
    bool begun = false;
    IDirect3DStateBlock9* saved = nullptr;
    uint64_t ver = 1;

    ~Effect() override;
    bool parse(const std::string& text, std::string& err);
    FxParam* param(D3DXHANDLE h);
    FxTechnique* technique(D3DXHANDLE h);
    UboImage* image_for(const ShaderReflection& r);
    UniformSource* uniforms_for(const ShaderReflection& r);
    bool compile_ps(FxPass& p);
    std::string vs_source(FxPass& p, const VertexDecl* decl, std::vector<VsInput>& inputs, std::string& err);
    void write_param(UboImage& img, FxParam& p);
    void commit();
    void apply_samplers(FxPass& p);

    // ID3DXBaseEffect
    HRESULT STDMETHODCALLTYPE GetDesc(D3DXEFFECT_DESC*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetParameterDesc(D3DXHANDLE, D3DXPARAMETER_DESC*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetTechniqueDesc(D3DXHANDLE, D3DXTECHNIQUE_DESC*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetPassDesc(D3DXHANDLE, D3DXPASS_DESC*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetFunctionDesc(D3DXHANDLE, D3DXFUNCTION_DESC*) override { return E_NOTIMPL; }
    D3DXHANDLE STDMETHODCALLTYPE GetParameter(D3DXHANDLE parent, UINT i) override;
    D3DXHANDLE STDMETHODCALLTYPE GetParameterByName(D3DXHANDLE parent, LPCSTR name) override;
    D3DXHANDLE STDMETHODCALLTYPE GetParameterBySemantic(D3DXHANDLE parent, LPCSTR sem) override;
    D3DXHANDLE STDMETHODCALLTYPE GetParameterElement(D3DXHANDLE, UINT) override { return nullptr; }
    D3DXHANDLE STDMETHODCALLTYPE GetTechnique(UINT i) override { return i < techs.size() ? techs[i].handle.c_str() : nullptr; }
    D3DXHANDLE STDMETHODCALLTYPE GetTechniqueByName(LPCSTR name) override;
    D3DXHANDLE STDMETHODCALLTYPE GetPass(D3DXHANDLE t, UINT i) override;
    D3DXHANDLE STDMETHODCALLTYPE GetPassByName(D3DXHANDLE t, LPCSTR name) override;
    D3DXHANDLE STDMETHODCALLTYPE GetFunction(UINT) override { return nullptr; }
    D3DXHANDLE STDMETHODCALLTYPE GetFunctionByName(LPCSTR) override { return nullptr; }
    D3DXHANDLE STDMETHODCALLTYPE GetAnnotation(D3DXHANDLE, UINT) override { return nullptr; }
    D3DXHANDLE STDMETHODCALLTYPE GetAnnotationByName(D3DXHANDLE, LPCSTR) override { return nullptr; }
    HRESULT STDMETHODCALLTYPE SetValue(D3DXHANDLE h, LPCVOID data, UINT bytes) override;
    HRESULT STDMETHODCALLTYPE GetValue(D3DXHANDLE h, LPVOID data, UINT bytes) override;
    HRESULT STDMETHODCALLTYPE SetBool(D3DXHANDLE h, BOOL b) override { return SetInt(h, b ? 1 : 0); }
    HRESULT STDMETHODCALLTYPE GetBool(D3DXHANDLE h, BOOL* b) override { INT v = 0; HRESULT r = GetInt(h, &v); if (b) *b = v != 0; return r; }
    HRESULT STDMETHODCALLTYPE SetBoolArray(D3DXHANDLE h, CONST BOOL* b, UINT n) override { return SetIntArray(h, (const INT*)b, n); }
    HRESULT STDMETHODCALLTYPE GetBoolArray(D3DXHANDLE h, BOOL* b, UINT n) override { return GetIntArray(h, (INT*)b, n); }
    HRESULT STDMETHODCALLTYPE SetInt(D3DXHANDLE h, INT n) override { return SetIntArray(h, &n, 1); }
    HRESULT STDMETHODCALLTYPE GetInt(D3DXHANDLE h, INT* n) override { return GetIntArray(h, n, 1); }
    HRESULT STDMETHODCALLTYPE SetIntArray(D3DXHANDLE h, CONST INT* n, UINT c) override;
    HRESULT STDMETHODCALLTYPE GetIntArray(D3DXHANDLE h, INT* n, UINT c) override;
    HRESULT STDMETHODCALLTYPE SetFloat(D3DXHANDLE h, FLOAT f) override { return SetFloatArray(h, &f, 1); }
    HRESULT STDMETHODCALLTYPE GetFloat(D3DXHANDLE h, FLOAT* f) override { return GetFloatArray(h, f, 1); }
    HRESULT STDMETHODCALLTYPE SetFloatArray(D3DXHANDLE h, CONST FLOAT* f, UINT c) override;
    HRESULT STDMETHODCALLTYPE GetFloatArray(D3DXHANDLE h, FLOAT* f, UINT c) override;
    HRESULT STDMETHODCALLTYPE SetVector(D3DXHANDLE h, CONST D3DXVECTOR4* v) override { return SetVectorArray(h, v, 1); }
    HRESULT STDMETHODCALLTYPE GetVector(D3DXHANDLE h, D3DXVECTOR4* v) override { return GetVectorArray(h, v, 1); }
    HRESULT STDMETHODCALLTYPE SetVectorArray(D3DXHANDLE h, CONST D3DXVECTOR4* v, UINT c) override;
    HRESULT STDMETHODCALLTYPE GetVectorArray(D3DXHANDLE h, D3DXVECTOR4* v, UINT c) override;
    HRESULT STDMETHODCALLTYPE SetMatrix(D3DXHANDLE h, CONST D3DXMATRIX* m) override { return set_matrices(h, m, 1, false); }
    HRESULT STDMETHODCALLTYPE GetMatrix(D3DXHANDLE h, D3DXMATRIX* m) override { return get_matrices(h, m, 1, false); }
    HRESULT STDMETHODCALLTYPE SetMatrixArray(D3DXHANDLE h, CONST D3DXMATRIX* m, UINT c) override { return set_matrices(h, m, c, false); }
    HRESULT STDMETHODCALLTYPE GetMatrixArray(D3DXHANDLE h, D3DXMATRIX* m, UINT c) override { return get_matrices(h, m, c, false); }
    HRESULT STDMETHODCALLTYPE SetMatrixPointerArray(D3DXHANDLE h, CONST D3DXMATRIX** pm, UINT c) override;
    HRESULT STDMETHODCALLTYPE GetMatrixPointerArray(D3DXHANDLE h, D3DXMATRIX** pm, UINT c) override;
    HRESULT STDMETHODCALLTYPE SetMatrixTranspose(D3DXHANDLE h, CONST D3DXMATRIX* m) override { return set_matrices(h, m, 1, true); }
    HRESULT STDMETHODCALLTYPE GetMatrixTranspose(D3DXHANDLE h, D3DXMATRIX* m) override { return get_matrices(h, m, 1, true); }
    HRESULT STDMETHODCALLTYPE SetMatrixTransposeArray(D3DXHANDLE h, CONST D3DXMATRIX* m, UINT c) override { return set_matrices(h, m, c, true); }
    HRESULT STDMETHODCALLTYPE GetMatrixTransposeArray(D3DXHANDLE h, D3DXMATRIX* m, UINT c) override { return get_matrices(h, m, c, true); }
    HRESULT STDMETHODCALLTYPE SetMatrixTransposePointerArray(D3DXHANDLE h, CONST D3DXMATRIX** pm, UINT c) override;
    HRESULT STDMETHODCALLTYPE GetMatrixTransposePointerArray(D3DXHANDLE h, D3DXMATRIX** pm, UINT c) override;
    HRESULT STDMETHODCALLTYPE SetString(D3DXHANDLE, LPCSTR) override { return D3DERR_INVALIDCALL; }
    HRESULT STDMETHODCALLTYPE GetString(D3DXHANDLE, LPCSTR*) override { return D3DERR_INVALIDCALL; }
    HRESULT STDMETHODCALLTYPE SetTexture(D3DXHANDLE h, LPDIRECT3DBASETEXTURE9 t) override;
    HRESULT STDMETHODCALLTYPE GetTexture(D3DXHANDLE h, LPDIRECT3DBASETEXTURE9* t) override;
    HRESULT STDMETHODCALLTYPE GetPixelShader(D3DXHANDLE, LPDIRECT3DPIXELSHADER9*) override { return D3DERR_INVALIDCALL; }
    HRESULT STDMETHODCALLTYPE GetVertexShader(D3DXHANDLE, LPDIRECT3DVERTEXSHADER9*) override { return D3DERR_INVALIDCALL; }
    HRESULT STDMETHODCALLTYPE SetArrayRange(D3DXHANDLE, UINT, UINT) override { return D3D_OK; }
    // ID3DXEffect
    HRESULT STDMETHODCALLTYPE GetPool(LPD3DXEFFECTPOOL* p) override { if (p) *p = nullptr; return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetTechnique(D3DXHANDLE h) override;
    D3DXHANDLE STDMETHODCALLTYPE GetCurrentTechnique() override { return cur ? cur->handle.c_str() : nullptr; }
    HRESULT STDMETHODCALLTYPE ValidateTechnique(D3DXHANDLE h) override;
    HRESULT STDMETHODCALLTYPE FindNextValidTechnique(D3DXHANDLE h, D3DXHANDLE* out) override;
    BOOL STDMETHODCALLTYPE IsParameterUsed(D3DXHANDLE, D3DXHANDLE) override { return TRUE; }
    HRESULT STDMETHODCALLTYPE Begin(UINT* passes, DWORD flags) override;
    HRESULT STDMETHODCALLTYPE BeginPass(UINT pass) override;
    HRESULT STDMETHODCALLTYPE CommitChanges() override;
    HRESULT STDMETHODCALLTYPE EndPass() override;
    HRESULT STDMETHODCALLTYPE End() override;
    HRESULT STDMETHODCALLTYPE GetDevice(LPDIRECT3DDEVICE9* d) override { if (!d) return D3DERR_INVALIDCALL; *d = dev; dev->AddRef(); return D3D_OK; }
    HRESULT STDMETHODCALLTYPE OnLostDevice() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE OnResetDevice() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetStateManager(LPD3DXEFFECTSTATEMANAGER) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetStateManager(LPD3DXEFFECTSTATEMANAGER* p) override { if (p) *p = nullptr; return D3D_OK; }
    HRESULT STDMETHODCALLTYPE BeginParameterBlock() override { return E_NOTIMPL; }
    D3DXHANDLE STDMETHODCALLTYPE EndParameterBlock() override { return nullptr; }
    HRESULT STDMETHODCALLTYPE ApplyParameterBlock(D3DXHANDLE) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE DeleteParameterBlock(D3DXHANDLE) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CloneEffect(LPDIRECT3DDEVICE9, LPD3DXEFFECT*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetRawValue(D3DXHANDLE h, LPCVOID data, UINT off, UINT bytes) override;

private:
    HRESULT set_matrices(D3DXHANDLE h, const D3DXMATRIX* m, UINT n, bool transpose);
    HRESULT get_matrices(D3DXHANDLE h, D3DXMATRIX* m, UINT n, bool transpose);
};

// ---------------------------------------------------------------------------------------------------------------------
// state names (sampler_state blocks and pass assignments)
static bool state_value(const std::string& v_in, DWORD& out)
{
    std::string v = upper(trim(v_in));
    static const std::pair<const char*, DWORD> k[] = {
        {"TRUE", 1}, {"FALSE", 0}, {"NONE", 0}, {"POINT", D3DTEXF_POINT}, {"LINEAR", D3DTEXF_LINEAR}, {"ANISOTROPIC", D3DTEXF_ANISOTROPIC},
        {"WRAP", D3DTADDRESS_WRAP}, {"MIRROR", D3DTADDRESS_MIRROR}, {"CLAMP", D3DTADDRESS_CLAMP}, {"BORDER", D3DTADDRESS_BORDER},
        {"MIRRORONCE", D3DTADDRESS_MIRRORONCE}, {"ZERO", D3DBLEND_ZERO}, {"ONE", D3DBLEND_ONE}, {"SRCCOLOR", D3DBLEND_SRCCOLOR},
        {"INVSRCCOLOR", D3DBLEND_INVSRCCOLOR}, {"SRCALPHA", D3DBLEND_SRCALPHA}, {"INVSRCALPHA", D3DBLEND_INVSRCALPHA},
        {"DESTALPHA", D3DBLEND_DESTALPHA}, {"INVDESTALPHA", D3DBLEND_INVDESTALPHA}, {"DESTCOLOR", D3DBLEND_DESTCOLOR},
        {"INVDESTCOLOR", D3DBLEND_INVDESTCOLOR}, {"SRCALPHASAT", D3DBLEND_SRCALPHASAT}, {"CW", D3DCULL_CW}, {"CCW", D3DCULL_CCW},
        {"NEVER", D3DCMP_NEVER}, {"LESS", D3DCMP_LESS}, {"EQUAL", D3DCMP_EQUAL}, {"LESSEQUAL", D3DCMP_LESSEQUAL},
        {"GREATER", D3DCMP_GREATER}, {"NOTEQUAL", D3DCMP_NOTEQUAL}, {"GREATEREQUAL", D3DCMP_GREATEREQUAL}, {"ALWAYS", D3DCMP_ALWAYS},
        {"SOLID", D3DFILL_SOLID}, {"WIREFRAME", D3DFILL_WIREFRAME}, {"ADD", D3DBLENDOP_ADD}, {"SUBTRACT", D3DBLENDOP_SUBTRACT},
        {"REVSUBTRACT", D3DBLENDOP_REVSUBTRACT}, {"MIN", D3DBLENDOP_MIN}, {"MAX", D3DBLENDOP_MAX},
    };
    for (auto& e : k)
        if (v == e.first) { out = e.second; return true; }
    if (v.empty()) return false;
    if (v.find('.') != std::string::npos || v.find('E') != std::string::npos) {
        float f = (float)atof(v.c_str());
        memcpy(&out, &f, 4);
        return true;
    }
    out = (DWORD)strtoul(v.c_str(), nullptr, 0);
    return true;
}
static int sampler_state_id(const std::string& n)
{
    std::string s = lower(n);
    if (s == "addressu") return D3DSAMP_ADDRESSU;
    if (s == "addressv") return D3DSAMP_ADDRESSV;
    if (s == "addressw") return D3DSAMP_ADDRESSW;
    if (s == "bordercolor") return D3DSAMP_BORDERCOLOR;
    if (s == "magfilter") return D3DSAMP_MAGFILTER;
    if (s == "minfilter") return D3DSAMP_MINFILTER;
    if (s == "mipfilter") return D3DSAMP_MIPFILTER;
    if (s == "mipmaplodbias") return D3DSAMP_MIPMAPLODBIAS;
    if (s == "maxmiplevel") return D3DSAMP_MAXMIPLEVEL;
    if (s == "maxanisotropy") return D3DSAMP_MAXANISOTROPY;
    if (s == "srgbtexture") return D3DSAMP_SRGBTEXTURE;
    return -1;
}
static int render_state_id(const std::string& n)
{
    static const std::pair<const char*, int> k[] = {
        {"zenable", D3DRS_ZENABLE}, {"fillmode", D3DRS_FILLMODE}, {"shademode", D3DRS_SHADEMODE}, {"zwriteenable", D3DRS_ZWRITEENABLE},
        {"alphatestenable", D3DRS_ALPHATESTENABLE}, {"srcblend", D3DRS_SRCBLEND}, {"destblend", D3DRS_DESTBLEND}, {"cullmode", D3DRS_CULLMODE},
        {"zfunc", D3DRS_ZFUNC}, {"alpharef", D3DRS_ALPHAREF}, {"alphafunc", D3DRS_ALPHAFUNC}, {"alphablendenable", D3DRS_ALPHABLENDENABLE},
        {"fogenable", D3DRS_FOGENABLE}, {"specularenable", D3DRS_SPECULARENABLE}, {"fogcolor", D3DRS_FOGCOLOR}, {"stencilenable", D3DRS_STENCILENABLE},
        {"lighting", D3DRS_LIGHTING}, {"colorwriteenable", D3DRS_COLORWRITEENABLE}, {"blendop", D3DRS_BLENDOP}, {"texturefactor", D3DRS_TEXTUREFACTOR},
        {"depthbias", D3DRS_DEPTHBIAS}, {"slopescaledepthbias", D3DRS_SLOPESCALEDEPTHBIAS}, {"scissortestenable", D3DRS_SCISSORTESTENABLE},
    };
    std::string s = lower(n);
    for (auto& e : k)
        if (s == e.first) return e.second;
    return -1;
}

// ---------------------------------------------------------------------------------------------------------------------
// parsing
static size_t match_brace(const std::string& s, size_t open)
{
    int d = 0;
    for (size_t i = open; i < s.size(); i++) {
        if (s[i] == '{') d++;
        else if (s[i] == '}') { if (--d == 0) return i; }
    }
    return std::string::npos;
}

static void parse_compile(const std::string& v, std::string& fn, std::vector<std::string>& args)
{
    // "compile vs_2_0 vs_main(PCF_NONE, true)"
    std::string s = trim(v);
    if (s.rfind("compile", 0) == 0) {
        s = trim(s.substr(7));
        size_t sp = s.find_first_of(" \t");
        s = trim(s.substr(sp));
    }
    size_t lp = s.find('(');
    fn = trim(s.substr(0, lp));
    if (lp != std::string::npos) {
        size_t rp = s.rfind(')');
        args = split_top(s.substr(lp + 1, rp - lp - 1));
    }
}

bool Effect::parse(const std::string& text_in, std::string& err)
{
    std::string text = strip_comments(text_in);
    std::string out;
    size_t i = 0, n = text.size();
    while (i < n) {
        // whitespace
        if (isspace((unsigned char)text[i])) { out += text[i++]; continue; }
        // preprocessor line
        if (text[i] == '#') {
            size_t e = text.find('\n', i);
            if (e == std::string::npos) e = n;
            std::string line = text.substr(i, e - i);
            while (!line.empty() && line.back() == '\\' && e < n) {     // continuation
                size_t e2 = text.find('\n', e + 1);
                if (e2 == std::string::npos) e2 = n;
                line += "\n" + text.substr(e + 1, e2 - e - 1);
                e = e2;
            }
            std::string l = trim(line.substr(1));
            if (l.rfind("define", 0) == 0) {
                std::string r = trim(l.substr(6));
                size_t sp = 0;
                while (sp < r.size() && is_ident(r[sp])) sp++;
                if (sp < r.size() && r[sp] != '(') macros[r.substr(0, sp)] = trim(r.substr(sp));
                else if (sp == r.size()) macros[r] = "";
            }
            out += line;
            i = e;
            continue;
        }
        // one top-level statement: up to ';' at depth 0, or a '{...}' body not followed by ';' (function / technique)
        size_t s0 = i;
        int depth = 0;
        size_t body_open = std::string::npos, body_close = std::string::npos;
        size_t paren = std::string::npos;
        while (i < n) {
            char c = text[i];
            if (c == '(' && depth == 0 && paren == std::string::npos) paren = i;
            if (c == '{') {
                if (depth == 0 && body_open == std::string::npos) body_open = i;
                depth++;
            } else if (c == '}') {
                depth--;
                if (depth == 0 && body_close == std::string::npos) {
                    body_close = i;
                    // a function or technique ends here unless a ';' (struct / initializer list) follows
                    size_t k = i + 1;
                    while (k < n && isspace((unsigned char)text[k])) k++;
                    if (k >= n || text[k] != ';') {
                        std::string head = trim(text.substr(s0, body_open - s0));
                        bool is_init = head.find('=') != std::string::npos;
                        if (!is_init) { i++; break; }
                    }
                }
            } else if (c == ';' && depth == 0) {
                i++;
                break;
            }
            i++;
        }
        std::string stmt = text.substr(s0, i - s0);
        std::string head = body_open != std::string::npos ? trim(text.substr(s0, body_open - s0)) : trim(stmt);
        std::string first;
        {
            size_t k = 0;
            while (k < head.size() && is_ident(head[k])) k++;
            first = head.substr(0, k);
        }
        auto keep_lines = [&](const std::string& s) { for (char c : s) if (c == '\n') out += '\n'; };
        if (first == "technique" || first == "technique10") {
            FxTechnique t;
            std::string h = trim(head.substr(first.size()));
            size_t lt = h.find('<');
            t.name = trim(lt == std::string::npos ? h : h.substr(0, lt));
            // passes
            std::string b = text.substr(body_open + 1, body_close - body_open - 1);
            size_t p = 0;
            while ((p = b.find("pass", p)) != std::string::npos) {
                if ((p > 0 && is_ident(b[p - 1])) || (p + 4 < b.size() && is_ident(b[p + 4]))) { p += 4; continue; }
                size_t ob = b.find('{', p);
                if (ob == std::string::npos) break;
                size_t cb = match_brace(b, ob);
                if (cb == std::string::npos) break;
                FxPass ps;
                std::string ph = trim(b.substr(p + 4, ob - p - 4));
                size_t plt = ph.find('<');
                ps.name = trim(plt == std::string::npos ? ph : ph.substr(0, plt));
                for (const std::string& a : split_top(b.substr(ob + 1, cb - ob - 1), ';')) {
                    size_t eq = a.find('=');
                    if (eq == std::string::npos) continue;
                    std::string k = trim(a.substr(0, eq)), v = trim(a.substr(eq + 1));
                    std::string kl = lower(k);
                    if (kl == "vertexshader") parse_compile(v, ps.vs_fn, ps.vs_args);
                    else if (kl == "pixelshader") parse_compile(v, ps.ps_fn, ps.ps_args);
                    else {
                        int id = render_state_id(k);
                        DWORD val;
                        if (id >= 0 && state_value(v, val)) ps.rstates.push_back({id, val});
                        else if (verbose()) log("effect: pass state %s ignored", k.c_str());
                    }
                }
                t.passes.push_back(ps);
                p = cb + 1;
            }
            techs.push_back(std::move(t));
            keep_lines(stmt);
            continue;
        }
        if (first == "struct") {
            FxStruct st;
            std::string h = trim(head.substr(6));
            st.name = h;
            std::string b = text.substr(body_open + 1, body_close - body_open - 1);
            for (const std::string& m : split_top(b, ';')) {
                Decl d;
                if (parse_decl(m, d)) st.members.push_back(d);
            }
            structs[st.name] = st;
            out += stmt;
            continue;
        }
        if (stmt.find("sampler_state") != std::string::npos) {
            // sampler NAME = sampler_state { ... };
            std::string h = trim(head.substr(0, head.find('=')));
            Decl d;
            if (!parse_decl(h, d)) { out += stmt; continue; }
            FxSampler sm;
            sm.name = d.name;
            std::string b = text.substr(body_open + 1, body_close - body_open - 1);
            for (const std::string& a : split_top(b, ';')) {
                size_t eq = a.find('=');
                if (eq == std::string::npos) continue;
                std::string k = trim(a.substr(0, eq)), v = trim(a.substr(eq + 1));
                if (lower(k) == "texture") {
                    std::string t = v;
                    for (char& c : t) if (c == '<' || c == '>' || c == '(' || c == ')') c = ' ';
                    sm.texture = trim(t);
                    continue;
                }
                int id = sampler_state_id(k);
                DWORD val;
                if (id >= 0 && state_value(v, val)) sm.states.push_back({id, val});
            }
            samplers.push_back(sm);
            sampler_by_name[sm.name] = &samplers.back();
            std::string type = d.type == "sampler" ? "sampler2D" : d.type;
            out += type + " " + d.name + ";";
            keep_lines(stmt);
            FxParam p;
            p.name = d.name;
            p.kind = FxParam::Sampler;
            params.push_back(p);
            continue;
        }
        if (body_open != std::string::npos && paren != std::string::npos && paren < body_open) {
            // function definition
            FxFunc f;
            std::string sig = head;
            size_t lp = sig.find('(');
            size_t rp = sig.rfind(')');
            std::string pre = trim(sig.substr(0, lp));
            size_t sp = pre.find_last_of(" \t");
            f.name = trim(pre.substr(sp + 1));
            std::string rtoks = trim(pre.substr(0, sp));
            size_t rs = rtoks.find_last_of(" \t");
            f.ret = rs == std::string::npos ? rtoks : trim(rtoks.substr(rs + 1));
            std::string after = trim(sig.substr(rp + 1));
            if (!after.empty() && after[0] == ':') f.ret_sem = trim(after.substr(1));
            for (const std::string& a : split_top(sig.substr(lp + 1, rp - lp - 1))) {
                Decl d;
                if (parse_decl(a, d)) f.params.push_back(d);
            }
            funcs[f.name] = f;
            out += stmt;
            continue;
        }
        // declarations
        std::string decl = trim(stmt);
        if (!decl.empty() && decl.back() == ';') decl.pop_back();
        bool keep = true;
        if (first == "texture" || first == "texture2D" || first == "Texture" || first == "texture3D" || first == "textureCUBE" || first == "Texture2D") {
            for (const std::string& part : split_top(decl.substr(first.size()))) {
                Decl d;
                if (!parse_decl("texture " + part, d)) continue;
                FxParam p;
                p.name = d.name;
                p.kind = FxParam::Texture;
                params.push_back(p);
            }
            keep = false;
        } else if (first == "string") {
            keep = false;
        } else if (!decl.empty()) {
            // `type a [: s] [= init], b ...`: the first declarator carries the qualifiers and type
            std::vector<std::string> parts = split_top(decl);
            Decl d0;
            if (!parts.empty() && parse_decl(parts[0], d0) && !d0.has("static")) {
                std::string prefix;
                for (const std::string& q : d0.quals) prefix += q + " ";
                prefix += d0.type + " ";
                for (size_t k = 0; k < parts.size(); k++) {
                    Decl d;
                    if (!parse_decl(k == 0 ? parts[0] : prefix + parts[k], d)) continue;
                    FxParam p;
                    p.name = d.name;
                    p.semantic = d.semantic;
                    p.t = parse_type(d.type);
                    if (d.has("row_major")) p.t.row_major = true;
                    if (p.t.base < 0) continue;
                    if (!d.array.empty()) { ConstEval ev{&macros, d.array}; p.elems = (int)ev.expr(); }
                    p.v.assign(p.count(), 0);
                    if (!d.init.empty()) {
                        std::vector<double> vals;
                        flatten_init(d.init, macros, vals);
                        for (size_t q = 0; q < p.v.size() && q < vals.size(); q++) {
                            if (p.t.base == 0) { float f = (float)vals[q]; memcpy(&p.v[q], &f, 4); }
                            else p.v[q] = (uint32_t)(int32_t)vals[q];
                        }
                        if (vals.size() == 1 && p.v.size() > 1)       // scalar initialiser of a vector: replicate
                            for (size_t q = 1; q < p.v.size(); q++) p.v[q] = p.v[0];
                    }
                    params.push_back(p);
                }
            }
        }
        if (keep) {
            // drop annotations, keep the rest for glslang
            std::string s = stmt;
            size_t lt;
            while ((lt = s.find('<')) != std::string::npos) {
                size_t eq = s.find('=');
                size_t gt = s.find('>', lt);
                if (gt == std::string::npos || (eq != std::string::npos && eq < lt)) break;
                std::string inside = s.substr(lt + 1, gt - lt - 1);
                if (inside.find(';') == std::string::npos && inside.find('"') == std::string::npos) break;
                s = s.substr(0, lt) + s.substr(gt + 1);
            }
            out += s;
        } else {
            keep_lines(stmt);
        }
    }
    source = out;
    for (FxParam& p : params) {
        p.handle = p.name;
        by_name[p.name] = &p;
        by_handle[p.handle.c_str()] = &p;
    }
    for (FxTechnique& t : techs) {
        t.handle = t.name;
        tech_by_handle[t.handle.c_str()] = &t;
        for (FxPass& p : t.passes) {
            p.handle = p.name;
            pass_by_handle[p.handle.c_str()] = &p;
        }
    }
    if (techs.empty()) { err = "effect has no techniques"; return false; }
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// semantics
static bool split_semantic(const std::string& sem_in, std::string& base, int& index)
{
    std::string s = upper(trim(sem_in));
    size_t k = s.size();
    while (k > 0 && isdigit((unsigned char)s[k - 1])) k--;
    base = s.substr(0, k);
    index = k < s.size() ? atoi(s.c_str() + k) : 0;
    return !base.empty();
}

int varying_for_semantic(const std::string& sem)
{
    std::string b;
    int i;
    if (!split_semantic(sem, b, i)) return -1;
    if (b == "COLOR" || b == "SV_TARGET") return i == 0 ? VAR_COLOR0 : i == 1 ? VAR_COLOR1 : -1;
    if (b == "TEXCOORD" && i < 8) return VAR_TEX0 + i;
    if (b == "FOG") return VAR_FOG;
    return -1;
}
static const char* varying_member(int v)
{
    static const char* names[] = {"v0", "v1", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "fog", "clipd"};
    return v >= 0 && v < VAR_COUNT ? names[v] : "v0";
}
static bool usage_for_semantic(const std::string& sem, BYTE& usage, BYTE& index)
{
    std::string b;
    int i;
    if (!split_semantic(sem, b, i)) return false;
    index = (BYTE)i;
    if (b == "POSITION" || b == "SV_POSITION") usage = D3DDECLUSAGE_POSITION;
    else if (b == "POSITIONT") usage = D3DDECLUSAGE_POSITIONT;
    else if (b == "BLENDWEIGHT") usage = D3DDECLUSAGE_BLENDWEIGHT;
    else if (b == "BLENDINDICES") usage = D3DDECLUSAGE_BLENDINDICES;
    else if (b == "NORMAL") usage = D3DDECLUSAGE_NORMAL;
    else if (b == "PSIZE") usage = D3DDECLUSAGE_PSIZE;
    else if (b == "TEXCOORD") usage = D3DDECLUSAGE_TEXCOORD;
    else if (b == "TANGENT") usage = D3DDECLUSAGE_TANGENT;
    else if (b == "BINORMAL") usage = D3DDECLUSAGE_BINORMAL;
    else if (b == "COLOR") usage = D3DDECLUSAGE_COLOR;
    else if (b == "FOG") usage = D3DDECLUSAGE_FOG;
    else if (b == "DEPTH") usage = D3DDECLUSAGE_DEPTH;
    else return false;
    return true;
}

// float4 expression `x` narrowed / converted to HLSL type `type`
static std::string from4(const std::string& type, const std::string& x)
{
    int n = type_components(type);
    const char* sw = n == 1 ? ".x" : n == 2 ? ".xy" : n == 3 ? ".xyz" : "";
    return "((" + type + ")(" + x + ")" + sw + ")";
}
static std::string from4_ok(const std::string& type, const std::string& x)
{
    int n = type_components(type);
    if (n >= 16) return "(" + type + ")0";
    std::string sw = n == 1 ? ".x" : n == 2 ? ".xy" : n == 3 ? ".xyz" : "";
    return "(" + type + ")((" + x + ")" + sw + ")";
}
static std::string to4(const std::string& type, const std::string& x)
{
    int n = type_components(type);
    if (n == 1) return "float4(" + x + ", 0, 0, 0)";
    if (n == 2) return "float4(" + x + ", 0, 0)";
    if (n == 3) return "float4(" + x + ", 0)";
    return "float4(" + x + ")";
}


static const char* k_hdr_vs = "#define MBP_VERTEX 1\n";
static const char* k_hdr_ps = "#define MBP_PIXEL 1\n";

std::string Effect::vs_source(FxPass& p, const VertexDecl* decl, std::vector<VsInput>& inputs, std::string& err)
{
    auto fi = funcs.find(p.vs_fn);
    if (fi == funcs.end()) { err = "vertex function " + p.vs_fn + " not found"; return ""; }
    const FxFunc& f = fi->second;
    std::string params_s, pre, args;
    uint32_t loc = 0;
    size_t ua = 0;
    auto input_expr = [&](const std::string& type, const std::string& sem) -> std::string {
        BYTE usage = 0, idx = 0;
        const D3DVERTEXELEMENT9* e = nullptr;
        if (usage_for_semantic(sem, usage, idx)) {
            e = decl->find(usage, idx);
            if (!e && usage == D3DDECLUSAGE_POSITION) e = decl->find(D3DDECLUSAGE_POSITIONT, idx);
        }
        if (!e) return from4_ok(type, "float4(0, 0, 0, 1)");
        for (const VsInput& in : inputs)
            if (in.usage == e->Usage && in.usage_index == e->UsageIndex) return from4_ok(type, decl_to_float4(e->Type, "a" + std::to_string(in.location)));
        std::string an = "a" + std::to_string(loc);
        if (!params_s.empty()) params_s += ", ";
        params_s += "[[vk::location(" + std::to_string(loc) + ")]] " + decl_hlsl_type(e->Type) + " " + an + " : MBPA" + std::to_string(loc);
        inputs.push_back(VsInput{loc, e->Stream, e->Offset, e->Type, e->Usage, e->UsageIndex});
        loc++;
        return from4_ok(type, decl_to_float4(e->Type, an));
    };
    std::string post;
    auto assign = [&](const std::string& expr, const std::string& type, const std::string& sem) {
        std::string b;
        int idx;
        if (!split_semantic(sem, b, idx)) return;
        if (b == "POSITION" || b == "SV_POSITION") post += "    o.pos = " + to4(type, expr) + ";\n";
        else if (b == "COLOR") { if (idx < 2) post += "    o.v" + std::to_string(idx) + " = saturate(" + to4(type, expr) + ");\n"; }
        else if (b == "TEXCOORD" && idx < 8) post += "    o.t" + std::to_string(idx) + " = " + to4(type, expr) + ";\n";
        else if (b == "FOG") post += "    o.fog = (" + expr + ")" + (type_components(type) > 1 ? ".x" : "") + ";\n";
    };
    int pi = 0;
    for (const Decl& d : f.params) {
        std::string a;
        if (d.has("uniform")) {
            a = ua < p.vs_args.size() ? p.vs_args[ua] : (!d.init.empty() ? d.init : "0");
            ua++;
        } else if (d.has("out") || d.has("inout")) {
            std::string ln = "o_" + std::to_string(pi);
            pre += "    " + d.type + " " + ln + " = (" + d.type + ")0;\n";
            if (structs.count(d.type)) for (const Decl& m : structs[d.type].members) assign(ln + "." + m.name, m.type, m.semantic);
            else assign(ln, d.type, d.semantic);
            a = ln;
        } else if (structs.count(d.type)) {
            std::string ln = "i_" + std::to_string(pi);
            pre += "    " + d.type + " " + ln + ";\n";
            for (const Decl& m : structs[d.type].members) pre += "    " + ln + "." + m.name + " = " + input_expr(m.type, m.semantic) + ";\n";
            a = ln;
        } else {
            a = input_expr(d.type, d.semantic);
        }
        if (!args.empty()) args += ", ";
        args += a;
        pi++;
    }
    std::string call;
    if (structs.count(f.ret)) {
        call = "    " + f.ret + " r = " + f.name + "(" + args + ");\n";
        for (const Decl& m : structs[f.ret].members) assign("r." + m.name, m.type, m.semantic);
    } else if (f.ret != "void") {
        call = "    " + f.ret + " r = " + f.name + "(" + args + ");\n";
        assign("r", f.ret, f.ret_sem.empty() ? "POSITION" : f.ret_sem);
    } else {
        call = "    " + f.name + "(" + args + ");\n";
    }
    std::string s = k_hdr_vs;
    s += source;
    s += "\n";
    s += k_src_mbp_common;
    s += "MBP_VSOUT mbp_vs(" + params_s + ")\n{\n" + pre + call;
    s += "    MBP_VSOUT o = (MBP_VSOUT)0;\n    o.fog = 1.0;\n" + post + "    mbp_vs_finish(o);\n    return o;\n}\n";
    return s;
}

static std::string ps_source(Effect& fx, FxPass& p, std::string& err)
{
    auto fi = fx.funcs.find(p.ps_fn);
    if (fi == fx.funcs.end()) { err = "pixel function " + p.ps_fn + " not found"; return ""; }
    const FxFunc& f = fi->second;
    auto vary = [&](const std::string& type, const std::string& sem) -> std::string {
        std::string b;
        int idx;
        if (split_semantic(sem, b, idx) && (b == "VPOS" || b == "SV_POSITION" || b == "POSITION")) return from4_ok(type, "i.pos");
        int v = varying_for_semantic(sem);
        if (v < 0) return from4_ok(type, "float4(0, 0, 0, 0)");
        if (v == VAR_FOG) return from4_ok(type, "float4(i.fog, 0, 0, 0)");
        return from4_ok(type, std::string("i.") + varying_member(v));
    };
    std::string pre, args;
    size_t ua = 0;
    int pi = 0;
    std::string color = "float4(0, 0, 0, 1)", depth;
    std::vector<std::pair<std::string, const Decl*>> outs;
    for (const Decl& d : f.params) {
        std::string a;
        if (d.has("uniform")) {
            a = ua < p.ps_args.size() ? p.ps_args[ua] : (!d.init.empty() ? d.init : "0");
            ua++;
        } else if (d.has("out") || d.has("inout")) {
            std::string ln = "o_" + std::to_string(pi);
            pre += "    " + d.type + " " + ln + " = (" + d.type + ")0;\n";
            outs.push_back({ln, &d});
            a = ln;
        } else if (fx.structs.count(d.type)) {
            std::string ln = "i_" + std::to_string(pi);
            pre += "    " + d.type + " " + ln + ";\n";
            for (const Decl& m : fx.structs[d.type].members) pre += "    " + ln + "." + m.name + " = " + vary(m.type, m.semantic) + ";\n";
            a = ln;
        } else {
            a = vary(d.type, d.semantic);
        }
        if (!args.empty()) args += ", ";
        args += a;
        pi++;
    }
    auto take = [&](const std::string& expr, const std::string& type, const std::string& sem) {
        std::string b;
        int idx;
        if (!split_semantic(sem, b, idx)) return;
        if ((b == "COLOR" || b == "SV_TARGET") && idx == 0) color = to4(type, expr);
        else if (b == "DEPTH" || b == "SV_DEPTH") depth = "(" + expr + ")";
    };
    std::string call;
    if (fx.structs.count(f.ret)) {
        call = "    " + f.ret + " r = " + f.name + "(" + args + ");\n";
        for (const Decl& m : fx.structs[f.ret].members) take("r." + m.name, m.type, m.semantic);
    } else if (f.ret != "void") {
        call = "    " + f.ret + " r = " + f.name + "(" + args + ");\n";
        take("r", f.ret, f.ret_sem.empty() ? "COLOR0" : f.ret_sem);
    } else {
        call = "    " + f.name + "(" + args + ");\n";
    }
    for (auto& o : outs) {
        if (fx.structs.count(o.second->type)) for (const Decl& m : fx.structs[o.second->type].members) take(o.first + "." + m.name, m.type, m.semantic);
        else take(o.first, o.second->type, o.second->semantic);
    }
    std::string s = k_hdr_ps;
    s += fx.source;
    s += "\n";
    s += k_src_mbp_common;
    if (!depth.empty()) {
        s += "struct MBP_PSOUT { float4 c : SV_Target0; float d : SV_Depth; };\n";
        s += "MBP_PSOUT mbp_ps(MBP_VSOUT i)\n{\n" + pre + call + "    MBP_PSOUT o;\n    o.c = mbp_ps_finish(" + color + ", i);\n    o.d = " + depth + ";\n    return o;\n}\n";
    } else {
        s += "float4 mbp_ps(MBP_VSOUT i) : SV_Target0\n{\n" + pre + call + "    return mbp_ps_finish(" + color + ", i);\n}\n";
    }
    return s;
}

UboImage* Effect::image_for(const ShaderReflection& r)
{
    if (r.ubo0.empty()) return nullptr;
    uint64_t sig = r.ubo0_size;
    for (const UboMember& m : r.ubo0) {
        sig = hash_bytes(m.name.data(), m.name.size(), sig);
        sig = hash_bytes(&m.offset, 4, sig);
    }
    for (auto& im : images)
        if (im->sig == sig) return im.get();
    auto im = std::make_unique<UboImage>();
    im->sig = sig;
    im->layout = r.ubo0;
    for (size_t i = 0; i < im->layout.size(); i++) im->idx[im->layout[i].name] = (int)i;
    im->data.assign(r.ubo0_size, 0);
    im->src.data = im->data.data();
    im->src.size = (uint32_t)im->data.size();
    im->src.version = ++ver;
    for (FxParam& p : params)
        if (p.kind == FxParam::Value) write_param(*im, p);
    images.push_back(std::move(im));
    return images.back().get();
}

UniformSource* Effect::uniforms_for(const ShaderReflection& r)
{
    UboImage* im = image_for(r);
    if (!im) return nullptr;
    subs.emplace_back();
    ShaderUbo& s = subs.back();
    s.img = im;
    s.src.data = im->data.data();
    s.src.size = (uint32_t)im->data.size();
    s.src.version = ++ver;
    s.reads.assign(params.size(), 0);
    std::unordered_set<std::string> active;
    for (const UboMember& m : r.ubo0)
        if (m.active) active.insert(m.name);
    for (size_t i = 0; i < params.size(); i++) s.reads[i] = active.count(params[i].name) ? 1 : 0;
    return &s.src;
}

void Effect::write_param(UboImage& img, FxParam& p)
{
    auto it = img.idx.find(p.name);
    if (it == img.idx.end()) return;
    const UboMember& m = img.layout[it->second];
    int E = std::max(1, p.elems);
    if (m.array_count) E = std::min<int>(E, (int)m.array_count);
    int R = p.t.rows, C = p.t.cols;
    uint8_t* base = img.data.data();
    auto put = [&](uint32_t off, uint32_t v) {
        if (off + 4 > img.data.size()) return;
        if (p.t.base == 0 && m.base != 0) { float f; memcpy(&f, &v, 4); v = m.base == 2 ? (uint32_t)(f != 0.0f) : (uint32_t)(int32_t)f; }
        else if (p.t.base != 0 && m.base == 0) { float f = (float)(int32_t)v; memcpy(&v, &f, 4); }
        else if (m.base == 2) v = v != 0;
        memcpy(base + off, &v, 4);
    };
    for (int e = 0; e < E; e++) {
        uint32_t eo = m.offset + (uint32_t)e * m.array_stride;
        if (p.t.matrix) {
            for (int r = 0; r < R; r++)
                for (int c = 0; c < C; c++) {
                    uint32_t v = p.v[((size_t)e * R + r) * C + c];
                    uint32_t off = m.spv_row_major ? eo + (uint32_t)c * m.matrix_stride + (uint32_t)r * 4 : eo + (uint32_t)r * m.matrix_stride + (uint32_t)c * 4;
                    put(off, v);
                }
        } else {
            int n = std::min<int>(R * C, (int)std::max<uint32_t>(1, m.vecsize));
            for (int c = 0; c < n; c++) put(eo + (uint32_t)c * 4, p.v[(size_t)e * R * C + c]);
        }
    }
}

void Effect::commit()
{
    for (size_t i = 0; i < params.size(); i++) {
        FxParam& p = params[i];
        if (!p.dirty || p.kind != FxParam::Value) continue;
        for (auto& im : images) write_param(*im, p);
        p.dirty = false;
        for (ShaderUbo& s : subs)
            if (s.reads[i]) s.src.version = ++ver;
    }
    if (cur && cur_pass >= 0 && cur_pass < (int)cur->passes.size()) {
        FxPass& ps = cur->passes[cur_pass];
        for (size_t k = 0; k < ps.samplers.size(); k++) {
            FxSampler* s = ps.samplers[k];
            if (!s) continue;
            auto t = by_name.find(s->texture);
            dev->SetTexture((DWORD)k, t != by_name.end() ? t->second->tex : nullptr);
        }
    }
}

void Effect::apply_samplers(FxPass& p)
{
    for (size_t k = 0; k < p.samplers.size(); k++) {
        FxSampler* s = p.samplers[k];
        if (!s) continue;
        for (auto& st : s->states) dev->SetSamplerState((DWORD)k, (D3DSAMPLERSTATETYPE)st.first, st.second);
    }
}

bool Effect::compile_ps(FxPass& p)
{
    if (p.ps_tried) return p.ps && p.ps->cs.shader;
    p.ps_tried = true;
    std::string err;
    std::string src = ps_source(*this, p, err);
    if (src.empty()) { log("effect pass %s: %s", p.name.c_str(), err.c_str()); return false; }
    PShader* ps = new PShader();
    ps->dev = dev;
    if (!compile_shader(dev->gpu, src, "mbp_ps", Stage::Fragment, ps->cs)) {
        log("effect pixel shader %s: %s", p.ps_fn.c_str(), ps->cs.error.c_str());
        ps->Release();
        return false;
    }
    for (size_t k = 0; k < ps->cs.refl.samplers.size(); k++) {
        ps->stage_of_binding.push_back((int)k);
        auto it = sampler_by_name.find(ps->cs.refl.samplers[k]);
        p.samplers.push_back(it != sampler_by_name.end() ? it->second : nullptr);
    }
    ps->ubo0 = uniforms_for(ps->cs.refl);
    p.ps = ps;
    return true;
}

VsVariant* FxVS::variant(const VertexDecl* decl)
{
    auto it = variants.find(decl->hash);
    if (it != variants.end()) return &it->second;
    VsVariant& v = variants[decl->hash];
    std::string err;
    std::string src = fx->vs_source(*pass, decl, v.inputs, err);
    if (src.empty()) { log("effect pass %s: %s", pass->name.c_str(), err.c_str()); return &v; }
    CompiledShader cs;
    if (!compile_shader(dev->gpu, src, "mbp_vs", Stage::Vertex, cs)) {
        log("effect vertex shader %s: %s", pass->vs_fn.c_str(), cs.error.c_str());
        return &v;
    }
    v.shader = cs.shader;
    v.refl = cs.refl;
    v.ok = true;
    if (!ubo0) ubo0 = fx->uniforms_for(cs.refl);
    return &v;
}

Effect::~Effect()
{
    SDL_GPUDevice* g = dev ? dev->gpu : nullptr;
    for (FxTechnique& t : techs)
        for (FxPass& p : t.passes) {
            if (p.vs) {
                for (auto& kv : p.vs->variants)
                    if (kv.second.shader && g) SDL_ReleaseGPUShader(g, kv.second.shader);
                p.vs->Release();
            }
            if (p.ps) {
                if (p.ps->cs.shader && g) SDL_ReleaseGPUShader(g, p.ps->cs.shader);
                p.ps->Release();
            }
        }
    for (FxParam& p : params) safe_release(p.tex);
    safe_release(saved);
    safe_release(static_cast<IDirect3DDevice9*>(dev));
}

// ---------------------------------------------------------------------------------------------------------------------
// handles / parameters
FxParam* Effect::param(D3DXHANDLE h)
{
    if (!h) return nullptr;
    auto it = by_handle.find((const void*)h);
    if (it != by_handle.end()) return it->second;
    auto n = by_name.find(h);
    return n != by_name.end() ? n->second : nullptr;
}
FxTechnique* Effect::technique(D3DXHANDLE h)
{
    if (!h) return nullptr;
    auto it = tech_by_handle.find((const void*)h);
    if (it != tech_by_handle.end()) return it->second;
    for (FxTechnique& t : techs)
        if (t.name == h) return &t;
    return nullptr;
}
D3DXHANDLE Effect::GetParameter(D3DXHANDLE parent, UINT i)
{
    if (parent) return nullptr;
    return i < params.size() ? params[i].handle.c_str() : nullptr;
}
D3DXHANDLE Effect::GetParameterByName(D3DXHANDLE parent, LPCSTR name)
{
    if (parent || !name) return nullptr;
    auto it = by_name.find(name);
    return it != by_name.end() ? it->second->handle.c_str() : nullptr;
}
D3DXHANDLE Effect::GetParameterBySemantic(D3DXHANDLE parent, LPCSTR sem)
{
    if (parent || !sem) return nullptr;
    for (FxParam& p : params)
        if (strcasecmp(p.semantic.c_str(), sem) == 0) return p.handle.c_str();
    return nullptr;
}
D3DXHANDLE Effect::GetTechniqueByName(LPCSTR name)
{
    if (!name) return nullptr;
    for (FxTechnique& t : techs)
        if (t.name == name) return t.handle.c_str();
    return nullptr;
}
D3DXHANDLE Effect::GetPass(D3DXHANDLE th, UINT i)
{
    FxTechnique* t = technique(th);
    return t && i < t->passes.size() ? t->passes[i].handle.c_str() : nullptr;
}
D3DXHANDLE Effect::GetPassByName(D3DXHANDLE th, LPCSTR name)
{
    FxTechnique* t = technique(th);
    if (!t || !name) return nullptr;
    for (FxPass& p : t->passes)
        if (p.name == name) return p.handle.c_str();
    return nullptr;
}

HRESULT Effect::SetValue(D3DXHANDLE h, LPCVOID data, UINT bytes)
{
    FxParam* p = param(h);
    if (!p || !data) return D3DERR_INVALIDCALL;
    if (p->kind == FxParam::Texture) return SetTexture(h, *(IDirect3DBaseTexture9* const*)data);
    if (p->kind != FxParam::Value) return D3DERR_INVALIDCALL;
    size_t n = std::min<size_t>(p->v.size() * 4, bytes);
    if (memcmp(p->v.data(), data, n) != 0) {
        memcpy(p->v.data(), data, n);
        p->dirty = true;
    }
    return D3D_OK;
}
HRESULT Effect::GetValue(D3DXHANDLE h, LPVOID data, UINT bytes)
{
    FxParam* p = param(h);
    if (!p || !data) return D3DERR_INVALIDCALL;
    if (p->kind == FxParam::Texture) {
        if (bytes < sizeof(void*)) return D3DERR_INVALIDCALL;
        *(IDirect3DBaseTexture9**)data = p->tex;
        safe_addref(p->tex);
        return D3D_OK;
    }
    memcpy(data, p->v.data(), std::min<size_t>(p->v.size() * 4, bytes));
    return D3D_OK;
}
HRESULT Effect::SetRawValue(D3DXHANDLE h, LPCVOID data, UINT off, UINT bytes)
{
    FxParam* p = param(h);
    if (!p || !data || p->kind != FxParam::Value || off > p->v.size() * 4) return D3DERR_INVALIDCALL;
    memcpy((uint8_t*)p->v.data() + off, data, std::min<size_t>(p->v.size() * 4 - off, bytes));
    p->dirty = true;
    return D3D_OK;
}
HRESULT Effect::SetIntArray(D3DXHANDLE h, CONST INT* v, UINT c)
{
    FxParam* p = param(h);
    if (!p || !v || p->kind != FxParam::Value) return D3DERR_INVALIDCALL;
    size_t n = std::min<size_t>(p->v.size(), c);
    for (size_t i = 0; i < n; i++) {
        uint32_t x;
        if (p->t.base == 0) { float f = (float)v[i]; memcpy(&x, &f, 4); }
        else x = (uint32_t)v[i];
        if (p->v[i] != x) { p->v[i] = x; p->dirty = true; }
    }
    return D3D_OK;
}
HRESULT Effect::GetIntArray(D3DXHANDLE h, INT* v, UINT c)
{
    FxParam* p = param(h);
    if (!p || !v || p->kind != FxParam::Value) return D3DERR_INVALIDCALL;
    for (size_t i = 0; i < std::min<size_t>(p->v.size(), c); i++) {
        if (p->t.base == 0) { float f; memcpy(&f, &p->v[i], 4); v[i] = (INT)f; }
        else v[i] = (INT)p->v[i];
    }
    return D3D_OK;
}
HRESULT Effect::SetFloatArray(D3DXHANDLE h, CONST FLOAT* v, UINT c)
{
    FxParam* p = param(h);
    if (!p || !v || p->kind != FxParam::Value) return D3DERR_INVALIDCALL;
    size_t n = std::min<size_t>(p->v.size(), c);
    for (size_t i = 0; i < n; i++) {
        uint32_t x;
        if (p->t.base == 0) memcpy(&x, &v[i], 4);
        else x = p->t.base == 2 ? (uint32_t)(v[i] != 0.0f) : (uint32_t)(int32_t)v[i];
        if (p->v[i] != x) { p->v[i] = x; p->dirty = true; }
    }
    return D3D_OK;
}
HRESULT Effect::GetFloatArray(D3DXHANDLE h, FLOAT* v, UINT c)
{
    FxParam* p = param(h);
    if (!p || !v || p->kind != FxParam::Value) return D3DERR_INVALIDCALL;
    for (size_t i = 0; i < std::min<size_t>(p->v.size(), c); i++) {
        if (p->t.base == 0) memcpy(&v[i], &p->v[i], 4);
        else v[i] = (float)(int32_t)p->v[i];
    }
    return D3D_OK;
}
HRESULT Effect::SetVectorArray(D3DXHANDLE h, CONST D3DXVECTOR4* v, UINT c)
{
    // D3DX: each vector fills one element (row) of the parameter with as many components as it has
    FxParam* p = param(h);
    if (!p || !v || p->kind != FxParam::Value) return D3DERR_INVALIDCALL;
    int comps = p->t.cols;
    size_t elems = p->count() / (size_t)comps;
    for (size_t e = 0; e < std::min<size_t>(elems, c); e++) {
        const float* f = &v[e].x;
        for (int k = 0; k < comps && k < 4; k++) {
            uint32_t x;
            if (p->t.base == 0) memcpy(&x, &f[k], 4);
            else x = (uint32_t)(int32_t)f[k];
            if (p->v[e * comps + k] != x) { p->v[e * comps + k] = x; p->dirty = true; }
        }
    }
    return D3D_OK;
}
HRESULT Effect::GetVectorArray(D3DXHANDLE h, D3DXVECTOR4* v, UINT c)
{
    FxParam* p = param(h);
    if (!p || !v || p->kind != FxParam::Value) return D3DERR_INVALIDCALL;
    int comps = p->t.cols;
    size_t elems = p->count() / (size_t)comps;
    for (size_t e = 0; e < std::min<size_t>(elems, c); e++) {
        float* f = &v[e].x;
        for (int k = 0; k < 4; k++) {
            if (k >= comps) { f[k] = 0; continue; }
            if (p->t.base == 0) memcpy(&f[k], &p->v[e * comps + k], 4);
            else f[k] = (float)(int32_t)p->v[e * comps + k];
        }
    }
    return D3D_OK;
}
HRESULT Effect::set_matrices(D3DXHANDLE h, const D3DXMATRIX* m, UINT n, bool tr)
{
    FxParam* p = param(h);
    if (!p || !m || p->kind != FxParam::Value) return D3DERR_INVALIDCALL;
    int R = p->t.rows, C = p->t.cols;
    size_t elems = (size_t)std::max(1, p->elems);
    for (size_t e = 0; e < std::min<size_t>(elems, n); e++)
        for (int r = 0; r < R; r++)
            for (int c = 0; c < C; c++) {
                float f = tr ? m[e].m[c][r] : m[e].m[r][c];
                uint32_t x;
                memcpy(&x, &f, 4);
                uint32_t& d = p->v[(e * R + r) * C + c];
                if (d != x) { d = x; p->dirty = true; }
            }
    return D3D_OK;
}
HRESULT Effect::get_matrices(D3DXHANDLE h, D3DXMATRIX* m, UINT n, bool tr)
{
    FxParam* p = param(h);
    if (!p || !m || p->kind != FxParam::Value) return D3DERR_INVALIDCALL;
    int R = p->t.rows, C = p->t.cols;
    size_t elems = (size_t)std::max(1, p->elems);
    for (size_t e = 0; e < std::min<size_t>(elems, n); e++) {
        memset(&m[e], 0, sizeof(D3DMATRIX));
        for (int r = 0; r < R; r++)
            for (int c = 0; c < C; c++) {
                float f;
                memcpy(&f, &p->v[(e * R + r) * C + c], 4);
                if (tr) m[e].m[c][r] = f; else m[e].m[r][c] = f;
            }
    }
    return D3D_OK;
}
HRESULT Effect::SetMatrixPointerArray(D3DXHANDLE h, CONST D3DXMATRIX** pm, UINT c)
{
    if (!pm) return D3DERR_INVALIDCALL;
    std::vector<D3DXMATRIX> v(c);
    for (UINT i = 0; i < c; i++) v[i] = *pm[i];
    return set_matrices(h, v.data(), c, false);
}
HRESULT Effect::GetMatrixPointerArray(D3DXHANDLE h, D3DXMATRIX** pm, UINT c)
{
    if (!pm) return D3DERR_INVALIDCALL;
    for (UINT i = 0; i < c; i++) {
        std::vector<D3DXMATRIX> v(i + 1);
        HRESULT r = get_matrices(h, v.data(), i + 1, false);
        if (FAILED(r)) return r;
        *pm[i] = v[i];
    }
    return D3D_OK;
}
HRESULT Effect::SetMatrixTransposePointerArray(D3DXHANDLE h, CONST D3DXMATRIX** pm, UINT c)
{
    if (!pm) return D3DERR_INVALIDCALL;
    std::vector<D3DXMATRIX> v(c);
    for (UINT i = 0; i < c; i++) v[i] = *pm[i];
    return set_matrices(h, v.data(), c, true);
}
HRESULT Effect::GetMatrixTransposePointerArray(D3DXHANDLE h, D3DXMATRIX** pm, UINT c)
{
    if (!pm) return D3DERR_INVALIDCALL;
    for (UINT i = 0; i < c; i++) {
        std::vector<D3DXMATRIX> v(i + 1);
        HRESULT r = get_matrices(h, v.data(), i + 1, true);
        if (FAILED(r)) return r;
        *pm[i] = v[i];
    }
    return D3D_OK;
}
HRESULT Effect::SetTexture(D3DXHANDLE h, LPDIRECT3DBASETEXTURE9 t)
{
    FxParam* p = param(h);
    if (!p || p->kind != FxParam::Texture) return D3DERR_INVALIDCALL;
    set_ref(p->tex, t);
    return D3D_OK;
}
HRESULT Effect::GetTexture(D3DXHANDLE h, LPDIRECT3DBASETEXTURE9* t)
{
    FxParam* p = param(h);
    if (!p || !t || p->kind != FxParam::Texture) return D3DERR_INVALIDCALL;
    *t = p->tex;
    safe_addref(p->tex);
    return D3D_OK;
}

// ---------------------------------------------------------------------------------------------------------------------
// techniques / passes
HRESULT Effect::SetTechnique(D3DXHANDLE h)
{
    FxTechnique* t = technique(h);
    if (!t) return D3DERR_INVALIDCALL;
    cur = t;
    return D3D_OK;
}
HRESULT Effect::ValidateTechnique(D3DXHANDLE h)
{
    FxTechnique* t = technique(h);
    if (!t) return D3DERR_INVALIDCALL;
    for (FxPass& p : t->passes)
        if (!compile_ps(p)) return E_FAIL;
    return D3D_OK;
}
HRESULT Effect::FindNextValidTechnique(D3DXHANDLE h, D3DXHANDLE* out)
{
    if (!out) return D3DERR_INVALIDCALL;
    size_t start = 0;
    if (h) {
        FxTechnique* t = technique(h);
        for (size_t i = 0; i < techs.size(); i++) if (&techs[i] == t) start = i + 1;
    }
    for (size_t i = start; i < techs.size(); i++)
        if (SUCCEEDED(ValidateTechnique(techs[i].handle.c_str()))) { *out = techs[i].handle.c_str(); return D3D_OK; }
    *out = nullptr;
    return S_FALSE;
}
HRESULT Effect::Begin(UINT* passes, DWORD flags)
{
    if (!cur) cur = techs.empty() ? nullptr : &techs[0];
    if (!cur) return D3DERR_INVALIDCALL;
    if (passes) *passes = (UINT)cur->passes.size();
    if (!(flags & D3DXFX_DONOTSAVESTATE)) {
        safe_release(saved);
        saved = nullptr;
        dev->CreateStateBlock(D3DSBT_ALL, &saved);
    }
    begun = true;
    return D3D_OK;
}
HRESULT Effect::BeginPass(UINT pass)
{
    if (!begun || !cur || pass >= cur->passes.size()) return D3DERR_INVALIDCALL;
    FxPass& p = cur->passes[pass];
    cur_pass = (int)pass;
    if (!p.vs) {
        p.vs = new FxVS();
        p.vs->dev = dev;
        p.vs->fx = this;
        p.vs->pass = &p;
    }
    compile_ps(p);
    dev->SetVertexShader(p.vs);
    dev->SetPixelShader(p.ps);
    for (auto& rs : p.rstates) dev->SetRenderState((D3DRENDERSTATETYPE)rs.first, rs.second);
    apply_samplers(p);
    commit();
    return D3D_OK;
}
HRESULT Effect::CommitChanges()
{
    if (!begun) return D3D_OK;
    commit();
    return D3D_OK;
}
HRESULT Effect::EndPass()
{
    cur_pass = -1;
    return D3D_OK;
}
HRESULT Effect::End()
{
    if (!begun) return D3D_OK;
    begun = false;
    cur_pass = -1;
    if (saved) {
        saved->Apply();
        saved->Release();
        saved = nullptr;
    }
    return D3D_OK;
}

} // namespace mbg

using namespace mbg;

extern "C" HRESULT WINAPI D3DXCreateEffectFromFileA(LPDIRECT3DDEVICE9 pDevice, LPCSTR file, CONST D3DXMACRO* defines, LPD3DXINCLUDE, DWORD,
                                                   LPD3DXEFFECTPOOL, LPD3DXEFFECT* ppEffect, LPD3DXBUFFER* ppErrors)
{
    if (ppEffect) *ppEffect = nullptr;
    if (ppErrors) *ppErrors = nullptr;
    if (!pDevice || !file || !ppEffect) return D3DERR_INVALIDCALL;
    std::vector<uint8_t> data;
    if (!read_file(file, data)) return D3DERR_NOTFOUND;
    std::string text(data.begin(), data.end());
    if (text.size() >= 4 && (uint8_t)text[0] == 0x01 && (uint8_t)text[1] == 0x09 && (uint8_t)text[2] == 0xff && (uint8_t)text[3] == 0xfe) {
        const char m[] = "compiled effect (.fxo) binaries are not supported: provide the .fx source";
        log("%s (%s)", m, file);
        if (ppErrors) *ppErrors = make_buffer(m, sizeof m);
        return D3DERR_INVALIDCALL;
    }
    std::string pre;
    for (const D3DXMACRO* d = defines; d && d->Name; d++) pre += std::string("#define ") + d->Name + " " + (d->Definition ? d->Definition : "1") + "\n";
    Effect* fx = new Effect();
    fx->dev = static_cast<Device*>(pDevice);
    fx->dev->AddRef();
    std::string err;
    if (!fx->parse(pre + text, err)) {
        log("D3DXCreateEffectFromFileA %s: %s", file, err.c_str());
        if (ppErrors) *ppErrors = make_buffer(err.c_str(), err.size() + 1);
        fx->Release();
        return D3DERR_INVALIDCALL;
    }
    if (getenv("MBG_FX_PRECOMPILE")) {
        for (FxTechnique& t : fx->techs)
            for (FxPass& p : t.passes) fx->compile_ps(p);
    }
    *ppEffect = fx;
    return D3D_OK;
}
