## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = gfli

gfli_SOURCES = @STRIP_BEGIN@ \
	fli.h		\
	fli.c		\
	gfli.c		\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)		\
@STRIP_END@

LDADD = @STRIP_BEGIN@ \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(INTLLIBS)							\
@STRIP_END@
