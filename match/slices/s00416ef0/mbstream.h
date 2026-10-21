// Shared declarations for slices s00414960 / s00415320: the engine's text (de)serialisation stream.
// Provisional names. Layouts verified by the byte-exact matches in these two slices.
#ifndef MB_STREAM_H
#define MB_STREAM_H
#include <stdio.h>
#include <stddef.h>

// rgl string with a read cursor (the engine's own class; 16 bytes). The pilot's rgl::string in rgl.h
// puts the buffer at +0; the code in this region shows the buffer at +4, a cursor at +0 and the length
// at +0xc, so it is declared locally under another name.
struct tstring {
    int pos;            // +0  read cursor (token parsing: 0x42c230, 0x42c270, 0x42c350)
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                 // 0x42bbd0
    tstring(const char* s);                    // 0x42bbe0
    ~tstring();                                // 0x42bc40
    void reserve(unsigned n);                  // 0x42bb50
    void assign(const char* s);                // 0x42bcb0
    void clear();                              // 0x42be50
    void push_back(int c);                    // 0x42be70
    bool equals(const tstring& o) const;       // 0x42bdd0
    void append(const tstring& o);             // 0x42c1e0
    void rewind();                             // 0x42bef0
    void read_from(FILE* f, size_t n)          // 0x4151e0 (inlined in 0x415d60)
    { reserve(n + 1); size_t got = fread(buf, 1, n, f); buf[got] = 0; len = got; }
    bool has_token();                          // 0x42c230 (skips blanks)
    void get_token(char* out, int* len);       // 0x42c270
    void get_string(tstring& out);             // 0x42c350
    void read(void* dst, int n);               // 0x42bf00 copy n bytes from the cursor, advance it
    tstring& operator=(const tstring& o);      // 0x42bc50
    // buf, or the address of the buf field itself when it is null
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

void mb_error(const char* fmt, ...);           // 0x42cc80
long mb_file_size(FILE* f);                    // 0x42cbc0
void mb_snprintf(char* buf, size_t n, const char* fmt, ...);   // 0x415100
int mb_parse_int(const char* s);               // 0x4159a0

// Root of the object hierarchy: vtable 0x54a6c8 (one slot, _purecall) and a name string at +4.
struct named_object {
    virtual void reset() = 0;
    tstring name;                              // +4
    ~named_object();                           // 0x4150f0
};

// Abstract stream, vtable 0x54a740 (27 slots). Each concrete stream (0x54a7d0, 0x54a890, ...) fills
// the typed read/write slots. Written values are followed by a blank, read values are whitespace tokens.
struct stream_base : named_object {
    int state;                                 // +0x14 (1 = expect the format tag next)
    char autoflush;                            // +0x18
    char pad19[7];
    tstring buf;                               // +0x20 text buffer
    virtual void reset();                      // slot 0   0x415220
    virtual void rewind();                     // slot 1   0x415240
    virtual stream_base& w_stream(stream_base* o) = 0;     // 2  0x415800
    virtual stream_base& w_float(float* v) = 0;            // 3  0x415790
    virtual stream_base& w_bool(char* v) = 0;              // 4  0x415730
    virtual stream_base& w_u64(unsigned __int64* v) = 0;   // 5  0x415670
    virtual stream_base& w_i64(__int64* v) = 0;            // 6  0x415590
    virtual stream_base& w_uint(unsigned* v) = 0;          // 7  0x415500
    virtual stream_base& w_int(int* v) = 0;                // 8  0x415500
    virtual stream_base& w_byte(unsigned char* v) = 0;     // 9  0x415470
    virtual stream_base& w_cstr(const char* s) = 0;        // 10 0x4153a0
    virtual stream_base& w_tstr(const tstring* s) = 0;     // 11 0x415410
    virtual stream_base& slot12(void* v) = 0;              // 12 0x415870
    virtual stream_base& r_stream(stream_base* o) = 0;              // 13 0x415cf0
    virtual stream_base& r_float(float* v) = 0;              // 14 0x415c80
    virtual stream_base& r_bool(char* v) = 0;              // 15 0x415c10
    virtual stream_base& r_uint(unsigned* v) = 0;          // 16 0x415930
    virtual stream_base& r_u64(unsigned __int64* v) = 0;   // 17 0x415b60
    virtual stream_base& r_i64(__int64* v) = 0;            // 18 0x415a60
    virtual stream_base& r_int(int* v) = 0;                // 19 0x415930
    virtual stream_base& r_byte(char* v) = 0;              // 20 0x4159f0
    virtual stream_base& r_tstr(tstring* v) = 0;           // 21 0x415880
    virtual stream_base& slot22(void* v) = 0;              // 22 0x415870
    virtual bool at_end();                                 // 23 0x415250
    virtual void set_autoflush(char f);                    // 24 0x415270
    virtual char get_autoflush();                          // 25 0x415280
    virtual int flush();                                   // 26 0x408020 (returns 0 in the base)
};

// Concrete text stream (vtables 0x54a7d0 and 0x54a890 share every slot): a FILE plus the text buffer.
struct text_stream : stream_base {
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    char is_open;                              // +0x35
    virtual stream_base& w_stream(stream_base* o);
    virtual stream_base& w_float(float* v);
    virtual stream_base& w_bool(char* v);
    virtual stream_base& w_u64(unsigned __int64* v);
    virtual stream_base& w_i64(__int64* v);
    virtual stream_base& w_uint(unsigned* v);
    virtual stream_base& w_int(int* v);
    virtual stream_base& w_byte(unsigned char* v);
    virtual stream_base& w_cstr(const char* s);
    virtual stream_base& w_tstr(const tstring* s);
    virtual stream_base& slot12(void* v);
    virtual stream_base& r_stream(stream_base* o);
    virtual stream_base& r_float(float* v);
    virtual stream_base& r_bool(char* v);
    virtual stream_base& r_uint(unsigned* v);
    virtual stream_base& r_u64(unsigned __int64* v);
    virtual stream_base& r_i64(__int64* v);
    virtual stream_base& r_int(int* v);
    virtual stream_base& r_byte(char* v);
    virtual stream_base& r_tstr(tstring* v);
    virtual stream_base& slot22(void* v);
    __declspec(noinline) void expect_token();   // 0x415900
    bool open_read(const char* path);          // 0x415290
    bool open_read_text(const char* path);     // 0x415d60
    int open_write(const char* path);          // 0x415320
    int close();                               // 0x415370
};
#endif
