// Slice s004c7290: the game's configuration / main-menu settings dialog (0x4c7290: read the render config,
// lay out the dialog's controls, localize them, load the menu background) and the map-editor object dialog's
// refresh (0x4c77c0: show the selected map object's name, position, rotation and the vertex / axis labels).
// Names provisional; offsets from the disassembly.
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define MB_NOTHROW throw()

// tstring: the engine's 16-byte string with a read cursor (layout from s00429890/tstring.h; copied locally
// because the shared include directory is not on cl.sh's include path).
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
    void append(const tstring& o);              // 0x42c1e0
    int  find(const char& c, int start) const;  // 0x42c080
    tstring substr(int start, int end) const;   // 0x42c4a0
    tstring operator+(const tstring& o) const;  // 0x42c840
};

struct StrTable {
    tstring* find(const tstring* key) MB_NOTHROW;       // 0x4c6010 (null when absent)
    tstring* lookup(const tstring& key) MB_NOTHROW;     // 0x43a1e0 (never null; buf null for a missing key)
};
extern StrTable g_strtable;                             // 0x5e2a18

// the renderer configuration as read from / written to the user's rgl_config.txt
struct RenderConfig {
    char data[0x10];                                    // (its first int: "settings were reloaded" is 0)
    RenderConfig();                                     // 0x4bc020
    void read(FILE* f);                                 // 0x4bbf70
    void apply(int fullscreen, const char* name);       // 0x4b9b90
    void write(FILE* f);                                // 0x415fb0
};
extern FILE* mb_open_user_file(const char* name, const char* mode);  // 0x415160

extern void* g_skin_name;                               // 0x5fe88c (its address stands in for an empty skin)
extern int g_show_download;                             // 0x5e29c4 (show the download button)
extern char g_skin_path[];                              // 0x595e14 (".\Skins\mb_skin_%s.smf")
extern char g_rgl_name[];                               // 0x54b048 ("rgl_config.txt")

// the settings / configuration dialog
struct ConfigDialog {
    char pad[0x364];                                    // (+0x354 is the main.bmp logo handle)
    HWND hwnd;                                          // 0x364
    void init();                                        // 0x4c7290
};

// the settings snapshot taken by the dialog: 0x230 bytes + 0x20 = a 0x250-byte frame
struct Settings {
    char data[0x230];
    RenderConfig cfg;                                   // 0x230
};

// a placed object of the map (the pawn the editor moves): its +0x58 is a 4x4 matrix
struct Pawn {
    char pad[0x58];
    float matrix[0x10];                                 // +0x58
};

extern Pawn* g_edited_object;                           // 0x5e292c+0x65e4 (the map's edited object)

// a position / direction in the world
struct Vec3 { float x, y, z; };

// the map editor's object dialog
struct MapEditorDialog {
    char pad[0x374];
    HWND hwnd;                                          // 0x374
    void refresh();                                     // 0x4c77c0
};

// a game frame's camera: +0x1a098 the edited map-object index, +0x1a09c the eye position, +0x1a0ac the
// camera's z, +0x1a0b8 the view / edit mode, +0x1a1b4 the selected record index
struct Frame {
    char pad[0x1a098];
    int object_index;                                   // 0x1a098
    Vec3 eye;                                           // 0x1a09c
    char pad1a0a8[0x1a0ac - 0x1a0a8];
    float keep;                                         // 0x1a0ac (read by the menu screen)
    char pad1a0b0[0x8];
    int mode;                                           // 0x1a0b8
    char pad1a0bc[0x1a1b4 - 0x1a0bc];
    int record;                                         // 0x1a1b4
};
extern Frame* g_frame;                                  // 0x5e2930

// a placed map object (record) of the map, stride 0x1c8: the name (0x148 bytes, its first 0x40 bytes a
// 3x3 rotation + translation), the position, the mesh name, the entry number and the var number
struct MapRecord {
    char pad0[0x18];
    char name[0x140];                                   // +0x18
    char pad158[0x30];                                  // +0x158 (matrix slot + 0x188 slot)
    int entry_no;                                       // +0x1a0
    int var_no;                                         // +0x1a4
};
extern void record_name(MapRecord* r, char* out);       // 0x4c6620

struct Map {
    char pad[0x65e4];
    Pawn* object;                                       // +0x65e4
    char pad65e8[0x668c - 0x65e8];
    MapRecord* records;                                 // +0x668c
};
extern Map* g_map;                                      // 0x5e292c

extern char g_path_prefix;                              // 0x8ee3c28 (global file-name prefix)

// the localized text of `key` (the table stores a missing key as an empty string)
inline const char* text(const char* key)
{
    tstring k(key);
    tstring* r = g_strtable.lookup(k);
    return r->buf ? r->buf : (char*)r + 4;
}

extern char g_ui_play[];                                // 0x595e0c
extern char g_ui_configure[];                           // 0x595dfc
extern char g_ui_check_for_new_version[];               // 0x595de0
extern char g_ui_download_new_settings[];               // 0x595dc4
extern char g_ui_website[];                             // 0x595db8
extern char g_ui_cancel2[];                             // 0x595dac
extern char g_ui_current_setting[];                     // 0x595d98
extern char g_bmp_main[];                               // 0x595d8c ("main.bmp")
extern char g_empty[];                                  // 0x54a480 ("")

// @ 0x4c7290
void ConfigDialog::init()
{
    Settings s;
    FILE* f = mb_open_user_file(g_rgl_name, "rb");
    if (f) {
        s.cfg.read(f);
        fclose(f);
    }
    if (*(int*)&s.cfg != 0) {
        *(int*)&s.cfg = 0;
        s.cfg.apply(1, NULL);
        f = mb_open_user_file(g_rgl_name, "wb");
        s.cfg.write(f);
        fclose(f);
    }
    SetWindowPos(hwnd, 0, 0, 0, 0x280, 0x1e0, 0x16);
    SetWindowPos(GetDlgItem(hwnd, 0x410), 0, 0x14, 0x46, 0xe6, 0x28, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x405), 0, 0x14, 0x6e, 0xe6, 0x28, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x41f), 0, 0x14, 0x96, 0xe6, 0x28, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x46b), 0, 0x14, 0xbe, 0xe6, 0x28, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x406), 0, 0x14, 0xe6, 0xe6, 0x28, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x469), 0, 0x14, 0x10e, 0xe6, 0x28, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x425), 0, 0x14, 0, 0x96, 0x46, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x420), 0, 0xb4, 0x1c, 0x19a, 0xa, 0x14);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(g_skin_path, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        SetDlgItemTextA(hwnd, 0x410, text(g_ui_play));
        SetDlgItemTextA(hwnd, 0x405, text(g_ui_configure));
        SetDlgItemTextA(hwnd, 0x41f, text(g_ui_check_for_new_version));
        SetDlgItemTextA(hwnd, 0x46b, text(g_ui_download_new_settings));
        SetDlgItemTextA(hwnd, 0x406, text(g_ui_website));
        SetDlgItemTextA(hwnd, 0x469, text(g_ui_cancel2));
        SetDlgItemTextA(hwnd, 0x425, text(g_ui_current_setting));
    } else {
        FindClose(h);
        SetDlgItemTextA(hwnd, 0x410, g_empty);
        SetDlgItemTextA(hwnd, 0x41f, g_empty);
        SetDlgItemTextA(hwnd, 0x46b, g_empty);
        SetDlgItemTextA(hwnd, 0x405, g_empty);
        SetDlgItemTextA(hwnd, 0x406, g_empty);
        SetDlgItemTextA(hwnd, 0x469, g_empty);
        SetDlgItemTextA(hwnd, 0x425, g_empty);
    }

    tstring file(&g_path_prefix);
    file.append(tstring(g_bmp_main));
    HANDLE logo = LoadImageA(0, file.buf ? file.buf : (char*)&file, 0, 0, 0, LR_LOADFROMFILE);
    if (!logo)
        logo = LoadImageA(0, g_bmp_main, 0, 0, 0, LR_LOADFROMFILE);
    if (g_show_download)
        ShowWindow(GetDlgItem(hwnd, 0x419), 5);
}

// @ 0x4c77c0
void MapEditorDialog::refresh()
{
    Frame* g = g_frame;
    MapRecord* r = 0;
    int var2_no = 0;
    char name[0xc8];
    float matrix[0x10];
    float x, y, z;
    int entry_no = 0;
    int var_no = 0;
    int mode = 0;

    if (g->object_index) {
        memcpy(matrix, g_edited_object->matrix, 0x28);
        matrix[12] += g->keep;
        mode = g->mode;
        x = g->eye.x;
        y = g->eye.y;
        z = g->eye.z;
        var_no = 0;
        strcpy(name, "New Object");
        entry_no = 0;
    } else if (g->record >= 0) {
        r = g_map->records + g->record;
        memcpy(matrix, r->name, sizeof(matrix));
        entry_no = r->entry_no;
        var_no = r->var_no;
        record_name(r, name);
    }

    if (!(g->record < 0 && g->object_index == 0)) {
        SetDlgItemTextA(hwnd, 0x439, name);
        _snprintf(name, 0x80, "%3f", matrix[0]);
        SetDlgItemTextA(hwnd, 0x435, name);
        _snprintf(name, 0x80, "%3f", matrix[1]);
        SetDlgItemTextA(hwnd, 0x436, name);
        _snprintf(name, 0x80, "%3f", matrix[2]);
        SetDlgItemTextA(hwnd, 0x437, name);
        _snprintf(name, 0x80, "%3f", x);
        SetDlgItemTextA(hwnd, 0x48b, name);
        _snprintf(name, 0x80, "%3f", y);
        SetDlgItemTextA(hwnd, 0x48c, name);
        _snprintf(name, 0x80, "%3f", z);
        SetDlgItemTextA(hwnd, 0x48d, name);
        SetDlgItemInt(hwnd, 0x43a, entry_no, 0);
        SetDlgItemInt(hwnd, 0x43b, var_no, 0);

        if (mode == 1) {
            SetWindowTextA(GetDlgItem(hwnd, 0x44e), "Entry No:");
            SetWindowTextA(GetDlgItem(hwnd, 0x44f), "");
        } else if (mode == 5) {
            SetWindowTextA(GetDlgItem(hwnd, 0x44e), "Entry No:");
            SetWindowTextA(GetDlgItem(hwnd, 0x44f), "Menu Item No:");
        } else {
            SetWindowTextA(GetDlgItem(hwnd, 0x44e), "Var No:");
            SetWindowTextA(GetDlgItem(hwnd, 0x44f), "Var2 No:");
        }
        return;
    }
    sprintf(name, "");
    SetDlgItemTextA(hwnd, 0x439, name);
    SetDlgItemTextA(hwnd, 0x435, name);
    SetDlgItemTextA(hwnd, 0x436, name);
    SetDlgItemTextA(hwnd, 0x437, name);
    SetDlgItemTextA(hwnd, 0x48b, name);
    SetDlgItemTextA(hwnd, 0x48c, name);
    SetDlgItemTextA(hwnd, 0x48d, name);
}
