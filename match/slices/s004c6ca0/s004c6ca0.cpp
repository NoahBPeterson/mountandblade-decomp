// Slice s004c6ca0: world-map object label helpers, the map's object-target update, the object insertion
// helpers, the map-menu reset, and the map-menu dialog refresh (control 0x465: the selection list of the
// map's objects). Names provisional; offsets from the disassembly.
#include <windows.h>
#include <stdlib.h>

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
    tstring* lookup(const tstring& key) MB_NOTHROW;     // 0x43a1e0
};
extern StrTable g_strtable;                             // 0x5e2a18

// ---- a map object (a placed location / entry of the map) -------------------------------------------------
struct MapObject {
    char pad0[0x10];
    tstring name;                                       // +0x10
    tstring mesh;                                       // +0x20
    tstring* mesh_ptr();                                // 0x4c6ca0
    tstring mesh_name();                                // 0x4c6d00
};

// @ 0x4c6ca0
tstring* MapObject::mesh_ptr()
{
    tstring* r;
    {
        tstring door("_door");
        r = g_strtable.find(&(mesh + door));
    }
    return r ? r : &name;
}

// @ 0x4c6d00
tstring MapObject::mesh_name()
{
    tstring* r;
    {
        tstring door("_door");
        r = g_strtable.find(&(mesh + door));
    }
    if (r)
        return *r;
    tstring s = name;
    s.append(tstring("{***}"));
    return s;
}

// ---- the world map ---------------------------------------------------------------------------------------
struct Renderer;                                        // defined by s004c7290 (frame / map-object index)
struct gui_manager { void reset(); };                   // 0x42d1b0
extern gui_manager g_gui;                               // 0x5d7080

struct MapRecord;                                       // an entry of the map's record array (below)
struct Target {                                         // vtable slot [0x5c] = set_name
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c();
    virtual void v50(); virtual void v54(); virtual void v58();
    virtual void set_name(tstring* name, void** view);  // [0x5c]
};

struct MapRecord {                                      // strides 0x1c8 (target) / 0x204 (insert)
    char pad0[0x18];
    tstring mesh_name;                                  // +0x18
    char pad28[0x158 - 0x28];
    void* view;                                         // +0x158
    char pad15c[0x1b4 - 0x15c];
    Target* target;                                     // +0x1b4
    char pad1b8[0x1c8 - 0x1b8];
};

struct SelList {                                        // selection list (vtable slot [0x10] = get_count)
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual int get_count(void* owner);                 // [0x10]
    virtual void v14(); virtual void v18(); virtual void v1c(); virtual void v20();
    virtual void v24(); virtual void v28();
    virtual int get_sel(void* owner, int a, int value, int b, int c, int d, int* flag);  // [0x2c]
};

// objects inserted into the map: vtable + payload
struct ListObj {
    virtual ~ListObj();                                 // scalar deleting destructor at vtable[0]
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
};

struct MapScreen;
extern void ctor_4c3d90(MapRecord* base, int index, int z, MapScreen* self, int h);  // 0x4c3d90
extern void ctor_4c3e10(MapRecord* base, int index, int z, MapScreen* self, int h);  // 0x4c3e10
extern void ctor_4c3e80(MapRecord* base, int index, int z, MapScreen* self, int h);  // 0x4c3e80
extern void ctor_4c3ef0(MapRecord* base, int index, int z, MapScreen* self, int h);  // 0x4c3ef0
extern void ctor_524cd0(MapRecord* base, int index, int z, MapScreen* self, int h);  // 0x524cd0
extern void ctor_4c1b60(MapRecord* base, int index, int z, MapScreen* self, int h);  // 0x4c1b60

struct MapScreen {
    char pad0[0x8];
    void* raw;                                          // +0x8
    void* raw_end;                                      // +0xc
    void* raw_cap;                                      // +0x10
    char pad14[0x4];                                    // +0x14
    ListObj** list_start;                               // +0x18
    ListObj** list_end;                                 // +0x1c
    ListObj** list_cap;                                 // +0x20
    char pad24[0x2c - 0x24];                            // +0x24
    int f2c;                                            // +0x2c (reset by reset_state)
    char pad30[0xf4 - 0x30];                            // +0x30
    SelList* list;                                      // +0xf4
    char padf8[0x380 - 0xf8];                           // +0xf8
    HWND hwnd;                                          // +0x380
    char pad384[0x59c - 0x384];                         // +0x384
    int mode;                                           // +0x59c
    char pad5a0[0x668c - 0x5a0];                        // +0x5a0
    MapRecord* records;                                 // +0x668c (the map's record array)

    void reset_state();                                 // 0x4c6e10
    void clear_objects();                               // 0x4c6e30
    void set_object_target(int n);                      // 0x4c6dc0
    MapRecord* insert_a(MapRecord* arr, int index, int z);       // 0x4c6eb0
    int* insert_b(int* arr, int index, int z);                   // 0x4c6ee0
    void place_a(MapRecord* arr, int index, int z);              // 0x4c6f10
    void place_b(MapRecord* arr, int index, int z);              // 0x4c6f30
    void place_c(MapRecord* arr, int index, int z);              // 0x4c6f50
    void place_d(MapRecord* arr, int index, int z);              // 0x4c6f70
    void refresh_list(char use_sel);                             // 0x4c6f90
};

extern int g_dlg_sel;                                   // 0x5aa0fc (selection of the map dialog)
extern int g_map_state;                                 // 0x5aa100

// @ 0x4c6dc0
void MapScreen::set_object_target(int n)
{
    MapRecord* o = &records[n];
    if (o->target)
        o->target->set_name(&o->mesh_name, &o->view);
}

// @ 0x4c6e00
void reset_gui()
{
    g_gui.reset();
}

// @ 0x4c6e10
void MapScreen::reset_state()
{
    g_gui.reset();
    f2c = 0;
}

// @ 0x4c6e30
void MapScreen::clear_objects()
{
    for (ListObj** p = list_start; p < list_end; ++p) {
        delete *p;
        *p = 0;
    }
    if (list_start)
        free(list_start);
    list_start = 0;
    list_end = 0;
    list_cap = 0;
    if (raw)
        free(raw);
    raw = 0;
    raw_end = 0;
    raw_cap = 0;
}

// @ 0x4c6eb0
MapRecord* MapScreen::insert_a(MapRecord* arr, int index, int z)
{
    ctor_4c3d90(arr, index, z, this, z);
    return (MapRecord*)((char*)arr + index * 0x204);
}

// @ 0x4c6ee0
int* MapScreen::insert_b(int* arr, int index, int z)
{
    ctor_4c3e10((MapRecord*)arr, index, z, this, z);
    return &arr[index];
}

// @ 0x4c6f10
void MapScreen::place_a(MapRecord* arr, int index, int z)
{
    ctor_4c3e80(arr, index, z, this, z);
}

// @ 0x4c6f30
void MapScreen::place_b(MapRecord* arr, int index, int z)
{
    ctor_4c3ef0(arr, index, z, this, z);
}

// @ 0x4c6f50
void MapScreen::place_c(MapRecord* arr, int index, int z)
{
    ctor_524cd0(arr, index, z, this, z);
}

// @ 0x4c6f70
void MapScreen::place_d(MapRecord* arr, int index, int z)
{
    ctor_4c1b60(arr, index, z, this, z);
}

// @ 0x4c6f90
void MapScreen::refresh_list(char use_sel)
{
    HWND hWnd = GetDlgItem(hwnd, 0x465);
    if (!hWnd)
        return;
    SendMessageA(hWnd, 0x14b, 0, 0);                    // LB_RESETCONTENT
    int count = list->get_count(list);
    for (int i = 0; i < count; ++i) {
        int kind;
        if (use_sel) {
            kind = g_dlg_sel;
            if (kind == 0) {
                int m = this->mode;
                if (m == 0x17 || m == 0x19 || m == 0x1a || m == 0x18 || m == 0x1e)
                    kind = 0x10;
                else
                    kind = 0x20;
            }
        } else {
            int sel = SendMessageA(GetDlgItem(hwnd, 0x45f), 0x147, 0, 0);   // CB_GETCURSEL
            if (sel == 0) {
                int m = this->mode;
                if (m == 0x17 || m == 0x19 || m == 0x1a || m == 0x18 || m == 0x1e)
                    kind = 0x10;
                else
                    kind = 0x20;
            } else {
                char buf[0x80];
                SendMessageA(GetDlgItem(hwnd, 0x45f), 0x148, sel, (LPARAM)buf);   // CB_GETLBTEXT
                tstring tmp(buf);
                char c = ' ';
                int p = tmp.find(c, 0);
                tstring sub = tmp.substr(p + 1, p + 3);
                kind = atoi(sub.buf ? sub.buf : (char*)&sub.buf);
            }
        }
        int data = 0x16 + (kind == 0x10);
        {
            tstring off("ui_off");
            tstring* r = g_strtable.lookup(off);
            if (SendMessageA(hWnd, 0x14c, 0, (LPARAM)(r->buf ? r->buf : (char*)&r->buf)) < 0) {   // LB_GETTEXT
                tstring off2("ui_off");
                tstring* r2 = g_strtable.lookup(off2);
                int idx = SendMessageA(hWnd, 0x14a, 0, (LPARAM)(r2->buf ? r2->buf : (char*)&r2->buf));  // LB_ADDSTRING
                SendMessageA(hWnd, 0x151, idx, 1);                                          // LB_SETITEMDATA
            }
        }
        int flag;
        if (list->get_sel(list, i, 1, data, 0, 2, &flag) >= 0) {
            if (flag > 0) {
                if (SendMessageA(hWnd, 0x14c, 0, (LPARAM)"2x") < 0) {
                    int idx = SendMessageA(hWnd, 0x14a, 1, (LPARAM)"2x");
                    SendMessageA(hWnd, 0x151, idx, 2);
                }
            }
        }
        if (list->get_sel(list, i, 1, data, 0, 4, &flag) >= 0) {
            if (flag > 0) {
                if (SendMessageA(hWnd, 0x14c, 0, (LPARAM)"4x") < 0) {
                    int idx = SendMessageA(hWnd, 0x14a, 2, (LPARAM)"4x");
                    SendMessageA(hWnd, 0x151, idx, 4);
                }
            }
        }
    }
    int cur = 0;
    if (use_sel) {
        if (g_map_state == 2)
            cur = 1;
        else if (g_map_state == 4)
            cur = 2;
    }
    SendMessageA(hWnd, 0x14e, cur, 0);                  // LB_SETCURSEL
}
