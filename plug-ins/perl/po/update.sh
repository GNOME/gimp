#!/bin/sh

cd ..
test -f MANIFEST || exec echo "must be started in plug-ins/perl/po"

./pxgettext `find . -name '*.pm' -o -name '*.xs' -o -path './examples/*'` Perl-Server |
   msgmerge -w 83 po/gimp-perl.pot - >gimp-perl.pot~ &&
   mv gimp-perl.pot~ po/gimp-perl.pot

for po in po/*.po; do
   msgmerge -w 83 $po po/gimp-perl.pot >$po~ && mv $po~ $po
done

