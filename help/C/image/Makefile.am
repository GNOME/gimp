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
	ln -sf ../dialogs $(helpdatadir)/dialogs
	rm -f $(helpdatadir)/file
	ln -sf ../file $(helpdatadir)/file
	rm -f $(helpdatadir)/filters
	ln -sf ../filters $(helpdatadir)/filters
	rm -f $(helpdatadir)/layers
	ln -sf ../layers $(helpdatadir)/layers
	rm -f $(helpdatadir)/toolbox
	ln -sf ../toolbox $(helpdatadir)/toolbox
	rm -f $(helpdatadir)/tools
	ln -sf ../tools $(helpdatadir)/tools
