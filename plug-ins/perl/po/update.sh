#!/bin/sh

test -f ../MANIFEST || exec echo "must be started in plug-ins/perl/po"

make update-po

