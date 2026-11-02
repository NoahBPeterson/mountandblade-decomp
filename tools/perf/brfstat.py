import struct,sys,glob,os,collections
import os
G=os.environ.get("MB_DIR", "/Users/noahpeterson/Library/Application Support/CrossOver/Bottles/Steam/drive_c/Program Files (x86)/Steam/steamapps/common/Mount and Blade")
class R:
    def __init__(s,b):s.b=b;s.p=0;s.scalars=0;s.strs=0;s.chars=0
    def i(s):
        v=struct.unpack_from('<i',s.b,s.p)[0];s.p+=4;s.scalars+=1;return v
    def f(s,n=1):
        s.p+=4*n;s.scalars+=n
    def skip(s,n):s.p+=4*n;s.scalars+=n
    def t(s):
        n=s.i();v=s.b[s.p:s.p+n].decode('latin1');s.p+=n;s.strs+=1;s.chars+=n;return v
def parse(b,C):
    r=R(b)
    tok=r.t()
    while tok!="end":
        n=r.i();C[tok]+=n
        for _ in range(n):
            if tok=="texture": r.t();r.i()
            elif tok=="shader":
                r.t();r.i();r.i();r.t();na=r.i()
                for _ in range(na):r.t()
                ns=r.i();r.skip(4*ns)
            elif tok=="material":
                r.t();r.i();r.t()
                for _ in range(5):r.t()
                r.f(4)
            elif tok=="mesh":
                r.t();r.i();r.t();np=r.i();r.f(3*np);C['pts']+=np
                nb=r.i()
                for _ in range(nb):
                    r.i();m=r.i();r.skip(2*m)
                kc=r.i()
                for _ in range(kc):
                    r.i();n1=r.i();r.f(3*n1);n2=r.i();r.f(3*n2)
                nv=r.i();r.skip(9*nv);C['verts']+=nv
                nf=r.i();r.skip(3*nf);C['faces']+=nf
            elif tok=="body":
                r.t();k=r.t();shape(r,k)
            elif tok=="skeleton":
                r.t();nb=r.i()
                for _ in range(nb):
                    r.i();r.t();r.i();r.f(12)
            elif tok=="skeleton_anim":
                r.t();ns=r.i()
                for _ in range(ns):
                    m=r.i();r.skip(5*m)
                m=r.i();r.skip(4*m)
            else: raise Exception(tok)
        tok=r.t()
    assert r.p==len(b),(r.p,len(b))
    C['scalars']+=r.scalars;C['strs']+=r.strs;C['chars']+=r.chars
def shape(r,k):
    if k=="sphere": r.f(4);r.i()
    elif k=="capsule": r.f(7);r.i()
    elif k=="face": n=r.i();r.f(3*n);r.i()
    elif k=="composite":
        n=r.i()
        for _ in range(n): shape(r,r.t())
    elif k=="manifold":
        n=r.i();r.f(3*n);m=r.i()
        for _ in range(m):
            r.i();r.i();k2=r.i();r.skip(k2)
    else: raise Exception(k)
if __name__=="__main__":
    files=sorted(glob.glob(G+"/CommonRes/*.brf"))+sorted(glob.glob(G+"/Modules/*/Resource/*.brf"))
    T=collections.Counter()
    for f in files:
        C=collections.Counter()
        try: parse(open(f,'rb').read(),C)
        except Exception as e: print("ERR",f,e);continue
        T.update(C);T['files']+=1;T['bytes']+=os.path.getsize(f)
    print(dict(T))
