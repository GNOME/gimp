#!/bin/sh

xgettext --default-domain=gimp-libgimp --directory=.. \
  --add-comments --keyword=_ --keyword=N_ \
  --files-from=./POTFILES.in \
&& test ! -f gimp-libgimp.po \
   || ( rm -f ./gimp-libgimp.pot \
    && mv gimp-libgimp.po ./gimp-libgimp.pot )
