## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Composite\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(CAIRO_CFLAGS)		\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

composite_libraries = \
	libcomposite3dnow.a	\
	libcompositealtivec.a	\
	libcompositemmx.a	\
	libcompositesse.a	\
	libcompositesse2.a	\
	libcompositevis.a

noinst_LIBRARIES = $(composite_libraries) libcomposite.a

libcomposite3dnow_a_SOURCES = \
	gimp-composite-3dnow.c		\
	gimp-composite-3dnow.h

libcompositealtivec_a_CFLAGS = $(ALTIVEC_EXTRA_CFLAGS)

libcompositealtivec_a_SOURCES = \
	gimp-composite-altivec.c	\
	gimp-composite-altivec.h

libcompositemmx_a_CFLAGS = $(MMX_EXTRA_CFLAGS)

libcompositemmx_a_SOURCES = \
	gimp-composite-mmx.c		\
	gimp-composite-mmx.h

libcompositesse_a_CFLAGS = $(SSE_EXTRA_CFLAGS)

libcompositesse_a_SOURCES = \
	gimp-composite-sse.c		\
	gimp-composite-sse.h

libcompositesse2_a_CFLAGS = $(SSE_EXTRA_CFLAGS)

libcompositesse2_a_SOURCES = \
	gimp-composite-sse2.c		\
	gimp-composite-sse2.h

libcompositevis_a_SOURCES = \
	gimp-composite-vis.c		\
	gimp-composite-vis.h

libcomposite_a_sources = \
	gimp-composite-generic.c	\
	gimp-composite-generic.h	\
	gimp-composite-util.h		\
	gimp-composite-x86.h		\
	gimp-composite.c		\
	gimp-composite.h

libcomposite_a_built_sources = \
	gimp-composite-3dnow-installer.c	\
	gimp-composite-altivec-installer.c	\
	gimp-composite-generic-installer.c	\
	gimp-composite-mmx-installer.c		\
	gimp-composite-sse-installer.c		\
	gimp-composite-sse2-installer.c		\
	gimp-composite-vis-installer.c

libcomposite_a_SOURCES = \
	$(libcomposite_a_built_sources)	\
	$(libcomposite_a_sources)

## This is a huge hack
libappcomposite.a: $(noinst_LIBRARIES)
	-rm -f libappcomposite.a
	$(AR) $(ARFLAGS) libappcomposite.a $(libcomposite_a_OBJECTS) \
	  $(libcomposite3dnow_a_OBJECTS) \
	  $(libcompositealtivec_a_OBJECTS) \
	  $(libcompositemmx_a_OBJECTS) \
	  $(libcompositesse_a_OBJECTS) \
	  $(libcompositesse2_a_OBJECTS) \
	  $(libcompositevis_a_OBJECTS)
	$(RANLIB) libappcomposite.a

all-local: libappcomposite.a

clean_libs = libappcomposite.a

regenerate: gimp-composite-generic.o $(libcomposite3dnow_a_OBJECTS) $(libcompositealtivec_a_OBJECTS) $(libcompositemmx_a_OBJECTS) $(libcompositesse_a_OBJECTS) $(libcompositesse2_a_OBJECTS) $(libcompositevis_a_OBJECTS)
	$(srcdir)/make-installer.py -f gimp-composite-generic.o
	$(srcdir)/make-installer.py -f $(libcompositemmx_a_OBJECTS) -t -r 'defined(COMPILE_MMX_IS_OKAY)' -c 'X86_MMX'
	$(srcdir)/make-installer.py -f $(libcompositesse_a_OBJECTS) -t -r 'defined(COMPILE_SSE_IS_OKAY)' -c 'X86_SSE' -c 'X86_MMXEXT'
	$(srcdir)/make-installer.py -f $(libcompositesse2_a_OBJECTS) -t -r 'defined(COMPILE_SSE2_IS_OKAY)' -c 'X86_SSE2'
	$(srcdir)/make-installer.py -f $(libcomposite3dnow_a_OBJECTS) -t -r 'defined(COMPILE_3DNOW_IS_OKAY)' -c 'X86_3DNOW' 
	$(srcdir)/make-installer.py -f $(libcompositealtivec_a_OBJECTS) -t -r 'defined(COMPILE_ALTIVEC_IS_OKAY)' -c 'PPC_ALTIVEC'
	$(srcdir)/make-installer.py -f $(libcompositevis_a_OBJECTS) -t -r 'defined(COMPILE_VIS_IS_OKAY)'

EXTRA_DIST = \
	make-installer.py	\
	makefile.msc 		\
	ns.py


#
# unit tests and meta tools for GimpComposite
#

TESTS = \
	gimp-composite-3dnow-test	\
	gimp-composite-altivec-test	\
	gimp-composite-mmx-test		\
	gimp-composite-sse-test		\
	gimp-composite-sse2-test	\
	gimp-composite-vis-test

EXTRA_PROGRAMS = gimp-composite-test $(TESTS)

CLEANFILES = $(EXTRA_PROGRAMS) $(clean_libs)

gimp_composite_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-test.c

gimp_composite_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_test_LDADD = \
	libappcomposite.a	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)


gimp_composite_mmx_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-mmx-test.c

gimp_composite_mmx_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_mmx_test_LDADD = \
	libappcomposite.a	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)

gimp_composite_sse_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-sse-test.c

gimp_composite_sse_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_sse_test_LDADD = \
	libappcomposite.a	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)

gimp_composite_sse2_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-sse2-test.c

gimp_composite_sse2_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_sse2_test_LDADD = \
	libappcomposite.a	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)


gimp_composite_3dnow_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-3dnow-test.c

gimp_composite_3dnow_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_3dnow_test_LDADD = \
	libappcomposite.a	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)


gimp_composite_altivec_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-altivec-test.c

gimp_composite_altivec_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_altivec_test_LDADD = \
	libappcomposite.a	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)

gimp_composite_vis_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-vis-test.c

gimp_composite_vis_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_vis_test_LDADD = \
	libappcomposite.a	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)
