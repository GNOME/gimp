## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/file

helpdata_DATA = \
        close.html \
        index.html \
        last_opened.html \
        quit.html \
        revert.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files install-data-local

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

install-data-local:
	rm -f $(helpdatadir)/dialogs
	ln -sf ../dialogs $(helpdatadir)/dialogs
	rm -f $(helpdatadir)/filters
	ln -sf ../filters $(helpdatadir)/filters
	rm -f $(helpdatadir)/open
	ln -sf ../open $(helpdatadir)/open
	rm -f $(helpdatadir)/save
	ln -sf ../save $(helpdatadir)/save
