## Process this file with automake to produce Makefile.in

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

install-data-local:
	rm -f $(helpdatadir)/dialogs
	ln -sf ../dialogs $(helpdatadir)/dialogs
	rm -f $(helpdatadir)/file
	ln -sf ../file $(helpdatadir)/file
	rm -f $(helpdatadir)/filters
	ln -sf ../filters $(helpdatadir)/filters
