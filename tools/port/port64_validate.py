#!/usr/bin/env python3
"""port64_validate.py: would tools/port/port64_worklist.py have caught the 64-bit crashes the port lanes fixed?

    .venv/bin/python tools/port/port64_validate.py [--only NAME] [--keep]

For each case (a fix commit from port/INTEGRATION_STATUS.md's history and the words that name the culprit), check out
the fix's parent in a temporary git worktree (work/port64/wt/), run the worklist there, and report whether a W / V
finding names the culprit. "caught" = a W or V finding matches the case's pattern; "weak" = only an L finding does;
"missed" = nothing. The findings of the fix's changed lines are listed too. Worktrees and their AST caches are removed
afterwards (--keep keeps them). Results: work/port64/validate.json (and stdout).
"""
import argparse, json, os, re, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, HERE)
import port64_ast

# (name, fix commit, regex over class / view / detail / text / func of a finding)
CASES = [
    ('AgentS pad0[4] vptr (MAgent / AgentEq)', '93d4415c', r'\bAgentS\b|\bAgentEq\b|\bMAgent\b'),
    ('ViewParams +0x30 int that is a texture* (sky_t)', 'cc2ec99c', r'\bsky_t\b|ViewParams::f30|ViewParams\+0x30'),
    ('LightCam over rgl::world', 'bd3a20bd', r'\bLightCam\b'),
    ('TroopRec::level_up raw +0x58', '8cf96c37', r'level_up|TroopRec\+0x58'),
    ('(SoaBlock*)note_times', 'f9dfbec4', r'SoaBlock|note_times'),
    ('FaceTexture / Ramp / E176 views', '2693ab79', r'\bRamp\b|\bE176\b|MissionRec2'),
    ('operator new(0x7c) FxDef', 'eea1ceeb', r'0x7c\)|\bFxDef\b.*(alloc|0x7c)|operator new\(0x7c'),
    ('GameG::tick FLD offsets / PartyModel / PlaneTrace', '37e960f3', r'\bGameG\b|PartyModel|PlaneTrace'),
    ('hnode / OpenItem by-value ABI (trivial in one TU only)', '37e960f3', r'\bhnode\b|OpenItem'),
    ('Level494 / PropKind / Rec78 / geom_elem +0x84', 'e277cac4', r'Level494|PropKind|\bRec78\b|geom_elem'),
    ('ScreenOps slot 0 a plain virtual', 'd1bb14ae', r'ScreenOps'),
    ('make_editor_list Camera size / face generator raw offsets', '6b67945d', r'make_editor_list|FaceGen'),
    ('TextSizeView set_text in set_font', '0dc4fe00', r'TextSizeView'),
    ('start_map_conversation raw offset / Big view', '0a24f617', r'start_map_conversation|\bBig\b'),
    ('PsOps / PsOpsW vtable views', '3ab096a8', r'PsOps'),
    ('E68W::purge (tableau materials)', 'b45b4fec', r'\bE68W\b|E68::v\b'),
    ('AgentBodyV / CamParams', '1eff9275', r'CamParams|AgentBodyV'),
    ('compute_normals casted hidden-pointer call', '8c627c09', r'tri_normal|compute_normals'),
]


def run(cmd, **k):
    return subprocess.run(cmd, capture_output=True, text=True, **k)


def changed_lines(sha):
    """{file: set(old line numbers)} of src/ lines the fix removed or changed."""
    d = run(['git', '-C', ROOT, 'diff', '-U0', sha + '^', sha, '--', 'src']).stdout
    out, f = {}, None
    for line in d.split('\n'):
        if line.startswith('--- a/'):
            f = line[6:]
        elif line.startswith('@@') and f:
            m = re.match(r'@@ -(\d+)(?:,(\d+))?', line)
            a, n = int(m[1]), int(m[2] or 1)
            out.setdefault(f, set()).update(range(a, a + max(n, 1)))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--only')
    ap.add_argument('--keep', action='store_true')
    ap.add_argument('--jobs', type=int, default=4)
    ap.add_argument('--md-only', action='store_true', help='rewrite docs/port64_validation.md from validate.json')
    a = ap.parse_args()
    jobs = a.jobs            # each worklist run takes the heavy.py lock itself
    res_path = os.path.join(ROOT, 'work', 'port64', 'validate.json')
    results = json.load(open(res_path)) if os.path.exists(res_path) else {}
    if a.md_only:
        return write_md(results)
    for name, sha, pat in CASES:
        if a.only and a.only.lower() not in name.lower():
            continue
        wt = os.path.join(ROOT, 'work', 'port64', 'wt', sha)
        if not os.path.exists(wt):
            r = run(['git', '-C', ROOT, 'worktree', 'add', '--detach', wt, sha + '^'])
            if r.returncode:
                print('%s: worktree failed: %s' % (name, r.stderr))
                continue
        out = os.path.join(ROOT, 'work', 'port64', 'validate_' + sha)
        os.makedirs(out, exist_ok=True)
        fj = os.path.join(out, 'findings.json')
        r = run([sys.executable, os.path.join(HERE, 'port64_worklist.py'), '--root', wt, '--out', out, '--json', fj,
                 '--jobs', str(jobs)])
        if r.returncode:
            print('%s: worklist failed: %s' % (name, r.stderr[-1500:]))
            continue
        F = json.load(open(fj))
        rx = re.compile(pat)
        lvl = lambda x: __import__('port64_worklist').CATS[x['cat']][0]
        hits = [x for x in F if rx.search(' '.join(str(x.get(k, '')) for k in ('cls', 'view', 'detail', 'text', 'func')))]
        ch = changed_lines(sha)
        inhunk = [x for x in F if x['line'] in ch.get(x['file'], ()) and lvl(x) in 'WVL']
        wv = [x for x in hits if lvl(x) in 'WV']
        verdict = 'caught' if wv else 'weak' if any(lvl(x) == 'L' for x in hits) else 'missed'
        results[name] = dict(sha=sha, verdict=verdict, hits=[(lvl(x), x['cat'], x['file'], x['line'], x['detail'])
                                                             for x in hits][:40],
                             in_hunks=[(lvl(x), x['cat'], x['file'], x['line'], x['detail']) for x in inhunk][:60],
                             n_hits=len(hits), n_wv=len(wv), n_in_hunks=len(inhunk),
                             n_in_hunks_w=sum(1 for x in inhunk if lvl(x) == 'W'))
        print('%-55s %s  %-7s W/V hits %d, all hits %d; fix hunks: %d W/V/L findings (%d W)' % (
            name, sha, verdict, len(wv), len(hits), len(inhunk), results[name]['n_in_hunks_w']))
        for x in wv[:3]:
            print('    %s %s %s:%d %s' % (lvl(x), x['cat'], x['file'], x['line'], x['detail'][:150]))
        json.dump(results, open(res_path, 'w'), indent=1)
        if not a.keep:
            run(['git', '-C', ROOT, 'worktree', 'remove', '--force', wt])
            shutil.rmtree(os.path.join(ROOT, 'work', 'port64', port64_ast.tag(wt)), ignore_errors=True)
    if not a.keep:
        run(['git', '-C', ROOT, 'worktree', 'prune'])
    write_md(results)


def write_md(results):
    """docs/port64_validation.md: one line per case (the committed record of what the worklist catches)."""
    L = ['# port64 worklist validation: the historical 64-bit crashes, at the commit before each fix', '',
         'Generated by `.venv/bin/python tools/port/port64_validate.py` (git worktree at the fix\'s parent, the',
         'worklist run there; ~2-4 min per case). **caught** = a W (64-bit wrong) or V (view to fold) finding names the',
         'culprit; **weak** = only an L (likely) finding does; **missed** = none. "fix hunks" counts the W / V / L',
         'findings on the lines the fix changed.', '',
         '| case | fix | verdict | W/V findings naming it | fix hunks (W) | first W / V finding |',
         '|---|---|---|---:|---:|---|']
    order = [c[0] for c in CASES]
    pats = {c[0]: re.compile(c[2]) for c in CASES}
    for name in sorted(results, key=lambda n: order.index(n) if n in order else 99):
        v = results[name]
        if name in pats:     # re-judge the stored findings with the current pattern
            rx = pats[name]
            m = [h for h in v['hits'] + v['in_hunks'] if rx.search(h[4])]
            rank = {'caught': 0, 'weak': 1, 'missed': 2}      # the stored lists are truncated: only upgrade
            new = 'caught' if any(h[0] in 'WV' for h in m) else 'weak' if m else 'missed'
            v['verdict'] = min(v['verdict'], new, key=rank.get)
            v['hits'] = m + [h for h in v['hits'] if h not in m]
            v['n_wv'] = max(v['n_wv'], sum(1 for h in m if h[0] in 'WV'))
        first = next((h for h in v['hits'] if h[0] in 'WV'), None) or (v['hits'][0] if v['hits'] else None)
        f = ('%s %s `%s:%d` %s' % (first[0], first[1], first[2], first[3], first[4][:120].replace('|', '/'))) \
            if first else ''
        L.append('| %s | %s | %s | %d | %d (%d) | %s |' % (name, v['sha'], v['verdict'], v['n_wv'], v['n_in_hunks'],
                                                         v['n_in_hunks_w'], f))
    c = {}
    for v in results.values():
        c[v['verdict']] = c.get(v['verdict'], 0) + 1
    L += ['', '**%s of %d caught** (%s).' % (c.get('caught', 0), len(results),
                                           ', '.join('%s %d' % kv for kv in sorted(c.items())))]
    open(os.path.join(ROOT, 'docs', 'port64_validation.md'), 'w').write('\n'.join(L) + '\n')


if __name__ == '__main__':
    main()
