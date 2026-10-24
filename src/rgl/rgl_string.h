// rgl_string.h: the engine's 16-byte string with a read cursor (rgl::string), and the logging/file helpers
// that live in the same object (code at 0x42bb50..0x42cc80, not migrated yet: declarations only).
//
// Layout verified by byte-exact functions of src/rgl (one layout for every module):
//   +0  pos   read cursor            (stream_base::at_end 0x415250: pos >= len; mem_stream::w_stream 0x417930)
//   +4  buf   heap buffer or null    (c_str() below; tstring::read_from 0x4151e0 writes buf[got] = 0)
//   +8  cap
//   +0xc len                         (read_from 0x4151e0, bin_writer::flush 0x417bb0)
// match/include/rgl.h declares rgl::string as {p, size, cap, extra} with "fields unverified": that order is
// wrong (the buffer is at +4), so src/rgl does not include rgl.h and uses this header instead.
#ifndef RGL_STRING_H
#define RGL_STRING_H
#include <stdio.h>
#include <stddef.h>

namespace rgl {

struct string {
    int pos;            // +0  read cursor (token parsing: 0x42c230, 0x42c270, 0x42c350)
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc

    string();                                  // 0x42bbd0
    string(const char* s);                     // 0x42bbe0
    ~string();                                 // 0x42bc40
    string& operator=(const string& o);        // 0x42bc50
    string operator+(const string& o) const;   // 0x42c840 (hidden return pointer, ret 8)
    void reserve(unsigned n);                  // 0x42bb50
    void assign(const char* s);                // 0x42bcb0
    bool operator==(const char* s) const;      // 0x42bd10 (compares with a C string)
    bool operator!=(const char* s) const;      // 0x42bdb0 (its negation)
    bool equals(const string& o) const;        // 0x42bdd0
    void clear();                              // 0x42be50
    void push_back(char c);                    // 0x42be70
    void rewind();                             // 0x42bef0 (pos = 0)
    void read(void* dst, int n);               // 0x42bf00 copy n bytes from the cursor, advance it
    void write(const void* src, int n);        // 0x42bf30 append n bytes
    void append(const string& o);              // 0x42c1e0
    bool has_token();                          // 0x42c230 (skips blanks)
    void get_token(char* out, int* len);       // 0x42c270
    void get_string(string& out);              // 0x42c350
    void read_from(FILE* f, size_t n);         // 0x4151e0 (src/rgl/text_stream.cpp; inlined by open_read)
    // buf, or the address of the buf field itself when it is null (an empty string reads as "")
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

} // namespace rgl

// logging / file helpers of the string object
void mb_error(const char* fmt, ...);           // 0x42cc80 (logs; printf-style)
long mb_file_size(FILE* f);                    // 0x42cbc0  // old name of file_length, removed after the names merge

#endif
