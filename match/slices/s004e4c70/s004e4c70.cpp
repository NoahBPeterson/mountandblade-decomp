// Slice s004e4c70: the save/load screen's label setup and the save-slot listing.
//   0x4e4c70  label_refresh: read "<key>_text_color", "_highlight_text_color", "_position_x/y", "_size_x/y",
//             "_text_size_x/y", "_text_position_x/y" and "_text_flags" out of the ini table and push every
//             one that exists into the label widget (vtable +0x98, +0x9c, +0x58, +0x64, +0x6c, +0x70, +0x74).
//   0x4e51e0  config_entry::parse_hex(idx): sscanf(value, "%x", &out), 0 when the index is out of range
//   0x4e5230  config_entry::get_float(idx): atof(value), or the constant at 0x549ca8 (-1.0f)
//   0x4e5270  SaveWindow::list_saves(): for slots 0..8 open "sgDD.sav" and fill the slot record
//   0x4e54e0  SaveWindow::parse_slot(stream*): the slot's own reader (date words, subtype, flags)
//   0x4e5570  make_short_button(...): a "short_button" widget with the pressed variant
// The ini table is at 0xa44240: entries start at 0xa4425c (0x2c bytes each, the value string pointer at +4 --
// a null pointer means the pointer field itself holds the string) and the entry count lives at +0x2c000.
// All names provisional.
#include <windows.h>
#include <stdio.h>
#include "rgl.h"

extern "C" int _fltused = 0;

// ---- engine string (0x10 bytes) -------------------------------------------------------------------------
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0 (ret 4)
    tstring(const tstring& o);                          // 0x42c110 (ret 4)
    ~tstring();                                         // 0x42bc40
    void operator=(const tstring& o);                   // 0x42bc50 (ret 4)
    void assign(const char* s);                         // 0x42bcb0 (ret 4)
    tstring operator+(const tstring& s) const;          // 0x42c840 (ret 8)
};

// ---- the ini / config table at 0xa44240 -----------------------------------------------------------------
struct config_entry {
    char pad[0x2c];
};
struct config_table {
    char pad0[0x1c];
    char entries[0x2c000 - 0x1c];                       // +0x1c (entry i's value pointer at +0x1c + 0x2c*i)
    int count;                                          // +0x2c000
    int  find(const tstring& key);                      // 0x4bd5a0 (ret 4)
    int  get_hex(int* idx);                             // 0x4e51e0 (ret 4)
    float get_float(int* idx);                          // 0x4e5230 (ret 4)
    char* value(int idx);                               // the +0x1c/+4 pointer idiom
    char* value_str(int idx);                           // the same with the null fallback
};
extern config_table g_config;                           // 0xa44240

extern "C" char g_save_prefix[];                        // 0x8ee3c3c
extern "C" float g_minus_one;                           // 0x549ca8 (-1.0f)
extern "C" int __cdecl sscanf(const char* s, const char* fmt, ...); // 0x535fe4
extern "C" double __cdecl atof_(const char* s);         // 0x537352 (atof)

// the inline entry accessors (part of the number getters 0x4e51e0 / 0x4e5230)
static char* entry_ptr(int i)  { return (char*)&g_config + 0x1c + 0x2c * i; }
static char* entry_text(int i)
{
    char* e = entry_ptr(i);
    char* v = *(char**)(e + 4);
    return v ? v : e + 4;
}

// ---- widgets --------------------------------------------------------------------------------------------
struct vector2 {
    float x, y;
};

// the label widget (the save screen's window): only the setters this slice calls.  The slot has to match
// the engine's own numbers: +0x58 position, +0x64 scale/size, +0x6c text size, +0x70 text position,
// +0x74 flags, +0x98 / +0x9c the two colours.
struct LabelWidget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23(); virtual void v24();
    virtual void v25(); virtual void v26(); virtual void v27(); virtual void v28(); virtual void v29();
    virtual void v30(); virtual void v31(); virtual void v32(); virtual void v33(); virtual void v34();
    virtual void v35(); virtual void v36(); virtual void v37(); virtual void v38(); virtual void v39();
    virtual void v40(); virtual void v41(); virtual void v42(); virtual void v43(); virtual void v44();
    virtual void v45(); virtual void v46(); virtual void v47(); virtual void v48(); virtual void v49();
    virtual void v50(); virtual void v51(); virtual void v52(); virtual void v53(); virtual void v54();
    virtual void v55(); virtual void v56(); virtual void v57(); virtual void v58(); virtual void v59();
    virtual void v60(); virtual void v61(); virtual void v62(); virtual void v63(); virtual void v64();
    virtual void v65(); virtual void v66(); virtual void v67(); virtual void v68(); virtual void v69();
    virtual void v70(); virtual void v71(); virtual void v72(); virtual void v73(); virtual void v74();
    virtual void v75(); virtual void v76(); virtual void v77(); virtual void v78(); virtual void v79();
    virtual void v80(); virtual void v81(); virtual void v82(); virtual void v83(); virtual void v84();
    virtual void v85(); virtual void v86(); virtual void v87(); virtual void v88(); virtual void v89();
    virtual void v90(); virtual void v91(); virtual void v92(); virtual void v93(); virtual void v94();
    virtual void v95(); virtual void v96(); virtual void v97(); virtual void v98(); virtual void v99();
    virtual void v100(); virtual void v101(); virtual void v102(); virtual void v103(); virtual void v104();
    virtual void v105(); virtual void v106(); virtual void v107(); virtual void v108(); virtual void v109();
    virtual void v110(); virtual void v111(); virtual void v112(); virtual void v113(); virtual void v114();
    virtual void v115(); virtual void v116(); virtual void v117(); virtual void v118(); virtual void v119();
    virtual void v120(); virtual void v121(); virtual void v122(); virtual void v123(); virtual void v124();
    virtual void v125(); virtual void v126(); virtual void v127(); virtual void v128(); virtual void v129();
    virtual void v130(); virtual void v131(); virtual void v132(); virtual void v133(); virtual void v134();
    virtual void v135(); virtual void v136(); virtual void v137(); virtual void v138(); virtual void v139();
    virtual void v140(); virtual void v141(); virtual void v142(); virtual void v143(); virtual void v144();
    virtual void v145(); virtual void v146(); virtual void v147();
    virtual void set_pos(const vector2* v);             // +0x58
    virtual void v149(); virtual void v150();
    virtual void set_scale(const vector2* v);           // +0x64
    virtual void v152(); virtual void v153();
    virtual void set_text_size(const vector2* v);       // +0x6c
    virtual void set_text_pos(const vector2* v);        // +0x70
    virtual void set_flags(int v);                      // +0x74
    virtual void v157(); virtual void v158(); virtual void v159(); virtual void v160(); virtual void v161();
    virtual void v162(); virtual void v163(); virtual void v164(); virtual void v165(); virtual void v166();
    virtual void v167(); virtual void v168(); virtual void v169(); virtual void v170(); virtual void v171();
    virtual void v172(); virtual void v173(); virtual void v174(); virtual void v175(); virtual void v176();
    virtual void v177(); virtual void v178(); virtual void v179();
    virtual void set_color(int c);                      // +0x98
    virtual void set_highlight(int c);                  // +0x9c
};

// @ 0x4e51e0   config_entry value as int: sscanf(value, "%x", out) (0 for an invalid index)
int config_table::get_hex(int* idx)                     // 0x4e51e0
{
    int i = *idx;
    if (i < 0 || i >= count)
        return 0;
    char* e = (char*)this + 0x1c + 0x2c * i;
    char* v = *(char**)(e + 4);
    int out;
    sscanf(v ? v : e + 4, "%x", &out);
    return out;
}

// @ 0x4e5230   config_entry value as float: atof(value) (-1.0f for an invalid index)
float config_table::get_float(int* idx)                 // 0x4e5230
{
    int i = *idx;
    if (i < 0 || i >= count)
        return g_minus_one;
    char* e = (char*)this + 0x1c + 0x2c * i;
    char* v = *(char**)(e + 4);
    return atof_(v ? v : e + 4);
}

// name = prefix + field, then the ini lookup (the key object is reused for the whole block)
static bool config_find(const tstring& prefix, tstring& name, const char* field, int* idx)
{
    name = prefix + tstring(field);
    *idx = g_config.find(name);
    return *idx >= 0;
}

// @ 0x4e4c70   the label widget's config-driven refresh: "<key>_..." ini values -> widget setters
void label_refresh(LabelWidget* self, const char* key)
{
    tstring prefix(key);
    tstring name(key);
    {
        int idx;
        if (config_find(prefix, name, "_text_color", &idx))
            self->set_color(g_config.get_hex(&idx));
    }
    {
        int idx;
        if (config_find(prefix, name, "_highlight_text_color", &idx))
            self->set_highlight(g_config.get_hex(&idx));
    }
    {
        vector2 v;
        int idx_x, idx_y;
        bool any = false;
        v.x = 0.0f;
        v.y = 0.0f;
        if (config_find(prefix, name, "_position_x", &idx_x)) {
            v.x = g_config.get_float(&idx_x);
            any = true;
        }
        if (config_find(prefix, name, "_position_y", &idx_y)) {
            v.y = g_config.get_float(&idx_y);
            if (any)
                self->set_pos(&v);
        } else if (any) {
            self->set_pos(&v);
        }
    }
    {
        vector2 v;
        int idx_x, idx_y;
        bool any = false;
        v.x = 0.0f;
        v.y = 0.0f;
        if (config_find(prefix, name, "_size_x", &idx_x)) {
            v.x = g_config.get_float(&idx_x);
            any = true;
        }
        if (config_find(prefix, name, "_size_y", &idx_y)) {
            v.y = g_config.get_float(&idx_y);
            if (any)
                self->set_scale(&v);
        } else if (any) {
            self->set_scale(&v);
        }
    }
    {
        vector2 v;
        int idx_x, idx_y;
        bool any = false;
        v.x = 0.0f;
        v.y = 0.0f;
        if (config_find(prefix, name, "_text_size_x", &idx_x)) {
            v.x = g_config.get_float(&idx_x);
            any = true;
        }
        if (config_find(prefix, name, "_text_size_y", &idx_y)) {
            v.y = g_config.get_float(&idx_y);
            if (any)
                self->set_text_size(&v);
        } else if (any) {
            self->set_text_size(&v);
        }
    }
    {
        vector2 v;
        int idx_x, idx_y;
        bool any = false;
        v.x = 0.0f;
        v.y = 0.0f;
        if (config_find(prefix, name, "_text_position_x", &idx_x)) {
            v.x = g_config.get_float(&idx_x);
            any = true;
        }
        if (config_find(prefix, name, "_text_position_y", &idx_y)) {
            v.y = g_config.get_float(&idx_y);
            if (any)
                self->set_text_pos(&v);
        } else if (any) {
            self->set_text_pos(&v);
        }
    }
    {
        int idx;
        if (config_find(prefix, name, "_text_flags", &idx))
            self->set_flags(g_config.get_hex(&idx));
    }
}

// @ 0x4e5570   the save screen's small button: "short_button" with the "short_button_down" pressed state
struct button;
button* make_button(void* parent, int a, int b, int c, int d,
                    const tstring* down, const tstring* up,
                    float h, float w, float x);         // 0x4dcc50
void* make_short_button(void* parent, int a, int b, int c, int d)   // 0x4e5570
{
    tstring down("short_button_down");
    tstring up("short_button");
    return make_button(parent, a, b, c, d, &down, &up, 3.0f, 0.072f, 0.085f);
}

// the slot records at +0x4c (see s004e4790)
struct SlotRecord {
    tstring name;                                       // +0x00
    tstring date;                                       // +0x10
    int seconds;                                        // +0x20
    int year;                                           // +0x24
    int month;                                          // +0x28
    int f2c;                                            // +0x2c
};
struct Reader;
struct SaveWindow {
    tstring title;                                      // +0x00
    tstring body;                                       // +0x10
    int f20;                                            // +0x20 (the sub-reader's record: first word + 4 pad)
    int recpad;                                         // +0x24
    int year;                                           // +0x28
    int month;                                          // +0x2c
    int f30;                                            // +0x30
    char pad34[0x4c - 0x34];
    SlotRecord slots[9];                                // +0x4c
    bool is_load;                                       // +0x234
    bool read_slot(Reader* r);                          // 0x4e54e0 (ret 4)
    int  list_saves();                                  // 0x4e5270
};

extern "C" float g_time_scale;                          // 0x590e28
extern "C" char g_date_fmt[];                           // 0x598124
extern int g_save_day;                                  // 0x5e29b8
extern "C" int __cdecl sprintf(char* dst, const char* fmt, ...);    // 0x536f8f
void mb_snprintf(char* dst, unsigned cap, const char* fmt, ...);    // 0x415100
void save_path(char* out, const char* in);                          // 0x4dd7b0 (ret 4)
int grid_round(double v);                                           // 0x535a64 (x87 double -> int)

// the stream object 0x4e5270 builds (0x54ae48 reader -> 0x54ad68 mem_stream) and 0x4e54e0 reads
struct stream {
    void* vptr;                                         // +0x00
    tstring name;                                       // +0x04
    char pad14[0xc];
    tstring data;                                       // +0x20
    FILE* file;                                         // +0x30
    char failed;                                        // +0x34
    char open_read(const char* path);                   // 0x415290 (ret 4)
};
struct OldSave {                                        // what 0x4e54e0 fills
    tstring a;                                          // +0x00
    tstring b;                                          // +0x10
    int f20;                                            // +0x20
    int year;                                           // +0x24
    int f28;                                            // +0x28
    int month;                                          // +0x2c
    int f30;                                            // +0x30
};
struct Reader {                                         // the 0x54ae48 reader's virtuals
    virtual void r0(); virtual void r1(); virtual void r2(); virtual void r3(); virtual void r4();
    virtual void r5(); virtual void r6(); virtual void r7(); virtual void r8(); virtual void r9();
    virtual void r10();
    virtual void skip(int* bytes);                      // +0x2c = entry 11
    virtual void r12(); virtual void r13();
    virtual void at(int* out);                          // +0x38 = entry 14
    virtual void r15(); virtual void r16(); virtual void r17(); virtual void r18();
    virtual void get_int(int* out);                     // +0x4c = entry 19
    virtual void r25();
    virtual void* sub(void* owner);                     // +0x54 = entry 21
};
struct strtab2 {
    tstring* lookup(const tstring& name);               // 0x43a1e0
};
extern strtab2 g_strtab2;                               // 0x5e2a18
char g_save_prefix_buf[];                               // 0x8ee3c3c

// @ 0x4e54e0   bool SaveWindow::read_slot(Reader* r): read the slot's own header into this window
bool SaveWindow::read_slot(Reader* r)
{
    r->get_int(&month);                                 // +0x2c
    r->get_int(&year);                                  // +0x28
    f30 = 0;                                            // +0x30
    if (year >= 0x2c0)
        r->get_int(&f30);
    if (year > 0x3c0)
        r->sub(this);
    Reader* inner = (Reader*)r->sub((char*)this + 0x10);   // the reader hung off +0x10
    inner->get_int(&f20);                               // +0x20
    if (year <= 0x3c0)
        title = body;                                   // +0x00 = +0x10
    r->at(&recpad);                                     // +0x38 -> +0x24
    return true;
}

// @ 0x4e5270   int SaveWindow::list_saves(): fill the nine slot records from "sgDD.sav"
int SaveWindow::list_saves()
{
    tstring name;
    tstring date;
    char* p = g_save_prefix_buf[0] || 1 ? g_save_prefix_buf : g_save_prefix_buf;
    SlotRecord* slot = slots;
    for (int i = 0; i < 9; ++i, ++slot) {
        char dir[0x100];
        char full[0x200];
        sprintf(dir, "%s", p);
        save_path(dir, dir);
        mb_snprintf(full, 0x200, "%s%s\\sg%d%d.sav", dir, p, i / 10, i % 10);
        stream s;
        s.vptr = (void*)0x54ae48;
        char ok = s.open_read(full);
        slot->seconds = 0;
        if (ok) {
            if (read_slot((Reader*)&s)) {
                slot->seconds = 1;
                int mins = grid_round(*(float*)(full + 0x100) * g_time_scale);
                int hh = grid_round(*(float*)(full + 0x100)) % 0x18;
                int mm = mins % 0x3c;
                tstring key("ui_day_datedisplay");
                tstring* e = g_strtab2.lookup(key);
                char* v = *(char**)&e->buf;
                char* fmt = v ? v : (char*)&e->buf;
                int tail = 1 - grid_round(*(float*)(full + 0x100) * (double)g_date_fmt[0]);
                sprintf(date.buf ? date.buf : (char*)&date.buf, fmt, mm / 10, mm % 10, hh, tail);
                slot->name = name;
                slot->year = *(int*)(full + 0x104);
                slot->month = *(int*)(full + 0x108);
                slot->name.assign(date.buf ? date.buf : (char*)&date.buf);
                if (((OldSave*)full)->f20 != 0x52445257)
                    ((char*)&slot->f2c)[0] = 1;
                if (slot->year < 1000 || slot->year > 0x3f3)
                    ((char*)&slot->f2c)[1] = 1;
                if (slot->month < g_save_day)
                    ((char*)&slot->f2c)[2] = 1;
            }
        }
        s.vptr = (void*)0x54ad68;
    }
    return 0;
}
