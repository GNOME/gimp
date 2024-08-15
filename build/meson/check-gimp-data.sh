#!/bin/sh

cd $MESON_SOURCE_ROOT
changes=$(git status --untracked-files=no --short --ignore-submodules=dirty gimp-data|wc -l)
exit $changes
