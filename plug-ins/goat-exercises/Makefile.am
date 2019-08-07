## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
else
libm = -lm
endif

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
framework_cocoa = -framework Cocoa
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
goat_exercise_RC = goat-exercise.rc.o
endif

libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la $(libm)
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la

AM_LDFLAGS = $(mwindows)

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)	\
	-I$(includedir)

goat_exercise_libexecdir = $(gimpplugindir)/plug-ins/goat-exercise

goat_exercise_libexec_PROGRAMS = goat-exercise
goat_exercise_libexec_DATA = goat-exercise.c

goat_exercise_SOURCES = \
	goat-exercise.c

goat_exercise_LDADD = \
	$(libgimp)		\
	$(libgimpmath)		\
	$(libgimpconfig)	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(CAIRO_LIBS)		\
	$(GDK_PIXBUF_LIBS)	\
	$(GEGL_LIBS)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(goat_exercise_RC)
