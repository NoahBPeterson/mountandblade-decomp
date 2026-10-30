// stream_fwd.h: the stream classes' slot declarations, for sources that keep their own (tstring-based) copy of the stream
// root class and cannot include stream.h (it pulls rgl_string.h next to match/include/rgl.h's rgl::string). Just enough of
// text_stream / mem_stream / bin_writer for the forwarding slot bodies of stream.h's STREAM_FWD_* macros: a qualified call
// of an owner's slot compiles to a bare `jmp` to the real function. Include this INSTEAD of stream.h, never with it.
#ifndef RGL_STREAM_FWD_H
#define RGL_STREAM_FWD_H
namespace rgl { struct string; }
struct stream_base;
// A source's own stream root class declares the 21 typed slots through this (RT: its return type, STR: its string type;
// P: "= 0" for the abstract root, empty for a concrete class).
#define STREAM_SLOTS_OF(RT, STR, P) \
    virtual RT& w_stream(stream_base* o) P; virtual RT& w_float(float* v) P; \
    virtual RT& w_bool(char* v) P; virtual RT& w_u64(unsigned __int64* v) P; \
    virtual RT& w_i64(__int64* v) P; virtual RT& w_uint(unsigned* v) P; \
    virtual RT& w_int(int* v) P; virtual RT& w_byte(unsigned char* v) P; \
    virtual RT& w_cstr(const char* s) P; virtual RT& w_tstr(const STR* s) P; \
    virtual RT& slot12(void* v) P; virtual RT& r_stream(stream_base* o) P; \
    virtual RT& r_float(float* v) P; virtual RT& r_bool(char* v) P; \
    virtual RT& r_uint(unsigned* v) P; virtual RT& r_u64(unsigned __int64* v) P; \
    virtual RT& r_i64(__int64* v) P; virtual RT& r_int(int* v) P; \
    virtual RT& r_byte(char* v) P; virtual RT& r_tstr(STR* v) P; \
    virtual RT& slot22(void* v) P;
#ifndef RGL_STREAM_H
namespace rgl { struct string; }
struct stream_base {                            // the base slots every concrete stream shares (rgl/stream.h)
    virtual void reset();                       // 0x415220
    virtual void rewind();                      // 0x415240
    virtual bool at_end();                      // 0x415250
    virtual void set_autoflush(char f);         // 0x415270
    virtual char get_autoflush();               // 0x415280
    virtual int flush();                        // 0x408020
};
#define STREAM_FWD_SLOT_DECLS \
    virtual stream_base& w_stream(stream_base* o); \
    virtual stream_base& w_float(float* v); \
    virtual stream_base& w_bool(char* v); \
    virtual stream_base& w_u64(unsigned __int64* v); \
    virtual stream_base& w_i64(__int64* v); \
    virtual stream_base& w_uint(unsigned* v); \
    virtual stream_base& w_int(int* v); \
    virtual stream_base& w_byte(unsigned char* v); \
    virtual stream_base& w_cstr(const char* s); \
    virtual stream_base& w_tstr(const rgl::string* s); \
    virtual stream_base& slot12(void* v); \
    virtual stream_base& r_stream(stream_base* o); \
    virtual stream_base& r_float(float* v); \
    virtual stream_base& r_bool(char* v); \
    virtual stream_base& r_uint(unsigned* v); \
    virtual stream_base& r_u64(unsigned __int64* v); \
    virtual stream_base& r_i64(__int64* v); \
    virtual stream_base& r_int(int* v); \
    virtual stream_base& r_byte(char* v); \
    virtual stream_base& r_tstr(rgl::string* v); \
    virtual stream_base& slot22(void* v);
struct text_stream {
    STREAM_FWD_SLOT_DECLS
    bool open_read_text(const char* path);     // 0x415d60
    int open_write(const char* path);          // 0x415320
};
struct mem_stream { STREAM_FWD_SLOT_DECLS };
struct bin_writer { virtual int flush(); };    // 0x417bb0
#endif

// ---- forwarding slot bodies -------------------------------------------------------------------------------------
// The original's linker folded identical slot functions across the concrete stream classes (OPT:ICF), so most slots of
// a class's table are the text_stream / mem_stream function. A class that declares STREAM_SLOTS defines each slot it does
// not implement itself as a forwarding stub: a qualified (non-virtual) call of the owner's slot, which /O2 compiles to a
// bare `jmp` (or the same code, which folds).
// (SF_RT: the slot's return type; a source with its own copy of the stream root class defines it before the macros --
// see stream_fwd.h)
#ifndef SF_RT
#define SF_RT stream_base
#endif
#ifndef SF_STR
#define SF_STR rgl::string      // the string type of the tstr slots (a source with its own tstring-based root overrides it)
#endif
#define SF_T(C, n, A, ofn, cast) SF_RT& C::n(A a) { return (SF_RT&)((text_stream*)this)->text_stream::ofn((cast)a); }
#define SF_M(C, n, A, ofn, cast) SF_RT& C::n(A a) { return (SF_RT&)((mem_stream*)this)->mem_stream::ofn((cast)a); }
// the slots every in-memory / binary-writer style table shares: slot12 / slot22 are text_stream::slot12 (return *this)
#define SF_COMMON_RET_THIS(C) SF_T(C, slot12, void*, slot12, void*) SF_T(C, slot22, void*, slot12, void*)
// text readers (tstream3, TextReader, file_text, text_reader): text_stream's read slots, write slots 3-8 and 11 return *this
#define STREAM_FWD_TEXT_READER(C) \
    SF_T(C, w_stream, stream_base*, w_stream, stream_base*) \
    SF_T(C, w_float, float*, slot12, void*) SF_T(C, w_bool, char*, slot12, void*) \
    SF_T(C, w_u64, unsigned __int64*, slot12, void*) SF_T(C, w_i64, __int64*, slot12, void*) \
    SF_T(C, w_uint, unsigned*, slot12, void*) SF_T(C, w_int, int*, slot12, void*) \
    SF_T(C, w_byte, unsigned char*, w_byte, unsigned char*) SF_T(C, w_cstr, const char*, w_cstr, const char*) \
    SF_T(C, w_tstr, const SF_STR*, slot12, void*) SF_COMMON_RET_THIS(C) \
    SF_T(C, r_stream, stream_base*, r_stream, stream_base*) SF_T(C, r_float, float*, r_float, float*) \
    SF_T(C, r_bool, char*, r_bool, char*) SF_T(C, r_uint, unsigned*, r_int, int*) \
    SF_T(C, r_u64, unsigned __int64*, r_u64, unsigned __int64*) SF_T(C, r_i64, __int64*, r_i64, __int64*) \
    SF_T(C, r_int, int*, r_int, int*) SF_T(C, r_byte, char*, r_byte, char*) \
    SF_T(C, r_tstr, SF_STR*, r_tstr, rgl::string*)
// text streams with both directions real (the 0x54a890 level: file_text_b, text_reader_b)
#define STREAM_FWD_TEXT_BOTH(C) \
    SF_T(C, w_stream, stream_base*, w_stream, stream_base*) \
    SF_T(C, w_float, float*, w_float, float*) SF_T(C, w_bool, char*, w_bool, char*) \
    SF_T(C, w_u64, unsigned __int64*, w_u64, unsigned __int64*) SF_T(C, w_i64, __int64*, w_i64, __int64*) \
    SF_T(C, w_uint, unsigned*, w_int, int*) SF_T(C, w_int, int*, w_int, int*) \
    SF_T(C, w_byte, unsigned char*, w_byte, unsigned char*) SF_T(C, w_cstr, const char*, w_cstr, const char*) \
    SF_T(C, w_tstr, const SF_STR*, w_tstr, const rgl::string*) SF_COMMON_RET_THIS(C) \
    SF_T(C, r_stream, stream_base*, r_stream, stream_base*) SF_T(C, r_float, float*, r_float, float*) \
    SF_T(C, r_bool, char*, r_bool, char*) SF_T(C, r_uint, unsigned*, r_int, int*) \
    SF_T(C, r_u64, unsigned __int64*, r_u64, unsigned __int64*) SF_T(C, r_i64, __int64*, r_i64, __int64*) \
    SF_T(C, r_int, int*, r_int, int*) SF_T(C, r_byte, char*, r_byte, char*) \
    SF_T(C, r_tstr, SF_STR*, r_tstr, rgl::string*)
// the log file stream (log_stream, 0x595990): text_stream's write slots, r_stream and r_byte; the other read slots return *this
#define STREAM_FWD_LOG_OUT(C) \
    SF_T(C, w_stream, stream_base*, w_stream, stream_base*) \
    SF_T(C, w_float, float*, w_float, float*) SF_T(C, w_bool, char*, w_bool, char*) \
    SF_T(C, w_u64, unsigned __int64*, w_u64, unsigned __int64*) SF_T(C, w_i64, __int64*, w_i64, __int64*) \
    SF_T(C, w_uint, unsigned*, w_int, int*) SF_T(C, w_int, int*, w_int, int*) \
    SF_T(C, w_byte, unsigned char*, w_byte, unsigned char*) SF_T(C, w_cstr, const char*, w_cstr, const char*) \
    SF_T(C, w_tstr, const SF_STR*, w_tstr, const rgl::string*) SF_COMMON_RET_THIS(C) \
    SF_T(C, r_stream, stream_base*, r_stream, stream_base*) SF_T(C, r_byte, char*, r_byte, char*) \
    SF_T(C, r_float, float*, slot12, void*) SF_T(C, r_bool, char*, slot12, void*) \
    SF_T(C, r_uint, unsigned*, slot12, void*) SF_T(C, r_u64, unsigned __int64*, slot12, void*) \
    SF_T(C, r_i64, __int64*, slot12, void*) SF_T(C, r_int, int*, slot12, void*) \
    SF_T(C, r_tstr, SF_STR*, slot12, void*)
// write halves of the in-memory binary table (mem_stream's)
#define STREAM_FWD_MEM_WRITE(C) \
    SF_M(C, w_stream, stream_base*, w_stream, stream_base*) \
    SF_M(C, w_float, float*, w_int, int*) SF_M(C, w_bool, char*, w_byte, unsigned char*) \
    SF_M(C, w_u64, unsigned __int64*, w_i64, __int64*) SF_M(C, w_i64, __int64*, w_i64, __int64*) \
    SF_M(C, w_uint, unsigned*, w_int, int*) SF_M(C, w_int, int*, w_int, int*) \
    SF_M(C, w_byte, unsigned char*, w_byte, unsigned char*) SF_M(C, w_cstr, const char*, w_cstr, const char*) \
    SF_M(C, w_tstr, const SF_STR*, w_tstr, const rgl::string*)
#define STREAM_FWD_MEM_READ(C) \
    SF_M(C, r_stream, stream_base*, r_stream, stream_base*) SF_M(C, r_float, float*, r_int, int*) \
    SF_M(C, r_bool, char*, r_byte, char*) SF_M(C, r_uint, unsigned*, r_int, int*) \
    SF_M(C, r_u64, unsigned __int64*, r_i64, __int64*) SF_M(C, r_i64, __int64*, r_i64, __int64*) \
    SF_M(C, r_int, int*, r_int, int*) SF_M(C, r_byte, char*, r_byte, char*) \
    SF_M(C, r_tstr, SF_STR*, r_tstr, rgl::string*)
// the whole in-memory binary table (mem_stream_inl, opt_mem_stream, file_stream_s)
#define STREAM_FWD_MEM(C) STREAM_FWD_MEM_WRITE(C) STREAM_FWD_MEM_READ(C) SF_COMMON_RET_THIS(C)
// binary file readers (bin_reader, file_reader, opt_reader): mem_stream's read slots; the write slots are mem_stream's
// for stream/byte/cstr and return *this for the rest
#define STREAM_FWD_BIN_READER(C) \
    STREAM_FWD_MEM_READ(C) SF_COMMON_RET_THIS(C) \
    SF_M(C, w_stream, stream_base*, w_stream, stream_base*) \
    SF_T(C, w_float, float*, slot12, void*) SF_T(C, w_bool, char*, slot12, void*) \
    SF_T(C, w_u64, unsigned __int64*, slot12, void*) SF_T(C, w_i64, __int64*, slot12, void*) \
    SF_T(C, w_uint, unsigned*, slot12, void*) SF_T(C, w_int, int*, slot12, void*) \
    SF_M(C, w_byte, unsigned char*, w_byte, unsigned char*) SF_M(C, w_cstr, const char*, w_cstr, const char*) \
    SF_T(C, w_tstr, const SF_STR*, slot12, void*)
// binary file writers (bin_writer, bin_writer_i): mem_stream's write slots; read slots return *this except stream/byte
#define STREAM_FWD_BIN_WRITER(C) \
    STREAM_FWD_MEM_WRITE(C) SF_COMMON_RET_THIS(C) \
    SF_M(C, r_stream, stream_base*, r_stream, stream_base*) SF_M(C, r_byte, char*, r_byte, char*) \
    SF_T(C, r_float, float*, slot12, void*) SF_T(C, r_bool, char*, slot12, void*) \
    SF_T(C, r_uint, unsigned*, slot12, void*) SF_T(C, r_u64, unsigned __int64*, slot12, void*) \
    SF_T(C, r_i64, __int64*, slot12, void*) SF_T(C, r_int, int*, slot12, void*) \
    SF_T(C, r_tstr, SF_STR*, slot12, void*)
#endif
