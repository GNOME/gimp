## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/open

helpdata_DATA = \
        index.html \
        open_by_extension.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files install-data-local

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

install-data-local:
	rm -f $(DESTDIR)$(helpdatadir)/dialogs
	$(LN_S) ../dialogs $(DESTDIR)$(helpdatadir)/dialogs
	rm -f $(DESTDIR)$(helpdatadir)/filters
	$(LN_S) ../filters $(DESTDIR)$(helpdatadir)/filters
