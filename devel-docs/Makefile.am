SUBDIRS = libgimpcolor libgimpmath libgimpbase libgimpwidgets libgimp pdb

EXTRA_DIST = \
	README			\
	README.gtkdoc		\
	gih.txt  		\
	gpb.txt  		\
	parasites.txt		\
	undo.txt		\
	xcf.txt

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files macros/*; do \
	  echo $$p; \
	done
