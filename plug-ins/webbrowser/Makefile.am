## Process this file with automake to produce Makefile.in

scriptdatadir = $(gimpdatadir)/scripts

scriptdata_DATA = web-browser.scm

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = webbrowser

webbrowser_SOURCES = \
	webbrowser.c   

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
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
