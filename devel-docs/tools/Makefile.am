## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la


AM_CPPFLAGS = $(GTK_CFLAGS) -DTOP_SRCDIR=\"$(top_srcdir)\"

INCLUDES = -I$(top_srcdir)


if ENABLE_GTK_DOC
noinst_PROGRAMS = $(DOC_SHOOTER)
endif

doc_shooter_SOURCES = \
	shadow.c	\
	shadow.h	\
	shooter.c	\
	units.c		\
	units.h		\
	widgets.c	\
	widgets.h

doc_shooter_LDADD = \
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimpmodule)	\
	$(libgimpmath)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(X_LIBS) -lX11 -lXext


EXTRA_PROGRAMS = doc-shooter

EXTRA_DIST = README.shooter


clean-local:
	rm -f *.png
