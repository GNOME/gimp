## Process this file with automake to produce Makefile.in

scriptdatadir = $(gimpdatadir)/scripts

scriptdata_DATA = web-browser.scm

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = webbrowser

webbrowser_SOURCES = @STRIP_BEGIN@ \
	webbrowser.c	\
@STRIP_END@

EXTRA_DIST = $(scriptdata_DATA)

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DLOCALEDIR=\""$(localedir)"\"	\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

LDADD = @STRIP_BEGIN@ \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(X_LIBS)							\
	$(LIBXMU)							\
	$(INTLLIBS)							\
@STRIP_END@
