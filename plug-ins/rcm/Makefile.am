## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = rcm

rcm_SOURCES = @STRIP_BEGIN@ \
	rcm.c		\
	rcm.h		\
	rcm_callback.c	\
	rcm_callback.h	\
	rcm_dialog.c	\
	rcm_dialog.h	\
	rcm_gdk.c	\
	rcm_gdk.h	\
	rcm_misc.c	\
	rcm_misc.h	\
@STRIP_END@

EXTRA_DIST = @STRIP_BEGIN@ \
	pixmaps/rcm_360.xpm	\
	pixmaps/rcm_a_b.xpm	\
	pixmaps/rcm_ccw.xpm	\
	pixmaps/rcm_cw.xpm	\
@STRIP_END@

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DLOCALEDIR=\""$(localedir)"\"	\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/intl		\
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
	$(INTLLIBS)							\
@STRIP_END@

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
