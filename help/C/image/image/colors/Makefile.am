## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/image/image/colors

helpdata_DATA = \
        desaturate.html \
        equalize.html \
        index.html \
        invert.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
