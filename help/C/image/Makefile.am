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
	rm -f $(helpdatadir)/dialogs
	$(LN_S) ../dialogs $(helpdatadir)/dialogs
	rm -f $(helpdatadir)/file
	$(LN_S) ../file $(helpdatadir)/file
	rm -f $(helpdatadir)/filters
	$(LN_S) ../filters $(helpdatadir)/filters
	rm -f $(helpdatadir)/layers
	$(LN_S) ../layers $(helpdatadir)/layers
	rm -f $(helpdatadir)/toolbox
	$(LN_S) ../toolbox $(helpdatadir)/toolbox
	rm -f $(helpdatadir)/tools
	$(LN_S) ../tools $(helpdatadir)/tools
