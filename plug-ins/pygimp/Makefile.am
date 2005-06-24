## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpprocbrowser = $(top_builddir)/plug-ins/dbbrowser/libgimpprocbrowser.la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

SUBDIRS = plug-ins

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

AM_CFLAGS = $(PYGIMP_EXTRA_CFLAGS)

INCLUDES = -I$(top_srcdir) $(PYTHON_INCLUDES) $(PYGTK_CFLAGS) $(GTK_CFLAGS)

pygimpdir = $(gimpplugindir)/python

pygimp_LTLIBRARIES = gimpmodule.la _gimpenumsmodule.la gimpcolormodule.la \
	gimpprocbrowsermodule.la

gimpmodule_la_SOURCES = \
	gimpmodule.c		\
	pygimp-display.c	\
	pygimp-drawable.c	\
	pygimp-image.c		\
	pygimp-parasite.c	\
	pygimp-pdb.c		\
	pygimp-tile.c		\
	pygimp.h		\
	pygimp-api.h

gimpmodule_la_LDFLAGS = -module -avoid-version $(no_undefined) \
	-export-symbols-regex initgimp

gimpmodule_la_LIBADD = \
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)		\
	$(RT_LIBS)

_gimpenumsmodule_la_SOURCES = \
	gimpenumsmodule.c

_gimpenumsmodule_la_LDFLAGS = -module -avoid-version $(no_undefined) \
	-export-symbols-regex init_gimpenums

_gimpenumsmodule_la_LIBADD = \
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)		\
	$(RT_LIBS)

gimpcolormodule_la_SOURCES = \
	gimpcolormodule.c	\
	pygimp-rgb.c		\
	pygimpcolor.h

gimpcolormodule_la_LDFLAGS = -module -avoid-version $(no_undefined) \
	-export-symbols-regex initgimpcolor

gimpcolormodule_la_LIBADD = \
	$(libgimpcolor)	\
	$(GLIB_LIBS)

gimpprocbrowsermodule_la_SOURCES = \
	procbrowser.c

gimpprocbrowsermodule_la_LDFLAGS = -module -avoid-version $(no_undefined) \
	-export-symbols-regex initgimpprocbrowser

gimpprocbrowsermodule_la_LIBADD = \
	$(libgimpprocbrowser)	\
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimpmath)		\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)

pygimp_PYTHON = \
	gimpplugin.py	\
	gimpenums.py	\
	gimpshelf.py	\
	gimpui.py	\
	gimpfu.py

pygimp_DATA = pygimp-logo.png

pyenvdir = $(gimpplugindir)/environ
pyinterpdir = $(gimpplugindir)/interpreters

pyenvfile = $(pyenvdir)/pygimp.env
pyinterpfile = $(pyinterpdir)/pygimp.interp

install-env-file:
	$(mkinstalldirs) '$(DESTDIR)$(pyenvdir)'
	echo ": PYTHONPATH=$(pygimpdir)" > '$(DESTDIR)$(pyenvfile)'

install-interp-file:
	$(mkinstalldirs) '$(DESTDIR)$(pyinterpdir)'
	echo 'python=$(PYTHON)' > '$(DESTDIR)$(pyinterpfile)'
	echo '/usr/bin/python=$(PYTHON)' >> '$(DESTDIR)$(pyinterpfile)'
	echo ':Python:E::py::python:' >> '$(DESTDIR)$(pyinterpfile)'

install-data-local: install-env-file install-interp-file

uninstall-local:
	rm -f '$(DESTDIR)$(pyenvfile)' '$(DESTDIR)$(pyinterpfile)'

EXTRA_DIST = \
	$(pygimp_DATA)	\
	makefile.msc
