// port/platform/fs.cpp: the game's file layer. Paths written "Data\\x.txt" in any case resolve to host paths
// (mbp_resolve_path); FindFirstFile / CreateDirectory / SHGetFolderPath / fopen / remove / rename sit on top of it.
#include "mbp_internal.h"
#include <mmsystem.h>
#include <shlobj.h>
#include <shellapi.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <pwd.h>
#include <ctype.h>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#if !defined(__APPLE__)
#include <stdio_ext.h>
#endif

using namespace mbp;

#if defined(__APPLE__)
#define MB_MTIME_NS(sb) ((long long)(sb).st_mtimespec.tv_sec * 1000000000ll + (sb).st_mtimespec.tv_nsec)
#else
#define MB_MTIME_NS(sb) ((long long)(sb).st_mtim.tv_sec * 1000000000ll + (sb).st_mtim.tv_nsec)
#endif

namespace {
struct DirIndex {
    long long mtime_ns = 0;
    std::chrono::steady_clock::time_point built;
    std::unordered_map<std::string, std::string> lower2real;
};
std::mutex g_paths_mu;
std::map<std::string, DirIndex> g_dirs;
bool g_force_scan = false;

std::string lower(const std::string& s)
{
    std::string r = s;
    for (char& c : r) c = (char)tolower((unsigned char)c);
    return r;
}
void build_index(const std::string& dir, DirIndex& ix)
{
    ix.lower2real.clear();
    struct stat sb;
    ix.mtime_ns = stat(dir.c_str(), &sb) == 0 ? MB_MTIME_NS(sb) : 0;
    ix.built = std::chrono::steady_clock::now();
    if (DIR* d = opendir(dir.c_str())) {
        while (struct dirent* e = readdir(d)) {
            std::string l = lower(e->d_name);
            auto it = ix.lower2real.find(l);
            if (it == ix.lower2real.end()) ix.lower2real[l] = e->d_name;
        }
        closedir(d);
    }
}
// find `comp` in `dir` ignoring case; the index is rebuilt on a miss when the directory changed (or is old).
bool lookup(const std::string& dir, const std::string& comp, std::string* real)
{
    std::string d = dir.empty() ? "." : dir;
    std::string key = lower(comp);
    auto it = g_dirs.find(d);
    if (it == g_dirs.end()) {
        build_index(d, g_dirs[d]);
        it = g_dirs.find(d);
    }
    auto hit = it->second.lower2real.find(key);
    if (hit != it->second.lower2real.end()) { *real = hit->second; return true; }
    struct stat sb;
    long long mt = stat(d.c_str(), &sb) == 0 ? MB_MTIME_NS(sb) : 0;
    bool old = std::chrono::steady_clock::now() - it->second.built > std::chrono::seconds(2);
    if (mt != it->second.mtime_ns || old) {
        build_index(d, it->second);
        hit = it->second.lower2real.find(key);
        if (hit != it->second.lower2real.end()) { *real = hit->second; return true; }
    }
    return false;
}
bool exists(const std::string& p)
{
    struct stat sb;
    return stat(p.c_str(), &sb) == 0;
}
}   // namespace

extern "C" {
void mbp_paths_force_scan(int on) { g_force_scan = on != 0; }
void mbp_paths_flush_cache(void) { std::lock_guard<std::mutex> g(g_paths_mu); g_dirs.clear(); }

int mbp_resolve_path(const char* in, char* out, size_t cap)
{
    std::string s = in ? in : "";
    for (char& c : s) if (c == '\\') c = '/';
    if (s.size() >= 2 && s[1] == ':' && isalpha((unsigned char)s[0])) s.erase(0, 2);     // "C:" drive prefix
    bool abs = !s.empty() && s[0] == '/';
    bool trailing = s.size() > 1 && s.back() == '/';
    // exact path first (case-insensitive volumes, or already right): one stat
    std::string result;
    int ok;
    {
        std::lock_guard<std::mutex> g(g_paths_mu);
        std::vector<std::string> comps;
        size_t i = 0;
        while (i <= s.size()) {
            size_t j = s.find('/', i);
            if (j == std::string::npos) j = s.size();
            std::string c = s.substr(i, j - i);
            if (!c.empty() && c != ".") {
                if (c == ".." && !comps.empty() && comps.back() != "..") comps.pop_back();
                else comps.push_back(c);
            }
            i = j + 1;
        }
        std::string plain;
        for (size_t k = 0; k < comps.size(); ++k) { if (k || abs) plain += '/'; plain += comps[k]; }
        if (abs && comps.empty()) plain = "/";
        if (plain.empty()) plain = ".";
        if (!g_force_scan && exists(plain)) {
            result = plain;
            ok = 1;
        } else {
            std::string cur = abs ? "/" : "";
            bool missing = false;
            for (size_t k = 0; k < comps.size(); ++k) {
                const std::string& c = comps[k];
                std::string cand = (cur.empty() || cur == "/") ? cur + c : cur + "/" + c;
                if (missing) { cur = cand; continue; }
                std::string dirpart = cur.empty() ? "" : cur;
                if (!g_force_scan && exists(cand)) { cur = cand; continue; }
                if (c == "..") { cur = cand; continue; }
                std::string real;
                if (lookup(dirpart, c, &real)) cur = (cur.empty() || cur == "/") ? cur + real : cur + "/" + real;
                else { cur = cand; missing = true; }
            }
            if (cur.empty()) cur = ".";
            result = cur;
            ok = !missing && exists(result);
        }
    }
    if (trailing && result.back() != '/') result += '/';
    if (cap) {
        size_t n = std::min(result.size(), cap - 1);
        memcpy(out, result.data(), n);
        out[n] = 0;
    }
    return ok;
}

// ---- stdio ------------------------------------------------------------------------------------------------------------
namespace {
// MSVC text-mode read: "\r\n" -> "\n". Wraps a binary FILE* in a funopen / fopencookie stream.
struct CrlfCookie { FILE* in; int pushed; };
ssize_t crlf_read(void* c, char* buf, size_t n)
{
    CrlfCookie* k = (CrlfCookie*)c;
    size_t got = 0;
    while (got < n) {
        int ch;
        if (k->pushed >= 0) { ch = k->pushed; k->pushed = -1; }
        else ch = fgetc(k->in);
        if (ch == EOF) break;
        if (ch == '\r') {
            int nx = fgetc(k->in);
            if (nx == '\n') ch = '\n';
            else if (nx != EOF) k->pushed = nx;
        }
        buf[got++] = (char)ch;
    }
    return (ssize_t)got;
}
int crlf_close(void* c)
{
    CrlfCookie* k = (CrlfCookie*)c;
    int r = fclose(k->in);
    delete k;
    return r;
}
#if defined(__APPLE__)
int crlf_read_f(void* c, char* b, int n) { return (int)crlf_read(c, b, (size_t)n); }
fpos_t crlf_seek_f(void* c, fpos_t off, int whence)
{
    CrlfCookie* k = (CrlfCookie*)c;
    k->pushed = -1;
    if (fseeko(k->in, (off_t)off, whence) != 0) return -1;
    return (fpos_t)ftello(k->in);
}
FILE* open_crlf(FILE* in)
{
    CrlfCookie* k = new CrlfCookie{in, -1};
    FILE* f = funopen(k, crlf_read_f, nullptr, crlf_seek_f, crlf_close);
    if (!f) { fclose(in); delete k; }
    return f;
}
#else
int crlf_seek_f(void* c, off64_t* off, int whence)
{
    CrlfCookie* k = (CrlfCookie*)c;
    k->pushed = -1;
    if (fseeko(k->in, (off_t)*off, whence) != 0) return -1;
    *off = ftello(k->in);
    return 0;
}
FILE* open_crlf(FILE* in)
{
    CrlfCookie* k = new CrlfCookie{in, -1};
    cookie_io_functions_t io = {crlf_read, nullptr, crlf_seek_f, crlf_close};
    FILE* f = fopencookie(k, "r", io);
    if (!f) { fclose(in); delete k; }
    return f;
}
#endif
}   // namespace

FILE* mb_fopen(const char* path, const char* mode)
{
    char host[4096];
    mbp_resolve_path(path, host, sizeof host);
    bool text_read = false;
    std::string m = mode ? mode : "r";
    if (!m.empty() && m[0] == 'r' && m.find('+') == std::string::npos && m.find('b') == std::string::npos) text_read = true;
    std::string host_mode;
    for (char c : m) if (c != 't') host_mode += c;       // 't' is MSVC's; POSIX has no text mode
    if (host_mode.empty()) host_mode = "r";
    FILE* f = fopen(host, host_mode.c_str());
    if (f && text_read) f = open_crlf(f);
    return f;
}
int mb_remove(const char* path)
{
    char host[4096];
    mbp_resolve_path(path, host, sizeof host);
    return remove(host);
}
int mb_rename(const char* from, const char* to)
{
    char a[4096], b[4096];
    mbp_resolve_path(from, a, sizeof a);
    mbp_resolve_path(to, b, sizeof b);
    if (exists(b)) { errno = EEXIST; return -1; }      // the CRT's rename does not replace an existing file
    return rename(a, b);
}
int mb_rmdir(const char* path)
{
    char host[4096];
    mbp_resolve_path(path, host, sizeof host);
    return rmdir(host);
}
int mb_access(const char* path, int mode)
{
    char host[4096];
    mbp_resolve_path(path, host, sizeof host);
    return access(host, mode);
}
int mb_mkdir(const char* path)
{
    char host[4096];
    mbp_resolve_path(path, host, sizeof host);
    return mkdir(host, 0777);
}
}   // extern "C"

// ---- directory enumeration -----------------------------------------------------------------------------------------------
namespace {
struct Finder {
    std::vector<mbp_dirent> list;
    size_t next = 0;
};
bool wild_match(const char* pat, const char* name)
{
    if (strcmp(pat, "*.*") == 0 || strcmp(pat, "*") == 0) return true;
    return fnmatch(pat, name, FNM_CASEFOLD) == 0;
}
}
extern "C" {
void* mbp_find_open(const char* spec, struct mbp_dirent* first)
{
    std::string s = spec ? spec : "";
    for (char& c : s) if (c == '\\') c = '/';
    size_t sl = s.rfind('/');
    std::string dirpart = sl == std::string::npos ? "" : s.substr(0, sl + 1);
    std::string pat = sl == std::string::npos ? s : s.substr(sl + 1);
    char host[4096];
    mbp_resolve_path(dirpart.empty() ? "." : dirpart.c_str(), host, sizeof host);
    DIR* d = opendir(host);
    if (!d) { errno = ENOENT; return nullptr; }
    Finder* f = new Finder;
    bool has_wild = pat.find_first_of("*?") != std::string::npos;
    while (struct dirent* e = readdir(d)) {
        const char* n = e->d_name;
        bool dots = !strcmp(n, ".") || !strcmp(n, "..");
        if (n[0] == '.' && !dots) continue;
        if (!has_wild ? strcasecmp(n, pat.c_str()) != 0 : !wild_match(pat.c_str(), n)) continue;
        std::string full = std::string(host) + "/" + n;
        struct stat sb;
        mbp_dirent de;
        memset(&de, 0, sizeof de);
        if (stat(full.c_str(), &sb) != 0) continue;
        strncpy(de.name, n, sizeof de.name - 1);
        de.attrib = S_ISDIR(sb.st_mode) ? 0x10 : 0x80;
        if (!(sb.st_mode & S_IWUSR)) de.attrib |= 1;
        de.size = S_ISDIR(sb.st_mode) ? 0 : (unsigned long long)sb.st_size;
        de.atime = sb.st_atime; de.mtime = sb.st_mtime; de.ctime = sb.st_ctime;
        f->list.push_back(de);
    }
    closedir(d);
    std::sort(f->list.begin(), f->list.end(), [](const mbp_dirent& a, const mbp_dirent& b) {
        bool da = a.name[0] == '.', db = b.name[0] == '.';
        if (da != db) return da;                                   // "." and ".." lead
        return strcasecmp(a.name, b.name) < 0;
    });
    if (f->list.empty()) { delete f; errno = ENOENT; return nullptr; }
    *first = f->list[0];
    f->next = 1;
    return f;
}
int mbp_find_next(void* h, struct mbp_dirent* out)
{
    Finder* f = (Finder*)h;
    if (!f || f->next >= f->list.size()) return -1;
    *out = f->list[f->next++];
    return 0;
}
void mbp_find_close(void* h) { delete (Finder*)h; }
}   // extern "C"

// ---- Win32 file API ---------------------------------------------------------------------------------------------------------
namespace {
struct FindH : KObj { void* h; };
void unix_to_filetime(long long t, FILETIME* ft)
{
    unsigned long long v = ((unsigned long long)t + 11644473600ull) * 10000000ull;
    ft->dwLowDateTime = (DWORD)(v & 0xFFFFFFFFu);
    ft->dwHighDateTime = (DWORD)(v >> 32);
}
void fill_find(const mbp_dirent& d, LPWIN32_FIND_DATAA out)
{
    memset(out, 0, sizeof *out);
    out->dwFileAttributes = d.attrib;
    unix_to_filetime(d.ctime, &out->ftCreationTime);
    unix_to_filetime(d.atime, &out->ftLastAccessTime);
    unix_to_filetime(d.mtime, &out->ftLastWriteTime);
    out->nFileSizeHigh = (DWORD)(d.size >> 32);
    out->nFileSizeLow = (DWORD)(d.size & 0xFFFFFFFFu);
    strncpy(out->cFileName, d.name, sizeof out->cFileName - 1);
}
DWORD errno_to_win(int e)
{
    switch (e) {
    case ENOENT: return 2;      // ERROR_FILE_NOT_FOUND
    case ENOTDIR: return 3;     // ERROR_PATH_NOT_FOUND
    case EEXIST: return 183;    // ERROR_ALREADY_EXISTS
    case EACCES: case EPERM: return 5;
    case ENOTEMPTY: return 145;
    default: return 31;         // ERROR_GEN_FAILURE
    }
}
int mkdir_p(const std::string& p)
{
    std::string cur;
    size_t i = 0;
    while (i <= p.size()) {
        size_t j = p.find('/', i);
        if (j == std::string::npos) j = p.size();
        cur = p.substr(0, j);
        if (!cur.empty() && mkdir(cur.c_str(), 0777) != 0 && errno != EEXIST) return -1;
        i = j + 1;
    }
    return 0;
}
}   // namespace

extern "C" {
HANDLE WINAPI FindFirstFileA(LPCSTR spec, LPWIN32_FIND_DATAA out)
{
    mbp_dirent d;
    void* h = mbp_find_open(spec, &d);
    if (!h) { set_last_error(2); return INVALID_HANDLE_VALUE; }
    fill_find(d, out);
    FindH* f = new FindH;
    f->magic = OBJ_FIND; f->h = h;
    return (HANDLE)f;
}
BOOL WINAPI FindNextFileA(HANDLE hf, LPWIN32_FIND_DATAA out)
{
    FindH* f = as<FindH>(hf, OBJ_FIND);
    if (!f) { set_last_error(6); return FALSE; }
    mbp_dirent d;
    if (mbp_find_next(f->h, &d) != 0) { set_last_error(18); return FALSE; }     // ERROR_NO_MORE_FILES
    fill_find(d, out);
    return TRUE;
}
BOOL WINAPI FindClose(HANDLE hf)
{
    FindH* f = as<FindH>(hf, OBJ_FIND);
    if (!f) { set_last_error(6); return FALSE; }
    mbp_find_close(f->h);
    delete f;
    return TRUE;
}
BOOL WINAPI CreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES)
{
    char host[4096];
    mbp_resolve_path(path, host, sizeof host);
    if (mkdir(host, 0777) == 0) return TRUE;
    set_last_error(errno_to_win(errno));
    return FALSE;
}
BOOL WINAPI RemoveDirectoryA(LPCSTR path)
{
    if (mb_rmdir(path) == 0) return TRUE;
    set_last_error(errno_to_win(errno));
    return FALSE;
}
BOOL WINAPI DeleteFileA(LPCSTR path)
{
    char host[4096];
    mbp_resolve_path(path, host, sizeof host);
    if (unlink(host) == 0) return TRUE;
    set_last_error(errno_to_win(errno));
    return FALSE;
}
BOOL WINAPI CopyFileA(LPCSTR from, LPCSTR to, BOOL fail_if_exists)
{
    char a[4096], b[4096];
    mbp_resolve_path(from, a, sizeof a);
    mbp_resolve_path(to, b, sizeof b);
    if (fail_if_exists && exists(b)) { set_last_error(80); return FALSE; }       // ERROR_FILE_EXISTS
    FILE* in = fopen(a, "rb");
    if (!in) { set_last_error(errno_to_win(errno)); return FALSE; }
    FILE* o = fopen(b, "wb");
    if (!o) { set_last_error(errno_to_win(errno)); fclose(in); return FALSE; }
    char buf[65536];
    size_t n;
    bool ok = true;
    while ((n = fread(buf, 1, sizeof buf, in)) > 0)
        if (fwrite(buf, 1, n, o) != n) { ok = false; break; }
    fclose(in);
    if (fclose(o) != 0) ok = false;
    if (!ok) set_last_error(112);       // ERROR_DISK_FULL
    return ok;
}
BOOL WINAPI MoveFileA(LPCSTR from, LPCSTR to)
{
    if (mb_rename(from, to) == 0) return TRUE;
    set_last_error(errno_to_win(errno));
    return FALSE;
}
DWORD WINAPI GetFileAttributesA(LPCSTR path)
{
    char host[4096];
    mbp_resolve_path(path, host, sizeof host);
    struct stat sb;
    if (stat(host, &sb) != 0) { set_last_error(errno_to_win(errno)); return 0xFFFFFFFFu; }
    DWORD a = S_ISDIR(sb.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : 0x80;
    if (!(sb.st_mode & S_IWUSR)) a |= 1;
    return a;
}
DWORD WINAPI GetCurrentDirectoryA(DWORD n, LPSTR buf)
{
    char cwd[4096];
    if (!getcwd(cwd, sizeof cwd)) return 0;
    size_t l = strlen(cwd);
    if (l + 1 > n) return (DWORD)(l + 1);
    memcpy(buf, cwd, l + 1);
    return (DWORD)l;
}
BOOL WINAPI SetCurrentDirectoryA(LPCSTR path)
{
    char host[4096];
    mbp_resolve_path(path, host, sizeof host);
    if (chdir(host) == 0) return TRUE;
    set_last_error(errno_to_win(errno));
    return FALSE;
}
DWORD WINAPI GetTempPathA(DWORD n, LPSTR buf)
{
    const char* t = getenv("TMPDIR");
    std::string s = (t && *t) ? t : "/tmp";
    if (s.back() != '/') s += '/';
    if (s.size() + 1 > n) return (DWORD)(s.size() + 1);
    memcpy(buf, s.c_str(), s.size() + 1);
    return (DWORD)s.size();
}

// ---- shell: special folders, ShellExecute ---------------------------------------------------------------------------
int mbp_user_dir(int csidl, char* out, size_t cap)
{
    int id = csidl & ~CSIDL_FLAG_CREATE;
    std::string base;
    if (const char* o = getenv("MB_USER_DIR")) {                          // an explicit sandbox for tests / portable installs
        base = o;
        base += id == CSIDL_PERSONAL ? "/Documents" : "";
    } else {
        const char* home = getenv("HOME");
        if (!home || !*home) { struct passwd* pw = getpwuid(getuid()); home = pw ? pw->pw_dir : "/tmp"; }
        switch (id) {
        case CSIDL_PERSONAL: base = std::string(home) + "/Documents"; break;
        case CSIDL_APPDATA: case CSIDL_LOCAL_APPDATA:
#if defined(__APPLE__)
            base = std::string(home) + "/Library/Application Support";
#else
        {
            const char* x = getenv("XDG_DATA_HOME");
            base = (x && *x) ? std::string(x) : std::string(home) + "/.local/share";
        }
#endif
            break;
        default: return 0;
        }
    }
    if (base.size() + 1 > cap) return 0;
    if (csidl & CSIDL_FLAG_CREATE) mkdir_p(base);
    memcpy(out, base.c_str(), base.size() + 1);
    return 1;
}
HRESULT WINAPI SHGetFolderPathA(HWND, int csidl, HANDLE, DWORD, LPSTR path)
{
    if (!path) return E_INVALIDARG;
    char buf[1024];
    int id = csidl & ~CSIDL_FLAG_CREATE;
    if (id != CSIDL_PERSONAL && id != CSIDL_APPDATA && id != CSIDL_LOCAL_APPDATA) return E_INVALIDARG;
    // the game's callers hand a 260-byte buffer and append "\\Mount&Blade\\name"; its own folder is created on demand
    if (!mbp_user_dir(csidl | CSIDL_FLAG_CREATE, buf, sizeof buf)) return E_FAIL;
    if (strlen(buf) >= 260 - 1) return HRESULT_FROM_WIN32(122);
    strcpy(path, buf);
    return S_OK;
}
HINSTANCE WINAPI ShellExecuteA(HWND, LPCSTR op, LPCSTR file, LPCSTR params, LPCSTR dir, INT)
{
    static HINSTANCE__ ok_instance;          // > 32 means success in ShellExecute
    if (!file) return (HINSTANCE)(INT_PTR)2;
    std::string f = file;
    bool url = f.find("://") != std::string::npos || f.compare(0, 7, "mailto:") == 0;
    (void)op; (void)params; (void)dir;
    if (url) {
        mbp_sdl_init();
        if (SDL_OpenURL(f.c_str())) return (HINSTANCE)(INT_PTR)&ok_instance;
        return (HINSTANCE)(INT_PTR)31;       // SE_ERR_NOASSOC
    }
    char host[4096];
    if (!mbp_resolve_path(f.c_str(), host, sizeof host)) return (HINSTANCE)(INT_PTR)2;
    std::string u = std::string("file://") + host;
    if (SDL_OpenURL(u.c_str())) return (HINSTANCE)(INT_PTR)&ok_instance;
    return (HINSTANCE)(INT_PTR)31;
}
}   // extern "C"
