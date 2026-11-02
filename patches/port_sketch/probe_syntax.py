# patches/port_sketch/probe_syntax.py: SKETCH. clang -fsyntax-only over every src/**/*.cpp with MSVC keywords stubbed out; prints the first error of each TU, grouped. Ticket 002. usage: probe_syntax.py REPO_ROOT
import subprocess, sys, glob, os, collections
root = sys.argv[1]
os.chdir(root)
files = sorted(glob.glob('src/**/*.cpp', recursive=True))
defs = ['-D__int64=long long', '-D__forceinline=inline', '-D__declspec(x)=', '-D__fastcall=', '-D__stdcall=', '-D__cdecl=', '-D__thiscall=']
ok = 0
first_err = collections.Counter()
fails = []
for f in files:
    r = subprocess.run(['clang', '-fsyntax-only', '-x', 'c++', '-std=c++03', '-w', '-ferror-limit=1'] + defs + [f],
                       capture_output=True, text=True)
    if r.returncode == 0:
        ok += 1
    else:
        line = [l for l in r.stderr.splitlines() if 'error' in l]
        msg = line[0].split('error:')[-1].strip() if line else r.stderr[:80]
        first_err[msg[:90]] += 1
        fails.append(f)
print('files', len(files), 'syntax-ok', ok)
for k, v in first_err.most_common(25):
    print(v, k)
