# tools/contrib/trim_tu.py [SRC.cpp] [KEEP] [OUT]: a copy of SRC for traced tools, with every out-of-line body dropped except KEEP's and
# the bodies KEEP can reach by calls (transitively). Under /O2 (/Ob2) cl may inline any function whose body it sees,
# so dropping a callee's body changes the target's code (0x4552c0 draw_name: 179 aligned on the full file, 360 on a
# copy that kept only the target, lane N1-A2 / main 2026-11-26). Types, inline / template / static bodies always stay.
# Always check chk.py on the copy against the full file once before relying on it.
import re, sys

SRC = sys.argv[1] if len(sys.argv) > 1 else 'src/game/agent/agent_logic.cpp'
KEEP = sys.argv[2] if len(sys.argv) > 2 else 'Agent::update_controls'
OUT = sys.argv[3] if len(sys.argv) > 3 else 'work/match/w8_trim.cpp'
src = open(SRC).read().split('\n')
n = len(src)

# pass 1: find the top-level function bodies ('{' and '}' alone in column 0) and their signatures
funcs = []          # (sig_start, body_open, body_close, sig_text, unqualified name, keep_always)
i = 0
while i < n:
    if src[i] == '{':
        j = i - 1
        while (j >= 0 and src[j].strip() and not src[j].startswith('//') and not src[j].startswith('#')
               and not src[j].rstrip().endswith((';', '}', '{'))):
            j -= 1
        sig = ' '.join(re.sub(r'//.*', '', x).rstrip() for x in src[j + 1:i])
        k = i + 1
        while k < n and src[k] != '}':
            k += 1
        head = sig.split('(')[0]
        name = re.findall(r'(~?\w+|operator\W+)\s*$', head)
        always = (not sig or 'inline' in sig or 'template' in sig or re.match(r'\s*static\b', sig)
                  or sig.startswith(('struct', 'class', 'namespace', 'enum', 'union', 'extern "C"')) or '= {' in sig
                  or (j >= 0 and src[j].strip().startswith('template')))
        funcs.append((j + 1, i, k, sig, name[0] if name else '', bool(always)))
        i = k + 1
        continue
    i += 1

# pass 2: KEEP and everything it can call, by unqualified name (over-keeps overloads: safe, only less trimming)
keep = {f for f in range(len(funcs)) if KEEP in funcs[f][3]}
if not keep:
    sys.exit('trim_tu.py: no function signature contains %r' % KEEP)
todo = list(keep)
by_name = {}
for f, fn in enumerate(funcs):
    by_name.setdefault(fn[4], []).append(f)
while todo:
    f = todo.pop()
    body = '\n'.join(src[funcs[f][1]:funcs[f][2] + 1])
    for called in set(re.findall(r'(~?\b\w+)\s*\(', body)):
        for g in by_name.get(called, []):
            if g not in keep:
                keep.add(g); todo.append(g)

# pass 3: write the copy; a dropped free function keeps a declaration (members are declared in their class)
out, pos = [], 0
for f, (s0, b0, b1, sig, name, always) in enumerate(funcs):
    out.extend(src[pos:s0])
    if always or f in keep:
        out.extend(src[s0:b1 + 1])
    elif '::' not in sig.split('(')[0]:
        out.append(sig + ';')
    pos = b1 + 1
out.extend(src[pos:])
open(OUT, 'w').write('\n'.join(out))
print('trim_tu.py: %d of %d bodies kept (%d reached from %s) -> %s, %d lines'
      % (sum(1 for f in funcs if f[5]) + len([f for f in keep if not funcs[f][5]]), len(funcs), len(keep), KEEP, OUT, len(out)))
