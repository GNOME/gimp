## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/image/select

helpdata_DATA = \
        all.html \
        float.html \
        index.html \
        invert.html \
        none.html \
        save_to_channel.html \
        sharpen.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files install-data-local

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

install-data-local:
	rm -f $(DESTDIR)$(helpdatadir)/dialogs
	$(LN_S) ../../dialogs $(DESTDIR)$(helpdatadir)/dialogs
