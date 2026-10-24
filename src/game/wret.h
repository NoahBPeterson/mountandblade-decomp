// wret.h: the 8-byte non-POD result the widget virtuals set_text / set_default / get_text_size return through a hidden pointer
// (the callers never read or destroy it; the ctor is a stub of another translation unit). ONE definition for src/game.
#ifndef MB_WRET_H
#define MB_WRET_H

struct WRet { int a, b; WRet(); };

#endif // MB_WRET_H
