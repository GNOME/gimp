#!/bin/sh

xgettext --default-domain=gimp --directory=.. \
  --add-comments --keyword=_ --keyword=N_ \
  --files-from=./POTFILES.in \
&& test ! -f gimp.po \
   || ( rm -f ./gimp.pot \
    && mv gimp.po ./gimp.pot )
