## Process this file with automake to produce Makefile.in

libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Composite\"	\
	@GIMP_THREAD_FLAGS@			\
	@GIMP_MP_FLAGS@

AM_CCASFLAGS = \
	-I$(top_builddir)	\
	-I$(top_srcdir) 	\
	-I$(top_srcdir)/app

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libappcomposite.a

libappcomposite_a_sources = \
	gimp-composite-3dnow.c		\
	gimp-composite-3dnow.h		\
	gimp-composite-altivec.c	\
	gimp-composite-altivec.h	\
	gimp-composite-generic.c	\
	gimp-composite-generic.h	\
	gimp-composite-mmx.c		\
	gimp-composite-mmx.h		\
	gimp-composite-sse.c		\
	gimp-composite-sse.h		\
	gimp-composite-sse2.c		\
	gimp-composite-sse2.h		\
	gimp-composite-util.h		\
	gimp-composite-vis.c		\
	gimp-composite-vis.h		\
	gimp-composite-x86.h		\
	gimp-composite.c		\
	gimp-composite.h

libappcomposite_a_built_sources = \
	gimp-composite-3dnow-installer.c	\
	gimp-composite-altivec-installer.c	\
	gimp-composite-generic-installer.c	\
	gimp-composite-mmx-installer.c		\
	gimp-composite-sse-installer.c		\
	gimp-composite-sse2-installer.c		\
	gimp-composite-vis-installer.c

libappcomposite_a_SOURCES = \
	$(libappcomposite_a_built_sources)	\
	$(libappcomposite_a_sources)

regenerate: gimp-composite-generic.o gimp-composite-mmx.o gimp-composite-sse.o gimp-composite-sse2.o gimp-composite-3dnow.o gimp-composite-altivec.o gimp-composite-vis.o
	$(srcdir)/make-installer.py                                          -f gimp-composite-generic.o
	$(srcdir)/make-installer.py -t -r 'defined(COMPILE_MMX_IS_OKAY)'     -f gimp-composite-mmx.o
	$(srcdir)/make-installer.py -t -r 'defined(COMPILE_SSE_IS_OKAY)'     -f gimp-composite-sse.o
	$(srcdir)/make-installer.py -t -r 'defined(COMPILE_SSE2_IS_OKAY)'    -f gimp-composite-sse2.o
	$(srcdir)/make-installer.py -t -r 'defined(COMPILE_3DNOW_IS_OKAY)'   -f gimp-composite-3dnow.o
	$(srcdir)/make-installer.py -t -r 'defined(COMPILE_ALTIVEC_IS_OKAY)' -f gimp-composite-altivec.o
	$(srcdir)/make-installer.py -t -r 'defined(COMPILE_VIS_IS_OKAY)'     -f gimp-composite-vis.o

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

CLEANFILES = $(EXTRA_PROGRAMS)

gimp_composite_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-test.c

gimp_composite_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_test_LDADD = \
	libappcomposite.a	\
	../base/libappbase.a	\
	$(libgimpcolor)		\
	$(GLIB_LIBS)


gimp_composite_mmx_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-mmx-test.c

gimp_composite_mmx_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_mmx_test_LDADD = \
	libappcomposite.a	\
	../base/libappbase.a	\
	$(libgimpcolor)		\
	$(GLIB_LIBS)

gimp_composite_sse_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-sse-test.c

gimp_composite_sse_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_sse_test_LDADD = \
	libappcomposite.a	\
	../base/libappbase.a	\
	$(libgimpcolor)		\
	$(GLIB_LIBS)

gimp_composite_sse2_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-sse2-test.c

gimp_composite_sse2_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_sse2_test_LDADD = \
	libappcomposite.a	\
	../base/libappbase.a	\
	$(libgimpcolor)		\
	$(GLIB_LIBS)


gimp_composite_3dnow_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-3dnow-test.c

gimp_composite_3dnow_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_3dnow_test_LDADD = \
	libappcomposite.a	\
	../base/libappbase.a	\
	$(libgimpcolor)		\
	$(GLIB_LIBS)


gimp_composite_altivec_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-altivec-test.c

gimp_composite_altivec_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_altivec_test_LDADD = \
	libappcomposite.a	\
	../base/libappbase.a	\
	$(libgimpcolor)		\
	$(GLIB_LIBS)

gimp_composite_vis_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-vis-test.c

gimp_composite_vis_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_vis_test_LDADD = \
	libappcomposite.a	\
	../base/libappbase.a	\
	$(libgimpcolor)		\
	$(GLIB_LIBS)
