// out_stream.h: the binary writer of the save files (OutStream, vtable of the SaveWriter object). ONE definition for
// src/game/char_sheet (save_game, save_state, save_records), merged from three per-slice views.
// Slot semantics (proved by the callers 0x4e8f10, 0x4e9850, 0x4e99d0, 0x4e9ab0): +0xc writes an int, +0x10 a
// pointer-sized int, +0x14 a 64-bit value (the save_records view called it w_int5), +0x18 an 8-byte element, +0x1c a
// count, +0x20 raw bytes, +0x28 a C string, +0x2c a nested structure. Every writer returns the stream (eax).
// +0x3c closes the file (0x4e8d70 caller via SaveWriter) and +0x68 opens a path (0x4e8980).
#ifndef MB_GAME_OUT_STREAM_H
#define MB_GAME_OUT_STREAM_H

struct OutStream {
    virtual OutStream& dec();                       // +0x0
    virtual OutStream& inc();                       // +0x4
    virtual OutStream& niy8();                      // +0x8
    virtual OutStream& w_int(int* p);               // +0xc
    virtual OutStream& w_int4(int* p);              // +0x10
    virtual OutStream& w_int8(int* p);              // +0x14
    virtual OutStream& w_int1(int* p);              // +0x18
    virtual OutStream& w_count(int* p);             // +0x1c
    virtual OutStream& w_raw(const void* p);        // +0x20
    virtual OutStream& niy24();                     // +0x24
    virtual OutStream& w_cstr(const char* s);       // +0x28
    virtual OutStream& w_struct(const void* p);     // +0x2c
    virtual void s0c(); virtual void s0d(); virtual void s0e();    // +0x30 .. +0x38
    virtual int  close();                           // +0x3c
    virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13();   // +0x40 ..
    virtual void s14(); virtual void s15(); virtual void s16(); virtual void s17();
    virtual void s18(); virtual void s19(); virtual void s1a(); virtual void s1b();
    virtual void s1c(); virtual void s1d(); virtual void s1e(); virtual void s1f();
    virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23();
    virtual void s24(); virtual void s25(); virtual void s26(); virtual void s27();
    virtual void s28(); virtual void s29(); virtual void s2a(); virtual void s2b();
    virtual void s2c(); virtual void s2d(); virtual void s2e(); virtual void s2f();
    virtual void s30(); virtual void s31(); virtual void s32(); virtual void s33();
    virtual void s34(); virtual void s35(); virtual void s36(); virtual void s37();
    virtual void s38(); virtual void s39(); virtual void s3a();     // .. +0x64
    virtual int  open(char* path);                  // +0x68
};

#endif // MB_GAME_OUT_STREAM_H
