import subprocess,sys,re,json,os
base=open('work/match/w8_cur.cpp').read()
def run(name,edits):
    s=base
    for a,b in edits:
        if a not in s: return name+': EDIT NOT APPLIED '+a[:40]
        s=s.replace(a,b,1)
    p='work/match/w8_v.cpp'
    open(p,'w').write(s)
    env=dict(os.environ,MB_RC_ALLOW_LOCAL='1')
    r=subprocess.run(['.venv/bin/python','tools/matching/chk.py',p,'0x4556d0'],capture_output=True,text=True,env=env).stdout
    m=re.search(r'diff\s+aligned\s+(\d+)',r) or re.search(r'aligned\s+(\d+)',r)
    r2=subprocess.run(['.venv/bin/python','tools/matching/insdiff.py',p,'0x4556d0'],capture_output=True,text=True,env=env).stdout
    l=[x for x in r2.split('\n') if 'orig-only' in x or 'aligned' in x]
    err=[x for x in (r2+r).split('\n') if 'error' in x][:2]
    return name+': '+(m.group(0) if m else r[-200:])+' | '+' / '.join(l)+' '+' '.join(err)
exec(open(sys.argv[1]).read())
for n,e in V:
    print(run(n,e),flush=True)
