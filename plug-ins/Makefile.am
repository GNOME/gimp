## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	makefile.mingw		\
	makefile.mingw.in	\
	makefile.msc

## if BUILD_PYTHON
## pygimp=pygimp
## else
## pygimp=
## endif

if OS_WIN32
TWAIN = twain
WINSNAP = winsnap
XJT =
else
PRINT = print
endif

SUBDIRS = 			\
	libgck			\
	dbbrowser		\
	script-fu		\
	$(GIMP_PERL)		\
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
        gflare			\
	gfli			\
	gimpressionist		\
	$(HELPBROWSER)		\
	ifscompose		\
	imagemap		\
	maze			\
	mosaic			\
	pagecurl		\
	$(PRINT)		\
	rcm			\
	sgi			\
	sel2path		\
	$(TWAIN)		\
	$(WEBBROWSER)		\
	$(WINSNAP)		\
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

makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/plug-ins/makefile.mingw.in
	cd $(top_builddir) && CONFIG_FILES=plug-ins/$@ CONFIG_HEADERS= $(SHELL) ./config.status
