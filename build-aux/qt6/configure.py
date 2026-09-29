#!/usr/bin/env python3
"""Export CMake's resolved Qt 6 usage requirements for the Autotools build.

Qt's .pc files omit dependencies of static libraries and platform plugins.
The CMake File API supplies these without a second, hand-maintained link list.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess

parser = argparse.ArgumentParser(__doc__)
parser.add_argument('--build-dir', required=True)
parser.add_argument('--prefix', default='')
parser.add_argument('--host-tools', default='')
parser.add_argument('--host', default='')
parser.add_argument('--dbus', default='auto')
args = parser.parse_args()
build = Path(args.build_dir).resolve()
query = build / '.cmake/api/v1/query'
query.mkdir(parents=True, exist_ok=True)
(query / 'codemodel-v2').touch()
command = ['cmake', '-S', str(Path(__file__).resolve().parent), '-B', str(build),
           '-G', 'Unix Makefiles', '-UQt6*', '-DCMAKE_BUILD_TYPE=Release', '-DUSE_DBUS=' + args.dbus]
if args.prefix:
    command += ['-DCMAKE_PREFIX_PATH=' + args.prefix,
                '-DQt6_DIR=' + args.prefix + '/lib/cmake/Qt6']
if args.host_tools:
    command += ['-DQT_HOST_PATH=' + args.host_tools]
    # LinguistTools belongs to the native Qt installation.
    command += ['-DQt6LinguistTools_DIR=' + args.host_tools + '/lib/cmake/Qt6LinguistTools']
if 'mingw' in args.host:
    command += ['-DCMAKE_SYSTEM_NAME=Windows', '-DCMAKE_RC_COMPILER=' + args.host + '-windres']
env = os.environ.copy()
# Autoconf's accumulated CPPFLAGS/LDFLAGS must not affect CMake's Qt discovery.
for key in ('CFLAGS', 'CXXFLAGS', 'CPPFLAGS', 'LDFLAGS', 'CMAKE_MODULE_PATH'):
    env.pop(key, None)
subprocess.run(command, env=env, check=True)
subprocess.run(['cmake', '--build', str(build), '--parallel', '2'], env=env, check=True)
reply = build / '.cmake/api/v1/reply'
index = json.loads(max(reply.glob('index-*.json'), key=lambda p: p.stat().st_mtime).read_text())
model = json.loads((reply / index['reply']['codemodel-v2']['jsonFile']).read_text())
values = {}


def libtool_link_fragment(fragment):
    # Libtool moves bare absolute .so paths ahead of static archives, where
    # --as-needed drops them. Keep shared libraries in CMake's dependency order
    # using GNU ld's exact-filename library syntax (Linux and MinGW targets).
    result = []
    for token in shlex.split(fragment):
        path = Path(token)
        if path.is_absolute() and (path.suffix == '.so' or '.so.' in path.name):
            result.extend(['-L' + str(path.parent), '-l:' + path.name])
        else:
            result.append(token)
    return shlex.join(result)


for target in model['configurations'][0]['targets']:
    name = target['name']
    if name not in ('qt_probe', 'qt_Test_probe', 'qt_DBus_probe'):
        continue
    data = json.loads((reply / target['jsonFile']).read_text())
    prefix = {'qt_probe': 'QT', 'qt_Test_probe': 'QT_TEST', 'qt_DBus_probe': 'QT_DBUS'}[name]
    flags = []
    for group in data['compileGroups']:
        flags.extend('-I' + shlex.quote(i['path']) for i in group.get('includes', []))
        flags.extend('-D' + shlex.quote(d['define']) for d in group.get('defines', []))
    values[prefix + '_INCLUDES'] = ' '.join(dict.fromkeys(flags))
    values[prefix + '_LIBS'] = ' '.join(
        libtool_link_fragment(f['fragment']) for f in data['link']['commandFragments']
        if f['role'] in ('libraries', 'libraryPath', 'frameworkPath')
    )
for line in (build / 'tools.txt').read_text().splitlines():
    key, value = line.split('=', 1)
    values[key] = value
with (build / 'qt-vars.sh').open('w') as out:
    for key, value in values.items():
        out.write(key + '=' + shlex.quote(value) + '\n')
