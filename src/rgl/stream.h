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

// The concrete classes' typed slots.
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

// File-backed streams keep their fields in an intermediate class whose inline ctor does the init: that is
// why the derived vptr store comes last in 0x417c10 / 0x417c50 / 0x4172d0.
struct file_writer_base : stream_base {
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    char is_open;                              // +0x35
    file_writer_base() { failed = 1; is_open = 0; state = 0; }
};
struct file_reader_base : stream_base {
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    file_reader_base() { fp = 0; state = 0; failed = 1; }
};

// Text stream (vtable 0x54a7d0): writes values followed by a blank, reads whitespace tokens.
// Slots defined in src/rgl/text_stream.cpp (0x415290..0x415880) and in the next object (0x415900..).
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

// Second text stream class (vtable 0x54a890, same slots as text_stream); only its dtor is in src/rgl.
struct text_stream2 : text_stream {
    ~text_stream2();                           // 0x417310
};

// Text reader (vtable 0x54ac88): text_stream's read slots, the write slots 3-8 and 11 return *this.
struct tstream3 : file_reader_base {
    STREAM_SLOTS
    tstream3();                                // 0x4172d0
};

// In-memory binary stream (vtable 0x54ad68): every read and write slot is real (src/rgl/mem_stream.cpp).
struct mem_stream : stream_base {
    STREAM_SLOTS
    ~mem_stream();                             // 0x417c90
};

// File binary writer (vtable 0x54add8): mem_stream's write slots (ICF), flush 0x417bb0.
struct bin_writer : file_writer_base {
    bin_writer();                              // 0x417c10
    STREAM_SLOTS
    virtual int flush();                       // 0x417bb0
};

// File binary reader (vtable 0x54ae48): mem_stream's read slots (ICF).
struct bin_reader : file_reader_base {
    bin_reader();                              // 0x417c50
    STREAM_SLOTS
};

#endif
