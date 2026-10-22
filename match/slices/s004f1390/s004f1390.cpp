// Slice s004f1390: the message log (object at 0x5d7080) — pop the oldest ring entry
// (0x4f13f0) and flush every pending message to the on-screen log (0x4f1390).
// The ring is the one of s004333c0: count, head, 32 tstrings, int[32] indices, int[32] values.
#include <windows.h>

// rgl string (16 bytes: read cursor, buffer pointer, capacity, length); s00429890 / s004dfd50
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();                                      // 0x42bbd0
    tstring(const tstring& o);                      // 0x42c110
    tstring& operator=(const tstring& o);           // 0x42bc50
    void assign(const char* s);                     // 0x42bcb0
    ~tstring();                                     // 0x42bc40
};

// message log, see s004333c0 / s004529e0 (global object at 0x5d7080)
struct MsgLog {
    char pad0[0xb0b8];
    struct Ring {
        int count;                                  // +0x00
        int head;                                   // +0x04
        tstring names[32];                          // +0x08
        int indices[32];                            // +0x208
        int vals[32];                               // +0x288
        void pop(tstring& out, int& index);         // 0x4f13f0 (this = ring at MsgLog+0xb0b8)
    } ring;                                         // +0xb0b8
    MsgLog();
    void add(tstring msg, int color);               // 0x452c00 (ret 0x14)
    void flush();                                   // 0x4f1390
};

// @ 0x4f13f0
__declspec(noinline) void MsgLog::Ring::pop(tstring& out, int& index)
{
    if (count != 0) {
        out = names[head];
        index = indices[head];
        head = (head + 1) % 32;
        count--;
        return;
    }
    out.assign("No messages");
    index = -1;
}

// @ 0x4f1390
void MsgLog::flush()
{
    tstring msg;
    int index;
    Ring* r = &ring;
    while (r->count != 0) {
        r->pop(msg, index);
        add(msg, index);
    }
}
