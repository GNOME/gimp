## Process this file with automake to produce Makefile.in

SUBDIRS = colors mode transforms

helpdatadir = $(gimpdatadir)/help/C/image/image

helpdata_DATA = \
        duplicate.html \
        index.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files install-data-local

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

install-data-local:
	rm -f $(helpdatadir)/dialogs
	ln -sf ../../dialogs $(helpdatadir)/dialogs
