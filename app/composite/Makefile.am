## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Compositing\"	\
	@GIMP_THREAD_FLAGS@			\
	@GIMP_MP_FLAGS@

AM_CCASFLAGS = \
	-I$(top_builddir)	\
	-I$(top_srcdir) 	\
	-I$(top_srcdir)/app \
	-g

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libappcomposite.a

libappcomposite_a_sources = \
	gimp-composite.c		         \
	gimp-composite.h	         	\
	gimp-composite-generic.c	  \
	gimp-composite-generic.h	  \
	gimp-composite-mmx.c     		\
	gimp-composite-mmx.h     		\
	gimp-composite-sse.c	     	\
	gimp-composite-sse.h     		\
	gimp-composite-altivec.c	 	\
	gimp-composite-altivec.h 		\
	gimp-composite-vis.c	     	\
	gimp-composite-vis.h     		\
	gimp-composite-util.h

libappcomposite_a_built_sources =   \
	gimp-composite-generic-installer.c	\
	gimp-composite-generic-installer.h \
	gimp-composite-mmx-installer.c		   \
	gimp-composite-mmx-installer.h     \
	gimp-composite-sse-installer.c		   \
	gimp-composite-sse-installer.h     \
	gimp-composite-altivec-installer.c \
	gimp-composite-altivec-installer.h \
	gimp-composite-vis-installer.c		   \
	gimp-composite-vis-installer.h

libappcomposite_a_SOURCES = $(libappcomposite_a_built_sources) $(libappcomposite_a_sources)

regenerate: gimp-composite-generic.o gimp-composite-mmx.o gimp-composite-sse.o gimp-composite-altivec.o gimp-composite-vis.o
	$(srcdir)/make-installer.py -t -f gimp-composite-generic.o
	$(srcdir)/make-installer.py -t -r '(__GNUC__ >= 3) && defined(USE_MMX) && defined(ARCH_X86)'  -f gimp-composite-mmx.o
	$(srcdir)/make-installer.py -t -r '(__GNUC__ >= 3) && defined(USE_MMX) && defined(ARCH_X86)'  -f gimp-composite-sse.o
	$(srcdir)/make-installer.py -t -r '(__GNUC__ >= 3) && defined(USE_MMX) && defined(ARCH_PPC)'  -f gimp-composite-altivec.o
	$(srcdir)/make-installer.py -t -r '(__GNUC__ >= 3) && (defined(sparc) || defined(__sparc__))' -f gimp-composite-vis.o

EXTRA_DIST = \
	make-installer.py	\
	ns.py

EXTRA_PROGRAMS = test-composite gimp-composite-test gimp-composite-mmx-test gimp-composite-sse-test gimp-composite-altivec-test gimp-composite-vis-test

#
# unit tests and meta tools for GimpComposite
#

TESTS = test-composite gimp-composite-test gimp-composite-mmx-test gimp-composite-sse-test gimp-composite-altivec-test gimp-composite-vis-test

test_composite_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	test-composite.c

test_composite_DEPENDENCIES = $(gimpcomposite_dependencies)

test_composite_LDADD = \
	libappcomposite.a					\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/app/base/libappbase.a	\
	$(GLIB_LIBS)

gimp_composite_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-test.c

gimp_composite_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_test_LDADD = \
	libappcomposite.a					\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/app/base/libappbase.a	\
	$(GLIB_LIBS)


gimp_composite_mmx_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-mmx-test.c

gimp_composite_mmx_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_mmx_test_LDADD = \
	libappcomposite.a					\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/app/base/libappbase.a	\
	$(GLIB_LIBS)

gimp_composite_sse_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-sse-test.c

gimp_composite_sse_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_sse_test_LDADD = \
	libappcomposite.a					\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/app/base/libappbase.a	\
	$(GLIB_LIBS)

gimp_composite_altivec_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-altivec-test.c

gimp_composite_altivec_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_altivec_test_LDADD = \
	libappcomposite.a					\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/app/base/libappbase.a	\
	$(GLIB_LIBS)

gimp_composite_vis_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-vis-test.c

gimp_composite_vis_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_vis_test_LDADD = \
	libappcomposite.a					\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/app/base/libappbase.a	\
	$(GLIB_LIBS)

CLEANFILES = $(EXTRA_PROGRAMS)
