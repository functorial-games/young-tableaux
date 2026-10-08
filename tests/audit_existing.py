#!/usr/bin/env python3
"""Rerun existing native suites without changing their claims or assertions."""
import argparse
import pathlib
import subprocess
import independent_audit as audit

parser=argparse.ArgumentParser();parser.add_argument('--compiler',required=True);args=parser.parse_args()
root=audit.ROOT;out=root/'build/existing-audit';out.mkdir(parents=True,exist_ok=True)
common=[args.compiler,'--target=x86_64-linux-gnu','--sysroot=/','-rtlib=libgcc','-unwindlib=libgcc','-std=c17','-O2','-Wall','-Wextra','-Werror','-Wpedantic','-Wshadow']+[f'-I{root/x}' for x in ['app/core','app/ui','app/console','app/render']]
sources=list(map(str,sorted((root/'app/core').glob('*.c'))))+[str(root/x) for x in ['app/ui/controls.c','app/console/console.c','app/console/nearby.c','app/render/raster.c','app/render/paint.c']]
for name in ['host_tests','consolidation_tests','operation_correspondence']:
    binary=out/name
    subprocess.run(common+[str(root/f'tests/{name}.c')]+sources+['-lm','-o',str(binary)],check=True)
    arguments=[str(root/'types/YoungTableaux/Types.idr'),str(root/'types/YoungTableaux/Operations.idric')] if name=='operation_correspondence' else []
    result=subprocess.run([str(binary)]+arguments,check=True,text=True,capture_output=True)
    (out/f'{name}.txt').write_text(result.stdout+result.stderr);print(result.stdout.strip(),flush=True)
subprocess.run(common[:7]+[str(root/'third_party/lua/onelua.c'),'-lm','-o',str(out/'lua')],check=True)
result=subprocess.run([str(out/'lua'),str(root/'tests/facts_test.lua')],cwd=root,check=True,text=True,capture_output=True)
(out/'lua.txt').write_text(result.stdout+result.stderr);print(result.stdout.strip(),flush=True)
# Demonstrate that SPEC comments alone were the old Edric gate's oracle.
original=(root/'types/YoungTableaux/Operations.idric').read_text()
old='rsk_permutation_signature = permutation -> permutation_rsk_result'
assert original.count(old)==1
mutant=out/'wrong-actual-type.idric';mutant.write_text(original.replace(old,'rsk_permutation_signature = word -> permutation_rsk_result'))
result=subprocess.run([str(out/'operation_correspondence'),str(root/'types/YoungTableaux/Types.idr'),str(mutant)],text=True,capture_output=True)
(out/'actual-type-mutant.txt').write_text(f'exit={result.returncode}\n'+result.stdout+result.stderr)
print('Legacy gate with wrong actual Idric signature:',result.returncode,flush=True)
