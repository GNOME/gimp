#!/bin/sh

xgettext --default-domain=gimp-script-fu --directory=.. \
  --add-comments --keyword=_ --keyword=N_ \
  --files-from=./POTFILES.in \
&& test ! -f gimp-script-fu.po \
   || ( rm -f ./gimp-script-fu.pot \
    && mv gimp-script-fu.po ./gimp-script-fu.pot )
