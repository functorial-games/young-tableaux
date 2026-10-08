#!/usr/bin/env python3
"""Compile disposable mathematical mutants; production sources are never edited.

Baseline family must pass; mutant must compile AND fail with a mathematical
counterexample in its TSV. Compiler errors and crashes do not count as kills.
"""
import argparse
import pathlib
import subprocess
import sys
import tempfile
import independent_audit as audit

MUTANTS=[
 ('hook-arm','app/core/partition.c','int h = p->rows[r-1] - c + 1;','int h = p->rows[r-1] - c + 2;','partition'),
 ('weak-bump','app/core/tableau.c','tableau->entries[row][column]<=value','tableau->entries[row][column]<value','word'),
 ('jeu-tie','app/core/jeu_de_taquin.c','< tableau->filling.entries[below.row-1][below.column-1]','<= tableau->filling.entries[below.row-1][below.column-1]','jeu'),
 ('LR-no-lattice','app/core/algebra.c','if(lr_prefix_lattice(state)) lr_search(state,index+1);','if(true) lr_search(state,index+1);','algebra'),
 ('MN-sign','app/core/algebra.c','if(!(height&1)) sub=-sub;','if(height&1) sub=-sub;','characters'),
 ('missing-edge','app/core/combinatorics.c','int count=grow?partition_addable(input,corners):partition_removable(input,corners);','int count=grow?partition_addable(input,corners):partition_removable(input,corners); if(grow && count) --count;','graph'),
 ('Bruhat-reversed','app/core/combinatorics.c','if(first>second) result=false;','if(first<second) result=false;','bruhat'),
 ('inverse-removal','app/core/combinatorics.c','for(int label=count;label>0;--label)','for(int label=1;label<=count;++label)','permutation'),
]

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--compiler',required=True);args=parser.parse_args()
    out=audit.ROOT/'build/mutations';out.mkdir(parents=True,exist_ok=True)
    baseline=audit.ROOT/'build/independent-audit.so';audit.compile_library(args.compiler,baseline)
    verified=set();results=[]
    for name,source,old,new,family in MUTANTS:
        if family not in verified:
            result=subprocess.run([sys.executable,str(audit.ROOT/'tests/independent_audit.py'),'--library',str(baseline),'--family',family,'--receipt',str(out/f'baseline-{family}.tsv')],stdout=subprocess.DEVNULL)
            if result.returncode:raise RuntimeError(f'Baseline {family} did not pass')
            verified.add(family)
        with tempfile.TemporaryDirectory(prefix='yt-mutant-') as temporary:
            temporary=pathlib.Path(temporary);text=(audit.ROOT/source).read_text();assert text.count(old)==1,(name,text.count(old))
            altered=temporary/pathlib.Path(source).name;altered.write_text(text.replace(old,new))
            library=temporary/'mutant.so';audit.compile_library(args.compiler,library,(source,altered))
            receipt=out/f'{name}.tsv'
            with (out/f'{name}.log').open('w') as log:
                result=subprocess.run([sys.executable,str(audit.ROOT/'tests/independent_audit.py'),'--library',str(library),'--family',family,'--receipt',str(receipt)],stdout=log,stderr=subprocess.STDOUT)
            evidence=[line for line in receipt.read_text().splitlines() if line.startswith('FAIL\t')] if receipt.exists() else []
            killed=result.returncode==1 and bool(evidence)
            results.append((name,family,'KILLED' if killed else 'SURVIVED_OR_INVALID',evidence[0] if evidence else 'no mathematical mismatch receipt'))
            print(*results[-1],sep='\t',flush=True)
    with (out/'summary.tsv').open('w') as summary:
        summary.write('mutant\tfamily\tresult\tfirst_counterexample\n')
        for row in results:summary.write('\t'.join(row)+'\n')
    return int(any(row[2]!='KILLED' for row in results))

if __name__=='__main__':sys.exit(main())
