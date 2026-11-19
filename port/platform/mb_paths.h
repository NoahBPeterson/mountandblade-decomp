// port/platform/mb_paths.h: the game-path layer. src/ names files "Data\\item_kinds1.txt", "data\\ground_specs.txt",
// ".\\Skins\\mb_skin_int.smf": backslashes and any case. Every file entry point (fopen, remove, rename, _stat,
// _findfirst, FindFirstFileA, CreateDirectoryA, GetFileAttributesA, D3DX loaders ...) goes through mbp_resolve_path.
#pragma once
#include <stddef.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
// Convert `in` to a host path: '\\' -> '/', drop "X:" drive prefixes, collapse "//" and "/./", and resolve every existing
// component case-insensitively against a cached directory index (a case-sensitive file system needs it; a
// case-insensitive one gets the exact-match fast path). Components that do not exist are kept as written (so create
// paths work). Returns 1 if the final path exists after resolution, else 0; `out` is always written.
int mbp_resolve_path(const char* in, char* out, size_t cap);
// Test hook: skip the exact-match fast path so directory-index resolution is exercised on a case-insensitive volume.
void mbp_paths_force_scan(int on);
void mbp_paths_flush_cache(void);

// fopen / remove / rename / rmdir / access through mbp_resolve_path. Text-mode read ("r", "rt") translates CRLF -> LF as
// the MSVC CRT does. mb_fopen's FILE* is a normal stdio FILE*.
FILE* mb_fopen(const char* path, const char* mode);
int mb_remove(const char* path);
int mb_rename(const char* from, const char* to);
int mb_rmdir(const char* path);
int mb_access(const char* path, int mode);
// The per-user data directory for CSIDL_* (created on demand); UTF-8 host path, no trailing slash.
int mbp_user_dir(int csidl, char* out, size_t cap);
#ifdef __cplusplus
}
#endif

// Directory enumeration shared by FindFirstFileA and _findfirst: entries sorted case-insensitively (NTFS order), "." and ".."
// first, dot-files (".DS_Store", "._x") omitted, pattern with '*' and '?' matched case-insensitively ("*.*" = everything).
#ifdef __cplusplus
extern "C" {
#endif
struct mbp_dirent { char name[260]; unsigned attrib; /* Win32 FILE_ATTRIBUTE_* */ unsigned long long size; long long ctime, atime, mtime; /* unix secs */ };
void* mbp_find_open(const char* spec, struct mbp_dirent* first);     // NULL when nothing matches (errno ENOENT)
int mbp_find_next(void* h, struct mbp_dirent* next);                  // 0 ok, -1 at the end
void mbp_find_close(void* h);
int mb_mkdir(const char* path);
#ifdef __cplusplus
}
#endif
