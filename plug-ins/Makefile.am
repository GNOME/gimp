## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	makefile.cygwin		\
	makefile.msc

SUBDIRS = 			\
	libgck			\
	megawidget		\
	gpc			\
	dbbrowser		\
	script-fu		\
	$(GIMP_PERL)		\
	AlienMap		\
	FractalExplorer		\
	Lighting		\
	MapObject		\
	bmp			\
	faxg3			\
	fits			\
	flame			\
	fp			\
	gap			\
	gdyntext		\
	gfig			\
	gfli			\
	ifscompose		\
	maze			\
	mosaic			\
	pagecurl		\
	print			\
	rcm			\
	sgi			\
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
