// Slice s0051c720: the "creating game" state machine and the two game initialisers.
//   0x51c720  Panel::sub(counter, unused, out)  -- one step of the new-game sequence
//   0x51cc10  Game::init(name, value, flag)     -- reset the loader fields and switch to the value
//   0x51cc80  Game::reset()                     -- clear every field of a freshly allocated Game
// Names are provisional.  Globals are addressed literally so the disassembly compares directly.
#include <windows.h>
#include <stdio.h>

// ---- rgl string (16 bytes) ---------------------------------------------------------------------------
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    tstring(const tstring& o);                          // 0x42c110
    ~tstring();                                         // 0x42bc40
    tstring& operator=(const tstring& o);               // 0x42bc50
    tstring& assign(const char* s);                     // 0x42bcb0
    void clear();                                       // 0x42be50
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct StrTable {
    tstring* lookup(const tstring* key);                // 0x43a1e0 (never null)
};

struct LogStream {                                      // the engine text log at 0x5a9f60 (slot 10 = w_cstr)
    virtual void l00(); virtual void l01(); virtual void l02(); virtual void l03(); virtual void l04();
    virtual void l05(); virtual void l06(); virtual void l07(); virtual void l08(); virtual void l09();
    virtual void w_cstr(const char* s);                 // +0x28
};

// ---- the savegame object -----------------------------------------------------------------------------
struct SaveGame {                                       // 0x38 bytes
    SaveGame();                                         // 0x417c50
    ~SaveGame();                                        // 0x417c90
    bool open_read(const char* path);                   // 0x415290
    char pad0[0x38];
};

// ---- the game object (0x5e2930) ----------------------------------------------------------------------
struct Troop {                                          // 0x508 bytes, table at game+0x13a00
    tstring name;                                       // +0x00
    char pad10[0x508 - 0x10];
};

struct Game {
    char data[0x1a2b0];                                 // the object is 0x1a2b0 bytes (operator new size)
    void init(const tstring& name, int value, char flag);   // 0x51cc10
    void reset();                                       // 0x51cc80
};

void mb_error(const char* msg);                         // 0x42cc80

// ---- the loader sub-object of the panel (at panel+0x58) -----------------------------------------------
struct PanelLoader {
    tstring a;                                          // +0x00  (panel+0x58)
    tstring b;                                          // +0x10  (panel+0x68)
    char pad20[0x28 - 0x20];
    int f28;                                            // +0x28  (panel+0x80)
    int f2c;                                            // +0x2c  (panel+0x84)
    int f30;                                            // +0x30  (panel+0x88)
    int f34;                                            // +0x34  (panel+0x8c)
    void load(SaveGame* sg);                            // 0x4e54e0
};

struct PanelText {                                      // vtable slot +0xa4 = set_text(text, s)
    char pad[0xa4];
    virtual void set_text(const char* text, const tstring& s);  // +0xa4
};

struct Panel {
    char pad0[4];
    int mode;                                           // +0x04
    char pad8[0x30 - 8];
    PanelText* text;                                    // +0x30
    char pad34[0x3c - 0x34];
    int counter;                                        // +0x3c
    int f40;                                            // +0x40
    SaveGame* f44;                                      // +0x44
    tstring f48;                                        // +0x48
    PanelLoader loader;                                 // +0x58
    bool sub(int* counter, int* unused, tstring* out);  // 0x51c720
};

// ---- external thiscall helpers (declared here, never defined) -----------------------------------------
struct Ext_5236f0 { void call(int v); };                // 0x5236f0 on g_game
struct Ext_51dbd0 { void call(SaveGame* sg); };         // 0x51dbd0 on g_game
struct Ext_51f1a0 { void call(); };                     // 0x51f1a0 on g_game
struct Ext_520b20 { void call(); };                     // 0x520b20 on g_game
struct Ext_51d2b0 { void call(); };                     // 0x51d2b0 on g_game
struct Ext_520d60 { void call(); };                     // 0x520d60 on g_game
struct Ext_4be730 { void call(); };                     // 0x4be730 on 0xa73300

// @ 0x51c720
bool Panel::sub(int* counter, int* unused, tstring* out)
{
    (void)unused;
    out->assign("");
    int step = *counter;
    if (step == 0) {
        if (mode == 1) {
            SaveGame* sg = new SaveGame;
            f44 = sg;
            if (!sg->open_read(f48.c_str()))
                mb_error("Unable to open found savegame!!!");
            loader.load(f44);
            *(int*)0x5e29bc = loader.f28;
        }
        *counter = *counter + 1;
        tstring k("ui_creating_game");
        out->assign(((StrTable*)0x5e2a18)->lookup(&k)->c_str());
        return false;
    }
    if (step == 1) {
        Game* g = new Game;
        *(Game**)0x5e2930 = g;
        g->reset();
        int m = mode;
        char b = (m == 2 || m == 3);
        ((Game*)g)->init(*(tstring*)0x8ee3c38, loader.f34, b);
        *(char*)((char*)*(Game**)0x5e2930 + 0x1a05c) = 1;
        *counter = *counter + 1;
        tstring k("ui_loading_setting_data");
        out->assign(((StrTable*)0x5e2a18)->lookup(&k)->c_str());
        return false;
    }
    if (step == 2) {
        ((Ext_5236f0*)*(Game**)0x5e2930)->call(*(int*)0xa75eb8);
        if (mode == 1) {
            Game* g = *(Game**)0x5e2930;
            {
                tstring tmp(loader.b);
                *(tstring*)((char*)(*(Troop**)((char*)g + 0x13a00)) + *(int*)((char*)g + 0x8c) * 0x508) = tmp;
            }
            *(tstring*)((char*)g + 8) = loader.a;
        }
        *counter = *counter + 1;
        tstring k("ui_loading_savegame_file");
        out->assign(((StrTable*)0x5e2a18)->lookup(&k)->c_str());
        return false;
    }
    if (step == 3) {
        if (mode == 1) {
            ((Ext_51dbd0*)*(Game**)0x5e2930)->call(f44);
            *(char*)((char*)f44 + 0x34) = 1;
            if (f44) {
                delete f44;
            }
            f44 = 0;
        }
        *counter = *counter + 1;
        tstring k("ui_loading_map_file");
        out->assign(((StrTable*)0x5e2a18)->lookup(&k)->c_str());
        return false;
    }
    if (step == 4) {
        int m = mode;
        if (m == 0 || m == 2 || m == 3) {
            ((Ext_51f1a0*)*(Game**)0x5e2930)->call();
            if (m == 2 || m == 3) {
                Game* g = *(Game**)0x5e2930;
                tstring key("ui_player");
                tstring name(*((StrTable*)0x5e2a18)->lookup(&key));
                *(tstring*)((char*)(*(Troop**)((char*)g + 0x13a00)) + *(int*)((char*)g + 0x8c) * 0x508) = name;
            }
        }
        if (mode != 2 && mode != 3)
            ((Ext_520b20*)*(Game**)0x5e2930)->call();
        ((LogStream*)0x5a9f60)->w_cstr("load_map_data complete.\n");
        *counter = *counter + 1;
        tstring k("ui_initializing_map");
        out->assign(((StrTable*)0x5e2a18)->lookup(&k)->c_str());
        return false;
    }
    if (step == 5) {
        if (mode != 2 && mode != 3)
            ((Ext_51d2b0*)*(Game**)0x5e2930)->call();
        *counter = *counter + 1;
        tstring k("ui_launching_game");
        out->assign(((StrTable*)0x5e2a18)->lookup(&k)->c_str());
        ((LogStream*)0x5a9f60)->w_cstr("Init_map complete.\n");
        return false;
    }
    if (step == 6) {
        ((Ext_4be730*)0xa73300)->call();
        ((LogStream*)0x5a9f60)->w_cstr("init_meta_mission complete.\n");
        if (mode != 2 && mode != 3) {
            ((Ext_520d60*)*(Game**)0x5e2930)->call();
            *(unsigned char*)0xa73090 = 1;
        }
        ((LogStream*)0x5a9f60)->w_cstr("launch complete.\n");
        *counter = *counter + 1;
        tstring k("ui_finished");
        out->assign(((StrTable*)0x5e2a18)->lookup(&k)->c_str());
        return false;
    }
    return true;
}

// @ 0x51cc10
void Game::init(const tstring& name, int value, char flag)
{
    *(int*)((char*)this + 0x00) = value;
    *(char*)((char*)this + 0x18) = flag;
    *(tstring*)((char*)this + 0x1c) = name;
    *(int*)((char*)this + 0x8c) = 0;
    *(int*)((char*)this + 0x90) = 1;
    *(int*)((char*)this + 0x78) = 0;
    *(int*)((char*)this + 0x84) = 0;
    *(char*)((char*)this + 0x88) = 1;
    *(int*)((char*)this + 0x80) = 2;
    *(int*)((char*)this + 0x98) = 0;
    *(int*)((char*)this + 0x9c) = 0;
    *(int*)((char*)this + 0xa0) = 0;
    *(int*)((char*)this + 0xa4) = 0;
}

// @ 0x51cc80
void Game::reset()
{
    int v[4];
    char* base = (char*)this;

    *(int*)(base + 0x0000) = -1;
    ((tstring*)(base + 0x0008))->assign("");
    *(int*)(base + 0x0084) = *(int*)(base + 0x0078);
    *(int*)(base + 0x1a0d4) = 0x40000000;
    *(int*)(base + 0x1a0dc) = 0x40000000;
    *(int*)(base + 0x1a0d8) = 0x40000000;
    *(int*)(base + 0x1a0f8) = *(int*)(base + 0x1a0fc) = *(int*)(base + 0x1a100) = 0x3f800000;
    *(int*)(base + 0x1a104) = v[3];
    *(char*)(base + 0x0018) = 0;
    *(char*)(base + 0x0095) = 0;
    *(int*)(base + 0x007c) = 1;
    *(char*)(base + 0x0088) = 1;
    *(int*)(base + 0x13acc) = 0x3f800000;
    *(char*)(base + 0x13aa0) = 0;
    *(int*)(base + 0x13aa4) = 1;
    *(int*)(base + 0x14058) = 0;
    *(char*)(base + 0x13ac8) = 0;
    *(int*)(base + 0x002c) = 0;
    *(int*)(base + 0x0044) = 0;
    *(int*)(base + 0x139e4) = 0;
    *(char*)(base + 0x13a3c) = 0;
    *(int*)(base + 0x13a44) = -1;
    *(int*)(base + 0x13a40) = -1;
    *(int*)(base + 0x13b60) = -1;
    *(int*)(base + 0x13ae8) = 0;
    *(int*)(base + 0x00b4) = 1;
    *(int*)(base + 0x1a060) = 0;
    *(int*)(base + 0x1a064) = 0;
    *(int*)(base + 0x1a090) = 0;
    *(int*)(base + 0x1a094) = 0;
    *(int*)(base + 0x1a0b0) = 1;
    *(int*)(base + 0x1a0b4) = 0;
    *(int*)(base + 0x1a1d0) = 0;
    *(int*)(base + 0x1a098) = 0;
    *(int*)(base + 0x1a0cc) = 0;
    *(int*)(base + 0x1a0d0) = 0;
    *(int*)(base + 0x1a0b8) = -1;
    *(int*)(base + 0x1a0bc) = -1;
    *(char*)(base + 0x1a0c8) = 0;
    *(int*)(base + 0x1a0c0) = -1;
    *(int*)(base + 0x1a0c4) = -1;
    *(int*)(base + 0x1a160) = 0;
    *(int*)(base + 0x1a0e0) = 0x3f800000;
    *(int*)(base + 0x1a0e8) = 0x3f800000;
    *(int*)(base + 0x1a0e4) = 0x3f800000;
    *(int*)(base + 0x1a0ec) = 0x3f800000;
    *(int*)(base + 0x1a0f4) = 0x3f800000;
    *(int*)(base + 0x1a0f0) = 0x3f800000;
    *(int*)(base + 0x1a108) = 0;
    *(int*)(base + 0x1a1b4) = -1;
    {
        void* p = *(void**)(base + 0x1a1bc);
        if (p)
            ::operator delete(p);
    }
    *(int*)(base + 0x1a1bc) = 0;
    *(int*)(base + 0x1a1c0) = 0;
    *(int*)(base + 0x1a1c4) = 0;
    *(int*)(base + 0x13a34) = 1;
    *(int*)(base + 0x13af0) = -1;
    *(int*)(base + 0x13af4) = -1;
    *(int*)(base + 0x13b04) = -1;
    *(int*)(base + 0x13b08) = -1;
    *(int*)(base + 0x13b0c) = -1;
    *(int*)(base + 0x13a38) = 0x3f800000;
    *(int*)(base + 0x00ac) = -1;
    *(int*)(base + 0x1a220) = -1;
    *(int*)(base + 0x1a1c8) = 0;
    *(int*)(base + 0x1a1cc) = 0;
    *(int*)(base + 0x1a1d4) = 0;
    *(int*)(base + 0x13af8) = 0;
    *(int*)(base + 0x13afc) = 0;
    *(int*)(base + 0x13b00) = 0;
    *(int*)(base + 0x13a08) = 0;
    *(int*)(base + 0x13a0c) = -100000;
    *(int*)(base + 0x13a10) = -100000;
    *(char*)(base + 0x13a18) = 0;
    *(int*)(base + 0x13a14) = 0;
    *(int*)(base + 0x13a24) = 0;
    *(int*)(base + 0x13a28) = 0;
    *(int*)(base + 0x13a2c) = 0;
    *(int*)(base + 0x13ae0) = 0;
    *(int*)(base + 0x13ae4) = 100;
    *(int*)(base + 0x13ad0) = 0xc1200000;
    *(int*)(base + 0x13ad4) = 0;
    *(int*)(base + 0x13ad8) = 0;
    *(int*)(base + 0x13adc) = 0;
    *(char*)(base + 0x0094) = 0;
    *(int*)(base + 0x13a20) = 20;
    *(int*)(base + 0x00a8) = 0;
    *(int*)(base + 0x13b3c) = 0x3f000000;
    *(int*)(base + 0x13b40) = 0x3f000000;
    *(int*)(base + 0x13b44) = 0x42c80000;
    *(int*)(base + 0x13b48) = 0;
    *(float*)(base + 0x13b4c) = (float)*(__int64*)0x5e2948 * *(float*)0x54a6d0;
    *(int*)(base + 0x13aec) = -1;
    *(int*)(base + 0x1a1f0) = -1;
    *(int*)(base + 0x1a1d8) = 0;
    *(int*)(base + 0x1a1dc) = 0;
    *(int*)(base + 0x1a1e0) = 0;
    *(int*)(base + 0x1a1e4) = 0;
    *(int*)(base + 0x1a1e8) = 0;
    *(int*)(base + 0x1a1ec) = 0;
    *(int*)(base + 0x1a20c) = 0;
    *(int*)(base + 0x00c8) = 0;
    *(int*)(base + 0x1a208) = -1;
    *(int*)(base + 0x1a238) = 0;
    *(int*)(base + 0x1a23c) = 0;

    tstring* p = (tstring*)(base + 0x1a240);
    for (int i = 0; i < 5; i++) {
        p->clear();
        *(char*)(base + 0x1a290 + i) = 0;
        p++;
    }

    int idx = *(int*)(base + 0x13a90);
    *(int*)(base + 0x13a98) = *(int*)(0x5e2948 + idx * 8);
    *(int*)(base + 0x13a9c) = *(int*)(0x5e294c + idx * 8);
}
