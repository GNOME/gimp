## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = print

print_SOURCES = \
        print.c			\
        print-image-gimp.c	\
        gimp_color_window.c	\
        gimp_main_window.c	\
        print_gimp.h

INCLUDES = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(GIMPPRINT_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(GIMPPRINT_LIBS)	\
	$(RT_LIBS)		\
	$(INTLLIBS)
