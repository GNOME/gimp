#!/bin/sh

xgettext --default-domain=gimp-std-plugins --directory=.. \
  --add-comments --keyword=_ --keyword=N_ \
  --files-from=./POTFILES.in \
&& test ! -f gimp-std-plugins.po \
   || ( rm -f ./gimp-std-plugins.pot \
    && mv gimp-std-plugins.po ./gimp-std-plugins.pot )
