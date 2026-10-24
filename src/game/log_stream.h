// log_stream.h: the engine's text log at 0x5a9f60 as the game code calls into it (views of the canonical stream_base /
// text_stream of src/rgl/stream.h, which cannot be included with match/include/rgl.h). Slot 10 (+0x28) is w_cstr.
// One definition for party2/camera.h (the script runner's log), options_panel/world_app.cpp (0x4d3ec0 logs "Creating
// new Registry Key") and inventory_panel/map_tooltip.cpp (the tooltip's text log call): all three call slot 10 on 0x5a9f60.
#ifndef GAME_LOG_STREAM_H
#define GAME_LOG_STREAM_H
struct LogStreamBase {
    virtual void reset();
    virtual void rewind();
    virtual LogStreamBase& w_stream(void* o) = 0;
    virtual LogStreamBase& w_float(void* v) = 0;
    virtual LogStreamBase& w_bool(void* v) = 0;
    virtual LogStreamBase& w_u64(void* v) = 0;
    virtual LogStreamBase& w_i64(void* v) = 0;
    virtual LogStreamBase& w_uint(void* v) = 0;
    virtual LogStreamBase& w_int(int* v) = 0;
    virtual LogStreamBase& w_byte(void* v) = 0;
    virtual LogStreamBase& w_cstr(const char* s) = 0;
};
struct LogStream : LogStreamBase {
    virtual LogStreamBase& w_stream(void* o);
    virtual LogStreamBase& w_float(void* v);
    virtual LogStreamBase& w_bool(void* v);
    virtual LogStreamBase& w_u64(void* v);
    virtual LogStreamBase& w_i64(void* v);
    virtual LogStreamBase& w_uint(void* v);
    virtual LogStreamBase& w_int(int* v);
    virtual LogStreamBase& w_byte(void* v);
    virtual LogStreamBase& w_cstr(const char* s);
};
#endif
