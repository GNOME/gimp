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


# C version.

goat_exercise_c_libexecdir = $(gimpplugindir)/plug-ins/goat-exercise-c
goat_exercise_c_datadir    = $(goat_exercise_c_libexecdir)

goat_exercise_c_libexec_PROGRAMS = goat-exercise-c
goat_exercise_c_libexec_DATA = goat-exercise-c.c

goat_exercise_c_SOURCES = \
	goat-exercise-c.c

goat_exercise_c_LDADD = \
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

# Javascript (GJS) version.

if BUILD_JAVASCRIPT
goat_exercise_gjsdir = $(gimpplugindir)/plug-ins/goat-exercise-gjs
goat_exercise_gjs_SCRIPTS = goat-exercise-gjs.js
endif

# Lua (lua-jit + LGI) version.

if BUILD_LUA
goat_exercise_luadir = $(gimpplugindir)/plug-ins/goat-exercise-lua
goat_exercise_lua_SCRIPTS = goat-exercise-lua.lua
endif

# Python 3 (pygobject) version.

if BUILD_PYTHON
goat_exercise_py3dir = $(gimpplugindir)/plug-ins/goat-exercise-py3
goat_exercise_py3_SCRIPTS = goat-exercise-py3.py
endif

# Vala (compiled to C) version

if ENABLE_VAPIGEN

goat_exercise_vala_libexecdir = $(gimpplugindir)/plug-ins/goat-exercise-vala
goat_exercise_vala_datadir    = $(goat_exercise_vala_libexecdir)

goat_exercise_vala_libexec_PROGRAMS = goat-exercise-vala
goat_exercise_vala_libexec_DATA = goat-exercise-vala.vala

goat_exercise_vala_SOURCES = goat-exercise-vala.vala
goat_exercise_vala_CFLAGS = -DGETTEXT_PACKAGE=\""$(GETTEXT_PACKAGE)"\"

goat_exercise_vala_LDADD = \
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

goat_exercise_vala_VALAFLAGS = \
	--vapidir=$(abs_top_builddir)/libgimp		\
	--pkg gimp-@GIMP_API_MAJOR_VERSION@		\
	--pkg gimp-ui-@GIMP_API_MAJOR_VERSION@

endif

EXTRA_DIST = \
	goat-exercise-gjs.js	\
	goat-exercise-lua.lua	\
	goat-exercise-py3.py
