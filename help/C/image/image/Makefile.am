## Process this file with automake to produce Makefile.in

SUBDIRS = transforms colors 

helpdatadir = $(gimpdatadir)/help/C/image/image

helpdata_DATA = \
        convert_to_grayscale.html \
        convert_to_rgb.html \
        duplicate.html \
        index.html 

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
