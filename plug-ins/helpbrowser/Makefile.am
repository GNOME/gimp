## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = helpbrowser

helpbrowser_SOURCES = \
	helpbrowser.c	\
	dialog.c	\
	dialog.h	\
	domain.c	\
	domain.h	\
	queue.c		\
	queue.h		\
	uri.c		\
	uri.h

INCLUDES = \
	-I$(top_srcdir)		\
	$(GTKHTML2_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTKHTML2_LIBS)						\
	$(RT_LIBS)							\
	$(INTLLIBS)
