## Process this file with automake to produce Makefile.in

SUBDIRS = libgimpcolor libgimpmath libgimpbase libgimpwidgets libgimp

EXTRA_DIST = \
	ChangeLog	\
	README		\
	README.gtkdoc	\
	gih.txt  	\
	gpb.txt  	\
	parasites.txt	\
	undo.txt	\
	xcf.txt

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files macros/*; do \
	  echo $$p; \
	done
