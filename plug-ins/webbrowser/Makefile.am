## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

LDFLAGS = $(mwindows)

scriptdatadir = $(gimpdatadir)/scripts

scriptdata_DATA = web-browser.scm

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = webbrowser

webbrowser_SOURCES = \
	webbrowser.c   

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\"

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/intl		\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(LIBXMU)				\
	$(INTLLIBS)

EXTRA_DIST = $(scriptdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
