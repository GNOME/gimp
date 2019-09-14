#!/usr/bin/env python3

import sys, os
from pathlib import Path
import argparse

import json

import shutil


class Singleton(type):
    _instances = {}
    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]

def copytree(src, dst, symlinks=False, ignore=None):
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            shutil.copytree(s, d, symlinks, ignore)
        else:
            shutil.copy2(s, d)

class MesonStatus(metaclass = Singleton):
    def __init__(self):
        self.get_build_dir()
        self.get_meson_info()
        self.get_meson_installed()

    def get_build_dir(self):
        self.buildroot = None

        # Set up by meson.
        cwd = Path(os.environ['MESON_BUILD_ROOT'])

        if (cwd / 'meson-info').exists():
            self.buildroot = cwd

        if self.buildroot is None:
            print('Error: build directory was not detected. Are you in it ?')

    def get_meson_info(self):
        with open(self.buildroot / 'meson-info' / 'meson-info.json') as file:
            self.meson_info = json.load(file)
        self.sourceroot = Path(self.meson_info['directories']['source'])

    def get_meson_installed(self):
        info = self.meson_info['directories']['info']
        inst = self.meson_info['introspection']['information']['installed']['file']

        with open(Path(info) / inst) as file:
            self.installed_files = json.load(file)


def get_files_of_part(part_name):
    files_of_part = {}
    sourcedir = str(MesonStatus().sourceroot / part_name)
    builddir  = str(MesonStatus().buildroot  / part_name)
    for file, target in MesonStatus().installed_files.items():
        if file.startswith(sourcedir + '/') or file.startswith(builddir + '/'):
            files_of_part[file] = target

    return files_of_part

def install_files(files, verbose):
    warnings = []
    for file in sorted(files.keys()):
        target = files[file]
        if verbose: print(file + '  →  ' + target, end='\n')

        if os.path.isdir(file):
            copytree(file, target)
        if os.path.isfile(file):
            try:
                shutil.copy2(file, target)
            except Exception as e:
                warnings += [(file, e)]
    if len(warnings) > 0:
        sys.stderr.write("\n*** WARNING: *** Some file installation failed:\n")
        for (file, e) in warnings:
            sys.stderr.write("- {}: {}\n".format(file, e))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('subdirs', nargs='+')
    parser.add_argument('--verbose', '-v', action='store_true')
    args = parser.parse_args()

    verbose = args.verbose

    for subdir in args.subdirs:
        files = get_files_of_part(subdir)
        if len(files) == 0:
            print('Error: subdir %s does not contain any installable file' % subdir)
        install_files(files, verbose)
