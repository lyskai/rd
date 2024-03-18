#!/usr/bin/env python3
# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary.

import sys, os, subprocess, re, glob
import argparse, shutil

# Regex filters for objdump
re_needed = re.compile('.*NEEDED');
re_rpath  = re.compile('.*R.*PATH');
re_march  = re.compile('.*achine:');

# Global excludes for dependencies
exclude_deps = [
#        re.compile('ld-linux.*.so.*'),
        ]

# Global excludes for paths
exclude_paths = [
#        re.compile('/tmp/dummy'),
        ]

verbose = False
def verb_print(s):
    if verbose: print(s, flush=True)

# Run command and filter its output with a regex
def run_subproc(cmd, re_filt=None):
    verb_print("run-subproc: {}".format(cmd))
    output = []
    r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    verb_print(r.stderr)
    for l in r.stdout.splitlines():
        l = l.decode()
        if not re_filt or re_filt.match(l):
            output.append(l)
    return output

# Check if string is in the blacklist d(dict)
def in_blacklist(d, n):
    for e in d:
        if e.match(n):
            return True
    return False

# Init blacklist d (dict) with exclusion regex list
def init_blacklist(d, ex_list):
    if not ex_list: return
    for e in ex_list:
        d.append(re.compile(e))

# Figure out RPATH for the ELF object
def get_rpath(o):
    verb_print("get-rpath: {}".format(o))
    rpath = []
    for r in run_subproc(['objdump', '-p', o], re_rpath):
        # Typical format is:
        #     RPATH  /lib:/opt/xyz/lib:
        for p in r.split()[1].strip(' :').split(':'):
            if len(p) and not in_blacklist(exclude_paths, p) and os.path.exists(p):
                rpath.append(p)
    return rpath

# Figure out MARCH for the ELF file
def get_march(o):
    verb_print("get-march: {}".format(o))
    arch = []
    for r in run_subproc(['readelf', '-h', o], re_march):
        # Typical format is:
        #     Machine:  Name
        return r.split(':')[1].strip()
    return 'unknown'

# Find a single file in rpath
def find_file(n, rpath):
    return run_subproc(['find'] + rpath + ['-name', n])

# Find all dependencies, and add to deps (dict)
def find_all_deps(o, march, rpath, deps):
    verb_print("checking: {} march: {} rpath: {}".format(o, march, rpath))
    for l in run_subproc(['objdump', '-p', o], re_needed):
        # Typical format is:
        #   NEEDED     libpthread.so.0
        #   NEEDED     libhogl.so.4
        n = l.split()[1]
        if n in deps or in_blacklist(exclude_deps, n):
            continue

        f_list = find_file(n, rpath)
        verb_print(f_list)
        for f in f_list:
            ma = get_march(f)
            if ma == march:
                deps[n] = f
                find_all_deps(f, march, rpath, deps)
                break

# Create exec wrapper
def create_wrapper(exe, deps, outdir):
    base = os.path.basename(exe)
    interp = None
    for i in deps.keys():
        if 'ld-linux' in i:
            interp = i

    if not interp:
        print('bundle-runtime: cannot find ld.so in {}'.format(deps))
        sys.exit(1)

    shutil.copyfile(exe, '{}/{}.bin'.format(args.outdir, base), follow_symlinks=True)
    wrapper_name = '{}/{}'.format(outdir, base)
    with open(wrapper_name, 'w') as f:
        f.write('#!/bin/bash\n')
        f.write('d=$(dirname $0)\n')
        f.write('exec $d/{} --inhibit-cache --library-path $d $d/{}.bin $@\n'.format(interp, base))
    os.chmod(wrapper_name, 0o755)

######
if __name__=='__main__':
    parser = argparse.ArgumentParser(description='bundle-runtime')
    parser.add_argument('--verbose', action='store_true', help='Enable verbose mode')
    parser.add_argument('--dryrun',  action='store_true', help='Do not copy anything just output the list to stdout')
    parser.add_argument('--outdir',  help='Copy all dependencies into this directory')
    parser.add_argument('--sysroot', default='', help='Sysroot for library searching')
    parser.add_argument('--path',    default='/lib64:/usr/lib64:/lib:/usr/lib', help='Additional path to add to the rpath of the executable')
    parser.add_argument('--exclude-deps',  action='append', help='Exclude dependencies that match regex')
    parser.add_argument('--exclude-paths', action='append', help='Exclude paths that match regex')
    parser.add_argument('--exec-wrap', action='store_true', help='Generate executable wrapper for running the original binary')
    parser.add_argument('input', nargs='+', help='Input file(s)')
    args = parser.parse_args()

    verbose = args.verbose

    if not args.dryrun and not args.outdir:
        print('--dryrun or --outdir options are mandatory');
        sys.exit(1)

    init_blacklist(exclude_deps, args.exclude_deps)
    init_blacklist(exclude_paths, args.exclude_paths)

    # Process dependencies
    deps = {}
    for i in args.input:
        if not os.path.isfile(i):
            print('file {} cannot be processed'.format(i));
            sys.exit(1)

        verb_print('bundle-runtime: {} --> {}'.format(i, args.outdir))
        rpath = get_rpath(i)
        for p in args.path.split(':'):
            p = args.sysroot + p
            if os.path.exists(p): rpath.append(p)
        march = get_march(i)
        find_all_deps(i, march, rpath, deps)

    if args.dryrun or verbose:
        print(deps)

    # Copy dependencies
    if not args.dryrun:
        shutil.rmtree(args.outdir, ignore_errors=True)
        os.makedirs(args.outdir, exist_ok=True)
        for d in deps.values():
            verb_print('adding {} to {}'.format(d, args.outdir));
            shutil.copy(d, args.outdir, follow_symlinks=True)
        print('bundle-runtime: {} deps in {} '.format(len(deps), args.outdir))

    # Generate wrappers
    if not args.dryrun and args.exec_wrap:
        for i in args.input:
            create_wrapper(i, deps, args.outdir)

    if args.dryrun or verbose:
        print(deps)
