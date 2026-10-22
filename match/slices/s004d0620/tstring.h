// tstring: the engine's own 16-byte string with a read cursor (layout 0x42bbd0/0x42bc40; see s00477530).
#ifndef MB_TSTRING_H
#define MB_TSTRING_H

struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    ~tstring();                                 // 0x42bc40
};
#endif
