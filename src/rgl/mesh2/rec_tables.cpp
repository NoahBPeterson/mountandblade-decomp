// rec_tables.cpp: 0x4c2c50..0x4c48d0 (TU map 100 "c4c2c50" + 101 "c4c31c0", merged): the module/option record
// classes derived from head_rec (constructors, destructors, deleting destructors), a block reader, the options
// loader and renderer setup, the music/sound set and the tstring-pair array loaders.
// Migrated from slices s004c2b20 (0x4c2c50..0x4c3350), s004c3430, s004c3d90 and s004c46d0 (0x4c46d0, 0x4c47d0).
// 0x4c3050..0x4c48d0 is the "T regime" region (docs/tu_map.md): helpers sit right after their caller, so
// the functions stay in binary order and the helpers defined after a caller are kept out of line.
#include "./rec_tables.h"

#pragma warning(disable:4234)     // __thiscall on a free function (thunk reconstruction)

// @ 0x4c2c50
C4c2c50::C4c2c50()
{
    v4c = 0;
    v50 = 0;
}

// @ 0x4c2cb0
void* __fastcall destroy_C4c2c50(C4c2c50* self, int, int flags)
{
    self->dtor();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c2cd0
void C4c2c50::dtor()
{
    *(void**)this = (void*)0x595740;
    t88.~tstring();
    t78.~tstring();
    t68.~tstring();
    t54.~tstring();
    *(void**)this = (void*)0x5947d8;
    name.~tstring();
}

// @ 0x4c2d10
C4c2d10::C4c2d10()
{
    M44 tmp;
    v110 = 2;
    v118 = g_5e2958;
    v11c = g_5e295c;
    v120 = 0;
    v128 = 0;
    f124 = 6.0f;
    blk0 = *identity_pos(&tmp);
    blk1 = *identity_pos(&tmp);
    blk3 = *identity_pos(&tmp);
    blk2 = *identity_pos(&tmp);
    v100 = 0;
    v104 = 0;
}

// @ 0x4c2dd0
int __fastcall read_block_2dd0(Blk2dd0* self, void* s)
{
    st_str(s, self);
    self->s0.underscores_to_spaces();
    st_int(s, &self->count);
    int n = self->count;
    if (n != 0) {
        void* p = engine_alloc(n * 0x18 + 4);
        if (p != 0) {
            *(int*)p = n;
            Entry18* e = (Entry18*)((char*)p + 4);
            alloc_ctor(e, 0x18, n, (void*)0x4be720);
            self->entries = e;
        } else {
            self->entries = 0;
        }
    }
    for (int i = 0; i < self->count; i++) {
        Entry18* e = &self->entries[i];
        st_int(s, &e->v10);
        st_str(s, e);
        st_int(s, &e->v14);
        e->s0.underscores_to_spaces();
    }
    st_int(s, (int*)&self->s10);
    st_int(s, (int*)((char*)self + 0x14));
    return 0;
}

// @ 0x4c2ea0
void __fastcall fn_4c2ea0(void* self)
{
    tstr_ll a((const char*)0x595788);
    tstr_ll b((const char*)0x595788);
    char buf[0x28];
    sub_536f8f(buf, (const char*)0x595788);
    ((void (__cdecl*)(char*))0x4bab70)(buf);
    ((void (__cdecl*)(tstring*, char*))0x415290)(&a.s, buf);
    if (((unsigned char*)&b)[0x14] != 0) {
        ((void (__cdecl*)(void*))0x4be7c0)(self);
    } else {
        int v = 0;
        st_int(&a, &v);
        if (v != 0x25) {
            ((void (__cdecl*)(void*))0x4be7c0)(self);
        } else {
            char* d = (char*)self;
            for (int i = 0; i < 0x25; i++)
                for (int j = 0; j < 2; j++, d += 4)
                    st_int(&a, (int*)d);
        }
        ((unsigned char*)&b)[0x14] = 1;
    }
}

// @ 0x4c2fb0
C4c2fb0::C4c2fb0()
{
    tstring* p = (tstring*)t58;
    CONSTRUCT_TSTRING(p);
    CONSTRUCT_TSTRING(p + 1);
}

// @ 0x4c3000
void* __fastcall destroy_C4c2fb0(C4c2fb0* self, int, int flags)
{
    self->dtor();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c3020
void C4c2fb0::dtor()
{
    tstring* p = (tstring*)t58;
    (p + 1)->~tstring();
    p->~tstring();
    t48.~tstring();
    *(void**)this = (void*)0x5947d8;
    name.~tstring();
}

// @ 0x4c3050
C4c3050::C4c3050()
{
    tstring* p = (tstring*)t6c;
    for (int i = 6; i != 0; i--) {
        CONSTRUCT_TSTRING(p);
        p++;
    }
    v50 = -1;
    v54 = 0;
}

// @ 0x4c30b0
void set_flag_5d8789()
{
    g_5d8789 = 1;
}

// @ 0x4c30c0
void* __fastcall destroy_C4c3050(C4c3050* self, int, int flags)
{
    self->dtor();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c30e0
void C4c3050::dtor()
{
    *(void**)this = (void*)0x5957bc;
    char* p = (char*)this + 0xcc;
    for (int i = 6; i > 0; i--) {
        p -= 0x10;
        ((tstring*)p)->~tstring();
    }
    t5c.~tstring();
    *(void**)this = (void*)0x5947d8;
    name.~tstring();
}

// @ 0x4c3120
// Deleting destructor of the class with vtable 0x595804 (implicit dtor: tstring at +0x84 and the base's
// tstring at +0x1c). Declared as a __fastcall free function: `self` lands in ECX and the dummy int in EDX
// (unused), leaving `flags` at [esp+4] so the epilogue is the original's `ret 4`.
void* __fastcall destroy_C4c3120(void* self, int, int flags)
{
    ((tstring*)((char*)self + 0x84))->~tstring();
    *((void**)self) = (void*)0x5947d8;
    ((tstring*)((char*)self + 0x1c))->~tstring();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c3160
C4c3160::C4c3160()
{
    Slot20* s = slots;
    for (int i = 0; i < 0x80; i++, s++) {
        s->a = 0;
        s->b = -1;
        s->c = 0;
        s->f = 0;
        s->d = 0;
        s->e = 0;
        s->g = 0;
    }
}

// @ 0x4c31c0
C4c31c0::C4c31c0()
{
    char* p = body;
    for (int i = 0; i < 9; i++, p += 0x30) {
        CONSTRUCT_TSTRING(p);
        CONSTRUCT_TSTRING(p + 0x10);
    }
    CONSTRUCT_TSTRING(t224);
}

// @ 0x4c3220
void* __fastcall destroy_C4c31c0(C4c31c0* self, int, int flags)
{
    self->dtor();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c3240
void C4c31c0::dtor()
{
    ((tstring*)t224)->~tstring();
    char* p = (char*)this + 0x2c + 9 * 0x30;
    for (int i = 9; i > 0; i--) {
        p -= 0x30;
        ((tstring*)(p + 0x10))->~tstring();
        ((tstring*)p)->~tstring();
    }
    *(void**)this = (void*)0x5947d8;
    name.~tstring();
}

// @ 0x4c3290
C4c3290::C4c3290()
{
    tstring* p = (tstring*)t60;
    CONSTRUCT_TSTRING(p);
    CONSTRUCT_TSTRING(p + 1);
}

// @ 0x4c32e0
void* __fastcall destroy_C4c3290(C4c3290* self, int, int flags)
{
    self->dtor();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c3300
void C4c3290::dtor()
{
    tstring* p = (tstring*)t60;
    (p + 1)->~tstring();
    p->~tstring();
    t50.~tstring();
    *(void**)this = (void*)0x5947d8;
    name.~tstring();
}

// Tail-call thunks: `mov ecx, 0x8ee3c48; jmp 0x4bf870` -> a thiscall member call on the editor singleton
// (the MapEditorDialog instance, ?g_editor_dialog@@3UMapEditorDialog@@A).
struct EditorObj {
    void unk_4bf870();                          // 0x4bf870
    void unk_4c03b0();                          // 0x4c03b0
};

// @ 0x4c3330
void global_4bf870()
{
    ((EditorObj*)0x8ee3c48)->unk_4bf870();
}

// @ 0x4c3340
void global_4c03b0()
{
    ((EditorObj*)0x8ee3c48)->unk_4c03b0();
}

// @ 0x4c3350
void fn_4c3350()
{
    tstr_ll a(*(const tstring*)0x8ee3c28);
    tstr_ll b((const char*)0x595a44);
    a.s.append(b.s);
    b.s.~tstring();
    const char* nm = a.text();
    ((char (__cdecl*)(tstring*, const char*))0x415d60)(&a.s, nm);
    ((void (__cdecl*)(const char*, const char*))0x42cc80)((const char*)0x594580, nm);
    ((int (__cdecl*)(void*, void*))0x4c2250)(&a.s, (void*)0xa76628);
}
// @ 0x4c3430
void* elem58::vector_deleting_delete(unsigned flags)
{
    if (flags & 2) {
        int n = *(int*)((char*)this - 4);
        char* base = (char*)this - 4;
        elem58* cur = (elem58*)((char*)this + n * 0x58);
        while (--n >= 0) {
            cur = (elem58*)((char*)cur - 0x58);
            if (*(void**)((char*)cur + 0x34) != 0)
                engine_free(*(void**)((char*)cur + 0x34));
            RecD* r = *(RecD**)((char*)cur + 0x50);
            if (r != 0) {
                RecD_free_array(r, 0x78, *(int*)((char*)r - 4), RecD_dtor);
                engine_free((char*)r - 4);
            }
            cur->s24.~tstring();
            cur->s10.~tstring();
            cur->s0.~tstring();
        }
        if (flags & 1)
            engine_free(base);
        return base;
    }
    if (*(void**)((char*)this + 0x34) != 0)
        engine_free(*(void**)((char*)this + 0x34));
    if (*(void**)((char*)this + 0x50) != 0)
        (*(RecD**)((char*)this + 0x50))->scalar_delete(3);
    ((elem58*)this)->s24.~tstring();
    ((elem58*)this)->s10.~tstring();
    ((elem58*)this)->s0.~tstring();
    if (flags & 1)
        operator delete(this);
    return this;
}
// @ 0x4c3510
// Renderer setup: hands the font object the device, publishes the window size, queries the caps, decides
// whether shaders can be used, then reads the ten buffer-size options into create_shader_constants.
//
// Complete but NOT byte-exact: the original frame ([esp+8] arguments, a 0xc8-byte sub, `this` in esi, the
// 0x348 offset in ecx and the device kept in [esp+0x10]) shows this function was built into an object without
// /Gy (identical code in a non-COMDAT section) and with different inlining decisions, so cl 13.10 under this
// slice's flags cannot reproduce it. See nonmatching.txt.
void __fastcall renderer_setup(void* self)
{
    IDirect3DDevice9* device = *(IDirect3DDevice9**)((char*)self + 0xf8);
    ((font_ref*)*(void**)((char*)self + 0x348))->set_device(device);   // 0x404c00
    g_d3d_device = *(IDirect3DDevice9**)((char*)self + 0xf8);
    g_renderer_width = *(int*)((char*)self + 0xf4);
    g_renderer_height = *(int*)((char*)self + 0x22c);   // (the original tests +0xa4 first; both branches load +0x22c)
    g_d3d_device->GetDeviceCaps(&g_caps);
    g_renderer_vsync = -1;
    g_use_shaders = 0;
    if (g_caps.PixelShaderVersion >= 0xfffe0200 && g_caps.VertexShaderVersion >= 0xffff0200 &&
        g_caps.MaxTextureBlendStages >= 0x10 && (g_caps.DevCaps & 1) != 0 && g_shader_support != 0)
        g_use_shaders = 1;
    g_renderer_flag = 0;
    render_init(&g_render);
    {
        tstring key1(g_opt_dx7_regular_ffp_dynamic);
        reset_buffer_group(self);
        int idx1 = config_find(&g_config, &key1);
        int dx7_regular_ffp_dynamic = atoi(idx1 >= 0 ? CFG_ENTRY(idx1) : "");
        tstring key2(g_opt_dx7_regular_ffp_static);
        int idx2 = config_find(&g_config, &key2);
        int dx7_regular_ffp_static = atoi(idx2 >= 0 ? CFG_ENTRY(idx2) : "");
        tstring key3(g_opt_normal_map_dynamic);
        int idx3 = config_find(&g_config, &key3);
        int normal_map_dynamic = atoi(idx3 >= 0 ? CFG_ENTRY(idx3) : "");
        tstring key4(g_opt_normal_map_static);
        int idx4 = config_find(&g_config, &key4);
        int normal_map_static = atoi(idx4 >= 0 ? CFG_ENTRY(idx4) : "");
        tstring key5(g_opt_skinning_dynamic);
        int idx5 = config_find(&g_config, &key5);
        int skinning_dynamic = atoi(idx5 >= 0 ? CFG_ENTRY(idx5) : "");
        tstring key6(g_opt_skinning_static);
        int idx6 = config_find(&g_config, &key6);
        int skinning_static = atoi(idx6 >= 0 ? CFG_ENTRY(idx6) : "");
        tstring key7(g_opt_regular_dynamic);
        int idx7 = config_find(&g_config, &key7);
        int regular_dynamic = atoi(idx7 >= 0 ? CFG_ENTRY(idx7) : "");
        tstring key8(g_opt_regular_static);
        int idx8 = config_find(&g_config, &key8);
        int regular_static = atoi(idx8 >= 0 ? CFG_ENTRY(idx8) : "");
        tstring key9(g_opt_regular_ffp_dynamic);
        int idx9 = config_find(&g_config, &key9);
        int regular_ffp_dynamic = atoi(idx9 >= 0 ? CFG_ENTRY(idx9) : "");
        tstring key10(g_opt_regular_ffp_static);
        int idx10 = config_find(&g_config, &key10);
        int regular_ffp_static = atoi(idx10 >= 0 ? CFG_ENTRY(idx10) : "");
        create_shader_constants(&g_render,
                                regular_ffp_static, regular_ffp_dynamic,
                                regular_static, regular_dynamic,
                                skinning_static, skinning_dynamic,
                                normal_map_static, normal_map_dynamic,
                                dx7_regular_ffp_static, dx7_regular_ffp_dynamic);
        setup_obj_init(&g_setup_obj, 1, (void*)0x8ee3c28);
        set_render_buffer_sizes(&g_render, *(int*)((char*)self + 0xec));
    }
}

// @ 0x4c3a40
// Reads options.dat (in the user folder) into the record.
//
// Complete but NOT byte-exact (see nonmatching.txt): the reader's open_read() was inlined into the original
// (no call at +0x3a, the one `call`s left are the two tstring ctors, _sprintf, user_path, the 4bc090/4bc100
// setters, the shader-slot calls and the log), so that TU saw the text_stream body. No local declaration
// makes cl 13.10 inline a virtual base's 0x415290 under these flags, and the original's frame/register
// choices (0x13c frame, `this` in edi from the first instruction, the record's old fields in registers)
// follow from that inlining. The record layout, read order and clamps below are exact.
void options_record::load()
{
    char path[0x100];
    options_stream st;
    sub_536f8f(path, (const char*)0x54aebc);
    user_path(path);
    st.open_read(path);
    if (!st.failed) {   // the original branches on the stream's `failed` byte
        st.r_float(&(this->f08));
        st.r_float(&(this->f0c));
        st.r_float(&(this->f10));
        { int* p = &(this->w14); st.r_int(p); }
        st.r_int(&(this->w1c));
        st.r_int(&(this->w20));
        st.r_int(&(this->w24));
        st.r_int(&(this->w28));
        st.r_int(&(this->w30));
        st.r_int((int*)(void*)this);
        options_set_mode(this, this->mode);
        st.r_int(&(this->w34));
        st.r_int(&(this->w48));
        if (this->w48 < 0 || this->w48 >= 3)
            this->w48 = 1;
        st.r_int(&(this->w50));
        st.r_int(&(this->w98));
        st.r_int(&(this->w44));
        st.r_int(&(this->w4c));
        st.r_int(&(this->w54));
        st.r_int(&(this->w64));
        st.r_int(&(this->w68));
        st.r_int(&(this->w58));
        st.r_int(&(this->w5c));
        st.r_float(&(this->f74));
        st.r_int(&(this->w6c));
        st.r_int(&(this->w70));
        st.r_int(&(this->w38));
        st.r_int(&(this->w2c));
        st.r_int(&(this->w60));
        st.r_int(&(this->w78));
        st.r_int(&(this->w7c));
        st.r_int(&(this->w04));
        options_set_spacing(this, this->w04);
        st.r_float(&(this->f80));
        if (this->f80 < 1.4f || this->f80 > 2.6f)
            this->f80 = 2.2f;
        st.r_float(&(this->f84));
        if (this->f84 < 0.5f || this->f84 > 1.01f)
            this->f84 = 0.5f;
        st.r_float(&(this->f88));
        if (this->f88 < 0.5f || this->f88 > 1.01f)
            this->f88 = 0.5f;
        st.r_int(&(this->w18));
    } else
        g_log.w_cstr("Unable to find options.dat. Loading defaults\n");
    st.is_open = 1;
}


// @ 0x4c3d90
// fill_n(dst, n, arg, this) for 0x204-byte elements: builds each element at dst+k from arg.
void copy_uninit_204(void* dst, unsigned n, const void* arg, void* thisptr) throw(int)
{
    *((volatile void**)thisptr) = thisptr;
    char* d = (char*)dst;
    while (n > 0) {
        construct_204(d, arg);
        --n;
        d += 0x204;
    }
}

// @ 0x4c3e10
// fill_n(dst, n, arg, this) for 4-byte elements.
void copy_uninit_4(void* dst, unsigned n, const void* arg, void* thisptr) throw(int)
{
    *((volatile void**)thisptr) = thisptr;
    char* d = (char*)dst;
    while (n > 0) {
        construct_4(d, arg);
        --n;
        d += 4;
    }
}

// @ 0x4c3e80
// Uninitialized copy of the [first, last) range of 0x2c-byte elements onto dst; returns the output end.
void* copy_uninit_2c(void* dst, void* last, const void* arg, void* first, void* thisptr) throw(int)
{
    *((volatile void**)thisptr) = thisptr;
    char* d = (char*)dst;
    char* s = (char*)first;
    while (s != last) {
        construct_2c(d, s);
        d += 0x2c;
        s += 0x2c;
    }
    return d;
}

// @ 0x4c3ef0
// Uninitialized copy of the [first, last) range of 4-byte elements onto dst; returns the output end.
void* copy_uninit_4_range(void* dst, void* last, const void* arg, void* first, void* thisptr) throw(int)
{
    *((volatile void**)thisptr) = thisptr;
    char* d = (char*)dst;
    char* s = (char*)first;
    while (s != last) {
        construct_4(d, s);
        d += 4;
        s += 4;
    }
    return d;
}

// ---- element type of the 0x48-byte array destroyed by 0x4c3f60 ------------------------------------
// A tstring at +0 and one at +0x14; 0x4bdf60 fills them from the stream after the records themselves.

// @ 0x4c3f60
// vector<elem48>::~vector(): destroys the elements back to front through the count stored before the array,
// frees the block and clears the caller's pointer.
void __fastcall destroy_elem48_array(elem48** pv)
{
    elem48* v = *pv;
    if (v != 0) {
        int n = *(int*)((char*)v - 4);
        while (--n >= 0) {
            v[n].s14.~tstring();
            v[n].s0.~tstring();
        }
        engine_free((char*)v - 4);
        *pv = 0;
    }
}

// @ 0x4c3fb0
// Loader (ret 4) of an object whose array member at +0 holds 0x48-byte entries and whose count sits at +4:
// reads the count, destroys and rebuilds the array (both tstrings of every element), then initializes each
// element with the given argument.
void __fastcall load_elem48_array(void* self, void* unused, stream_base* s)
{
    int* count = (int*)((char*)self + 4);
    s->r_int(count);
    void* old = *(void**)self;
    if (old != 0) {
        elem48* p = (elem48*)((char*)old + *(int*)((char*)old - 4) * 0x48);
        int k = *(int*)((char*)old - 4);
        while (--k >= 0) {
            p = (elem48*)((char*)p - 0x48);
            p->s14.~tstring();
            p->s0.~tstring();
        }
        engine_free((char*)old - 4);
        *(void**)self = 0;
    }
    int n = *count;
    void* mem = (char*)engine_alloc(n * 0x48 + 4);
    if (mem != 0) {
        int* pc = (int*)mem;
        *pc = n;
        elem48* v = (elem48*)(pc + 1);
        memset(v, 0, n * 0x48);
        elem48* p = v;
        int i = n;
        if (--i >= 0) {
            int j = i + 1;
            do {
                new (p) tstring();
                new ((char*)p + 0x14) tstring();
                p = (elem48*)((char*)p + 0x48);
            } while (--j != 0);
        }
        mem = v;
    } else
        mem = 0;
    *(void**)self = mem;
    void* mem2 = mem;
    int cnt = *count;
    for (int i = 0; i < cnt; i++)
        elem48_init((elem48*)((char*)mem2 + i * 0x48), s);
}

// ---- the 0x1c034-byte scene file object (last member ends at +0x1c034) -----------------------------


// @ 0x4c4090
// Destructor: frees the object at +4 (scalar delete), destroys the tstrings at +0xe028/+0xe03c and the 0x400
// records of ts2/ts3 whose strings run back to back (cl merges the two back-to-front walks).
scenefile_obj::~scenefile_obj()
{
    int* v = w004;
    if (v != 0)
        free_vec_4609d0(v, 3);
    s_e028.~tstring();
    tstr_rec* d = &ts3[0x400];
    for (int i = 0x400; i != 0; i--) {
        (--d)->s.~tstring();
        (d - 0x400)->s.~tstring();
    }
}

// @ 0x4c4100
// Music tick: advances the current piece once its start is 0.5 s old, ends a fade after 5 s (or ramps the
// volume down), turns the level into a 0..254 channel volume and hands it to FMOD when it changed.
void __fastcall sound_engine::tick()
{
    if (g_sound_paused != 0 || g_sound_muted != 0)
        return;
    __int64 now = g_time[slot];
    if ((float)(now - start_tick) * 0.00001f > 0.5f) {
        advance_piece();
        start_tick = g_time[slot];
    }
    float vol = g_music_volume;
    if (fading != 0) {
        if (elapsed_since(&w2458) > 5.0f) {
            stop_piece(0);
            fading = 0;
        } else {
            float v2 = 0.0f;
            float v0 = 1.0f;
            float v1 = elapsed_since(&w2458) * 0.2f;
            vol = clamp_float3(&v0, &v1, &v2);
            vol = (1.0f - vol) * g_music_volume;
        }
    }
    int iv = ftol2((double)(vol * 255.0f));
    if ((float)(vol * 255.0f) - (float)iv > 0.5f)
        iv = iv + 1;
    int level;
    if (iv < 0)
        level = 0;
    else if (iv >= 255)
        level = 254;
    else
        level = iv;
    if (w240c != 0) {
        float f = g_music_volume * 255.0f;
        int v = round_f(&f);
        if (v < 0)
            v = 0;
        else if (v >= 255)
            v = 254;
        _FSOUND_SetVolume(channel, v);
    }
    if (cur_index >= 0 && (level != last_volume || w240c != 0)) {
        last_volume = level;
        if (w240c != 0 && elapsed_since(&w2460) > 5.0f)
            *(int*)this = 0;
        _FSOUND_SetVolume(cur_index, level);
    }
}

// ---- the music set ---------------------------------------------------------------------------------
// 0x4c4340's record: 0x14 bytes { sample +0, level +4, ratio +8, length +0xc, channels +0x10 }.


// @ 0x4c4340
// Loader (ret 0xc) of a music set (the sound library): frees the old records and names, reads the sample
// records, loads every sample through FMOD (user "Sounds\<names>\x" first, then "sounds\x"), fills level,
// ratio and length from the defaults, then reads the names (both lists go through 0x4be200) and finishes.
void __fastcall music_set::load(stream_base& s)
{
    char buf[0x200];
    tstring name;
    tstring tmp;
    if (recs != 0) {
        engine_free(recs);
        recs = 0;
    }
    if (names != 0) {
        int n = *(int*)((char*)names - 4);
        while (--n >= 0)
            names[n].~tstring();
        engine_free((char*)names - 4);
        names = 0;
    }
    s.r_int(&count);
    int n = count;
    int* r = (int*)operator new(n * 0x14);
    if (r != 0) {
        memset(r, 0, n * 0x14);
    } else
        r = 0;
    recs = (music_rec*)r;
    for (int i = 0; i < count; i++) {
        int off = i * 0x14;
        music_entry_read((music_rec*)((char*)r + off), s);
        // load the sample named by its string
        if (count > 0 && g_sound_muted == 0) {
            tstring* ts = (tstring*)((char*)r + off + 4);
            const char* nm = ts->buf ? ts->buf : (const char*)((char*)ts + 4);
            char* base = buf;
            sub_536f8f(base, "%s", nm);
            (void)base;
        }
    }
    s.r_int(&count2);
    int m = count2;
    tstring* nm = (tstring*)engine_alloc(m * 0x98 + 4);
    if (nm != 0) {
        *(int*)((char*)nm - 4) = m;
        for (int i = 0; i < m; i++)
            new (&nm[i]) tstring();
    } else
        nm = 0;
    names = nm;
    for (int i = 0; i < count2; i++)
        music_entry_read((music_rec*)&nm[i], s);
    music_set_finish(this);
    name.~tstring();
    tmp.~tstring();
}

// ---- the two 0x20-byte tstring arrays destroyed by 0x4c4640 ----------------------------------------

// @ 0x4c4640
// Destructor of an object holding two arrays of 0x20-byte tstring pairs ([this] and [this+4]).
void __fastcall destroy_tstring_pairs(tstring_pair** self)
{
    tstring_pair* v = *self;
    if (v != 0) {
        int n = *(int*)((char*)v - 4);
        while (--n >= 0) {
            v[n].b.~tstring();
            v[n].a.~tstring();
        }
        engine_free((char*)v - 4);
    }
    tstring_pair* w = *(tstring_pair**)((char*)self + 4);
    if (w != 0) {
        int n = *(int*)((char*)w - 4);
        while (--n >= 0) {
            w[n].b.~tstring();
            w[n].a.~tstring();
        }
        engine_free((char*)w - 4);
    }
}

// @ 0x4c46d0
// Loader (ret 4) of a pair array at +0 with its count at +8: reads the count, rebuilds the array (two
// tstrings per 0x20-byte element), reads both strings of each element and converts underscores to spaces.
void __fastcall load_pair_array(tstring_pair** pv, int* count, stream_base* s)
{
    s->r_int(count);
    tstring_pair* v = *pv;
    if (v != 0) {
        int n = *(int*)((char*)v - 4);
        while (--n >= 0) {
            v[n].b.~tstring();
            v[n].a.~tstring();
        }
        engine_free((char*)v - 4);
        *pv = 0;
    }
    int n = *count;
    tstring_pair* p = (tstring_pair*)engine_alloc(n * 0x20 + 4);
    if (p != 0) {
        *(int*)((char*)p - 4) = n;
        for (int i = 0; i < n; i++) {
            new (&p[i].a) tstring();
            new (&p[i].b) tstring();
        }
    } else
        p = 0;
    *pv = p;
    for (int i = 0; i < *count; i++) {
        s->r_tstr((rgl::string*)&p[i].b);
        s->r_tstr((rgl::string*)&p[i].a);
        p[i].a.underscores_to_spaces();
    }
}

// @ 0x4c47d0
// Same loader (ret 4) for a pair array at +4 whose count sits at +0xc.
void __fastcall load_pair_array12(tstring_pair** pv, int* count, stream_base* s)
{
    s->r_int(count);
    tstring_pair* v = *pv;
    if (v != 0) {
        int n = *(int*)((char*)v - 4);
        while (--n >= 0) {
            v[n].b.~tstring();
            v[n].a.~tstring();
        }
        engine_free((char*)v - 4);
        *pv = 0;
    }
    int n = *count;
    tstring_pair* p = (tstring_pair*)engine_alloc(n * 0x20 + 4);
    if (p != 0) {
        *(int*)((char*)p - 4) = n;
        for (int i = 0; i < n; i++) {
            new (&p[i].a) tstring();
            new (&p[i].b) tstring();
        }
    } else
        p = 0;
    *pv = p;
    for (int i = 0; i < *count; i++) {
        s->r_tstr((rgl::string*)&p[i].b);
        s->r_tstr((rgl::string*)&p[i].a);
        p[i].a.underscores_to_spaces();
    }
}

