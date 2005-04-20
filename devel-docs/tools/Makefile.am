## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la


INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)

LDADDS = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimpmath)		\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)

noinst_PROGRAMS = doc-shooter

doc_shooter_SOURCES = \
	shadow.c	\
	shadow.h	\
	shooter.c	\
	widgets.c	\
	widgets.h

doc_shooter_LDADD = $(LDADDS)


clean-local:
	rm -f *.png
