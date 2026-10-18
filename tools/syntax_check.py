#!/usr/bin/env python3
"""Catch host C syntax mistakes with tiny header shims; NOT a PSP runtime test."""
import os,re,subprocess,tempfile,pathlib
root=pathlib.Path(__file__).resolve().parents[1]
src=(root/'src/main.c').read_text()
with tempfile.TemporaryDirectory() as temp:
 p=pathlib.Path(temp)
 h=['#pragma once','#include <stdint.h>','#include <stddef.h>',
  'typedef unsigned int SceSize;',
  'typedef struct {float x,y,z;} ScePspFVector3;',
  'typedef struct {unsigned int Buttons;unsigned char Lx,Ly;} SceCtrlData;',
  '#define PSP_MODULE_INFO(a,b,c,d)',
  '#define PSP_MAIN_THREAD_ATTR(a)',
  '#define THREAD_ATTR_USER 1',
  '#define THREAD_ATTR_VFPU 2']
 for c in sorted(set(re.findall(r'\bGU_[A-Z][A-Z_0-9]*\b|\bPSP_CTRL_[A-Z_]+\b',src))):
  h.append('#define '+c+' '+str(len(h)%999 + 1))
 names=sorted(set(re.findall(r'\b(sce[A-Za-z0-9_]+)\s*\(',src)))
 for name in names:
  if name in ('sceGuGetMemory',):h.append('void *'+name+'();')
  elif name in ('sceKernelCreateThread','sceKernelCreateCallback',):h.append('int '+name+'();')
  else:h.append('void '+name+'();')
 (p/'shim.h').write_text('\n'.join(h)+'\n')
 for name in ['pspkernel.h','pspctrl.h','pspdisplay.h','pspgu.h','pspgum.h']:
  (p/name).write_text('#include "shim.h"\n')
 cmd=['gcc','-std=gnu99','-O1','-fsyntax-only','-Wall','-Wextra','-Wno-unused-function','-I',str(p),str(root/'src/main.c')]
 res=subprocess.run(cmd,text=True,capture_output=True)
 print('Syntax-check command:', ' '.join(cmd))
 print(res.stdout+res.stderr)
 if res.returncode:raise SystemExit(res.returncode)
 print('PASS: host C syntax (not PSP SDK compilation).')
 exe=str(p/'test_host')
 link=['gcc','-std=gnu99','-O2','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I',str(p),str(root/'tools/test_model.c'),'-lm','-o',exe]
 res=subprocess.run(link,text=True,capture_output=True)
 print(res.stdout+res.stderr)
 if res.returncode:raise SystemExit('Host gameplay test compile failed')
 res=subprocess.run([exe],text=True,capture_output=True)
 print(res.stdout+res.stderr)
 if res.returncode:raise SystemExit('Host gameplay test failed')
