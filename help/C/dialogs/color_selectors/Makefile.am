## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/dialogs/color_selectors

helpdata_DATA = \
        built_in.html \
        gtk.html \
        index.html \
        triangle.html \
        watercolor.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
