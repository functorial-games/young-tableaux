#!/usr/bin/env python3
"""Independent small-domain mathematical audit; Python is explicitly authorized.

No reference function calls production. ctypes is confined to observations.
Run: python3 tests/independent_audit.py --compiler /absolute/NDK/.../clang
Exit 1 means a counterexample (including known ones); no expected-failure masking.
"""
import argparse
import ctypes as C
import itertools as I
import math
import pathlib
import re
import subprocess
import sys
from collections import Counter, defaultdict
from fractions import Fraction as F
from functools import lru_cache

ROOT = pathlib.Path(__file__).resolve().parents[1]
class Partition(C.Structure): _fields_ = [('count',C.c_int),('rows',C.c_int*64)]
class Cell(C.Structure): _fields_ = [('row',C.c_int),('column',C.c_int)]
class Tableau(C.Structure): _fields_ = [('shape',Partition),('entries',(C.c_int*64)*64)]
class Permutation(C.Structure): _fields_ = [('count',C.c_int),('values',C.c_int*64)]
class Trace(C.Structure):
    _fields_ = [('count',C.c_int),('step',C.c_int),('values',C.c_int*64),('inserted',C.c_int),('path_count',C.c_int),('path',Cell*64),('p',Tableau),('q',Tableau),('complete',C.c_bool)]
class Pair(C.Structure): _fields_ = [('p',Tableau),('q',Tableau)]
class InsertionTrace(C.Structure): _fields_ = [('count',C.c_int),('cells',Cell*64),('new_cell',Cell)]
class SkewShape(C.Structure): _fields_ = [('outer',Partition),('inner',Partition)]
class Skew(C.Structure): _fields_ = [('shape',SkewShape),('entries',(C.c_int*64)*64)]
class Jeu(C.Structure): _fields_ = [('filling',Skew),('hole',Cell),('active',C.c_bool)]
class SkewList(C.Structure): _fields_ = [('count',C.c_int),('values',Skew*32)]
class PartitionList(C.Structure): _fields_ = [('count',C.c_int),('values',Partition*32)]
class Path(C.Structure): _fields_ = [('count',C.c_int),('values',Partition*17)]
class Paths(C.Structure): _fields_ = [('count',C.c_int),('values',Path*128)]
class Numbers(C.Structure): _fields_ = [('count',C.c_int),('values',C.c_int*256)]
class Rational(C.Structure): _fields_ = [('numerator',C.c_int64),('denominator',C.c_uint64)]
class Term(C.Structure): _fields_ = [('index',Partition),('coefficient',Rational)]
class Function(C.Structure): _fields_ = [('count',C.c_int),('terms',Term*32),('basis',C.c_int)]
class Matrix(C.Structure): _fields_ = [('row_count',C.c_int),('column_count',C.c_int),('entries',(C.c_int*64)*64)]
class Tiles(C.Structure): _fields_ = [('count',C.c_int),('rows',C.c_int*64),('starts',C.c_int*64),('values',(C.c_int*64)*64),('marks',(C.c_uint8*64)*64),('numbers',C.c_bool)]

def partition(shape): return Partition(len(shape),(C.c_int*64)(*shape))
def shape(p): return tuple(p.rows[:p.count])
def tableau(rows):
    out=Tableau(); out.shape=partition(tuple(map(len,rows)))
    for r,row in enumerate(rows):
        for c,value in enumerate(row): out.entries[r][c]=value
    return out
def rows(t): return tuple(tuple(t.entries[r][:t.shape.rows[r]]) for r in range(t.shape.count))
def perm(values): return Permutation(len(values),(C.c_int*64)(*values))
def cellset(p): return {(r,c) for r,n in enumerate(p) for c in range(n)}
def rowtext(t): return ';'.join(','.join(map(str,row)) or '[]' for row in t) or '[]'
def parttext(p): return ','.join(map(str,p)) or '[]'
def keycells(cells): return tuple(sorted(cells))
def skew(outer,inner,values):
    out=Skew(); out.shape=SkewShape(partition(outer),partition(inner))
    for (r,c),value in values.items(): out.entries[r][c]=value
    return out
def skewvalues(t): return {(r,c):t.entries[r][c] for r,c in cellset(shape(t.shape.outer))-cellset(shape(t.shape.inner))}

@lru_cache(None)
def partitions(size,maximum=None):
    if size==0: return ((),)
    maximum=min(size,size if maximum is None else maximum)
    return tuple((first,)+tail for first in range(maximum,0,-1) for tail in partitions(size-first,first))
def validshape(p): return all(0<x<=64 for x in p) and all(a>=b for a,b in zip(p,p[1:])) and len(p)<=64 and sum(p)<=256
def neighbors(p,grow):
    cells=cellset(p); result=[]
    candidates={(r,c+1) for r,c in cells}|{(r+1,c) for r,c in cells}|{(0,0)} if grow else cells
    for point in candidates:
        if grow and point in cells: continue
        new=cells|{point} if grow else cells-{point}
        q=tuple(sum(r==row for r,c in new) for row in range(max((r for r,c in new),default=-1)+1))
        if validshape(q) and cellset(q)==new: result.append((point,q))
    return sorted(result)
@lru_cache(None)
def syt(p):
    """Enumerate linear extensions by placing the largest label at each maximal cell."""
    if not p:return ((),)
    output=[]
    for (r,c),smaller in neighbors(p,False):
        for previous in syt(smaller):
            new=[list(row) for row in previous]
            if r==len(new):new.append([])
            new[r].append(sum(p)); output.append(tuple(map(tuple,new)))
    return tuple(output)
def semistandard(values):
    return all(value>0 and ((r,c-1) not in values or values[r,c-1]<=value) and ((r-1,c) not in values or values[r-1,c]<value) for (r,c),value in values.items())
def insert(previous,value):
    result=[list(row) for row in previous]; path=[]
    for r,row in enumerate(result):
        larger=[c for c,entry in enumerate(row) if entry>value]
        c=larger[0] if larger else len(row); path.append((r,c))
        if c==len(row): row.append(value); return tuple(map(tuple,result)),path
        row[c],value=value,row[c]
    path.append((len(result),0)); result.append([value]); return tuple(map(tuple,result)),path
def rsk(values,records=None):
    p=(); q=[]; traces=[]
    for value,record in zip(values,records if records is not None else range(1,len(values)+1)):
        before=p; p,path=insert(p,value); r,c=path[-1]
        if r==len(q): q.append([])
        q[r].append(record); traces.append((before,p,tuple(map(tuple,q)),path))
    return p,tuple(map(tuple,q)),traces
def subsequence_length(values,increasing=True,weak=False):
    lengths=[]
    for index,value in enumerate(values):
        eligible=[lengths[j] for j in range(index) if (values[j]<=value if weak else values[j]<value) if increasing]
        if not increasing: eligible=[lengths[j] for j in range(index) if values[j]>value]
        lengths.append(1+max(eligible,default=0))
    return max(lengths,default=0)
def slide(outer,inner,values,corner):
    values=dict(values); innercells=cellset(inner)-{corner}; hole=corner; moves=[]
    while True:
        r,c=hole; candidates=[p for p in [(r,c+1),(r+1,c)] if p in values]
        if not candidates: break
        source=min(candidates,key=lambda p:(values[p],-p[0]))
        values[hole]=values.pop(source); hole=source; moves.append((dict(values),hole))
    outercells=cellset(outer)-{hole}
    def asshape(cells): return tuple(sum(r==row for r,c in cells) for row in range(max((r for r,c in cells),default=-1)+1))
    return asshape(outercells),asshape(innercells),values,hole,moves
def promote(t,evacuate=False):
    p=tuple(map(len,t)); work={(r,c):v for r,row in enumerate(t) for c,v in enumerate(row)}; output={}; original=p
    for label in range(sum(p),0,-1):
        del work[0,0]; p,_,work,hole,_=slide(p,(1,),work,(0,0))
        work={point:value-1 for point,value in work.items()}
        output[hole]=label
        if not evacuate: output.update(work); break
    return tuple(tuple(output[r,c] for c in range(n)) for r,n in enumerate(original))

CHECKS=Counter(); FAILURES=[]; FAIL_COUNTS=Counter(); LIB=None
def check(family,fixture,actual,expected):
    CHECKS[family]+=1
    if actual!=expected:
        FAIL_COUNTS[family]+=1
        if FAIL_COUNTS[family]<=20 and not any(f[0]==family and f[1]==repr(fixture) for f in FAILURES):
            FAILURES.append((family,repr(fixture),repr(actual),repr(expected)))
            if sum(f[0]==family for f in FAILURES)<=3: print('FAIL',family,fixture,'actual',actual,'expected',expected,flush=True)
        return False
    return True
def call(name,*args): return getattr(LIB,name)(*[C.byref(a) if isinstance(a,C.Structure) and not isinstance(a,Cell) else a for a in args])
def public(op,fields,reset=True):
    if reset: LIB.audit_reset()
    for field,value in fields.items(): LIB.audit_set(field,str(value).encode())
    LIB.audit_run(OPS.index(op)); return LIB.audit_output(SECTIONS[op]).decode()

def partition_checks():
    for size in range(11):
        for p in partitions(size):
            source=partition(p); out=Partition(); cells=(Cell*257)(); number=C.c_uint64()
            check('partition',p,call('partition_parse',parttext(p).encode(),out),0)
            check('partition',('size',p),call('partition_size',source),size)
            conjugate=LIB.partition_conjugate(C.byref(source))
            check('partition',('conjugate',p),shape(conjugate),tuple(sum(n>c for n in p) for c in range(p[0] if p else 0)))
            count=call('partition_cells',source,cells)
            check('partition',('cells',p),{(x.row-1,x.column-1) for x in cells[:count]},cellset(p))
            hookproduct=1
            for r,c in cellset(p):
                expected=sum(rr==r and cc>=c or cc==c and rr>r for rr,cc in cellset(p)); hookproduct*=expected
                check('hooks',('hook',p,r,c),call('partition_hook',source,r+1,c+1),expected)
            status=call('partition_hook_product',source,C.byref(number)); check('hooks',('product',p),(status,number.value),(0,hookproduct))
            status=call('partition_standard_count',source,C.byref(number)); check('hooks',('count',p),(status,number.value),(0,len(syt(p))))
            for grow,name in [(True,'addable'),(False,'removable')]:
                expected=neighbors(p,grow); count=call('partition_'+name,source,cells)
                check('corners',(p,grow),{(x.row-1,x.column-1) for x in cells[:count]},{point for point,q in expected})
                for point,q in expected:
                    status=call('partition_add_cell' if grow else 'partition_remove_cell',source,Cell(point[0]+1,point[1]+1),out)
                    check('corners',(p,grow,point),(status,shape(out)),(0,q))
    for text in ['0','1,2','1,','[1','1]','-1','1;1','2147483648','65']:
        check('partition validation',text,call('partition_parse',text.encode(),Partition())!=0,True)

def permutation_checks():
    @lru_cache(None)
    def reduced_words(values):
        if values==tuple(sorted(values)):return frozenset({()})
        output=set()
        for position in range(len(values)-1):
            if values[position]>values[position+1]:
                smaller=values[:position]+(values[position+1],values[position])+values[position+2:]
                output.update(word+(position+1,) for word in reduced_words(smaller))
        return frozenset(output)
    for size in range(8):
        pairs=set()
        for values in I.permutations(range(1,size+1)):
            expected_p,expected_q,_=rsk(values); trace=Trace(); source=perm(values)
            status=call('rsk_permutation',source,64,trace)
            check('RSK permutation',values,(status,rows(trace.p),rows(trace.q)),(0,expected_p,expected_q))
            pairs.add((rows(trace.p),rows(trace.q)))
            unseen=set(range(size));cycles=[]
            while unseen:
                position=next(iter(unseen));cycle=set()
                while position not in cycle:
                    cycle.add(position);position=values[position]-1
                unseen-=cycle;cycles.append(len(cycle))
            cycle_type=Partition();status=call('permutation_cycle_type',source,cycle_type)
            check('cycle type',values,(status,shape(cycle_type)),(0,tuple(sorted(cycles,reverse=True))))
            pair=Pair(trace.p,trace.q); recovered=Permutation()
            status=call('inverse_rsk',pair,recovered)
            check('inverse RSK',values,(status,tuple(recovered.values[:recovered.count])),(0,values))
            inv=tuple(values.index(v)+1 for v in range(1,size+1)); invtrace=Trace()
            call('rsk_permutation',perm(inv),64,invtrace)
            check('RSK symmetry',values,(rows(invtrace.p),rows(invtrace.q)),(expected_q,expected_p))
            revtrace=Trace(); call('rsk_permutation',perm(values[::-1]),64,revtrace)
            check('RSK symmetry',('reverse shape',values),shape(revtrace.p.shape),tuple(sum(len(row)>c for row in expected_p) for c in range(len(expected_p[0]) if expected_p else 0)))
            for increasing,name in [(True,'lis'),(False,'lds')]:
                result=Numbers(); status=call('permutation_'+name,source,result); seq=tuple(result.values[:result.count])
                length=subsequence_length(values,increasing)
                check(name,values,(status,len(seq)),(0,length))
                iterator=iter(values); legal=all(any(v==x for v in iterator) for x in seq) and all((a<b if increasing else a>b) for a,b in zip(seq,seq[1:]))
                check(name,('support',values),legal,True)
                check(name,('RSK shape',values),len(expected_p[0]) if increasing and expected_p else len(expected_p) if not increasing else 0,length)
            result=Numbers(); status=call('permutation_coxeter_reduced_word',source,result); rebuilt=list(range(1,size+1))
            word=tuple(result.values[:result.count])
            for generator in word: rebuilt[generator-1],rebuilt[generator]=rebuilt[generator],rebuilt[generator-1]
            inversions=sum(a>b for i,a in enumerate(values) for b in values[i+1:])
            check('Coxeter',values,(status,tuple(rebuilt),len(word)),(0,values,inversions))
            if size<=5:check('Coxeter complete oracle',values,word in reduced_words(values),True)
        check('RSK bijection',size,len(pairs),math.factorial(size))

def word_checks():
    for size in range(7):
        for values in I.product(range(1,4),repeat=size):
            p,q,traces=rsk(values); trace=Trace(); status=call('rsk_word',perm(values),64,trace)
            check('RSK word',values,(status,rows(trace.p),rows(trace.q)),(0,p,q))
            check('RSK word',('LIS weak',values),len(p[0]) if p else 0,subsequence_length(values,weak=True))
            check('RSK word',('LDS strict',values),len(p),subsequence_length(values,False))
            for step,(before,after,recorded,path) in enumerate(traces,1):
                call('rsk_word',perm(values),step,trace)
                check('RSK trace',(values,step),(rows(trace.p),rows(trace.q),[(v.row-1,v.column-1) for v in trace.path[:trace.path_count]]),(after,recorded,path))
                out=Tableau(); ejected=C.c_int(); status=call('tableau_reverse_insert',trace.p,Cell(path[-1][0]+1,path[-1][1]+1),out,C.byref(ejected))
                check('insertion reverse',(values,step),(status,rows(out),ejected.value),(0,before,values[step-1]))
    for entries in I.product(range(3),repeat=4):
        matrix=Matrix(); matrix.row_count=matrix.column_count=2
        for index,value in enumerate(entries): matrix.entries[index//2][index%2]=value
        pairs=[(i+1,j+1) for i in range(2) for j in range(2) for _ in range(matrix.entries[i][j])]
        p,q,_=rsk(tuple(b for a,b in pairs),tuple(a for a,b in pairs)); trace=Trace()
        status=call('rsk_matrix',matrix,64,trace)
        check('RSK matrix',entries,(status,rows(trace.p),rows(trace.q)),(0,p,q))
        text=parttext(tuple(a for a,b in pairs))+';'+parttext(tuple(b for a,b in pairs))
        # Two ints per biletter, following count; no native conversion oracle.
        biword=(C.c_int*129)(); status=LIB.biword_parse(text.encode(),biword)
        if status==0: status=LIB.rsk_biword(biword,64,C.byref(trace))
        check('RSK biword',pairs,(status,rows(trace.p),rows(trace.q)),(0,p,q))
        transposed=Matrix(); transposed.row_count=transposed.column_count=2
        for i in range(2):
            for j in range(2):transposed.entries[i][j]=matrix.entries[j][i]
        call('rsk_matrix',transposed,64,trace)
        check('RSK matrix symmetry',entries,(rows(trace.p),rows(trace.q)),(q,p))

def tableau_checks():
    for size in range(6):
        for p in partitions(size):
            for flat in I.product(range(1,4),repeat=size):
                points=sorted(cellset(p)); values=dict(zip(points,flat)); t=tableau(tuple(tuple(values[r,c] for c in range(n)) for r,n in enumerate(p)))
                for decreasing in [False,True]:
                    for kind in range(5):
                        alphabet=sorted(flat)==list(range(1,size+1))
                        rowgood=all((values[r,c-1]>v if decreasing else values[r,c-1]<v) for (r,c),v in values.items() if (r,c-1) in values)
                        colgood=all((values[r-1,c]>v if decreasing else values[r-1,c]<v) for (r,c),v in values.items() if (r-1,c) in values)
                        weak=all((values[r,c-1]>=v if decreasing else values[r,c-1]<=v) for (r,c),v in values.items() if (r,c-1) in values)
                        expected=[alphabet and rowgood and colgood,True,alphabet and rowgood,alphabet and colgood,weak and colgood][kind]
                        valid=C.c_bool(); status=call('tableau_check_kind',t,kind,decreasing,C.byref(valid))
                        check('tableau kinds',(p,flat,kind,decreasing),(status,valid.value),(0,expected))
                transposed=Tableau(); status=call('tableau_transpose',t,transposed)
                expected=tuple(tuple(values[r,c] for r in range(len(p)) if (r,c) in values) for c in range(p[0] if p else 0))
                check('transpose',(p,flat),(status,rows(transposed)),(0,expected))
                if semistandard(values):
                    ordered=sorted(points,key=lambda point:(values[point],point[1],point[0])); labels={point:i+1 for i,point in enumerate(ordered)}
                    expected=tuple(tuple(labels[r,c] for c in range(n)) for r,n in enumerate(p)); out=Tableau()
                    status=call('tableau_standardize',t,out)
                    check('standardization',(p,flat),(status,rows(out)),(0,expected))
    for size in range(8):
        for p in partitions(size):
            for t in syt(p):
                for evacuation,name in [(False,'tableau_promote'),(True,'tableau_evacuate')]:
                    source=tableau(t); out=Tableau(); status=call(name,source,out)
                    check(name,t,(status,rows(out)),(0,promote(t,evacuation)))
                    if evacuation:
                        twice=Tableau();call(name,out,twice);check(name,('involution',t),rows(twice),t)

def jeu_checks():
    @lru_cache(None)
    def rectifications(outer,inner,items):
        if not inner:return frozenset({(outer,items)})
        output=set()
        for corner,_ in neighbors(inner,False):
            a,b,v,_,_=slide(outer,inner,dict(items),corner)
            output.update(rectifications(a,b,tuple(sorted(v.items()))))
        return frozenset(output)
    for size in range(6):
        for outer in partitions(size):
            for inner_size in range(size+1):
                for inner in partitions(inner_size):
                    if not cellset(inner)<=cellset(outer):continue
                    points=sorted(cellset(outer)-cellset(inner))
                    fillings=set(I.product(range(1,4),repeat=len(points)))|set(I.permutations(range(1,len(points)+1)))
                    for flat in sorted(fillings):
                        values=dict(zip(points,flat))
                        if not semistandard(values):continue
                        source=skew(outer,inner,values)
                        for corner,_ in neighbors(inner,False):
                            a,b,v,hole,moves=slide(outer,inner,values,corner); actual=Jeu(source,Cell(),False)
                            status=call('jeu_begin',actual,Cell(corner[0]+1,corner[1]+1))
                            check('jeu step',('begin',outer,inner,flat,corner),status,0)
                            for expected,h in moves:
                                status=call('jeu_step',actual)
                                observed=skewvalues(actual.filling); observed.pop((actual.hole.row-1,actual.hole.column-1),None)
                                check('jeu step',(outer,inner,flat,corner,h),(status,observed,(actual.hole.row-1,actual.hole.column-1)),(1,expected,h))
                            status=call('jeu_step',actual)
                            check('jeu slide',(outer,inner,flat,corner),(status,shape(actual.filling.shape.outer),shape(actual.filling.shape.inner),skewvalues(actual.filling)),(2,a,b,v))
                        expected=rectifications(outer,inner,tuple(sorted(values.items())))
                        check('rectification order',(outer,inner,flat),len(expected),1)
                        actual=Jeu(source,Cell(),False); status=call('jeu_rectify',actual)
                        observed=(shape(actual.filling.shape.outer),tuple(sorted(skewvalues(actual.filling).items())))
                        check('rectification',(outer,inner,flat),(status,observed in expected),(0,True))

def graph_checks():
    def paths(start,end):
        if start==end:return [(start,)]
        return [(start,)+tail for point,nextshape in neighbors(start,True) if cellset(nextshape)<=cellset(end) for tail in paths(nextshape,end)]
    for size in range(8):
        for p in partitions(size):
            for grow in [False,True]:
                result=PartitionList();status=call('partition_branch_up' if grow else 'partition_branch_down',partition(p),result)
                check('branch',(p,grow),(status,{shape(x) for x in result.values[:result.count]}),(0,{q for point,q in neighbors(p,grow)}))
            expected=paths((),p); actual=Paths();status=call('young_graph_paths',partition(()),partition(p),actual)
            if len(expected)>128:check('graph paths',p,status,5)
            else:
                observed=[tuple(shape(q) for q in path.values[:path.count]) for path in actual.values[:actual.count]]
                check('graph paths',p,(status,len(observed),set(observed)),(0,len(expected),set(expected)))
                check('graph paths',('SYT',p),len(observed),len(syt(p)))

def bruhat_checks():
    # Subword criterion, independent of production's rank matrix.
    for size in range(6):
        permutations=list(I.permutations(range(1,size+1)))
        for upper in permutations:
            working=list(upper); reduced=[]
            while working!=sorted(working):
                pos=next(i for i in range(size-1) if working[i]>working[i+1]); working[pos],working[pos+1]=working[pos+1],working[pos]; reduced.append(pos)
            reachable={tuple(range(1,size+1))}
            for pos in reversed(reduced):
                reachable|={p[:pos]+(p[pos+1],p[pos])+p[pos+2:] for p in list(reachable)}
            for lower in permutations:
                result=C.c_bool();status=call('permutation_bruhat_leq',perm(lower),perm(upper),C.byref(result))
                check('Bruhat',(lower,upper),(status,result.value),(0,lower in reachable))

def compile_library(compiler,out,mutant=None):
    sources=sorted((ROOT/'app/core').glob('*.c'))+[ROOT/x for x in ['app/ui/controls.c','app/console/console.c','app/console/nearby.c','app/render/raster.c','app/render/paint.c','tests/audit_bridge.c']]
    if mutant:sources=[mutant[1] if x==ROOT/mutant[0] else x for x in sources]
    command=[compiler,'--target=x86_64-linux-gnu','--sysroot=/','-rtlib=libgcc','-unwindlib=libgcc','-std=c17','-O2','-shared','-fPIC']+[f'-I{ROOT/x}' for x in ['app/core','app/ui','app/console','app/render']]+list(map(str,sources))+['-lm','-o',str(out)]
    subprocess.run(command,check=True)

OPS=re.findall(r'^OP\((\w+)',(ROOT/'app/console/operations.def').read_text(),re.M)
SECTIONS={name:int(section) for name,section in re.findall(r'^OP\((\w+),(\d+)',(ROOT/'app/console/operations.def').read_text(),re.M)}

def main():
    global LIB
    parser=argparse.ArgumentParser();parser.add_argument('--compiler');parser.add_argument('--library');parser.add_argument('--family',default='all');parser.add_argument('--receipt',default=str(ROOT/'build/independent-audit.tsv'));args=parser.parse_args()
    output=ROOT/'build';output.mkdir(exist_ok=True)
    library=args.library or str(output/'independent-audit.so')
    if not args.library:
        if not args.compiler:parser.error('--compiler is required when building')
        compile_library(args.compiler,pathlib.Path(library))
    LIB=C.CDLL(library); LIB.partition_conjugate.restype=Partition;LIB.audit_output.restype=C.c_char_p
    LIB.audit_jeu.restype=C.POINTER(Jeu)
    LIB.audit_tiles.restype=C.POINTER(Tiles)
    families={'partition':partition_checks,'permutation':permutation_checks,'word':word_checks,'tableau':tableau_checks,'jeu':jeu_checks,'graph':graph_checks,'bruhat':bruhat_checks}
    # Algebra, UI and random families are registered below as this audit grows.
    families.update(EXTRA_FAMILIES)
    for name,function in families.items():
        if args.family not in ('all',name):continue
        print('BEGIN',name,flush=True);function();print('END',name,'checks',sum(CHECKS.values()),'counterexamples',len(FAILURES),flush=True)
    with open(args.receipt,'w') as receipt:
        receipt.write('kind\tfamily\tfixture_or_count\tactual\texpected\n')
        for family,count in CHECKS.items():receipt.write(f'CHECKS\t{family}\t{count}\t-\t-\n')
        for family,count in FAIL_COUNTS.items():receipt.write(f'FAIL_COUNT\t{family}\t{count}\t-\t-\n')
        for failure in FAILURES:receipt.write('FAIL\t'+'\t'.join(failure)+'\n')
    print('TOTAL',sum(CHECKS.values()),'COUNTEREXAMPLES',len(FAILURES),flush=True)
    return bool(FAILURES)

def lr_fillings(inner,content,outer):
    if not cellset(inner)<=cellset(outer) or sum(inner)+sum(content)!=sum(outer):return set()
    points=sorted(cellset(outer)-cellset(inner)); letters=tuple(v for v,n in enumerate(content,1) for _ in range(n)); output=set()
    # Deliberately enumerate complete fillings, then filter. No prefix pruning.
    for flat in set(I.permutations(letters)):
        values=dict(zip(points,flat))
        if not semistandard(values):continue
        word=[values[r,c] for r,c in sorted(points,key=lambda p:(p[0],-p[1]))]
        counts=Counter(); lattice=True
        for value in word:
            counts[value]+=1
            if any(counts[v]<counts[v+1] for v in range(1,len(content))):lattice=False
        if lattice:output.add(tuple(sorted(values.items())))
    return output

@lru_cache(None)
def character(shape_,cycles):
    """Jacobi--Trudi alternating sum of Young permutation characters.

    h_a1 ... h_ar is induced trivial from row stabilizers: its value counts
    ways of assigning whole cycles to labelled boxes of capacities a_i.
    This uses neither rim hooks nor the production character routine.
    """
    @lru_cache(None)
    def assignments(remaining,capacities):
        if not remaining:return int(not any(capacities))
        cycle=remaining[0]; total=0
        for index,capacity in enumerate(capacities):
            if capacity>=cycle:total+=assignments(remaining[1:],capacities[:index]+(capacity-cycle,)+capacities[index+1:])
        return total
    result=0
    for permutation in I.permutations(range(len(shape_))):
        capacities=tuple(value-i+permutation[i] for i,value in enumerate(shape_))
        if min(capacities,default=0)<0:continue
        sign=(-1)**sum(a>b for i,a in enumerate(permutation) for b in permutation[i+1:])
        result+=sign*assignments(cycles,capacities)
    return result

def character_checks():
    for size in range(9):
        ps=partitions(size); table=[]
        for p in ps:
            row=[]
            for cycle in ps:
                number=C.c_int64();status=call('symmetric_character_cycle_type',partition(p),partition(cycle),C.byref(number));expected=character(p,cycle)
                check('characters',(p,cycle),(status,number.value),(0,expected));row.append(number.value)
            table.append(row)
        z=[math.prod(value**count*math.factorial(count) for value,count in Counter(cycle).items()) for cycle in ps]
        for i in range(len(ps)):
            for j in range(len(ps)):
                check('characters orthogonality',('rows',size,i,j),sum(F(table[i][k]*table[j][k],z[k]) for k in range(len(ps))),int(i==j))
                check('characters orthogonality',('columns',size,i,j),sum(table[k][i]*table[k][j] for k in range(len(ps))),z[i] if i==j else 0)
        for index,p in enumerate(ps):
            check('characters identity',p,table[index][-1],len(syt(p)))
        check('characters trivial',size,table[0],[1]*len(ps))
        check('characters sign',size,table[-1],[(-1)**(size-len(p)) for p in ps])

def poly_add(left,right,scale=1):
    result=defaultdict(F,left)
    for monomial,value in right.items():result[monomial]+=scale*value
    return {key:value for key,value in result.items() if value}
def poly_mul(left,right):
    result=defaultdict(F)
    for a,x in left.items():
        for b,y in right.items():result[tuple(i+j for i,j in zip(a,b))]+=x*y
    return {key:value for key,value in result.items() if value}
@lru_cache(None)
def weak_compositions(size,variables):
    if variables==0:return ((),) if size==0 else ()
    return tuple((first,)+tail for first in range(size+1) for tail in weak_compositions(size-first,variables-1))
@lru_cache(None)
def polynomial(basis,index,variables=5):
    unit={(0,)*variables:F(1)}
    if basis==2:
        if len(index)>variables:return {}
        return {m:F(1) for m in set(I.permutations(index+(0,)*(variables-len(index))))}
    if basis==0:
        result={}
        for permutation in I.permutations(range(len(index))):
            degrees=tuple(index[i]-i+permutation[i] for i in range(len(index)))
            if min(degrees,default=0)<0:continue
            term=unit
            for degree in degrees:term=poly_mul(term,polynomial(3,(degree,),variables))
            sign=(-1)**sum(a>b for i,a in enumerate(permutation) for b in permutation[i+1:])
            result=poly_add(result,term,sign)
        return result
    result=unit
    for degree in index:
        if basis==1:factor={tuple(degree if i==j else 0 for i in range(variables)):F(1) for j in range(variables)}
        elif basis==3:factor={m:F(1) for m in weak_compositions(degree,variables)}
        else:factor={tuple(int(i in selected) for i in range(variables)):F(1) for selected in I.combinations(range(variables),degree)}
        result=poly_mul(result,factor)
    return result
def function(basis,terms):
    result=Function();result.basis=basis;result.count=len(terms)
    for i,(p,coefficient) in enumerate(terms.items()):
        coefficient=F(coefficient);result.terms[i]=Term(partition(p),Rational(coefficient.numerator,coefficient.denominator))
    return result
def function_terms(f):return {shape(term.index):F(term.coefficient.numerator,term.coefficient.denominator) for term in f.terms[:f.count] if term.coefficient.numerator}
def function_poly(f,variables=5):
    result={}
    for index,coefficient in function_terms(f).items():result=poly_add(result,polynomial(f.basis,index,variables),coefficient)
    return result
def evaluate(poly,alphabet):return sum(coefficient*math.prod(value**exponent for value,exponent in zip(alphabet,monomial)) for monomial,coefficient in poly.items())

def algebra_checks():
    for total in range(7):
        for left_size in range(total+1):
            right_size=total-left_size
            for left in partitions(left_size):
                for right in partitions(right_size):
                    product={}
                    for outer in partitions(total):
                        expected=lr_fillings(left,right,outer);number=C.c_uint64();listed=SkewList()
                        status=call('littlewood_richardson_coefficient',partition(left),partition(right),partition(outer),C.byref(number))
                        check('LR coefficient',(left,right,outer),(status,number.value),(0,len(expected)))
                        status=call('littlewood_richardson_tableaux',partition(left),partition(right),partition(outer),listed)
                        observed=[tuple(sorted(skewvalues(t).items())) for t in listed.values[:listed.count]]
                        check('LR enumeration',(left,right,outer),(status,len(observed),set(observed)),(0,len(expected),expected))
                        swapped=C.c_uint64();status=call('littlewood_richardson_coefficient',partition(right),partition(left),partition(outer),C.byref(swapped))
                        check('LR symmetry',(left,right,outer),(status,swapped.value),(0,len(expected)))
                        if expected:product[outer]=F(len(expected))
                    actual=Function();status=call('symmetric_schur_product',partition(left),partition(right),actual)
                    check('Schur product',(left,right),(status,function_terms(actual)),(0,product))
                    if total<=5:
                        check('Schur polynomial',(left,right),function_poly(actual),poly_mul(polynomial(0,left),polynomial(0,right)))
    for degree in range(6):
        for index in partitions(degree):
            for basis in range(5):
                source=function(basis,{index:F(1)});expected=polynomial(basis,index)
                for destination in range(5):
                    out=Function();status=call('symmetric_change_basis',source,destination,out)
                    check('basis conversion',(basis,index,destination),(status,function_poly(out)),(0,expected))
                for alphabet in [(),(0,),(1,2),(-1,2),(1,1,1,1,1)]:
                    value=Rational();status=call('symmetric_specialize',source,(C.c_int*len(alphabet))(*alphabet),len(alphabet),value)
                    expected_value=evaluate(expected,alphabet+(0,)*(5-len(alphabet)))
                    check('specialization',(basis,index,alphabet),(status,F(value.numerator,value.denominator or 1)),(0,expected_value))
    # Independent polynomial Adams substitution: p_k acts by x_i -> x_i**k,
    # leaving rational scalar coefficients fixed. Outer Schur uses our character oracle.
    outer_terms=[(basis,index) for degree in range(3) for index in partitions(degree) for basis in range(5)]
    inner_terms=[(basis,index) for degree in range(3) for index in partitions(degree) for basis in range(5)]
    for outer_basis,outer_index in outer_terms:
        # obtain reference p coordinates by solving on independent monomial polynomials
        power_coordinates=power_expansion(outer_basis,outer_index)
        for inner_basis,inner_index in inner_terms:
            inner_polynomial=polynomial(inner_basis,inner_index);expected={}
            for powers,coefficient in power_coordinates.items():
                term={(0,)*5:F(1)}
                for power in powers:
                    transformed={tuple(e*power for e in m):v for m,v in inner_polynomial.items()}
                    term=poly_mul(term,transformed)
                expected=poly_add(expected,term,coefficient)
            actual=Function();status=call('symmetric_plethysm',function(outer_basis,{outer_index:1}),function(inner_basis,{inner_index:1}),actual)
            check('plethysm',(outer_basis,outer_index,inner_basis,inner_index),(status,function_poly(actual)),(0,expected))

@lru_cache(None)
def power_expansion(basis,index):
    # Coefficient matching in the faithful degree<=5 polynomial representation.
    ps=partitions(sum(index));source=polynomial(basis,index);columns=[polynomial(1,p) for p in ps]
    monomials=[p+(0,)*(5-len(p)) for p in ps]
    matrix=[[column.get(m,F(0)) for column in columns]+[source.get(m,F(0))] for m in monomials]
    for j in range(len(ps)):
        pivot=next(i for i in range(j,len(ps)) if matrix[i][j]);matrix[pivot],matrix[j]=matrix[j],matrix[pivot]
        divisor=matrix[j][j];matrix[j]=[x/divisor for x in matrix[j]]
        for i in range(len(ps)):
            if i!=j:
                multiple=matrix[i][j];matrix[i]=[a-multiple*b for a,b in zip(matrix[i],matrix[j])]
    return {p:matrix[i][-1] for i,p in enumerate(ps) if matrix[i][-1]}

def random_checks():
    rng=C.c_uint64(20261007);sample_count=24000
    counts=Counter();tableaux=Counter();shapes=Counter();p=(3,2,1);expected_tableaux=set(syt(p))
    for sample in range(sample_count):
        permutation=Permutation();t=Tableau();q=Partition()
        a=call('random_permutation',4,C.byref(rng),permutation)
        b=call('random_standard_tableau',partition(p),C.byref(rng),t)
        c=call('sample_plancherel_partition',4,C.byref(rng),q)
        v=tuple(permutation.values[:permutation.count]);w=rows(t);s=shape(q)
        check('random support',sample,(a,b,c,sorted(v),w in expected_tableaux,s in partitions(4)),(0,0,0,[1,2,3,4],True,True))
        counts[v]+=1;tableaux[w]+=1;shapes[s]+=1
    for name,observed,probabilities in [('permutation',counts,{p:F(1,24) for p in I.permutations(range(1,5))}),('SYT',tableaux,{t:F(1,len(expected_tableaux)) for t in expected_tableaux}),('Plancherel',shapes,{p:F(len(syt(p))**2,24) for p in partitions(4)})]:
        statistic=sum((observed[item]-sample_count*probability)**2/(sample_count*probability) for item,probability in probabilities.items())
        check('random distribution',name,statistic<80,True)
        print('CHI_SQUARED',name,float(statistic),'threshold 80',flush=True)

def tile_rows(which):
    projection=LIB.audit_tiles(which).contents
    return tuple(tuple(projection.values[r][:projection.rows[r]]) for r in range(projection.count))
def tile_marks(which):
    projection=LIB.audit_tiles(which).contents
    return {(r,c+projection.starts[r]):projection.marks[r][c] for r in range(projection.count) for c in range(projection.rows[r]) if projection.marks[r][c]}
def tableau_display(t):return ''.join(''.join(str(v)+' ' for v in row)+'\n' for row in t) if t else '[]\n'
def ui_checks():
    # Exact public results for each registry operation. These are small hand-
    # checkable fixtures, not comparison to another native calculation path.
    fixtures={
        'DisplayPartition':({1:'2,1'},'size |λ| = 3'),
        'ConjugatePartition':({1:'3,1'},'conjugate = [2,1,1]'),
        'ListCells':({1:'2,1'},'cells: (1,1) (1,2) (2,1)'),
        'DisplayTableau':({1:'2,1',2:'1,3;2'},'filling\n1 3 \n2 \n'),
        'ValidateTableau':({1:'2,1',2:'1,3;2'},'standard (1..n once): YES'),
        'StandardizeTableau':({2:'1,1;2'},'standardized\n1 2 \n3 \n'),
        'InsertLetter':({2:'1,3;2',28:'2'},'result\n1 2 \n2 3 \n'),
        'ReverseInsert':({2:'1,2;2,3',6:'2,2'},'ejected 2\nresult\n1 3 \n2 \n'),
        'TransposeTableau':({2:'1,3;2'},'transpose\n1 2 \n3 \n'),
        'ComputeHookLengths':({1:'2,1'},'Hook product = 3'),
        'ComputeHookProduct':({1:'2,1'},'Hook product = 3'),
        'CountStandardTableaux':({1:'2,1'},'gives 2 standard tableaux'),
        'FindCorners':({1:'1'},'(1,1)'),
        'FindAddableCells':({1:'[]'},'(1,1)'),
        'FindRemovableCells':({1:'1'},'(1,1)'),
        'AddCell':({1:'[]',6:'1,1'},'add (1,1) -> [1]'),
        'RemoveCell':({1:'1',6:'1,1'},'remove (1,1) -> []'),
        'RSKPermutation':({3:'2,1'},'P: insertion tableau\n1 \n2 \nQ: recording tableau\n1 \n2 \n'),
        'RSKWord':({7:'1,1'},'P: insertion tableau\n1 1 \nQ: recording tableau\n1 2 \n'),
        'RSKBiword':({8:'1,1;1,2'},'P: insertion tableau\n1 2 \nQ: recording tableau\n1 1 \n'),
        'RSKMatrix':({9:'0,1;1,0'},'P: insertion tableau\n1 \n2 \nQ: recording tableau\n1 \n2 \n'),
        'InverseRSK':({2:'1;2',29:'1;2'},'[2,1]'),
        'JeuDeTaquinSlide':({1:'2,1',4:'1',27:'1;1',6:'1,1'},'inner μ = []'),
        'Rectify':({1:'2,1',4:'1',27:'1;1',6:'1,1'},'inner μ = []'),
        'Promote':({2:'1,2;3'},'Promotion\n1 3 \n2 \n'),
        'Evacuate':({2:'1,2;3'},'Evacuation\n1 3 \n2 \n'),
        'LittlewoodRichardsonCoefficient':({1:'1',4:'1',5:'2'},'c(λ, μ; ν) = 1'),
        'EnumerateLRTableaux':({1:'1',4:'1',5:'2'},'1 LR tableaux of ν ÷ λ, content μ\nTableau 1\n1 \n'),
        'MultiplySchurFunctions':({1:'1',4:'1'},'1 × s[2] + 1 × s[1,1]'),
        'CharacterValue':({1:'1,1',3:'2,1'},'Ordinary complex character χλ = -1'),
        'RepresentationDimension':({1:'2,1'},'Ordinary complex Specht dimension = 2'),
        'ChangeBasis':({20:'0;1;2',16:'1'},'1÷2 × p[2] + 1÷2 × p[1,1]'),
        'Specialize':({20:'0;1;2',19:'1,2'},'Finite alphabet value = 7 ÷ 1'),
        'Plethysm':({20:'0;1;2',31:'0;1;2'},'1 × s[4] + 1 × s[2,2]'),
        'BranchUp':({1:'1'},'[2]\n[1,1]'),
        'BranchDown':({1:'1'},'[]'),
        'EnumerateYoungGraphPaths':({1:'[]',5:'2'},'1 paths\n[] → [1] → [2]'),
        'GenerateRandomPermutation':({10:'1'},'[1]'),
        'GenerateRandomStandardTableau':({1:'1'},'Uniform standard tableau\n1 \n'),
        'SamplePlancherelPartition':({10:'1'},'[1]'),
        'ComputeLongestIncreasingSubsequence':({3:'2,1'},'[2]'),
        'ComputeLongestDecreasingSubsequence':({3:'2,1'},'[2,1]'),
        'CoxeterReducedWord':({3:'2,1'},'[1]'),
        'BruhatRelations':({3:'1,2',24:'2,1'},'first ≤ second = YES'),
    }
    assert set(fixtures)==set(OPS)
    with (ROOT/'build/public-dispatch.tsv').open('w') as receipt:
        receipt.write('operation\tfields\tactual\texpected_substring\tbutton_visible\toutput_rendered\n')
        for op,(fields,expected) in fixtures.items():
            LIB.audit_reset()
            for field,value in fields.items():LIB.audit_set(field,value.encode())
            if op in ['JeuDeTaquinSlide','Rectify']:LIB.audit_event(520)
            output=public(op,{},reset=False)
            check('public dispatch',op,output if expected not in output else expected,expected)
            visible=LIB.audit_visible(1000+OPS.index(op));rendered=LIB.audit_rendered(SECTIONS[op])
            receipt.write('\t'.join([op,repr(fields),repr(output),repr(expected),str(visible),str(rendered)])+'\n')
            if SECTIONS[op]>=3:check('public rendering',op,(visible,rendered),(1,1))
    # Every RSK family: before/after, all bump marks, Q new cell, navigation.
    for op,field,text,values,records in [
        ('RSKPermutation',3,'3,1,4,2',(3,1,4,2),None),
        ('RSKWord',7,'2,1,1',(2,1,1),None),
        ('RSKBiword',8,'1,1,2;1,2,1',(1,2,1),(1,1,2)),
        ('RSKMatrix',9,'1,1;1,0',(1,2,1),(1,1,2))]:
        public(op,{field:text});LIB.audit_event(500)
        check('visual RSK',(op,0),tile_rows(2),())
        for step,(before,after,q,path) in enumerate(rsk(values,records)[2],1):
            LIB.audit_event(502)
            check('visual RSK',(op,step),(tile_rows(4),tile_rows(2),tile_rows(3),tile_marks(2),tile_marks(3)),(before,after,q,{point:1 for point in path},{path[-1]:1}))
        LIB.audit_event(501);check('visual RSK',(op,'prev'),LIB.audit_rsk_step(),len(values)-1)
        LIB.audit_event(503);check('visual RSK',(op,'end'),LIB.audit_rsk_step(),len(values))
    # Fresh edits take the same path as the on-screen keypad; no direct cache manipulation.
    public('RSKPermutation',{3:'2,1'});LIB.audit_set(7,b'1,');LIB.audit_edit(7,0)
    check('UI edits',('word after permutation'),tile_rows(2),((1,1),))
    LIB.audit_set(7,b'1,');LIB.audit_edit(7,1)
    check('UI edits',('change word'),tile_rows(2),((1,2),))
    # Genuine equal-neighbor tie; below must move into the hole first.
    LIB.audit_reset()
    for field,text in {1:'2,1',4:'1',27:'1;1',6:'1,1'}.items():LIB.audit_set(field,text.encode())
    LIB.audit_event(520);LIB.audit_event(521)
    check('visual jeu','tie before',(tile_rows(6),tile_marks(6)),(((0,1),(1,)),{(0,0):2,(0,1):1,(1,0):1}))
    check('visual jeu','tie after',(tile_rows(5),tile_marks(5)),(((1,1),(0,)),{(1,0):2}))
    LIB.audit_event(522);check('visual jeu','slide',tile_rows(5),((1,1),))
    LIB.audit_event(520);LIB.audit_event(523);check('visual jeu','rectify',tile_rows(5),((1,1),))
    LIB.audit_set(27,b'1;');LIB.audit_edit(27,1);LIB.audit_event(523)
    check('UI edits','jeu after edit',tile_rows(5),((1,),(2,)))
    # Smallest observed wrapped positive row; do not accept an out-of-range index.
    for op,fields in [('AddCell',{1:'[]',6:'4294967297,1'}),('RemoveCell',{1:'1',6:'4294967297,1'}),('ReverseInsert',{2:'1',6:'4294967297,1'})]:
        output=public(op,fields);check('cell validation',op,output,'INVALID INPUT' if 'INVALID INPUT' not in output else output)
    LIB.audit_reset()
    for field,text in {1:'1',4:'1',27:'[]',6:'4294967297,1'}.items():LIB.audit_set(field,text.encode())
    LIB.audit_event(520);output=public('JeuDeTaquinSlide',{},reset=False)
    check('cell validation','JeuDeTaquinSlide',output,'Selected cell must be a removable inner corner of μ.' if 'Selected cell must' not in output else output)
    # Parsing cannot impose a semantic default different from a literal number.
    for op,field in [('RSKPermutation',3),('RSKWord',7),('RSKBiword',8),('RSKMatrix',9),('ChangeBasis',20),('Specialize',20),('Plethysm',20),('CharacterValue',3),('InverseRSK',2),('Promote',2),('Evacuate',2),('BranchUp',1),('BranchDown',1),('EnumerateYoungGraphPaths',1),('LittlewoodRichardsonCoefficient',4),('EnumerateLRTableaux',4),('MultiplySchurFunctions',4),('ComputeLongestIncreasingSubsequence',3),('ComputeLongestDecreasingSubsequence',3),('CoxeterReducedWord',3),('BruhatRelations',24)]:
        output=public(op,{field:'1,'});check('public malformed',(op,'1,'),'INVALID' in output,True)

def boundary_checks():
    for text,expected in [('64',0),(','.join(['1']*64),0),('64,64,64,64',0),('64,64,64,64,1',2),('65',2),(','.join(['1']*65),2)]:
        check('bounds',('partition',text),call('partition_parse',text.encode(),Partition()),expected)
    for values in [tuple(range(1,65)),tuple(range(64,0,-1))]:
        trace=Trace();status=call('rsk_permutation',perm(values),64,trace);p,q,_=rsk(values)
        check('bounds',('RSK64',values[0]),(status,rows(trace.p),rows(trace.q)),(0,p,q))
    number=C.c_uint64();check('bounds','hook overflow',call('partition_hook_product',partition((64,)),C.byref(number)),3)
    check('bounds','dimension overflow',call('partition_standard_count',partition((64,64)),C.byref(number)),3)
    check('bounds','dimension row64',(call('partition_standard_count',partition((64,)),C.byref(number)),number.value),(0,1))
    for p in partitions(8):
        # Integer-alphabet specialization at 1^8 counts SSYT by the independent
        # Weyl dimension product; no native hook or character values are used.
        padded=p+(0,)*(8-len(p));expected=math.prod(F(padded[i]-padded[j]+j-i,j-i) for i in range(8) for j in range(i+1,8))
        actual=Rational();status=call('symmetric_specialize',function(0,{p:1}),(C.c_int*8)(*([1]*8)),8,actual)
        check('degree8 specialization',p,(status,F(actual.numerator,actual.denominator or 1)),(0,expected))
    for basis in range(5):
        out=Function();check('bounds',('degree9',basis),call('symmetric_change_basis',function(basis,{(9,):1}),0,out),5)
    # Invalid native skew shape: increasing row lengths cannot be a partition.
    invalid=Jeu(skew((1,2),(),{(0,0):1,(1,0):2,(1,1):3}),Cell(),False)
    check('native skew validation','outer=(1,2), inner=(), rows 1;2,3',call('jeu_rectify',invalid),1)
    # Empty, zero, unit, rational constants and mixed degrees for plethysm.
    for outer,inner,expected in [
        (function(1,{}),function(1,{(1,):1}),{}),
        (function(1,{():1}),function(1,{}),{():F(1)}),
        (function(1,{(2,):1}),function(1,{():F(1,2)}),{():F(1,2)}),
        (function(1,{(1,):1,():1}),function(1,{(2,):2,():F(1,2)}),None)]:
        out=Function();status=call('symmetric_plethysm',outer,inner,out)
        expected_poly={(0,)*5:value for p,value in expected.items()} if expected is not None else poly_add(polynomial(1,(2,)),polynomial(1,(2,)))
        if expected is None:expected_poly=poly_add(expected_poly,{(0,)*5:F(3,2)})
        check('plethysm constants',(function_terms(outer),function_terms(inner)),(status,function_poly(out)),(0,expected_poly))

def type_checks():
    text=(ROOT/'types/YoungTableaux/Operations.idric').read_text()
    registry=(ROOT/'app/console/operations.def').read_text()
    tokens={'Partition':'partition','DiagramResult':'diagram_result','Cell':'cell','Tableau':'tableau','Entry':'entry','Permutation':'permutation','Word':'word','Biword':'biword','NatMatrix':'nat_matrix','PermutationRSKResult':'permutation_rsk_result','WordRSKResult':'word_rsk_result','BiwordRSKResult':'biword_rsk_result','SkewTableau':'skew_tableau','StandardTableau':'standard_tableau','SymmetricFunction':'symmetric_function','SymmetricFunctionBasis':'symmetric_function_basis','Specialization':'specialization','Rational':'rational','RNGState':'rng_state'}
    def translated(signature):return re.sub(r'\b\w+\b',lambda m:tokens.get(m[0],m[0]),signature)
    for op,inp,out in re.findall(r'^OP\((\w+),\d+,"[^"]+","([^"]+)","([^"]+)"\)',registry,re.M):
        snake=re.sub(r'(?<=[a-z])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])','_',op).lower()
        for owner,expected in [('input_for '+snake,translated(inp)),('output_for '+snake,translated(out)),(snake+'_signature',translated(inp)+' -> '+translated(out))]:
            actual=re.findall('^'+re.escape(owner)+r' = (.+)$',text,re.M)
            check('actual Idric declarations',owner,actual,[expected])

def power_product(left,right):
    result=defaultdict(F)
    for a,x in left.items():
        for b,y in right.items():result[tuple(sorted(a+b,reverse=True))]+=x*y
    return {p:c for p,c in result.items() if c}
@lru_cache(None)
def labelled_set_partitions(size):
    if not size:return ((),)
    output=[]
    for previous in labelled_set_partitions(size-1):
        output.append(previous+((size-1,),))
        for block in range(len(previous)):output.append(previous[:block]+(previous[block]+(size-1,),)+previous[block+1:])
    return tuple(output)
@lru_cache(None)
def reference_power(basis,index):
    """Full degree-eight exact algebra, independent of production's Kostka inverse.

    Monomials use Möbius inversion on set partitions of labelled factors.
    Schur uses independently computed permutation-module characters.
    """
    if basis==1:return {index:F(1)}
    if basis==0:
        return {p:F(character(index,p),math.prod(v**n*math.factorial(n) for v,n in Counter(p).items())) for p in partitions(sum(index)) if character(index,p)}
    if basis==2:
        result=defaultdict(F);divisor=math.prod(math.factorial(n) for n in Counter(index).values())
        for blocks in labelled_set_partitions(len(index)):
            powers=tuple(sorted((sum(index[i] for i in block) for block in blocks),reverse=True))
            coefficient=math.prod((-1)**(len(block)-1)*math.factorial(len(block)-1) for block in blocks)
            result[powers]+=F(coefficient,divisor)
        return {p:c for p,c in result.items() if c}
    result={():F(1)}
    for degree in index:
        factor={p:F((-1)**(degree-len(p)) if basis==4 else 1,math.prod(v**n*math.factorial(n) for v,n in Counter(p).items())) for p in partitions(degree)}
        result=power_product(result,factor)
    return result
def reference_function_power(f):
    result={}
    for index,coefficient in function_terms(f).items():result=poly_add(result,reference_power(f.basis,index),coefficient)
    return result
def reference_plethysm(outer,inner):
    result={};inner=reference_function_power(inner)
    for powers,coefficient in reference_function_power(outer).items():
        term={():coefficient}
        for power in powers:term=power_product(term,{tuple(p*power for p in index):c for index,c in inner.items()})
        result=poly_add(result,term)
    return result
def high_algebra_checks():
    # First cross-validate this second reference algebra against direct polynomials.
    for degree in range(6):
        for index in partitions(degree):
            for basis in range(5):
                check('reference algebra agreement',(basis,index),reference_power(basis,index),power_expansion(basis,index))
    for degree in range(6,9):
        for index in partitions(degree):
            for basis in range(5):
                source=function(basis,{index:1});expected=reference_power(basis,index)
                for destination in range(5):
                    out=Function();status=call('symmetric_change_basis',source,destination,out)
                    check('basis degree6-8',(basis,index,destination),(status,reference_function_power(out)),(0,expected))
        print('HIGH BASIS DEGREE',degree,'complete',flush=True)
    # Every pair of homogeneous singleton basis elements whose degree product
    # is at most eight, with positive degrees; constants/zero/mixed sums below.
    for left_degree in range(1,9):
        for right_degree in range(1,8//left_degree+1):
            for left in partitions(left_degree):
                for right in partitions(right_degree):
                    for left_basis in range(5):
                        for right_basis in range(5):
                            outer=function(left_basis,{left:1});inner=function(right_basis,{right:1});out=Function()
                            expected=reference_plethysm(outer,inner);status=call('symmetric_plethysm',outer,inner,out)
                            check('plethysm degree8',(left_basis,left,right_basis,right),(status,reference_function_power(out)),(0,expected))
        print('HIGH PLETHYSM OUTER DEGREE',left_degree,'complete',flush=True)
    for basis in range(5):
        source=function(basis,{():F(-1,2),(1,):2,(2,):F(3,2),(1,1):F(-1,3)})
        for destination in range(5):
            out=Function();status=call('symmetric_change_basis',source,destination,out)
            check('mixed rational sums',(basis,destination),(status,reference_function_power(out)),(0,reference_function_power(source)))
        for inner_basis in range(5):
            inner=function(inner_basis,{():F(1,2),(1,):2,(2,):F(-1,3)})
            out=Function();status=call('symmetric_plethysm',source,inner,out)
            check('mixed rational plethysm',(basis,inner_basis),(status,reference_function_power(out)),(0,reference_plethysm(source,inner)))

EXTRA_FAMILIES={'characters':character_checks,'algebra':algebra_checks,'random':random_checks,'ui':ui_checks,'bounds':boundary_checks,'types':type_checks,'high-algebra':high_algebra_checks}
if __name__=='__main__':sys.exit(main())
