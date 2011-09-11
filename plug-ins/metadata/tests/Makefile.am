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
	$(GLIB_LIBS)


INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(EXIF_CFLAGS)	\
	-I$(includedir)	\
	-I ../


TESTS = test-xmp-model$(EXEEXT)

EXTRA_PROGRAMS = $(TESTS)

test_xmp_model_SOURCES = \
	test-xmp-model.c	\
	../base64.c		\
	../base64.h		\
	../xmp-parse.c		\
	../xmp-parse.h		\
	../xmp-schemas.c	\
	../xmp-schemas.h	\
	../xmp-model.c		\
	../xmp-model.h

CLEANFILES = $(EXTRA_PROGRAMS)

