SUBDIRS = \
	files

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la


LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimp)		\
	$(libgimpmath)		\
	$(libgimpconfig)	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(GLIB_LIBS)		\
	../base64.o		\
	../xmp-parse.o		\
	../xmp-schemas.o	\
	../xmp-encode.o		\
	../xmp-model.o


AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(EXIF_CFLAGS)	\
	-I$(includedir)	\
	-I ../


TESTS_ENVIRONMENT = \
	GIMP_TESTING_ABS_TOP_SRCDIR=@abs_top_srcdir@

TESTS = \
	test-xmp-model			\
	test-xmp-model-functional

EXTRA_PROGRAMS = $(TESTS)

CLEANFILES = $(EXTRA_PROGRAMS)

