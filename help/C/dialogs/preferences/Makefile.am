## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/dialogs/preferences

helpdata_DATA = \
	default_comment.html \
        directories.html \
        display.html \
        environment.html \
        index.html \
        interface.html \
        monitor.html \
        new_file.html \
        preferences.html \
        session.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
