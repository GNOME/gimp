## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	makefile.cygwin		\
	makefile.msc		\
	twain/README		\
	twain/tw_dump.c		\
	twain/tw_dump.h		\
	twain/tw_func.c		\
	twain/tw_func.h		\
	twain/tw_sess.c		\
	twain/tw_util.c		\
	twain/tw_util.h		\
	twain/twain.c		\
	twain/twain.h		\
	winsnap/resource.h	\
	winsnap/select.cur	\
	winsnap/small.ico	\
	winsnap/winsnap.c	\
	winsnap/winsnap.h	\
	winsnap/winsnap.ico	\
	winsnap/winsnap.rc

if BUILD_PYTHON
pygimp=pygimp
else
pygimp=
endif

SUBDIRS = 			\
	libgck			\
	megawidget		\
	gpc			\
	dbbrowser		\
	script-fu		\
	$(GIMP_PERL)		\
	AlienMap		\
	AlienMap2		\
	FractalExplorer		\
	Lighting		\
	MapObject		\
	bmp			\
	borderaverage		\
	faxg3			\
	fits			\
	flame			\
	fp			\
	gap			\
	gdyntext		\
	gfig			\
	gfli			\
	gimpressionist		\
	ifscompose		\
	maze			\
	mosaic			\
	pagecurl		\
	print			\
	$(pygimp)		\
	rcm			\
	sgi			\
	sel2path		\
	sinus			\
	struc			\
	unsharp			\
	$(WEBBROWSER)		\
	$(XJT)			\
	common

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
