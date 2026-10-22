// tstring: the engine's rgl-style string with a read cursor (16 bytes). Copy of match/slices/s00476bb0/tstring.h
// with only what this slice needs (shared header proposal: move tstring into match/include).
#ifndef MB_TSTRING_H
#define MB_TSTRING_H

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
};
#endif
