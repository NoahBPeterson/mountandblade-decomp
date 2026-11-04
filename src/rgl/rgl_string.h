// rgl_string.h: the engine's 16-byte string with a read cursor (rgl::string), and the logging/file helpers
// that live in the same object (code at 0x42bb50..0x42cc80, not migrated yet: declarations only).
//
// Layout verified by byte-exact functions of src/rgl (one layout for every module):
//   +0  pos   read cursor            (stream_base::at_end 0x415250: pos >= len; mem_stream::w_stream 0x417930)
//   +4  buf   heap buffer or null    (c_str() below; tstring::read_from 0x4151e0 writes buf[got] = 0)
//   +8  cap
//   +0xc len                         (read_from 0x4151e0, bin_writer::flush 0x417bb0)
// (The slice header match/include/rgl.h still has its own ctor-only rgl::string {pos, p, cap, size} for the
// not yet migrated slices; src/ does not include it.)
#ifndef RGL_STRING_H
#define RGL_STRING_H
#include <stdio.h>
#include <stddef.h>
#include "../game/tstring.h"

// The engine has one string class: ::tstring (src/game/tstring.h); rgl::string is the same class.
namespace rgl { typedef ::tstring string; }

// logging / file helpers of the string object
void mb_error(const char* fmt, ...);           // 0x42cc80 (logs; printf-style)
long mb_file_size(FILE* f);                    // 0x42cbc0  // old name of file_length, removed after the names merge

#endif
