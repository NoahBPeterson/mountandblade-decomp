// port/gfx/shader_compiler.cpp: HLSL (SM2 effect / fixed-function / translated asm) -> SDL_GPUShader.
//   glslang (HLSL front end, DX9 compatibility: sampler2D + tex2D) -> SPIR-V; the resource bindings are then patched to
//   SDL_GPU's contract (gfx_internal.h); SPIRV-Cross reflects the user uniform block and, on Metal, emits MSL.
// Compiled results are cached on disk (key = hash of source + entry + stage + backend) so a run compiles a shader once.
#include "gfx_internal.h"

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
#include <spirv_cross/spirv_cross_c.h>
#include <sys/stat.h>

namespace mbg {

static bool g_glslang_ready = false;

static bool spirv_patch_binding(std::vector<uint32_t>& spv, uint32_t id, uint32_t set, uint32_t binding)
{
    // OpDecorate = 71: [wc|op] target decoration literal ; DescriptorSet = 34, Binding = 33
    bool have_set = false, have_bind = false;
    size_t last_annot = 5;
    for (size_t i = 5; i < spv.size();) {
        uint32_t wc = spv[i] >> 16, op = spv[i] & 0xffff;
        if (wc == 0) return false;
        if (op == 71 && wc >= 4 && spv[i + 1] == id) {
            if (spv[i + 2] == 34) { spv[i + 3] = set; have_set = true; }
            if (spv[i + 2] == 33) { spv[i + 3] = binding; have_bind = true; }
        }
        if (op == 71 || op == 72 || op == 332 || op == 5632 || op == 5633) last_annot = i + wc;
        i += wc;
    }
    std::vector<uint32_t> ins;
    if (!have_set) { ins.push_back((4u << 16) | 71); ins.push_back(id); ins.push_back(34); ins.push_back(set); }
    if (!have_bind) { ins.push_back((4u << 16) | 71); ins.push_back(id); ins.push_back(33); ins.push_back(binding); }
    if (!ins.empty()) spv.insert(spv.begin() + (long)last_annot, ins.begin(), ins.end());
    return true;
}

static bool glslang_compile(const std::string& src, const char* entry, Stage stage, std::vector<uint32_t>& out, std::string& err)
{
    if (!g_glslang_ready) {
        glslang_initialize_process();
        g_glslang_ready = true;
    }
    glslang_input_t in{};
    in.language = GLSLANG_SOURCE_HLSL;
    in.stage = stage == Stage::Vertex ? GLSLANG_STAGE_VERTEX : GLSLANG_STAGE_FRAGMENT;
    in.client = GLSLANG_CLIENT_VULKAN;
    in.client_version = GLSLANG_TARGET_VULKAN_1_0;
    in.target_language = GLSLANG_TARGET_SPV;
    in.target_language_version = GLSLANG_TARGET_SPV_1_0;
    in.code = src.c_str();
    in.default_version = 100;
    in.default_profile = GLSLANG_NO_PROFILE;
    in.messages = (glslang_messages_t)(GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT | GLSLANG_MSG_READ_HLSL_BIT |
                                       GLSLANG_MSG_HLSL_DX9_COMPATIBLE_BIT | GLSLANG_MSG_HLSL_LEGALIZATION_BIT);
    in.resource = glslang_default_resource();
    glslang_shader_t* sh = glslang_shader_create(&in);
    glslang_shader_set_entry_point(sh, entry);
    glslang_shader_set_options(sh, GLSLANG_SHADER_AUTO_MAP_BINDINGS | GLSLANG_SHADER_AUTO_MAP_LOCATIONS);
    if (!glslang_shader_preprocess(sh, &in) || !glslang_shader_parse(sh, &in)) {
        err = glslang_shader_get_info_log(sh);
        glslang_shader_delete(sh);
        return false;
    }
    glslang_program_t* prog = glslang_program_create();
    glslang_program_add_shader(prog, sh);
    if (!glslang_program_link(prog, in.messages)) {
        err = glslang_program_get_info_log(prog);
        glslang_program_delete(prog);
        glslang_shader_delete(sh);
        return false;
    }
    glslang_program_map_io(prog);
    glslang_spv_options_t opt{};
    opt.disable_optimizer = false;
    opt.validate = false;
    glslang_program_SPIRV_generate_with_options(prog, in.stage, &opt);
    out.resize(glslang_program_SPIRV_get_size(prog));
    glslang_program_SPIRV_get(prog, out.data());
    const char* msg = glslang_program_SPIRV_get_messages(prog);
    if (msg && *msg && verbose()) log("glslang: %s", msg);
    glslang_program_delete(prog);
    glslang_shader_delete(sh);
    return !out.empty();
}

static void reflect_block(spvc_compiler c, spvc_type_id type_id, ShaderReflection& r)
{
    spvc_type t = spvc_compiler_get_type_handle(c, type_id);
    size_t sz = 0;
    spvc_compiler_get_declared_struct_size(c, t, &sz);
    r.ubo0_size = (uint32_t)sz;
    unsigned n = spvc_type_get_num_member_types(t);
    for (unsigned i = 0; i < n; i++) {
        UboMember m;
        const char* nm = spvc_compiler_get_member_name(c, type_id, i);
        m.name = nm ? nm : "";
        unsigned off = 0;
        spvc_compiler_type_struct_member_offset(c, t, i, &off);
        m.offset = off;
        spvc_type_id mt_id = spvc_type_get_member_type(t, i);
        spvc_type mt = spvc_compiler_get_type_handle(c, mt_id);
        m.vecsize = spvc_type_get_vector_size(mt);
        m.columns = spvc_type_get_columns(mt);
        spvc_basetype bt = spvc_type_get_basetype(mt);
        m.base = bt == SPVC_BASETYPE_FP32 ? 0 : bt == SPVC_BASETYPE_INT32 ? 1 : 2;
        if (spvc_type_get_num_array_dimensions(mt) > 0) {
            m.array_count = spvc_type_get_array_dimension(mt, 0);
            unsigned st = 0;
            spvc_compiler_type_struct_member_array_stride(c, t, i, &st);
            m.array_stride = st;
        }
        if (m.columns > 1) {
            unsigned ms = 0;
            spvc_compiler_type_struct_member_matrix_stride(c, t, i, &ms);
            m.matrix_stride = ms;
            m.spv_row_major = spvc_compiler_has_member_decoration(c, type_id, i, SpvDecorationRowMajor);
        }
        r.ubo0.push_back(m);
    }
}

// ---- disk cache ----
std::string shader_cache_dir()
{
    static std::string dir;
    if (dir.empty()) {
        const char* env = getenv("MBG_SHADER_CACHE");
        if (env && *env) dir = env;
        else {
            char* p = SDL_GetPrefPath("mb_port", "shadercache");
            dir = p ? p : "";
            SDL_free(p);
        }
        if (!dir.empty() && dir.back() != '/') dir += '/';
    }
    return dir;
}

static const uint32_t k_cache_magic = 0x4d424753, k_cache_version = 3;

static void put_u32(std::vector<uint8_t>& b, uint32_t v) { b.insert(b.end(), (uint8_t*)&v, (uint8_t*)&v + 4); }
static void put_str(std::vector<uint8_t>& b, const std::string& s) { put_u32(b, (uint32_t)s.size()); b.insert(b.end(), s.begin(), s.end()); }
struct Reader {
    const uint8_t* p; const uint8_t* e; bool ok = true;
    uint32_t u32() { if (e - p < 4) { ok = false; return 0; } uint32_t v; memcpy(&v, p, 4); p += 4; return v; }
    std::string str() { uint32_t n = u32(); if ((size_t)(e - p) < n) { ok = false; return ""; } std::string s((const char*)p, n); p += n; return s; }
};

static void cache_store(const std::string& path, const std::vector<uint8_t>& code, const std::string& entry, const ShaderReflection& r)
{
    std::vector<uint8_t> b;
    put_u32(b, k_cache_magic);
    put_u32(b, k_cache_version);
    put_str(b, std::string(code.begin(), code.end()));
    put_str(b, entry);
    put_u32(b, r.ubo0_size);
    put_u32(b, (uint32_t)r.ubo0.size());
    for (const UboMember& m : r.ubo0) {
        put_str(b, m.name);
        put_u32(b, m.offset); put_u32(b, m.array_stride); put_u32(b, m.matrix_stride); put_u32(b, m.array_count);
        put_u32(b, m.vecsize); put_u32(b, m.columns); put_u32(b, m.spv_row_major); put_u32(b, (uint32_t)m.base);
    }
    put_u32(b, (uint32_t)r.samplers.size());
    for (size_t i = 0; i < r.samplers.size(); i++) { put_str(b, r.samplers[i]); put_u32(b, (uint32_t)r.sampler_global[i]); }
    std::string tmp = path + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return;
    fwrite(b.data(), 1, b.size(), f);
    fclose(f);
    rename(tmp.c_str(), path.c_str());
}

static bool cache_load(const std::string& path, std::vector<uint8_t>& code, std::string& entry, ShaderReflection& r)
{
    std::vector<uint8_t> b;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    b.resize((size_t)std::max(0L, n));
    size_t got = fread(b.data(), 1, b.size(), f);
    fclose(f);
    if (got != b.size()) return false;
    Reader rd{b.data(), b.data() + b.size()};
    if (rd.u32() != k_cache_magic || rd.u32() != k_cache_version) return false;
    std::string c = rd.str();
    code.assign(c.begin(), c.end());
    entry = rd.str();
    r.ubo0_size = rd.u32();
    uint32_t nm = rd.u32();
    for (uint32_t i = 0; i < nm && rd.ok; i++) {
        UboMember m;
        m.name = rd.str();
        m.offset = rd.u32(); m.array_stride = rd.u32(); m.matrix_stride = rd.u32(); m.array_count = rd.u32();
        m.vecsize = rd.u32(); m.columns = rd.u32(); m.spv_row_major = rd.u32() != 0; m.base = (int)rd.u32();
        r.ubo0.push_back(m);
    }
    uint32_t ns = rd.u32();
    for (uint32_t i = 0; i < ns && rd.ok; i++) { r.samplers.push_back(rd.str()); r.sampler_global.push_back((int)rd.u32()); }
    return rd.ok;
}

bool compile_shader(SDL_GPUDevice* gpu, const std::string& source, const char* entry, Stage stage, CompiledShader& out)
{
    SDL_GPUShaderFormat fmts = SDL_GetGPUShaderFormats(gpu);
    bool msl = (fmts & SDL_GPU_SHADERFORMAT_MSL) != 0;
    bool spirv = (fmts & SDL_GPU_SHADERFORMAT_SPIRV) != 0;
    if (!msl && !spirv) {
        out.error = "the GPU device takes neither MSL nor SPIR-V";
        return false;
    }
    uint64_t key = hash_bytes(source.data(), source.size());
    key = hash_bytes(entry, strlen(entry), key);
    uint32_t tag = (stage == Stage::Vertex ? 1u : 2u) | (msl ? 4u : 8u);
    key = hash_bytes(&tag, 4, key);
    char name[64];
    snprintf(name, sizeof name, "%016llx.%s", (unsigned long long)key, msl ? "msl" : "spv");
    std::string dir = shader_cache_dir();
    std::string cpath = dir.empty() ? std::string() : dir + name;

    std::vector<uint8_t> code;
    std::string entry_out;
    ShaderReflection refl;
    bool cached = !cpath.empty() && !getenv("MBG_NO_SHADER_CACHE") && cache_load(cpath, code, entry_out, refl);
    if (!cached) {
        std::vector<uint32_t> spv;
        std::string err;
        if (!glslang_compile(source, entry, stage, spv, err)) {
            out.error = err;
            return false;
        }
        // reflect + patch bindings
        spvc_context ctx = nullptr;
        spvc_context_create(&ctx);
        spvc_parsed_ir ir = nullptr;
        if (spvc_context_parse_spirv(ctx, spv.data(), spv.size(), &ir) != SPVC_SUCCESS) {
            out.error = std::string("SPIRV-Cross parse: ") + spvc_context_get_last_error_string(ctx);
            spvc_context_destroy(ctx);
            return false;
        }
        spvc_compiler comp = nullptr;
        spvc_context_create_compiler(ctx, SPVC_BACKEND_NONE, ir, SPVC_CAPTURE_MODE_COPY, &comp);
        spvc_resources res = nullptr;
        spvc_set active = nullptr;
        spvc_compiler_get_active_interface_variables(comp, &active);
        spvc_compiler_create_shader_resources_for_active_variables(comp, &res, active);
        uint32_t ubo_set = stage == Stage::Vertex ? 1 : 3;
        uint32_t smp_set = stage == Stage::Vertex ? 0 : 2;
        const spvc_reflected_resource* list = nullptr;
        size_t count = 0;
        spvc_resources_get_resource_list_for_type(res, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, &list, &count);
        for (size_t i = 0; i < count; i++) {
            const char* tn = spvc_compiler_get_name(comp, list[i].base_type_id);
            std::string tname = tn ? tn : "";
            std::string vname = list[i].name ? list[i].name : "";
            bool dev_block = tname.rfind("Mbp", 0) == 0 || vname.rfind("Mbp", 0) == 0;
            spirv_patch_binding(spv, list[i].id, ubo_set, dev_block ? 1 : 0);
            if (!dev_block) reflect_block(comp, list[i].base_type_id, refl);
        }
        spvc_resources_get_resource_list_for_type(res, SPVC_RESOURCE_TYPE_SAMPLED_IMAGE, &list, &count);
        // keep the declaration order (the original binding) when compacting
        std::vector<std::pair<uint32_t, size_t>> order;
        for (size_t i = 0; i < count; i++)
            order.push_back({spvc_compiler_get_decoration(comp, list[i].id, SpvDecorationBinding), i});
        std::sort(order.begin(), order.end());
        for (size_t k = 0; k < order.size(); k++) {
            const spvc_reflected_resource& rr = list[order[k].second];
            spirv_patch_binding(spv, rr.id, smp_set, (uint32_t)k);
            refl.samplers.push_back(rr.name ? rr.name : "");
            refl.sampler_global.push_back((int)order[k].first);
        }
        spvc_context_destroy(ctx);

        if (msl) {
            spvc_context_create(&ctx);
            spvc_context_parse_spirv(ctx, spv.data(), spv.size(), &ir);
            spvc_context_create_compiler(ctx, SPVC_BACKEND_MSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &comp);
            spvc_compiler_options opts = nullptr;
            spvc_compiler_create_compiler_options(comp, &opts);
            spvc_compiler_options_set_uint(opts, SPVC_COMPILER_OPTION_MSL_VERSION, SPVC_MAKE_MSL_VERSION(2, 1, 0));
            spvc_compiler_options_set_uint(opts, SPVC_COMPILER_OPTION_MSL_PLATFORM, SPVC_MSL_PLATFORM_MACOS);
            spvc_compiler_options_set_bool(opts, SPVC_COMPILER_OPTION_MSL_ENABLE_DECORATION_BINDING, SPVC_TRUE);
            spvc_compiler_install_compiler_options(comp, opts);
            const char* text = nullptr;
            if (spvc_compiler_compile(comp, &text) != SPVC_SUCCESS) {
                out.error = std::string("SPIRV-Cross MSL: ") + spvc_context_get_last_error_string(ctx);
                spvc_context_destroy(ctx);
                return false;
            }
            code.assign(text, text + strlen(text));
            const char* cl = spvc_compiler_get_cleansed_entry_point_name(comp, entry,
                                                                         stage == Stage::Vertex ? SpvExecutionModelVertex : SpvExecutionModelFragment);
            entry_out = cl ? cl : entry;
            spvc_context_destroy(ctx);
        } else {
            code.assign((uint8_t*)spv.data(), (uint8_t*)(spv.data() + spv.size()));
            entry_out = entry;
        }
        if (!cpath.empty()) cache_store(cpath, code, entry_out, refl);
    }
    if (msl) code.push_back(0);
    SDL_GPUShaderCreateInfo ci{};
    ci.code = code.data();
    ci.code_size = msl ? code.size() - 1 : code.size();
    ci.entrypoint = entry_out.c_str();
    ci.format = msl ? SDL_GPU_SHADERFORMAT_MSL : SDL_GPU_SHADERFORMAT_SPIRV;
    ci.stage = stage == Stage::Vertex ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT;
    ci.num_samplers = (Uint32)refl.samplers.size();
    ci.num_uniform_buffers = 2;
    out.shader = SDL_CreateGPUShader(gpu, &ci);
    if (!out.shader) {
        out.error = std::string("SDL_CreateGPUShader: ") + SDL_GetError();
        if (verbose()) log("%s\n%.*s", out.error.c_str(), (int)ci.code_size, (const char*)code.data());
        if (cached && !cpath.empty()) remove(cpath.c_str());
        return false;
    }
    out.refl = std::move(refl);
    return true;
}

} // namespace mbg
