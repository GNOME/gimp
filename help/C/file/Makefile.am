## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/file

helpdata_DATA = \
        close.html \
        index.html \
        last_opened.html \
        quit.html \
        revert.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
