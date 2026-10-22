// mbbin.h: the engine's text reader streams (provisional names). Copied from s0042d360 (same layout) and
// reduced to what slice s004d0c20 needs.
#ifndef MB_BIN_H
#define MB_BIN_H
#include <stdio.h>
#include <stddef.h>
#include <string.h>

// rgl string with a read cursor (16 bytes).
struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                 // 0x42bbd0
    tstring(const char* s);                    // 0x42bbe0
    ~tstring();                                // 0x42bc40
    tstring& operator=(const tstring& o);      // 0x42bc50
    void assign(const char* s);                // 0x42bcb0
    void underscores_to_spaces();              // 0x42bf80
};

void mb_error(const char* fmt, ...);           // 0x42cc80

// Root of the stream classes: vptr (vtable 0x54a740), name at +4 (see s00416ef0/mbstream.h).
struct stream_base {
    virtual void reset();                      // slot 0
    virtual void rewind();                     // 1
    virtual stream_base& s2(stream_base* o);
    virtual stream_base& s3(float* v);
    virtual stream_base& s4(char* v);
    virtual stream_base& s5(unsigned __int64* v);
    virtual stream_base& s6(__int64* v);
    virtual stream_base& s7(unsigned* v);
    virtual stream_base& s8(int* v);
    virtual stream_base& s9(unsigned char* v);
    virtual stream_base& s10(const char* s);
    virtual stream_base& s11(const tstring* s);
    virtual stream_base& s12(void* v);
    virtual stream_base& s13(stream_base* o);
    virtual stream_base& r_float(float* v);            // 14
    virtual stream_base& r_bool(char* v);              // 15
    virtual stream_base& r_uint(unsigned* v);          // 16
    virtual stream_base& r_u64(unsigned __int64* v);   // 17
    virtual stream_base& r_i64(__int64* v);            // 18
    virtual stream_base& r_int(int* v);                // 19 (slot 0x4c)
    virtual stream_base& r_byte(char* v);              // 20 (slot 0x50)
    virtual stream_base& r_tstr(tstring* v);           // 21 (slot 0x54)
    virtual stream_base& s22(void* v);
    virtual bool at_end();
    virtual void set_autoflush(char f);
    virtual char get_autoflush();
    virtual int flush();
    tstring name;                              // +4
};

// Concrete text stream (vtable 0x54ac88). Layout from the ctor 0x4172d0 (s00416ef0).
struct text_stream : stream_base {
    int state;                                 // +0x14
    char autoflush;                            // +0x18
    int f1c;                                   // +0x1c
    tstring buf;                               // +0x20
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    bool open_read_text(const char* path);     // 0x415d60
    bool open_read(const char* path);          // 0x415290
};

// The 0x54a890 class' destructor (s00416ef0).
struct tstream2 : stream_base {
    int state;                                 // +0x14
    char autoflush;                            // +0x18
    int f1c;                                   // +0x1c
    tstring buf;                               // +0x20
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    ~tstream2();                               // 0x417310
};
#endif
