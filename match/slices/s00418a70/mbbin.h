// Shared declarations for the binary (memory / file) serialisation streams of s00417850..s00418a70.
// Layouts verified by the byte-exact matches in these slices. Names provisional.
#ifndef MB_BIN_H
#define MB_BIN_H
#include <stdio.h>
#include <stddef.h>

// rgl string with a read cursor (copied from s00415c80/mbstream.h, plus the binary write at 0x42bf30).
struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                 // 0x42bbd0
    tstring(const char* s);                    // 0x42bbe0
    ~tstring();                                // 0x42bc40
    tstring& operator=(const tstring& o);      // 0x42bc50
    tstring operator+(const tstring& o) const; // 0x42c840 (hidden return pointer, ret 8)
    void assign(const char* s);                // 0x42bcb0
    bool equals(const tstring& o) const;       // 0x42bdd0
    bool operator==(const char* s) const;      // 0x42bd10 (compares with a C string; 0x42bdb0 is its negation)
    bool operator!=(const char* s) const;      // 0x42bdb0
    void push_back(char c);                    // 0x42be70
    void read(void* dst, int n);               // 0x42bf00 copy n bytes from the cursor, advance it
    void write(const void* src, int n);        // 0x42bf30 append n bytes
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// Root of the stream classes: vptr (vtable 0x54a740), name at +4. (The ctor at 0x417c10 stores this vptr
// first and no named_object vptr, so the name string is a member of stream_base itself.)
struct stream_base {
    tstring name;                              // +4
    int state;                                 // +0x14
    char autoflush;                            // +0x18
    int flushed;                               // +0x1c  bytes of buf already written to the file
    tstring buf;                               // +0x20
    stream_base() { autoflush = 0; flushed = 0; }
    virtual void reset();                      // 0x415220
    virtual void rewind();                     // 0x415240
    virtual stream_base& w_stream(stream_base* o) = 0;     // 2
    virtual stream_base& w_float(float* v) = 0;            // 3
    virtual stream_base& w_bool(char* v) = 0;              // 4
    virtual stream_base& w_u64(unsigned __int64* v) = 0;   // 5
    virtual stream_base& w_i64(__int64* v) = 0;            // 6
    virtual stream_base& w_uint(unsigned* v) = 0;          // 7
    virtual stream_base& w_int(int* v) = 0;                // 8
    virtual stream_base& w_byte(unsigned char* v) = 0;     // 9
    virtual stream_base& w_cstr(const char* s) = 0;        // 10
    virtual stream_base& w_tstr(const tstring* s) = 0;     // 11
    virtual stream_base& slot12(void* v) = 0;              // 12
    virtual stream_base& r_stream(stream_base* o) = 0;     // 13
    virtual stream_base& r_float(float* v) = 0;            // 14
    virtual stream_base& r_bool(char* v) = 0;              // 15
    virtual stream_base& r_uint(unsigned* v) = 0;          // 16
    virtual stream_base& r_u64(unsigned __int64* v) = 0;   // 17
    virtual stream_base& r_i64(__int64* v) = 0;            // 18
    virtual stream_base& r_int(int* v) = 0;                // 19
    virtual stream_base& r_byte(char* v) = 0;              // 20
    virtual stream_base& r_tstr(tstring* v) = 0;           // 21
    virtual stream_base& slot22(void* v) = 0;              // 22
    virtual bool at_end();                                 // 23
    virtual void set_autoflush(char f);                    // 24
    virtual char get_autoflush();                          // 25
    virtual int flush();                                   // 26
};

// Binary streams share the same slot set; they differ in which slots are real.
#define BIN_STREAM_SLOTS \
    virtual stream_base& w_stream(stream_base* o); \
    virtual stream_base& w_float(float* v); \
    virtual stream_base& w_bool(char* v); \
    virtual stream_base& w_u64(unsigned __int64* v); \
    virtual stream_base& w_i64(__int64* v); \
    virtual stream_base& w_uint(unsigned* v); \
    virtual stream_base& w_int(int* v); \
    virtual stream_base& w_byte(unsigned char* v); \
    virtual stream_base& w_cstr(const char* s); \
    virtual stream_base& w_tstr(const tstring* s); \
    virtual stream_base& slot12(void* v); \
    virtual stream_base& r_stream(stream_base* o); \
    virtual stream_base& r_float(float* v); \
    virtual stream_base& r_bool(char* v); \
    virtual stream_base& r_uint(unsigned* v); \
    virtual stream_base& r_u64(unsigned __int64* v); \
    virtual stream_base& r_i64(__int64* v); \
    virtual stream_base& r_int(int* v); \
    virtual stream_base& r_byte(char* v); \
    virtual stream_base& r_tstr(tstring* v); \
    virtual stream_base& slot22(void* v);

// vtable 0x54ad68: in-memory stream, all read and write slots real.
struct mem_stream : stream_base {
    BIN_STREAM_SLOTS
    ~mem_stream();                                         // 0x417c90 (body)
};

// The file-backed streams keep their fields in an intermediate class whose inline ctor does the init, which is
// why the derived vptr store comes last in 0x417c10 / 0x417c50.
struct bin_writer_base : stream_base {
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    char is_open;                              // +0x35
    bin_writer_base() { failed = 1; is_open = 0; state = 0; }
};
// vtable 0x54add8: file writer; flush slot 26 = 0x417bb0.
struct bin_writer : bin_writer_base {
    bin_writer() {}                            // 0x417c10 (inline in this slice: 0x4181c0 expands it)
    BIN_STREAM_SLOTS
    virtual int flush();
};

struct bin_reader_base : stream_base {
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    bin_reader_base() { fp = 0; state = 0; failed = 1; }
};
// vtable 0x54ae48: reader.
struct bin_reader : bin_reader_base {
    bin_reader() {}                            // 0x417c50
    BIN_STREAM_SLOTS
};

// Root of the engine's named resources (vtable 0x54a6c8: slot 0 is a pure virtual destructor, whose inline
// body stores this vptr and destroys name). Derived vtables: slot 0 deleting dtor, slot 1 = type id.
struct named_object {
    virtual ~named_object() = 0;
    tstring name;                              // +4
    named_object() {}
};
inline named_object::~named_object() {}
#endif
