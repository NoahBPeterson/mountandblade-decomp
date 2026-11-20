// port/platform/registry.cpp: the registry subset (RegCreateKeyEx / RegOpenKeyEx / RegQueryValueEx / RegSetValueEx /
// RegCloseKey) over a small text store, <user data>/Mount&Blade/registry.ini:
//   [HKCU\Software\MountAndBladeKeys]
//   last_module=sz:Native
//   volume=dword:80
//   blob=bin:0a0b0c
// Written through on every change; read on first use.
#include "mbp_internal.h"
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

using namespace mbp;

namespace {
struct Value { DWORD type; std::string data; };
struct KeyData { std::string display; std::map<std::string, Value> values; std::map<std::string, std::string> names; };   // names: lower -> display
struct KeyH { uint32_t magic; std::string path; };          // path: lowercase "hkcu\\software\\x"
const uint32_t KEY_MAGIC = 0x4d42524b;

std::mutex g_mu;
std::map<std::string, KeyData>& g_keys() { static std::map<std::string, KeyData>* v = new std::map<std::string, KeyData>(); return *v; }   // constructed on first use: src/'s static initializers (GameApp, world_app.cpp) call in before this file's would run      // by lowercase full path
bool g_loaded = false;

std::string store_path()
{
    char dir[1024];
    if (!mbp_user_dir(CSIDL_APPDATA | CSIDL_FLAG_CREATE, dir, sizeof dir)) return "";
    std::string d = std::string(dir) + "/Mount&Blade";
    mkdir(d.c_str(), 0777);
    return d + "/registry.ini";
}
std::string lowerstr(std::string s) { for (char& c : s) c = (char)tolower((unsigned char)c); return s; }
std::string esc(const std::string& s)
{
    std::string o;
    for (char c : s) { if (c == '\n') o += "\\n"; else if (c == '\r') o += "\\r"; else if (c == '\\') o += "\\\\"; else o += c; }
    return o;
}
std::string unesc(const std::string& s)
{
    std::string o;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) { ++i; o += s[i] == 'n' ? '\n' : s[i] == 'r' ? '\r' : s[i]; }
        else o += s[i];
    }
    return o;
}
void load_locked()
{
    if (g_loaded) return;
    g_loaded = true;
    std::string p = store_path();
    FILE* f = p.empty() ? nullptr : fopen(p.c_str(), "rb");
    if (!f) return;
    char line[8192];
    KeyData* cur = nullptr;
    while (fgets(line, sizeof line, f)) {
        std::string l = line;
        while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back();
        if (l.empty()) continue;
        if (l[0] == '[' && l.back() == ']') {
            std::string disp = l.substr(1, l.size() - 2);
            KeyData& k = g_keys()[lowerstr(disp)];
            k.display = disp;
            cur = &k;
        } else if (cur) {
            size_t eq = l.find('=');
            if (eq == std::string::npos) continue;
            std::string name = unesc(l.substr(0, eq)), rest = l.substr(eq + 1);
            size_t c = rest.find(':');
            if (c == std::string::npos) continue;
            std::string t = rest.substr(0, c), v = rest.substr(c + 1);
            Value val;
            if (t == "sz") { val.type = REG_SZ; val.data = unesc(v); val.data.push_back(0); }
            else if (t == "dword") { val.type = REG_DWORD; DWORD d = (DWORD)strtoul(v.c_str(), nullptr, 10); val.data.assign((char*)&d, 4); }
            else if (t == "bin") {
                val.type = REG_BINARY;
                for (size_t i = 0; i + 1 < v.size(); i += 2) val.data.push_back((char)strtol(v.substr(i, 2).c_str(), nullptr, 16));
            } else continue;
            std::string ln = lowerstr(name);
            cur->values[ln] = val;
            cur->names[ln] = name;
        }
    }
    fclose(f);
}
void save_locked()
{
    std::string p = store_path();
    if (p.empty()) return;
    std::string tmp = p + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return;
    for (auto& kv : g_keys()) {
        fprintf(f, "[%s]\n", kv.second.display.c_str());
        for (auto& v : kv.second.values) {
            const std::string& name = kv.second.names[v.first];
            if (v.second.type == REG_SZ) {
                std::string s = v.second.data;
                if (!s.empty() && s.back() == 0) s.pop_back();
                fprintf(f, "%s=sz:%s\n", esc(name).c_str(), esc(s).c_str());
            } else if (v.second.type == REG_DWORD && v.second.data.size() >= 4) {
                DWORD d; memcpy(&d, v.second.data.data(), 4);
                fprintf(f, "%s=dword:%u\n", esc(name).c_str(), d);
            } else {
                fprintf(f, "%s=bin:", esc(name).c_str());
                for (unsigned char c : v.second.data) fprintf(f, "%02x", c);
                fputc('\n', f);
            }
        }
        fputc('\n', f);
    }
    fclose(f);
    rename(tmp.c_str(), p.c_str());
}
// predefined root -> (lowercase prefix, display prefix); false if `h` is a real key
bool root_of(HKEY h, std::string* low, std::string* disp)
{
    uintptr_t v = (uintptr_t)h;
    uintptr_t hi = v >> 32;
    if (hi != 0 && hi != 0xFFFFFFFFu) return false;
    switch ((uint32_t)v) {
    case 0x80000000u: *low = "hkcr"; *disp = "HKCR"; return true;
    case 0x80000001u: *low = "hkcu"; *disp = "HKCU"; return true;
    case 0x80000002u: *low = "hklm"; *disp = "HKLM"; return true;
    }
    return false;
}
bool resolve(HKEY h, LPCSTR sub, std::string* low, std::string* disp)
{
    if (!root_of(h, low, disp)) {
        KeyH* k = (KeyH*)h;
        if (!k || k->magic != KEY_MAGIC) return false;
        *low = k->path;
        auto it = g_keys().find(k->path);
        *disp = it != g_keys().end() ? it->second.display : k->path;
    }
    if (sub && *sub) {
        std::string s = sub;
        for (char& c : s) if (c == '/') c = '\\';
        while (!s.empty() && s[0] == '\\') s.erase(0, 1);
        while (!s.empty() && s.back() == '\\') s.pop_back();
        if (!s.empty()) { *low += "\\" + lowerstr(s); *disp += "\\" + s; }
    }
    return true;
}
HKEY make_handle(const std::string& low)
{
    KeyH* k = new KeyH{KEY_MAGIC, low};
    return (HKEY)k;
}
}   // namespace

extern "C" {
LONG WINAPI RegCreateKeyExA(HKEY hKey, LPCSTR sub, DWORD, LPSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY out, LPDWORD disp)
{
    std::lock_guard<std::mutex> g(g_mu);
    load_locked();
    std::string low, d;
    if (!resolve(hKey, sub, &low, &d)) return 6;           // ERROR_INVALID_HANDLE
    bool created = g_keys().find(low) == g_keys().end();
    if (created) {
        // every ancestor exists too
        size_t pos = 0;
        while ((pos = low.find('\\', pos + 1)) != std::string::npos) {
            std::string a = low.substr(0, pos);
            if (g_keys().find(a) == g_keys().end()) g_keys()[a].display = d.substr(0, pos);
        }
        g_keys()[low].display = d;
        save_locked();
    }
    if (disp) *disp = created ? 1 : 2;                     // REG_CREATED_NEW_KEY / REG_OPENED_EXISTING_KEY
    if (out) *out = make_handle(low);
    return ERROR_SUCCESS;
}
LONG WINAPI RegOpenKeyExA(HKEY hKey, LPCSTR sub, DWORD, REGSAM, PHKEY out)
{
    std::lock_guard<std::mutex> g(g_mu);
    load_locked();
    std::string low, d;
    if (!resolve(hKey, sub, &low, &d)) return 6;
    if (g_keys().find(low) == g_keys().end() && low.find('\\') != std::string::npos) return ERROR_FILE_NOT_FOUND;
    if (out) *out = make_handle(low);
    return ERROR_SUCCESS;
}
LONG WINAPI RegQueryValueExA(HKEY hKey, LPCSTR name, LPDWORD, LPDWORD type, LPBYTE data, LPDWORD cb)
{
    std::lock_guard<std::mutex> g(g_mu);
    load_locked();
    std::string low, d;
    if (!resolve(hKey, nullptr, &low, &d)) return 6;
    auto k = g_keys().find(low);
    if (k == g_keys().end()) return ERROR_FILE_NOT_FOUND;
    auto v = k->second.values.find(lowerstr(name ? name : ""));
    if (v == k->second.values.end()) return ERROR_FILE_NOT_FOUND;
    if (type) *type = v->second.type;
    DWORD need = (DWORD)v->second.data.size();
    if (data) {
        if (!cb || *cb < need) { if (cb) *cb = need; return 234; }      // ERROR_MORE_DATA
        memcpy(data, v->second.data.data(), need);
    }
    if (cb) *cb = need;
    return ERROR_SUCCESS;
}
LONG WINAPI RegSetValueExA(HKEY hKey, LPCSTR name, DWORD, DWORD type, const BYTE* data, DWORD cb)
{
    std::lock_guard<std::mutex> g(g_mu);
    load_locked();
    std::string low, d;
    if (!resolve(hKey, nullptr, &low, &d)) return 6;
    auto k = g_keys().find(low);
    if (k == g_keys().end()) return ERROR_FILE_NOT_FOUND;
    std::string n = name ? name : "";
    Value v;
    v.type = type;
    v.data.assign((const char*)data, cb);
    if (type == REG_SZ && (v.data.empty() || v.data.back() != 0)) v.data.push_back(0);
    k->second.values[lowerstr(n)] = v;
    k->second.names[lowerstr(n)] = n;
    save_locked();
    return ERROR_SUCCESS;
}
LONG WINAPI RegCloseKey(HKEY hKey)
{
    std::string a, b;
    if (root_of(hKey, &a, &b)) return ERROR_SUCCESS;
    KeyH* k = (KeyH*)hKey;
    if (!k || k->magic != KEY_MAGIC) return 6;
    k->magic = 0;
    delete k;
    return ERROR_SUCCESS;
}
}
