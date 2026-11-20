#!/usr/bin/env python3
"""tools/port/layout64_scan.py SRC_DIR OUT.json: text counts of 64-bit layout hazards per file (ticket 006,
docs/layout64_inventory.md): raw (char*)p + N, F(x, 0xNN) macros, char padNN[] arrays, (int)this / (int)&x casts,
int fields commented as pointers, plain `long`, offset macros (`MI(this, 0x..)`, `GD(int, 0x..)`), slot-numbered
virtuals (`virtual void v04()`) and their calls, and constant-size allocations (`operator new(0x7c)`, `malloc(0x..)`).
Prints the totals; writes the per-file counts. Text only and approximate: the clang-based, judged list is
tools/port/port64_worklist.py -> docs/port64_worklist.md."""
import re,sys,os,collections,json
root=sys.argv[1]
pats={
 'raw_off': re.compile(r'\(\s*\(?(?:unsigned\s+|const\s+)?char\s*\*\)\s*[A-Za-z_][\w>.\-\[\]]*\s*\)?\s*\+\s*(?:0x[0-9a-fA-F]+|\d+)'),
 'pad': re.compile(r'\bchar\s+(?:_?pad|_?unk|gap|u)\w*\s*\['),
 'ptr2int': re.compile(r'\((?:unsigned\s+)?(?:int|long|unsigned|DWORD|UINT|LONG|uint32_t|int32_t)\)\s*(?:this\b|&\s*\w)'),
 'int_ptr_comment': re.compile(r'\b(?:int|unsigned|DWORD|UINT)\s+\w+\s*(?:\[[^\]]*\])?;.*//.*\b(?:ptr|pointer|->|\*)'),
 'Fmacro': re.compile(r'\b(?:F|MAT|FLD|AT)\(\s*\w+\s*,\s*0x'),
 'long': re.compile(r'(?<!long )\blong\b(?!\s+long)(?!\s+double)'),
 'macro_off': re.compile(r'\b[A-Z][A-Z0-9_]*\(\s*[\w*:<> ]+\s*,\s*0x[0-9a-fA-F]+\s*\)'),
 'slot_virtual': re.compile(r'\bvirtual\b[^;(]*\b(?:v|s|slot|vf|fn)_?(?:0x)?[0-9a-f]{1,3}\s*\('),
 'slot_call': re.compile(r'(?:->|\.)(?:v|s|slot|vf)_?(?:0x)?[0-9a-f]{1,3}\s*\(|\)\s*\[\s*(?:0x)?[0-9a-f]+\s*\]\s*\)\s*\('),
 'alloc_const': re.compile(r'\b(?:operator\s+new(?:\[\])?|malloc|calloc|HeapAlloc|_aligned_malloc)\s*\(\s*(?:[^,()]*,\s*){0,2}0x[0-9a-fA-F]+\s*[,)]'),
}
res=collections.defaultdict(collections.Counter)
for dp,dn,fn in os.walk(root):
  for f in fn:
    if not f.endswith(('.cpp','.h')): continue
    p=os.path.join(dp,f)
    try: t=open(p,errors='replace').read()
    except: continue
    for k,r in pats.items():
      n=len(r.findall(t))
      if n: res[p][k]+=n
tot=collections.Counter()
for p,c in res.items(): tot.update(c)
print(json.dumps(tot))
json.dump({p:dict(c) for p,c in res.items()},open(sys.argv[2],'w'),indent=0)
