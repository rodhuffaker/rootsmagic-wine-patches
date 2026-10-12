#!/usr/bin/env python3
"""Run offline synthetic probes using an explicitly isolated Wine prefix."""
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time

if len(sys.argv) != 3:
    sys.exit(f"Usage: {sys.argv[0]} /private/runtime/bin/wine /isolated/test-prefix")
base = Path(__file__).resolve().parent
wine = Path(sys.argv[1]).expanduser().resolve()
prefix = Path(sys.argv[2]).expanduser().resolve()
if not wine.is_file() or not prefix.is_dir():
    sys.exit("Wine runner and an existing isolated prefix are required")
if prefix == (Path.home() / '.wine').resolve():
    sys.exit("Refusing to run probes in the daily ~/.wine prefix")
for tool in ('i686-w64-mingw32-gcc', 'x86_64-w64-mingw32-gcc'):
    if not __import__('shutil').which(tool):
        sys.exit(f"Missing compiler: {tool}")
env = os.environ.copy()
for key in list(env):
    if key.startswith('ROOTSMAGIC_') or key in ('WINEARCH', 'WINEDLLPATH', 'WINELOADER', 'WINESERVER'):
        env.pop(key)
env.update(WINEPREFIX=str(prefix), WINEDEBUG='-all', WINE_D3D_CONFIG='renderer=vulkan',
           WINEDLLOVERRIDES='d2d1,dwrite=b;d3d11,d3d10core,dxgi=b;winhttp,wininet=b;mscoree,mshtml=')

def run(cmd, **kwargs):
    subprocess.run([str(c) for c in cmd], check=True, **kwargs)

def server_probe(name, exe):
    with socket.socket() as check:
        check.bind(('127.0.0.1', 19717))
    proc = subprocess.Popen([sys.executable, str(base / 'tests' / name)],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    try:
        for _ in range(100):
            if proc.poll() is not None:
                raise RuntimeError(f"Probe server exited: {proc.stderr.read().decode()}")
            try:
                with socket.create_connection(('127.0.0.1', 19717), timeout=.1):
                    break
            except OSError:
                time.sleep(.05)
        else:
            raise RuntimeError('Probe server did not start')
        run([wine, exe], env=env, timeout=60)
    finally:
        proc.terminate()
        try:
            proc.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.communicate()

with tempfile.TemporaryDirectory(prefix='rm-wine-probes-') as tmp:
    d = Path(tmp)
    for name in ('disjoint-layer-clip', 'pedigree-line-join-probe'):
        exe = d / f'{name}.exe'
        run(['i686-w64-mingw32-gcc', '-std=c99', '-O2', '-Wall', '-Werror',
             base/'tests'/f'{name}.c', '-o', exe,
             '-ld2d1', '-ld3d11', '-ldxguid', '-luuid', '-lole32'])
        print(f'Running {name}', flush=True)
        run([wine, exe], env=env, timeout=60)
    for arch, compiler in (('i386', 'i686-w64-mingw32-gcc'),
                           ('x64', 'x86_64-w64-mingw32-gcc')):
        exe = d / f'winhttp-{arch}.exe'
        run([compiler, '-std=c99', '-O2', '-Wall', '-Werror',
             base/'tests'/'winhttp-gzip-reuse.c', '-o', exe, '-lwinhttp'])
        print(f'Running gzip/chunked WinHTTP probe: {arch}', flush=True)
        server_probe('winhttp-gzip-chunked-server.py', exe)
print('PASS: Direct2D layer/join and 32/64-bit compressed HTTP probes')
