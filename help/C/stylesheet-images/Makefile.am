## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/stylesheet-images

helpdata_DATA = \
		note.gif \
		tip.gif \
		warning.gif


EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
