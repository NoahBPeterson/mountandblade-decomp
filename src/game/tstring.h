// tstring.h: the engine's 16-byte string (out-of-line members at 0x42bac0..0x42c900, all in one later TU).
// Shared by src/game/entity.cpp (entity::type_name returns one) and src/game/dialog_manager.cpp.
// Layout verified by the byte-exact users: s00415c80, s00418150, s0045ff30 (expand_token / expand read pos,
// buf, len directly), s004608d0. match/include/rgl.h declares the same class as rgl::string with unverified
// field names; this header is the verified layout. Names provisional (likely rgl engine code).
#ifndef GAME_TSTRING_H
#define GAME_TSTRING_H

extern int   g_str_grow_add;    // 0x5a8e60
extern float g_str_grow_mul;    // 0x5a8e64

struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4  (null while empty)
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
    void grow(unsigned n);                      // 0x42bac0 reserve n chars, keeping the contents
    void reserve(unsigned n);                   // 0x42bb50 reserve n chars, discarding the contents
    void operator=(const tstring& o);           // 0x42bc50
    void assign(const char* s);                 // 0x42bcb0
    char equals(const char* s) const;           // 0x42bd10
    bool differs(const char* s) const;          // 0x42bdb0
    char equals(const tstring& o) const;        // 0x42bdd0
    void clear();                               // 0x42be50
    void push_back(char c);                     // 0x42be70
    void erase(int start, int n);               // 0x42bea0
    void rewind();                              // 0x42bef0
    void read(void* dst, int n);                // 0x42bf00
    void write(const void* src, int n);         // 0x42bf30
    void underscores_to_spaces();               // 0x42bf80
    void spaces_to_underscores();               // 0x42bfb0
    void to_upper();                            // 0x42bfe0
    void to_lower();                            // 0x42c020
    void capitalize();                          // 0x42c060
    int  find(const char& c, int start) const;  // 0x42c080
    int  rfind(const char& c, int start, int end) const;  // 0x42c0b0
    bool range_equals(const int& start, const int& end, const tstring& o) const;   // 0x42c160
    void append(const tstring& o);              // 0x42c1e0
    bool has_token();                           // 0x42c230
    void get_token(char* out, int* n);          // 0x42c270
    void get_string(tstring& out);              // 0x42c350
    int  find(const tstring& s, int start) const;   // 0x42c440
    tstring substr(int start, int end) const;   // 0x42c4a0
    void erase_range(int start, int end);       // 0x42c560
    tstring strip_tags() const;                 // 0x42c5d0
    void trim();                                // 0x42c7a0
    tstring operator+(const tstring& o) const;  // 0x42c840
    void insert(int at, const tstring& s);      // 0x42c900
};
int min_int(const int& a, const int& b);       // 0x42c0f0

#endif // GAME_TSTRING_H
