## Process this file with automake to produce Makefile.in

SUBDIRS = auto

helpdatadir = $(gimpdatadir)/help/C/image/image/colors

helpdata_DATA = \
        desaturate.html \
        index.html \
        invert.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done
