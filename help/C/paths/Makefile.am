## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/paths

helpdata_DATA = \
        copy_path.html \
        delete_path.html \
        duplicate_path.html \
        index.html \
        new_path.html \
        paste_path.html \
        path_to_selection.html \
        stroke_path.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files install-data-local

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

install-data-local:
	rm -f $(DESTDIR)$(helpdatadir)/dialogs
	$(LN_S) ../dialogs/paths $(DESTDIR)$(helpdatadir)/dialogs
	rm -f $(DESTDIR)$(helpdatadir)/filters
	$(LN_S) ../filters $(DESTDIR)$(helpdatadir)/filters
