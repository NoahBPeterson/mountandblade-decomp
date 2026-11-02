// patches/perf/stream_read_slots.cpp: cheaper scalar and string reads from the in-memory binary stream (patch set "perf").
//
// What it changes. The binary resource reader (file_reader, resmgr.h) is a mem_stream whose whole file is already in
// memory (text_stream::open_read -> rgl::string::read_from, one fread). Every scalar of a .brf (a float, an int) is then
// one virtual call to a forwarding slot -> mem_stream::r_int (0x417b10) -> rgl::string::read (0x42bf00) -> memcpy: five
// calls to copy four bytes. 148 .brf files hold ~19.7 million scalars (docs/patches.md, "perf"). These replacements do
// the same copy inline:
//   0x417b10 mem_stream::r_int   *v = *(int*)(buf + pos); pos += 4     (r_float/r_uint are forwarders to this slot)
//   0x417b30 mem_stream::r_i64   the same for 8 bytes
//   0x417af0 mem_stream::r_byte  the same for 1 byte
//   0x417a90 mem_stream::r_tstr  the length, then ONE append of the n bytes instead of n push_back calls (each of which
//                                re-checks and may regrow the string)
//
// Why it is equivalent. The faithful versions are byte-exact (not in any *.nonmatching.txt; no equivalence row needed).
// The data read, the new cursor, the returned stream and the string contents are the same for every input the original
// accepts. Differences, all unobservable: (1) r_tstr leaves the string with capacity max(growth rule, n+1) from one
// grow() instead of the sequence of per-character growths, i.e. the buffer may be a different size; (2) a negative or
// zero length leaves the string empty (as the original loop does) without the append call; (3) no function reads
// past the data any differently: like the original it does not bounds-check the cursor.
//
// Not touched: the text reader (.trf, text_stream slots), the writers.
#include <windows.h>
#include <string.h>
#include <new>
#include "../../src/rgl/rgl_string.h"
#include "../../src/game/tstring.h"       // tstring: the class that defines the rgl::string members called below
#define TS(s) ((tstring&)(s))
#include "../../src/rgl/stream.h"

// @ replaces 0x417b10
stream_base& mem_stream::r_int(int* v)
{
    int p = buf.pos;
    *v = *(const int*)(buf.buf + p);
    buf.pos = p + 4;
    return *this;
}

// @ replaces 0x417b30
stream_base& mem_stream::r_i64(__int64* v)
{
    int p = buf.pos;
    *v = *(const __int64*)(buf.buf + p);
    buf.pos = p + 8;
    return *this;
}

// @ replaces 0x417af0
stream_base& mem_stream::r_byte(char* v)
{
    int p = buf.pos;
    *v = buf.buf[p];
    buf.pos = p + 1;
    return *this;
}

// @ replaces 0x417a90
stream_base& mem_stream::r_tstr(rgl::string* v)
{
    int n;
    r_int(&n);
    v->assign("");
    if (n > 0) {
        int p = buf.pos;
        TS(*v).write(buf.buf + p, n);
        buf.pos = p + n;
    }
    return *this;
}
