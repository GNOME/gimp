## Process this file with automake to produce Makefile.in

SUBDIRS = gck

EXTRA_DIST = \
	ChangeLog				\
	NEWS					\
	README					\
	TODO					\
	docs/html/gck.html			\
	docs/html/gck_application_window.html	\
	docs/html/gck_dialog_window.html	\
	docs/html/gckcolor.html			\
	docs/html/gckmath.html			\
	docs/html/gckui.html			\
	docs/html/gckvector.html

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null `; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done
