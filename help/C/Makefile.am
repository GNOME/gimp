## Process this file with automake to produce Makefile.in

SUBDIRS = dialogs tools layers channels paths toolbox image open save filters file 

helpdatadir = $(gimpdatadir)/help/C

helpdata_DATA = \
        contents.html \
        index.html \
        welcome.html 

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
