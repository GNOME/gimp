## Process this file with automake to produce Makefile.in

SUBDIRS = help

helpdatadir = $(gimpdatadir)/help/C/toolbox

helpdata_DATA = \
        index.html \
        toolbox.html 

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
	$(LN_S) ../dialogs $(helpdatadir)/dialogs
	rm -f $(helpdatadir)/file
	$(LN_S) ../file $(helpdatadir)/file
	rm -f $(helpdatadir)/filters
	$(LN_S) ../filters $(helpdatadir)/filters
