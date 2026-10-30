// rec_tables.cpp: 0x4c2c50..0x4c48d0 (TU map 100 "c4c2c50" + 101 "c4c31c0", merged): the module/option record
// classes derived from head_rec (constructors, destructors, deleting destructors), a block reader, the options
// loader and renderer setup, the music/sound set and the tstring-pair array loaders.
// Migrated from slices s004c2b20 (0x4c2c50..0x4c3350), s004c3430, s004c3d90 and s004c46d0 (0x4c46d0, 0x4c47d0).
// 0x4c3050..0x4c48d0 is the "T regime" region (docs/tu_map.md): helpers sit right after their caller, so
// the functions stay in binary order and the helpers defined after a caller are kept out of line.
#include "./rec_tables.h"
#include "../../game/ini_file.h"      // g_game_vars (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/mat4.h"
#include "../../game/config/config.h"        // canonical: clip_slot (Entry18), music_player, InputSet, Config, loaded_snd_table, prsnt_record, sound_record
#include "../../game/render_batch.h"          // render_batch (g_batch 0x5fe898)
#include "../../include/dialog_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

// agent.h's allocator tag of the one-int record (0x436d80); declared here to avoid that header
struct int1 { int a; };
struct alloc_int1 {
    void construct(int1* p, const int1& v);   // 0x436d80
};
// rgl/core/resmgr.h's resmgr (the object at 0x5d6fa0), declared here: that header's g_log clashes with gui_manager.h's
struct resmgr {
    int load_all(int check_exists, rgl::string* root);   // 0x41c540
};

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
    pos tmp;
    v110 = 2;
    v118 = ((int*)g_ticks)[4];
    v11c = ((int*)g_ticks)[5];
    v120 = 0;
    v128 = 0;
    f124 = 6.0f;
    blk0 = *(M44*)identity_pos(&tmp);
    blk1 = *(M44*)identity_pos(&tmp);
    blk3 = *(M44*)identity_pos(&tmp);
    blk2 = *(M44*)identity_pos(&tmp);
    v100 = 0;
    v104 = 0;
}

// The class whose constructor the linker folded into clip_slot's (0x4be720): its own constructor forwards to it.
struct Entry18 : clip_slot {
    ~Entry18() { ((tstring*)&name)->~tstring(); }
};

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
        s->r_tstr((rgl::string*)&entries[i].name);
        s->r_int((int*)&entries[i].v14);
        ((tstring*)&entries[i].name)->underscores_to_spaces();   // raw_tstring is tstring's layout without the constructor (config.h)
    }
    s->r_int(&v10);
    s->r_int(&v14);
}

// The key-binding file: a bin_reader built inline on the stack (vtable 0x54ae48; the original inlines its
// constructor 0x417c50), the name made per-user by user_path, read with the file readers' shared open 0x415290.
// 0x25 bindings of two ints each, or the default table (code_table::init) when the file is missing or wrong.
static void user_path(char* name);                  // below (a local copy; its argument travels in ESI)
struct code_table { void init(); };                 // 0x4be7c0 (game/triggers/trigger_vec.cpp)
extern const char g_key_file[];                     // 0x595788 the key file's name

// @ 0x4c2ea0
void __fastcall fn_4c2ea0(void* self)
{
    bin_reader r;
    char path[0x104];
    sprintf(path, g_key_file);
    user_path(path);
    ((text_stream*)&r)->open_read(path);
    if (r.failed) {
        ((code_table*)self)->init();
    } else {
        int n = 0;
        r.r_int(&n);
        if (n != 0x25) {
            ((code_table*)self)->init();
        } else {
            int* d = (int*)self;
            for (int i = 0; i < 0x25; i++)
                for (int j = 0; j < 2; j++)
                    r.r_int(d++);
        }
        r.failed = 1;
    }
}                                                   // ~bin_reader: the two strings (inlined in the original)

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
    g_gui.f1709 = 1;
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
// The targets are dialog procedures: 0x4bf870 is Core::script_debugger_proc (a thiscall member taking the four window-procedure
// arguments, which the thunk leaves on the stack) and 0x4c03b0 is the stdcall folder_dlg_proc (it ignores ecx). The thunk calls each
// definition's own symbol through the argument-less thiscall member type the original's `mov ecx; jmp` stands for.
INT_PTR CALLBACK folder_dlg_proc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);   // 0x4c03b0 (game/triggers/trigger_vec.h)
struct EditorObj {
    void thunk();
};
extern EditorObj g_app_obj_thunk;                       // 0x8ee3c48 (the application object, g_app_obj)
union DlgThunk {
    void (EditorObj::*thunk)();
    INT_PTR (Core::*script_debugger_proc)(HWND, UINT, WPARAM, LPARAM);
    INT_PTR (CALLBACK *folder_dlg_proc)(HWND, UINT, WPARAM, LPARAM);
};

// @ 0x4c3330
void global_4bf870()
{
    DlgThunk f;
    f.script_debugger_proc = &Core::script_debugger_proc;
    ((g_app_obj_thunk).*f.thunk)();
}

// @ 0x4c3340
void script_dialog_proc()
{
    DlgThunk f;
    f.folder_dlg_proc = ::folder_dlg_proc;
    ((g_app_obj_thunk).*f.thunk)();
}

// skyboxes.txt of the module: a tstream3 text reader (vtable 0x54ac88; constructor 0x4172d0 inlined in the
// original) over <module path> + the file name; an error box when it cannot be opened; then the sky box table
// (0xa76628) reads itself from it (entry_vec_resize 0x4c2250).
struct TriggerEntryVec;
int entry_vec_resize(stream_base* s, TriggerEntryVec* v);   // 0x4c2250 (game/triggers/trigger_vec.cpp)
void mb_error(const char* fmt, ...);                // 0x42cc80
extern const char g_skyboxes_file[];                // 0x595a44
extern const char g_cant_open_fmt[];                // 0x594580
extern char g_skyboxes[];                           // 0xa76628

// @ 0x4c3350
void load_skyboxes()
{
    tstream3 r;
    tstring path(g_module_path);
    path.append(tstring(g_skyboxes_file));
    const char* nm = path.buf ? path.buf : (const char*)&path.buf;
    if (!((text_stream*)&r)->open_read_text(nm))
        mb_error(g_cant_open_fmt, nm);
    entry_vec_resize(&r, (TriggerEntryVec*)g_skyboxes);
    r.failed = 1;
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
                operator delete[](*(void**)((char*)cur + 0x34));
            RecD* r = *(RecD**)((char*)cur + 0x50);
            if (r != 0) {
                vector_destructor_iterator(r, 0x78, *(int*)((char*)r - 4), RecD_dtor);
                operator delete[]((char*)r - 4);
            }
            cur->s24.~tstring();
            cur->s10.~tstring();
            cur->s0.~tstring();
        }
        if (flags & 1)
            operator delete[](base);
        return base;
    }
    if (*(void**)((char*)this + 0x34) != 0)
        operator delete[](*(void**)((char*)this + 0x34));
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
    g_render.dev = self->device;
    g_renderer_width = self->width;
    if (self->windowed)
        g_renderer_height = self->height;
    else
        g_renderer_height = self->height;
    g_render.dev->GetDeviceCaps(&g_caps);
    g_renderer_vsync = -1;
    g_render.pad504 = 0;
    g_use_shaders = 0;
    if (((unsigned*)&g_caps)[0xc4 / 4] >= 0xfffe0200 && ((unsigned*)&g_caps)[0xcc / 4] >= 0xffff0200 &&
        ((unsigned*)&g_caps)[0xbc / 4] >= 0x10 && (((unsigned*)&g_caps)[0xd4 / 4] & 1) != 0 && g_shader_support != 0)
        g_use_shaders = 1;
    apply_input_settings((InputSet*)0xa17910);
    ((void (__fastcall*)(void*))load_game_variables)(self);
    {
        tstring k1("buffer_size_dx7_regular_ffp_dynamic");
        int v1 = (int)((ini_file_x&)g_game_vars).get_float(k1);
        tstring k2("buffer_size_dx7_regular_ffp_static");
        int v2 = (int)((ini_file_x&)g_game_vars).get_float(k2);
        tstring k3("buffer_size_normal_map_dynamic");
        int v3 = (int)((ini_file_x&)g_game_vars).get_float(k3);
        tstring k4("buffer_size_normal_map_static");
        int v4 = (int)((ini_file_x&)g_game_vars).get_float(k4);
        tstring k5("buffer_size_skinning_dynamic");
        int v5 = (int)((ini_file_x&)g_game_vars).get_float(k5);
        tstring k6("buffer_size_skinning_static");
        int v6 = (int)((ini_file_x&)g_game_vars).get_float(k6);
        tstring k7("buffer_size_regular_dynamic");
        int v7 = (int)((ini_file_x&)g_game_vars).get_float(k7);
        tstring k8("buffer_size_regular_static");
        int v8 = (int)((ini_file_x&)g_game_vars).get_float(k8);
        tstring k9("buffer_size_regular_ffp_dynamic");
        int v9 = (int)((ini_file_x&)g_game_vars).get_float(k9);
        tstring k10("buffer_size_regular_ffp_static");
        int v10 = (int)((ini_file_x&)g_game_vars).get_float(k10);
        g_batch.init_decls(&v10, &v9, &v8, &v7, &v6, &v5, &v4, &v3, &v2, &v1);
    }
    ((resmgr*)0x5d6fa0)->load_all(1, (rgl::string*)&g_module_path);
    ((InputSet*)0xa170c8)->init_devices(self->hwnd);
    return 0;
}

// The user-data path builder at 0x4bab70 takes its argument in ESI: a static helper of another object.
// Reproduced with the same body so cl picks the same register convention for the call (a local copy, not in
// the binary: the original calls 0x4bab70; see symbols/slices/rec_tables.txt).
static __declspec(noinline) void user_path(char* name)
{
    char folder[260];
    char buf[260];                                  // one scratch buffer for the directory and the name copy (0x4bab70: 0x208-byte frame)
    SHGetFolderPathA(0, CSIDL_APPDATA, 0, 0, folder);
    sprintf(buf, "%s\\Mount&Blade", folder);
    CreateDirectoryA(buf, 0);
    sprintf(buf, "%s", name);
    sprintf(name, "%s\\Mount&Blade\\%s", folder, buf);
}

// @ 0x4c3a40
// Reads options.dat (in the user folder) into the record through a binary file reader built on the stack.
void options_record::load()
{
    char path[0x104];
    opt_reader br;
    sprintf(path, "options.dat");
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
        ((Config*)this)->set_triple_buffering(mode);
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
        ((Config*)this)->set_texture_quality(w04);
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
            ((alloc_int1&)al).construct((int1*)dst, *(const int1*)arg);   // Alloc4 is the empty allocator tag; 0x436d80 is alloc_int1::construct (agent.h)
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
            ((alloc_int1&)al).construct((int1*)dst, *(const int1*)first);   // 0x436d80, as above
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
        ((prsnt_record*)&self->arr[i])->save(*s);   // prsnt_record is the 0x48-byte record (tstring +0, tstring +0x14); 0x4bdf60
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
    if ((float)(g_ticks[t2418.clock] - t2418.start) * 1e-5f > 0.5f) {
        ((music_player*)this)->advance_piece();   // the music player object; 0x4c25e0
        t2418.start = g_ticks[t2418.clock];
    }
    float vol = g_music_volume;
    if (fading != 0) {
        if ((float)(g_ticks[t2450.clock] - t2450.start) * 1e-5f > 5.0f) {
            vol = 0.0f;
            ((music_player*)this)->stop(0);   // 0x4be0a0
            fading = 0;
        } else {
            vol = (1.0f - clamp(t2450.elapsed() * 0.2f, 0.0f, 1.0f)) * vol;
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
        int v = round_float(&f);
        if (v < 0)
            v = 0;
        else if (v >= 255)
            v = 254;
        FSOUND_SetVolume(channel, v);
    }
    idx = cur_index;
    if (idx >= 0 && (level != last_volume || w240c != 0)) {
        last_volume = level;
        if (w240c != 0 && t2460.elapsed() > 5.0f)
            w0 = 0;
        FSOUND_SetVolume(idx, level);
    }
}

// ---- the music set ---------------------------------------------------------------------------------
// 0x4c4340's record: 0x14 bytes { sample +0, level +4, ratio +8, length +0xc, channels +0x10 }.


// @ 0x4c4340
// Loader (ret 0xc) of a music set: frees the old records and names, reads the sample records (name string and
// flag bits), loads every sample through FMOD ("<dir>Sounds\<name>" first, then "sounds\<name>") unless muted,
// derives priority, ratio and length from the sample defaults, then reads the name records and finishes.
int mb_fast_load(void);   // inventory_panel/module_files.cpp

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
            sprintf(buf, "%sSounds\\%s", dir->c_str(), name.c_str());
            recs[i].sample = FSOUND_Sample_Load(-1, buf, (level & 1) ? 0x2000 : 0x1000, 0, 0);
        }
        if (recs[i].sample == 0 && g_sound_muted == 0 && !mb_fast_load()) {
            sprintf(buf, "sounds\\%s", name.c_str());
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
        ((sound_record*)&names[j])->load(*s);   // music_name is the 0x98-byte sound_record (0x4be200)
    ((loaded_snd_table*)this)->load_used();   // 0x4be270 reads count at +8 and records at +0xc: music_set's count2 / names
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

