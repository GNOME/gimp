## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = ifscompose

ifscompose_SOURCES = @STRIP_BEGIN@ \
	ifscompose.c		\
	ifscompose.h		\
	ifscompose_utils.c      \
	ifscompose_storage.c	\
@STRIP_END@

EXTRA_DIST = README.ifscompose

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
