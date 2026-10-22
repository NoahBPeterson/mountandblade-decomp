// text_stream.h: what src/rgl/text_stream.cpp (0x4150f0..0x4158ff) defines besides the stream and resource
// classes (stream.h, resource.h): small file helpers used across the engine and game code.
#ifndef RGL_TEXT_STREAM_H
#define RGL_TEXT_STREAM_H
#include <stdio.h>
#include <stddef.h>

void mb_snprintf(char* buf, size_t n, const char* fmt, ...);          // 0x415100 (_vsnprintf wrapper)
bool mb_file_exists(const char* path);                                // 0x415120
// fopen("<CSIDL_APPDATA>\Mount&Blade\<name>", mode), creating the directory
FILE* mb_open_user_file(const char* name, const char* mode);          // 0x415160

#endif
