## Process this file with automake to produce Makefile.in

SUBDIRS = stack 

helpdatadir = $(gimpdatadir)/help/C/layers

helpdata_DATA = \
        add_alpha_channel.html \
        alpha_to_selection.html \
        anchor_layer.html \
        delete_layer.html \
        duplicate_layer.html \
        flatten_image.html \
        index.html \
        mask_to_selection.html \
        merge_down.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

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
