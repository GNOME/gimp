## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/toolbox/help

helpdata_DATA = \
	context_help.html \
        index.html

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files install-data-local

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

install-data-local:
	rm -f $(helpdatadir)/dialogs
	$(LN_S) ../../dialogs $(helpdatadir)/dialogs
	rm -f $(helpdatadir)/filters
	$(LN_S) ../filters $(helpdatadir)/filters
