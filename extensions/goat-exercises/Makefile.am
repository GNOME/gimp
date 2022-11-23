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
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
goat_exercise_RC = goat-exercise.rc.o
endif

libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la $(libm)
libligmamodule = $(top_builddir)/libligmamodule/libligmamodule-$(LIGMA_API_VERSION).la
libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la

AM_LDFLAGS = $(mwindows)

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)	\
	-I$(includedir)


# C version.

goat_exercise_c_libexecdir = $(ligmaplugindir)/extensions/org.ligma.extension.goat-exercises
goat_exercise_c_datadir    = $(goat_exercise_c_libexecdir)

goat_exercise_c_libexec_PROGRAMS = goat-exercise-c
goat_exercise_c_libexec_DATA = goat-exercise-c.c

PLUG_INS = 'goat-exercise-c$(EXEEXT)'

goat_exercise_c_SOURCES = \
	goat-exercise-c.c

goat_exercise_c_LDADD = \
	$(libligma)		\
	$(libligmamath)		\
	$(libligmaconfig)	\
	$(libligmacolor)		\
	$(libligmabase)		\
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(CAIRO_LIBS)		\
	$(GDK_PIXBUF_LIBS)	\
	$(GEGL_LIBS)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(goat_exercise_RC)

# Javascript (GJS) version.

if BUILD_JAVASCRIPT
goat_exercise_gjsdir = $(goat_exercise_c_libexecdir)
goat_exercise_gjs_SCRIPTS = goat-exercise-gjs.js
endif

# Lua (lua-jit + LGI) version.

if BUILD_LUA
goat_exercise_luadir = $(goat_exercise_c_libexecdir)
goat_exercise_lua_SCRIPTS = goat-exercise-lua.lua
endif

# Python 3 (pygobject) version.

if BUILD_PYTHON
goat_exercise_py3dir = $(goat_exercise_c_libexecdir)
goat_exercise_py3_SCRIPTS = goat-exercise-py3.py
endif

# Vala (compiled to C) version

if ENABLE_VAPIGEN

goat_exercise_vala_libexecdir = $(goat_exercise_c_libexecdir)
goat_exercise_vala_datadir    = $(goat_exercise_vala_libexecdir)

goat_exercise_vala_libexec_PROGRAMS = goat-exercise-vala
goat_exercise_vala_libexec_DATA = goat-exercise-vala.vala

goat_exercise_vala_SOURCES = goat-exercise-vala.vala
goat_exercise_vala_CFLAGS = -DGETTEXT_PACKAGE=\""$(GETTEXT_PACKAGE)"\"

goat_exercise_vala_LDADD = \
	$(libligma)		\
	$(libligmamath)		\
	$(libligmaconfig)	\
	$(libligmacolor)		\
	$(libligmabase)		\
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(CAIRO_LIBS)		\
	$(GDK_PIXBUF_LIBS)	\
	$(GEGL_LIBS)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(goat_exercise_RC)

goat_exercise_vala_VALAFLAGS = \
	--vapidir=$(abs_top_builddir)/libligma	\
	--pkg ligma-@LIGMA_API_VERSION@		\
	--pkg ligma-ui-@LIGMA_API_VERSION@	\
	--disable-since-check

endif

appstream_in_files = \
	org.ligma.extension.goat-exercises.metainfo.xml.in

appstream_files = $(appstream_in_files:.xml.in=.xml)

appdatadir = $(goat_exercise_c_libexecdir)
appdata_DATA = $(appstream_files)

org.ligma.extension.goat-exercises.metainfo.xml: org.ligma.extension.goat-exercises.metainfo.xml.in $(wildcard $(top_srcdir)/po-plug-ins/*.po)
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po-plug-ins -o $@

.PHONY: org.ligma.extension.goat-exercises.metainfo.xml

EXTRA_DIST = \
	goat-exercise-gjs.js	\
	goat-exercise-lua.lua	\
	goat-exercise-py3.py	\
	$(appstream_in_files)

DISTCLEANFILES = $(appstream_files)
