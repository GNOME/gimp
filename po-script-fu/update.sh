#!/bin/sh

xgettext --default-domain=gimp-script-fu --directory=.. \
  --add-comments --keyword=_ --keyword=N_ \
  --files-from=./POTFILES.in \
&& ./script-fu-xgettext \
	../plug-ins/script-fu/scripts/*.scm \
        ../plug-ins/gap/sel-to-anim-img.scm \
        ../plug-ins/webbrowser/web-browser.scm \
        >> gimp-script-fu.po \
&& test ! -f gimp-script-fu.po \
   || ( rm -f ./gimp-script-fu.pot \
    && mv gimp-script-fu.po ./gimp-script-fu.pot )
