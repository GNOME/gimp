## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/images/examples

helpdata_DATA = \
		border-selection-01.png \
		border-selection-02.png

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

