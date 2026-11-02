// stream_inline.h: the stream classes' user-declared constructors and destructors, inline. The original defines them in
// the class (empty, or just the file fields' initial values), so every loader that builds a stream on its stack inlines
// them and the vtable stores of the constructor/destructor chain end up in the loader itself; the out-of-line copies
// (0x4172d0 tstream3, 0x417310 text_stream2, 0x417c10 bin_writer, 0x417c50 bin_reader, 0x417c90 mem_stream) exist in
// src/rgl/mem_stream.cpp and resource_manager.cpp, which do not include this header. Include it AFTER stream.h in a
// source whose original inlines them.
#ifndef RGL_STREAM_INLINE_H
#define RGL_STREAM_INLINE_H
#include "./stream.h"

inline text_stream2::~text_stream2() {}
inline mem_stream::~mem_stream() {}
__forceinline tstream3::tstream3() {}
__forceinline bin_reader::bin_reader() {}

#endif
