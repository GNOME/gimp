## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/image/edit

helpdata_DATA = \
        clear.html \
        copy.html \
        cut.html \
        fill.html \
        index.html \
        paste.html \
        paste_as_new.html \
        paste_into.html \
        redo.html \
        stroke.html \
        undo.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
