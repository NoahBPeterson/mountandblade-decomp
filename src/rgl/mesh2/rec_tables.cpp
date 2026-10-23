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
void Blk2dd0::read(stream_base* s)
{
    s->r_tstr((rgl::string*)&s0);
    s0.underscores_to_spaces();
    s->r_int(&count);
    if (count != 0)
        entries = new Entry18[count];
    for (int i = 0; i < count; i++) {
        s->r_int(&entries[i].v10);
        s->r_tstr((rgl::string*)&entries[i].s0);
        s->r_int(&entries[i].v14);
        entries[i].s0.underscores_to_spaces();
    }
    s->r_int(&v10);
    s->r_int(&v14);
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
// GameApp::InitDeviceObjects: hands the font object the device, publishes the window size, queries the caps,
// decides whether shaders can be used, then reads the ten buffer-size options into the renderer's buffer table.
// The original brackets two groups of statements with 18-byte WinLicense markers (jmp +0x10 over the 16-byte
// record "WL  ",12/13,0,"WL  ") that cl cannot emit from C++ (an _emit block would be an asm transcription):
// they are left out here, everything between them is the original's code.
int __fastcall renderer_setup(GameAppView* self)
{
    self->font->set_device(self->device);
    g_d3d_device = self->device;
    g_renderer_width = self->width;
    if (self->windowed)
        g_renderer_height = self->height;
    else
        g_renderer_height = self->height;
    g_d3d_device->GetDeviceCaps(&g_caps);
    g_renderer_vsync = -1;
    g_renderer_flag = 0;
    g_use_shaders = 0;
    if (((unsigned*)&g_caps)[0xc4 / 4] >= 0xfffe0200 && ((unsigned*)&g_caps)[0xcc / 4] >= 0xffff0200 &&
        ((unsigned*)&g_caps)[0xbc / 4] >= 0x10 && (((unsigned*)&g_caps)[0xd4 / 4] & 1) != 0 && g_shader_support != 0)
        g_use_shaders = 1;
    apply_input_settings((InputGlobal*)0xa17910);
    reset_buffer_group(self);
    {
        tstring k1("buffer_size_dx7_regular_ffp_dynamic");
        int v1 = (int)g_ini.get_float(k1);
        tstring k2("buffer_size_dx7_regular_ffp_static");
        int v2 = (int)g_ini.get_float(k2);
        tstring k3("buffer_size_normal_map_dynamic");
        int v3 = (int)g_ini.get_float(k3);
        tstring k4("buffer_size_normal_map_static");
        int v4 = (int)g_ini.get_float(k4);
        tstring k5("buffer_size_skinning_dynamic");
        int v5 = (int)g_ini.get_float(k5);
        tstring k6("buffer_size_skinning_static");
        int v6 = (int)g_ini.get_float(k6);
        tstring k7("buffer_size_regular_dynamic");
        int v7 = (int)g_ini.get_float(k7);
        tstring k8("buffer_size_regular_static");
        int v8 = (int)g_ini.get_float(k8);
        tstring k9("buffer_size_regular_ffp_dynamic");
        int v9 = (int)g_ini.get_float(k9);
        tstring k10("buffer_size_regular_ffp_static");
        ((RenderSetup*)0x5fe898)->init_decls((int)g_ini.get_float(k10), v9, v8, v7, v6, v5, v4, v3, v2, v1);
    }
    ((ResMgrX*)0x5d6fa0)->set_path_prefix(1, &g_path_prefix);
    ((InputDevices*)0xa170c8)->init_devices(self->hwnd);
    return 0;
}

// The user-data path builder at 0x4bab70 takes its argument in ESI: a static helper of another object.
// Reproduced with the same body so cl picks the same register convention for the call (a local copy, not in
// the binary: the original calls 0x4bab70; see symbols/slices/rec_tables.txt).
static __declspec(noinline) void user_path(char* name)
{
    char folder[260];
    char dir[260];
    char tmp[260];
    SHGetFolderPathA(0, CSIDL_APPDATA, 0, 0, folder);
    sprintf(dir, "%s\\Mount&Blade", folder);
    CreateDirectoryA(dir, 0);
    sprintf(tmp, "%s", name);
    sprintf(name, "%s\\Mount&Blade\\%s", folder, tmp);
}

// @ 0x4c3a40
// Reads options.dat (in the user folder) into the record through a binary file reader built on the stack.
void options_record::load()
{
    char path[0x104];
    opt_reader br;
    sub_536f8f(path, "options.dat");
    user_path(path);
    ((text_stream*)&br)->open_read(path);
    if (!br.failed) {
        stream_base& st = br;
        st.r_float(&f08);
        st.r_float(&f0c);
        st.r_float(&f10);
        st.r_int(&w14);
        st.r_int(&w1c);
        st.r_int(&w20);
        st.r_int(&w24);
        st.r_int(&w28);
        st.r_int(&w30);
        st.r_int(&mode);
        set_mode(mode);
        st.r_int(&w34);
        st.r_int(&w48);
        if (w48 < 0 || w48 >= 3)
            w48 = 1;
        st.r_int(&w50);
        st.r_int(&w98);
        st.r_int(&w44);
        st.r_int(&w4c);
        st.r_int(&w54);
        st.r_int(&w64);
        st.r_int(&w68);
        st.r_int(&w58);
        st.r_int(&w5c);
        st.r_float(&f74);
        st.r_int(&w6c);
        st.r_int(&w70);
        st.r_int(&w38);
        st.r_int(&w2c);
        st.r_int(&w60);
        st.r_int(&w78);
        st.r_int(&w7c);
        st.r_int(&w04);
        set_spacing(w04);
        st.r_float(&f80);
        if (!(1.4f <= f80) || !(f80 < 2.6f))
            f80 = 2.2f;
        st.r_float(&f84);
        if (!(-0.001f <= f84) || !(f84 < 1.01f))
            f84 = 0.5f;
        st.r_float(&f88);
        if (!(-0.001f <= f88) || !(f88 < 1.01f))
            f88 = 0.5f;
        st.r_int(&w18);
    } else
        ((stream_base&)g_log).w_cstr("Unable to find options.dat. Loading defaults\n");
    br.failed = 1;
}

// @ 0x4c3d90
// std::_Uninit_fill_n(first, n, val, alloc) for 0x204-byte elements; the try/catch(...) { destroy; throw; } of the
// STL is kept (cl without /EH still builds the frame for an explicit try); the elements are trivial so the
// catch path is a bare rethrow (funclet 0x4c3df7).
void copy_uninit_204(char* dst, unsigned n, const void* arg, Alloc204& al)
{
    char* first = dst;
    try {
        for (; 0 < n; --n, dst += 0x204)
            al.construct(dst, arg);
    } catch (...) {
        throw;
    }
}

// @ 0x4c3e10
// _Uninit_fill_n for 4-byte elements (funclet 0x4c3e74).
void copy_uninit_4(char* dst, unsigned n, const void* arg, Alloc4& al)
{
    char* first = dst;
    try {
        for (; 0 < n; --n, dst += 4)
            al.construct(dst, arg);
    } catch (...) {
        throw;
    }
}

// @ 0x4c3e80
// _Uninit_copy of [first, last) of 0x2c-byte elements onto dst; returns the output end (funclet 0x4c3ee6).
void* copy_uninit_2c(char* first, char* last, char* dst, Alloc2c& al)
{
    char* dest0 = dst;
    try {
        for (; first != last; dst += 0x2c, first += 0x2c)
            al.construct(dst, first);
    } catch (...) {
        throw;
    }
    return dst;
}

// @ 0x4c3ef0
// _Uninit_copy for 4-byte elements (funclet 0x4c3f56).
void* copy_uninit_4_range(char* first, char* last, char* dst, Alloc4& al)
{
    char* dest0 = dst;
    try {
        for (; first != last; dst += 4, first += 4)
            al.construct(dst, first);
    } catch (...) {
        throw;
    }
    return dst;
}

// ---- element type of the 0x48-byte array destroyed by 0x4c3f60 ------------------------------------
// A tstring at +0 and one at +0x14; 0x4bdf60 fills them from the stream after the records themselves.

// @ 0x4c3f60
// vector<elem48>::~vector(): destroys the elements back to front through the count stored before the array,
// frees the block and clears the caller's pointer.
void __fastcall destroy_elem48_array(elem48** pv)
{
    if (*pv != 0) {
        delete[] *pv;
        *pv = 0;
    }
}

// @ 0x4c3fb0
// Loader (ret 4) of an object whose array member at +0 holds 0x48-byte entries and whose count sits at +4:
// reads the count, destroys and rebuilds the array (both tstrings of every element), then initializes each
// element from the stream.
void __fastcall load_elem48_array(elem48_holder* self, int, stream_base* s)
{
    s->r_int(&self->count);
    if (self->arr) {
        delete[] self->arr;
        self->arr = 0;
    }
    self->arr = new elem48[self->count];
    for (int i = 0; i < self->count; i++)
        self->arr[i].init(s);
}

// ---- the 0x1c034-byte scene file object (last member ends at +0x1c034) -----------------------------


// @ 0x4c4090
// Destructor: frees the object at +4, then the members die in reverse order: s_e028, the 0x400 records of ts_b,
// the 0x400 records of ts_a (each record's tstring is at +0xc).
scenefile_obj::~scenefile_obj()
{
    if (w004 != 0)
        w004->vdel(3);
}

// @ 0x4c4100
// Music tick: advances the current piece once its start is 0.5 s old, ends a fade after 5 s (or ramps the
// volume down), turns the level into a 0..254 channel volume and hands it to FMOD when it changed.
void __fastcall sound_engine::tick()
{
    if (g_sound_paused != 0 || g_sound_muted != 0)
        return;
    if ((float)(g_time[t2418.slot] - t2418.start) * 1e-5f > 0.5f) {
        advance_piece();
        t2418.start = g_time[t2418.slot];
    }
    float vol = g_music_volume;
    if (fading != 0) {
        if ((float)(g_time[t2450.slot] - t2450.start) * 1e-5f > 5.0f) {
            vol = 0.0f;
            stop_piece(0);
            fading = 0;
        } else {
            vol = (1.0f - clamp_float3(t2450.elapsed() * 0.2f, 0.0f, 1.0f)) * vol;
        }
    }
    vol = vol * 255.0f;
    float fl = (float)floor((double)vol);
    int n = (int)fl;
    int iv = (vol - fl <= g_549d70) ? n : n + 1;
    int idx;
    int level = (iv < 0) ? 0 : ((iv >= 255) ? 254 : iv);
    if (w240c != 0) {
        float f = g_music_volume * 255.0f;
        int v = round_f(&f);
        if (v < 0)
            v = 0;
        else if (v >= 255)
            v = 254;
        _FSOUND_SetVolume(channel, v);
    }
    idx = cur_index;
    if (idx >= 0 && (level != last_volume || w240c != 0)) {
        last_volume = level;
        if (w240c != 0 && t2460.elapsed() > 5.0f)
            w0 = 0;
        _FSOUND_SetVolume(idx, level);
    }
}

// ---- the music set ---------------------------------------------------------------------------------
// 0x4c4340's record: 0x14 bytes { sample +0, level +4, ratio +8, length +0xc, channels +0x10 }.


// @ 0x4c4340
// Loader (ret 0xc) of a music set: frees the old records and names, reads the sample records (name string and
// flag bits), loads every sample through FMOD ("<dir>Sounds\<name>" first, then "sounds\<name>") unless muted,
// derives priority, ratio and length from the sample defaults, then reads the name records and finishes.
void music_set::load(stream_base* s, int load_samples, const tstring* dir)
{
    char buf[0x200];
    if (recs) {
        delete[] recs;
        recs = 0;
    }
    if (names) {
        delete[] names;
        names = 0;
    }
    s->r_int(&count);
    recs = new music_rec[count];
    tstring name;
    for (int i = 0; i < count; i++) {
        int level;
        s->r_tstr((rgl::string*)&name).r_int(&level);
        recs[i].sample = 0;
        if (load_samples != 0 && g_sound_muted == 0) {
            sub_536f8f(buf, "%sSounds\\%s", dir->c_str(), name.c_str());
            recs[i].sample = FSOUND_Sample_Load(-1, buf, (level & 1) ? 0x2000 : 0x1000, 0, 0);
        }
        if (recs[i].sample == 0 && g_sound_muted == 0) {
            sub_536f8f(buf, "sounds\\%s", name.c_str());
            recs[i].sample = FSOUND_Sample_Load(-1, buf, (level & 1) ? 0x2000 : 0x1000, 0, 0);
        }
        recs[i].level = level;
        FSOUND_SAMPLE* smp = recs[i].sample;
        if (smp) {
            int ch = (recs[i].level >> 4) & 0xf;
            if (ch == 0)
                ch = 5;
            FSOUND_Sample_SetDefaults(smp, -1, -1, -1, (int)((float)ch * g_590b60 * g_54ab2c));
            int len = FSOUND_Sample_GetLength(recs[i].sample);
            FSOUND_Sample_GetDefaults(recs[i].sample, &recs[i].freq, 0, 0, 0);
            recs[i].ratio = (float)len / recs[i].freq;
            recs[i].length = len;
        }
    }
    s->r_int(&count2);
    names = new music_name[count2];
    for (int j = 0; j < count2; j++)
        names[j].read(*s);
    music_set_finish(this);
}

// ---- the two 0x20-byte tstring arrays destroyed by 0x4c4640 ----------------------------------------

// @ 0x4c4640
// Destructor of an object holding two arrays of 0x20-byte tstring pairs ([this] and [this+4]).
void __fastcall destroy_tstring_pairs(tstring_pair** self)
{
    if (self[0])
        delete[] self[0];
    if (self[1])
        delete[] self[1];
}

// @ 0x4c46d0
// Loader (ret 4) of the first pair array: reads the count, rebuilds the array (two tstrings per 0x20-byte element),
// reads both strings of each element and converts the underscores of the first to spaces.
void pair_holder::load1(stream_base* s)
{
    s->r_int(&count1);
    if (arr1) {
        delete[] arr1;
        arr1 = 0;
    }
    arr1 = new tstring_pair[count1];
    for (int i = 0; i < count1; i++) {
        s->r_tstr((rgl::string*)&arr1[i].b);
        s->r_tstr((rgl::string*)&arr1[i].a);
        arr1[i].a.underscores_to_spaces();
    }
}

// @ 0x4c47d0
// Same loader for the second array (+4, count at +0xc).
void pair_holder::load2(stream_base* s)
{
    s->r_int(&count2);
    if (arr2) {
        delete[] arr2;
        arr2 = 0;
    }
    arr2 = new tstring_pair[count2];
    for (int i = 0; i < count2; i++) {
        s->r_tstr((rgl::string*)&arr2[i].b);
        s->r_tstr((rgl::string*)&arr2[i].a);
        arr2[i].a.underscores_to_spaces();
    }
}

