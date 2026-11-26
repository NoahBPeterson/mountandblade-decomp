import re,sys
src=open('src/game/agent/agent_logic.cpp').read().split('\n')
out=[];i=0;n=len(src)
KEEP='Agent::process_blow'
while i<n:
    l=src[i]
    if l=='{' and out:
        # signature: walk back in out while lines look like signature
        j=len(out)-1
        sig=[]
        while j>=0 and out[j].strip() and not out[j].startswith('//') and not out[j].rstrip().endswith((';','}','{')) and not out[j].startswith('#'):
            sig.insert(0,out[j]);j-=1
        s=' '.join(re.sub(r'//.*','',x).rstrip() for x in sig)
        # find end of body
        k=i+1
        while k<n and src[k]!='}': k+=1
        if not sig or 'inline' in s or 'template' in s or KEEP in s or s.startswith(('struct','class','namespace','enum','union','extern "C"')) or '= {' in s or 'operator' in s and False:
            out.extend(src[i:k+1]);i=k+1;continue
        # also keep if preceding line is template<...>
        prev=out[j] if j>=0 else ''
        if prev.strip().startswith('template') :
            out.extend(src[i:k+1]);i=k+1;continue
        del out[j+1:]
        if '::' not in s.split('(')[0]:
            out.append(s+';')
        i=k+1;continue
    out.append(l);i+=1
open('work/match/w8s_trim.cpp','w').write('\n'.join(out))
