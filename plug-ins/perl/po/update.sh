#!/bin/sh

cd ..
test -f MANIFEST || exec echo "must be started in plug-ins/perl/po"

./pxgettext `find . -name '*.pm' -o -name '*.xs' -o -path './examples/*'` Perl-Server > po/gimp-perl.pot

