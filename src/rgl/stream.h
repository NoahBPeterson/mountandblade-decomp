// stream.h: the engine's serialisation streams (one header for src/rgl/text_stream.cpp, mem_stream.cpp and
// resource_manager.cpp). Names are provisional (the slices' names, kept because symbols/slices/*.txt maps the
// vtables by them).
//
// stream_base is its own root (vtable 0x54a740, 27 slots, NO destructor slot): the stream ctors store 0x54a740
// first and never the named_object vtable 0x54a6c8 (0x417c10, 0x417c50, 0x4172d0), so the name string is a
// member of stream_base itself, not inherited from named_object (s00414960 had it the other way round).
// Layout (offsets from 0x415250, 0x417bb0, 0x417c10, 0x4172d0):
//   +0x04 name, +0x14 state, +0x18 autoflush, +0x1c flushed, +0x20 buf (text/binary buffer),
//   file streams: +0x30 fp, +0x34 failed, +0x35 is_open (writers only).
// Slot functions are folded by /OPT:ICF across the concrete classes (e.g. bin_writer's write slots are
// mem_stream's, every "return *this" slot is 0x415870); the vtables (from the binary):
//   0x54a7d0 text_stream / 0x54a890 text_stream2 (identical slots), 0x54ac88 tstream3 (text reader: write
//   slots 3-8, 11 are 0x415870), 0x54ad68 mem_stream, 0x54add8 bin_writer, 0x54ae48 bin_reader.
#ifndef RGL_STREAM_H
#define RGL_STREAM_H
#include <stdio.h>
#include "./rgl_string.h"

struct stream_base {
    rgl::string name;                          // +0x04 file name
    int state;                                 // +0x14 (1 = expect the format tag next)
    char autoflush;                            // +0x18
    int flushed;                               // +0x1c bytes of buf already written to the file
    rgl::string buf;                           // +0x20
    stream_base() { autoflush = 0; flushed = 0; }
    virtual void reset();                                  // 0  0x415220
    virtual void rewind();                                 // 1  0x415240
    virtual stream_base& w_stream(stream_base* o) = 0;     // 2
    virtual stream_base& w_float(float* v) = 0;            // 3
    virtual stream_base& w_bool(char* v) = 0;              // 4
    virtual stream_base& w_u64(unsigned __int64* v) = 0;   // 5
    virtual stream_base& w_i64(__int64* v) = 0;            // 6
    virtual stream_base& w_uint(unsigned* v) = 0;          // 7
    virtual stream_base& w_int(int* v) = 0;                // 8
    virtual stream_base& w_byte(unsigned char* v) = 0;     // 9
    virtual stream_base& w_cstr(const char* s) = 0;        // 10
    virtual stream_base& w_tstr(const rgl::string* s) = 0; // 11
    virtual stream_base& slot12(void* v) = 0;              // 12
    virtual stream_base& r_stream(stream_base* o) = 0;     // 13
    virtual stream_base& r_float(float* v) = 0;            // 14
    virtual stream_base& r_bool(char* v) = 0;              // 15
    virtual stream_base& r_uint(unsigned* v) = 0;          // 16
    virtual stream_base& r_u64(unsigned __int64* v) = 0;   // 17
    virtual stream_base& r_i64(__int64* v) = 0;            // 18
    virtual stream_base& r_int(int* v) = 0;                // 19
    virtual stream_base& r_byte(char* v) = 0;              // 20
    virtual stream_base& r_tstr(rgl::string* v) = 0;       // 21
    virtual stream_base& slot22(void* v) = 0;              // 22
    virtual bool at_end();                                 // 23 0x415250
    virtual void set_autoflush(char f);                    // 24 0x415270
    virtual char get_autoflush();                          // 25 0x415280
    virtual int flush();                                   // 26 0x408020 (returns 0; ICF-shared)
};

// The typed slots of the classes that declare all of them.
#define STREAM_SLOTS \
    virtual stream_base& w_stream(stream_base* o); \
    virtual stream_base& w_float(float* v); \
    virtual stream_base& w_bool(char* v); \
    virtual stream_base& w_u64(unsigned __int64* v); \
    virtual stream_base& w_i64(__int64* v); \
    virtual stream_base& w_uint(unsigned* v); \
    virtual stream_base& w_int(int* v); \
    virtual stream_base& w_byte(unsigned char* v); \
    virtual stream_base& w_cstr(const char* s); \
    virtual stream_base& w_tstr(const rgl::string* s); \
    virtual stream_base& slot12(void* v); \
    virtual stream_base& r_stream(stream_base* o); \
    virtual stream_base& r_float(float* v); \
    virtual stream_base& r_bool(char* v); \
    virtual stream_base& r_uint(unsigned* v); \
    virtual stream_base& r_u64(unsigned __int64* v); \
    virtual stream_base& r_i64(__int64* v); \
    virtual stream_base& r_int(int* v); \
    virtual stream_base& r_byte(char* v); \
    virtual stream_base& r_tstr(rgl::string* v); \
    virtual stream_base& slot22(void* v);

// File-backed writers keep their fields in an intermediate class.
struct file_writer_base : stream_base {
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    char is_open;                              // +0x35
    file_writer_base() { failed = 1; is_open = 0; state = 0; }
};

// ---- the text streams -------------------------------------------------------------------------------------------
// Text stream (vtable 0x54a7d0): writes values followed by a blank, reads whitespace tokens. The engine's text log and its
// derived file classes (options_panel.h's outfile_b) build on it.
// Slots: text_stream.cpp (0x4153a0..0x415880), resource_io.cpp (0x415900..0x415d60).
struct text_stream : file_writer_base {
    virtual stream_base& w_stream(stream_base* o);         // 0x415800
    virtual stream_base& w_float(float* v);                // 0x415790
    virtual stream_base& w_bool(char* v);                  // 0x415730
    virtual stream_base& w_u64(unsigned __int64* v);       // 0x415670
    virtual stream_base& w_i64(__int64* v);                // 0x415590
    virtual stream_base& w_uint(unsigned* v);              // 0x415500 (ICF with w_int)
    virtual stream_base& w_int(int* v);                    // 0x415500
    virtual stream_base& w_byte(unsigned char* v);         // 0x415470
    virtual stream_base& w_cstr(const char* s);            // 0x4153a0
    virtual stream_base& w_tstr(const rgl::string* s);     // 0x415410
    virtual stream_base& slot12(void* v);                  // 0x415870 (returns *this)
    virtual stream_base& r_stream(stream_base* o);         // 0x415cf0
    virtual stream_base& r_float(float* v);                // 0x415c80
    virtual stream_base& r_bool(char* v);                  // 0x415c10
    virtual stream_base& r_uint(unsigned* v);              // 0x415930 (ICF with r_int)
    virtual stream_base& r_u64(unsigned __int64* v);       // 0x415b60
    virtual stream_base& r_i64(__int64* v);                // 0x415a60
    virtual stream_base& r_int(int* v);                    // 0x415930
    virtual stream_base& r_byte(char* v);                  // 0x4159f0
    virtual stream_base& r_tstr(rgl::string* v);           // 0x415880
    virtual stream_base& slot22(void* v);                  // 0x415870 (ICF with slot12)
    void expect_token();                       // 0x415900 (EOF check, "Unexpected End of File ...")
    bool open_read(const char* path);          // 0x415290
    int open_write(const char* path);          // 0x415320
    int close();                               // 0x415370
    bool open_read_text(const char* path);     // 0x415d60
};

// The engine's text log (global g_log at 0x5a9f60; vtable 0x595618): a text_stream whose flush (slot 26, 0x42cc10) writes the
// unflushed tail of the buffer, and whose destructor closes the file. The export file (options_panel.h's outfile) derives from it.
struct log_stream : text_stream {
    log_stream() {}
    virtual int flush();                                            // 0x42cc10 (gui_manager.cpp)
    void shut()
    {
        if (is_open) {
            if (flush() == 0)
                fclose(fp);
            failed = 1;
            is_open = 0;
        }
    }
    ~log_stream() { shut(); }
};
extern log_stream g_log;            // 0x5a9f60

// The text reader chain (vtable 0x54a890): text_stream's slots, folded by the original's linker, each class with its own
// copy. This class has a user-declared empty destructor (0x417310); the loaders that build readers on their stack inline it
// (stream_inline.h), which leaves its vtable store in their destructors.
struct text_stream2 : stream_base {
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    virtual stream_base& w_stream(stream_base* o);         // 0x415800
    virtual stream_base& w_float(float* v);                // 0x415790
    virtual stream_base& w_bool(char* v);                  // 0x415730
    virtual stream_base& w_u64(unsigned __int64* v);       // 0x415670
    virtual stream_base& w_i64(__int64* v);                // 0x415590
    virtual stream_base& w_uint(unsigned* v);              // 0x415500 (ICF with w_int)
    virtual stream_base& w_int(int* v);                    // 0x415500
    virtual stream_base& w_byte(unsigned char* v);         // 0x415470
    virtual stream_base& w_cstr(const char* s);            // 0x4153a0
    virtual stream_base& w_tstr(const rgl::string* s);     // 0x415410
    virtual stream_base& slot12(void* v);                  // 0x415870 (returns *this)
    virtual stream_base& r_stream(stream_base* o);         // 0x415cf0
    virtual stream_base& r_float(float* v);                // 0x415c80
    virtual stream_base& r_bool(char* v);                  // 0x415c10
    virtual stream_base& r_uint(unsigned* v);              // 0x415930 (ICF with r_int)
    virtual stream_base& r_u64(unsigned __int64* v);       // 0x415b60
    virtual stream_base& r_i64(__int64* v);                // 0x415a60
    virtual stream_base& r_int(int* v);                    // 0x415930
    virtual stream_base& r_byte(char* v);                  // 0x4159f0
    virtual stream_base& r_tstr(rgl::string* v);           // 0x415880
    virtual stream_base& slot22(void* v);                  // 0x415870 (ICF with slot12)
    __forceinline text_stream2() {}
    ~text_stream2();                           // 0x417310 (stream_inline.h: the inline copy)
    void expect_token();                       // 0x415900
    bool open_read(const char* path);          // 0x415290
    bool open_read_text(const char* path);     // 0x415d60
};

// The text readers' constructor level (no virtuals of its own): the file fields' initial values are set here, which is why a
// reader's own vtable store comes after them (0x4172d0).
struct text_reader_base : text_stream2 {
    __forceinline text_reader_base() { failed = 1; fp = 0; state = 0; }
};

// Text reader (vtable 0x54ac88): text_stream2's read slots, the write slots 3-8 and 11 return *this.
struct tstream3 : text_reader_base {
    tstream3();                                // 0x4172d0 (stream_inline.h: the inline copy)
    virtual stream_base& w_float(float* v);                // 0x415870
    virtual stream_base& w_bool(char* v);                  // 0x415870
    virtual stream_base& w_u64(unsigned __int64* v);       // 0x415870
    virtual stream_base& w_i64(__int64* v);                // 0x415870
    virtual stream_base& w_uint(unsigned* v);              // 0x415870
    virtual stream_base& w_int(int* v);                    // 0x415870
    virtual stream_base& w_tstr(const rgl::string* s);     // 0x415870
};

// ---- the binary streams -----------------------------------------------------------------------------------------
// In-memory binary stream (vtable 0x54ad68): every read and write slot is real (src/rgl/mem_stream.cpp). The loaders
// that build one on their stack inline the user-declared destructor (stream_inline.h); the out-of-line copy is 0x417c90.
struct mem_stream : stream_base {
    virtual stream_base& w_stream(stream_base* o);         // 0x417930
    virtual stream_base& w_float(float* v);                // 0x4179e0 (ICF with w_int)
    virtual stream_base& w_bool(char* v);                  // 0x417a70 (ICF with w_byte)
    virtual stream_base& w_u64(unsigned __int64* v);       // 0x417a00 (ICF with w_i64)
    virtual stream_base& w_i64(__int64* v);                // 0x417a00
    virtual stream_base& w_uint(unsigned* v);              // 0x4179e0 (ICF with w_int)
    virtual stream_base& w_int(int* v);                    // 0x4179e0
    virtual stream_base& w_byte(unsigned char* v);         // 0x417a70
    virtual stream_base& w_cstr(const char* s);            // 0x417990
    virtual stream_base& w_tstr(const rgl::string* s);     // 0x417a20
    virtual stream_base& slot12(void* v);                  // 0x415870 (returns *this)
    virtual stream_base& r_stream(stream_base* o);         // 0x417b50
    virtual stream_base& r_float(float* v);                // 0x417b10 (ICF with r_int)
    virtual stream_base& r_bool(char* v);                  // 0x417af0 (ICF with r_byte)
    virtual stream_base& r_uint(unsigned* v);              // 0x417b10 (ICF with r_int)
    virtual stream_base& r_u64(unsigned __int64* v);       // 0x417b30 (ICF with r_i64)
    virtual stream_base& r_i64(__int64* v);                // 0x417b30
    virtual stream_base& r_int(int* v);                    // 0x417b10
    virtual stream_base& r_byte(char* v);                  // 0x417af0
    virtual stream_base& r_tstr(rgl::string* v);           // 0x417a90
    virtual stream_base& slot22(void* v);                  // 0x415870 (returns *this)
    __forceinline mem_stream() {}
    ~mem_stream();                             // 0x417c90 (stream_inline.h: the inline copy)
};

// The binary file reader's constructor level (no virtuals of its own, see text_reader_base).
struct bin_reader_base : mem_stream {
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    __forceinline bin_reader_base() { failed = 1; fp = 0; state = 0; }
};

// File binary writer (vtable 0x54add8): its own copies of mem_stream's write slots (the original's linker folded them), the
// read slots return *this; flush 0x417bb0. Not derived from mem_stream: its destructor does not store that vtable.
struct bin_writer : file_writer_base {
    bin_writer();                              // 0x417c10
    STREAM_SLOTS
    virtual int flush();                       // 0x417bb0
};

// File binary reader (vtable 0x54ae48): mem_stream's read slots, the write slots 3-8 and 11 return *this.
struct bin_reader : bin_reader_base {
    bin_reader();                              // 0x417c50 (stream_inline.h: the inline copy)
    virtual stream_base& w_float(float* v);                // 0x415870
    virtual stream_base& w_bool(char* v);                  // 0x415870
    virtual stream_base& w_u64(unsigned __int64* v);       // 0x415870
    virtual stream_base& w_i64(__int64* v);                // 0x415870
    virtual stream_base& w_uint(unsigned* v);              // 0x415870
    virtual stream_base& w_int(int* v);                    // 0x415870
    virtual stream_base& w_tstr(const rgl::string* s);     // 0x415870
};

#endif
