#!/bin/sh

test -f ../MANIFEST || exec echo "must be started in plug-ins/perl/po"

if test $# -eq 1 -a "$1" = "--pot"; then
	perl ../pxgettext `find .. -type f -print | grep '\.pm$\|\.xs$\|examples/\|Perl-Server'` > gimp-perl.pot
else
	make update-po
fi
