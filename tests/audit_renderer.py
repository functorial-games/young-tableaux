#!/usr/bin/env python3
"""Isolated rendering bound probes: traps cannot terminate the oracle process."""
import argparse
import pathlib
import subprocess
import sys

parser=argparse.ArgumentParser();parser.add_argument('--compiler',required=True)
parser.add_argument('--source',default=str(pathlib.Path(__file__).resolve().parents[1]))
args=parser.parse_args();root=pathlib.Path(args.source).resolve()
out=root/'build/render-audit';out.mkdir(parents=True,exist_ok=True)
test=pathlib.Path(__file__).with_name('audit_render_bounds.c')
sources=list(sorted((root/'app/core').glob('*.c')))+[root/x for x in ['app/ui/controls.c','app/console/console.c','app/console/nearby.c','app/render/raster.c','app/render/paint.c']]
command=[args.compiler,'--target=x86_64-linux-gnu','--sysroot=/','-rtlib=libgcc','-unwindlib=libgcc','-std=c17','-O1','-g','-fsanitize=signed-integer-overflow','-fsanitize-trap=signed-integer-overflow']+[f'-I{root/x}' for x in ['app/core','app/ui','app/console','app/render']]+[str(test)]+list(map(str,sources))+['-lm','-o',str(out/'render-bounds')]
subprocess.run(command,check=True);failed=False
with (out/'receipt.tsv').open('w') as receipt:
    receipt.write('operation\tinput\tactual_exit\texpected_exit\tresult\n')
    for operation,text in [('word','1'),('biword','1;1'),('word','2147483647'),('biword','1;2147483647')]:
        result=subprocess.run([str(out/'render-bounds'),operation,text],capture_output=True,text=True)
        status='PASS' if result.returncode==0 else 'UNVERIFIED' if result.returncode==77 else 'FAIL'
        receipt.write(f'{operation}\t{text}\t{result.returncode}\t0\t{status}\n')
        print(operation,text,status,result.returncode,flush=True)
        failed|=status=='FAIL'
sys.exit(int(failed))
