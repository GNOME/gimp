## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help

helpdata_DATA = \
	welcome.html	\
	contents.html	\
	index.html	\
	wilber.png

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
