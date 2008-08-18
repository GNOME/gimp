## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la
libgimpthumb = $(top_builddir)/libgimpthumb/libgimpthumb-$(GIMP_API_VERSION).la

SUBDIRS = plug-ins doc

if PLATFORM_WIN32
no_undefined = -no-undefined
path_separator = ;
else
path_separator = :
endif

AM_CFLAGS = $(PYGIMP_EXTRA_CFLAGS)

INCLUDES = -I$(top_srcdir) $(PYTHON_INCLUDES) $(PYGTK_CFLAGS) $(GTK_CFLAGS)

pygimpbase = python

pygimpdir = $(gimpplugindir)/$(pygimpbase)
pygimpmoduledir = $(pygimpdir)/gimp


pygimpmodule_LTLIBRARIES = _gimp.la color.la thumb.la _ui.la _enums.la

_gimp_la_SOURCES = \
	gimpmodule.c		\
	pygimp-display.c	\
	pygimp-drawable.c	\
	pygimp-image.c		\
	pygimp-parasite.c	\
	pygimp-pdb.c		\
	pygimp-tile.c		\
	pygimp-vectors.c	\
	pygimp.h		\
	pygimp-api.h		\
	pygimp-intl.h		\
	pygimp-util.h

_gimp_la_LDFLAGS = -module -avoid-version $(no_undefined) \
	-export-symbols-regex init_gimp

_gimp_la_LIBADD = \
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)		\
	$(PYLINK_LIBS)		\
	$(RT_LIBS)

_enums_la_SOURCES = \
	gimpenumsmodule.c

_enums_la_LDFLAGS = -module -avoid-version $(no_undefined) \
	-export-symbols-regex init_enums

_enums_la_LIBADD = \
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)		\
	$(PYLINK_LIBS)		\
	$(RT_LIBS)

color_la_SOURCES = \
	gimpcolormodule.c	\
	pygimp-colors.c		\
	pygimpcolor.h		\
	pygimpcolor-api.h

color_la_LDFLAGS = -module -avoid-version $(no_undefined) \
	-export-symbols-regex initcolor

color_la_LIBADD = \
	$(libgimpcolor)		\
	$(GLIB_LIBS)		\
	$(PYLINK_LIBS)

_ui_la_SOURCES = \
	gimpuimodule.c		\
	gimpui.c

_ui_la_LDFLAGS = -module -avoid-version $(no_undefined) \
	-export-symbols-regex init_ui

_ui_la_LIBADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimpmath)		\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(PYLINK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)

thumb_la_SOURCES = \
	gimpthumbmodule.c \
	gimpthumb.c

thumb_la_LDFLAGS = -module -avoid-version $(no_undefined) \
	-export-symbols-regex initthumb

thumb_la_LIBADD = \
	$(libgimpthumb)		\
	$(GDK_PIXBUF_LIBS)	\
	$(PYLINK_LIBS)

pygimp_PYTHON = \
	gimpenums.py	\
	gimpfu.py	\
	gimpplugin.py	\
	gimpshelf.py	\
	gimpui.py	\
	gimpthumb.py	\
	gimpcolor.py	\
	gimpplugin2.py

pygimp_DATA = pygimp-logo.png

pygimpmodule_PYTHON = \
	__init__.py \
	context.py	\
	ui.py	\
	shelf.py	\
	enums.py	\
	tools.py

codegen_files = \
	gimpcolor-types.defs	\
	gimpenums-types.defs	\
	gimpthumb.defs		\
	gimpthumb.override	\
	gimpui.defs		\
	gimpui.override

pyenvdir = $(gimpplugindir)/environ
pyinterpdir = $(gimpplugindir)/interpreters

pyenvfile = $(pyenvdir)/pygimp.env
pyinterpfile = $(pyinterpdir)/pygimp.interp

install-env-file:
	$(mkinstalldirs) '$(DESTDIR)$(pyenvdir)'
	echo '$(path_separator) PYTHONPATH=$${gimp_plug_in_dir}/$(pygimpbase)' > '$(DESTDIR)$(pyenvfile)'

install-interp-file:
	$(mkinstalldirs) '$(DESTDIR)$(pyinterpdir)'
	echo 'python=$(PYBIN_PATH)' > '$(DESTDIR)$(pyinterpfile)'
	echo '/usr/bin/python=$(PYBIN_PATH)' >> '$(DESTDIR)$(pyinterpfile)'
	echo ':Python:E::py::python:' >> '$(DESTDIR)$(pyinterpfile)'

install-data-local: install-env-file install-interp-file

uninstall-local:
	rm -f '$(DESTDIR)$(pyenvfile)' '$(DESTDIR)$(pyinterpfile)'

EXTRA_DIST = \
	$(pygimp_DATA)		\
	$(codegen_files)	\
	makefile.msc

gimpui.c: gimpui.defs gimpui.override gimpcolor-types.defs gimpenums-types.defs

gimpthumb.c: gimpthumb.defs gimpthumb.override

CLEANFILES = gimpui.c gimpthumb.c

.defs.c:
	(cd $(srcdir) \
	 && $(PYGTK_CODEGEN) \
	    --override $*.override \
	    --register $(PYGTK_DEFSDIR)/gdk-types.defs \
	    --register $(PYGTK_DEFSDIR)/gtk-types.defs \
	    --register $(srcdir)/gimpcolor-types.defs \
	    --register $(srcdir)/gimpenums-types.defs \
	    --prefix $* $*.defs) > gen-$*.c \
	 && cp gen-$*.c $*.c \
	 && rm -f gen-$*.c
