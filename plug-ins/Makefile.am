## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	makefile.mingw		\
	makefile.mingw.in	\
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

## if BUILD_PYTHON
## pygimp=pygimp
## else
## pygimp=
## endif

SUBDIRS = 			\
	libgck			\
	dbbrowser		\
	script-fu		\
	FractalExplorer		\
	Lighting		\
	MapObject		\
	bmp			\
	faxg3			\
	fits			\
	flame			\
	fp			\
	gfig			\
	gflare			\
	gfli			\
	$(GIMP_PERL)		\
	gimpressionist		\
	$(HELPBROWSER)		\
	ifscompose		\
	imagemap		\
	maze			\
	mosaic			\
	pagecurl		\
	print			\
	rcm			\
	sgi			\
	sel2path		\
	$(WEBBROWSER)		\
	$(XJT)			\
	plugin-helper		\
	common

##	gap			\
##	gdyntext		\

## this is probably cruft, but may not be.  -klm
## makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/plug-ins/makefile.mingw.in
##	cd $(top_builddir) && CONFIG_FILES=plug-ins/$@ CONFIG_HEADERS= $(SHELL) ./config.status
