## Process this file with automake to produce Makefile.in

SUBDIRS = edit select view image 

helpdatadir = $(gimpdatadir)/help/C/image

helpdata_DATA = \
        image_window.html \
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
	rm -f $(DESTDIR)$(helpdatadir)/dialogs
	$(LN_S) ../dialogs $(DESTDIR)$(helpdatadir)/dialogs
	rm -f $(DESTDIR)$(helpdatadir)/file
	$(LN_S) ../file $(DESTDIR)$(helpdatadir)/file
	rm -f $(DESTDIR)$(helpdatadir)/filters
	$(LN_S) ../filters $(DESTDIR)$(helpdatadir)/filters
	rm -f $(DESTDIR)$(helpdatadir)/layers
	$(LN_S) ../layers $(DESTDIR)$(helpdatadir)/layers
	rm -f $(DESTDIR)$(helpdatadir)/toolbox
	$(LN_S) ../toolbox $(DESTDIR)$(helpdatadir)/toolbox
	rm -f $(DESTDIR)$(helpdatadir)/tools
	$(LN_S) ../tools $(DESTDIR)$(helpdatadir)/tools
