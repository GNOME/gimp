## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpprocbrowser = $(top_builddir)/plug-ins/dbbrowser/libgimpprocbrowser.la

SUBDIRS = doc plug-ins

AM_CFLAGS = $(PYGIMP_EXTRA_CFLAGS)

INCLUDES = -I$(top_srcdir) $(PYTHON_INCLUDES) $(PYGTK_CFLAGS) $(GTK_CFLAGS)

pygimpdir = $(gimpplugindir)/python

pygimp_LTLIBRARIES = gimpmodule.la gimpprocbrowsermodule.la

gimpmodule_la_SOURCES = \
	gimpmodule.c		\
	pygimp-image.c		\
	pygimp-drawable.c	\
	pygimp-tile.c		\
	pygimp-display.c	\
	pygimp-parasite.c	\
	pygimp-pdb.c		\
	pygimp.h

gimpmodule_la_LDFLAGS = -module -avoid-version \
	-export-symbols-regex 'initgimp|PLUG_IN_INFO'

gimpmodule_la_LIBADD = \
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)		\
	$(RT_LIBS)

gimpprocbrowsermodule_la_SOURCES = \
	procbrowser.c

gimpprocbrowsermodule_la_LDFLAGS = -module -avoid-version

gimpprocbrowsermodule_la_LIBADD = \
	$(libgimpprocbrowser)	\
	$(libgimpui)		\
	$(libgimpwidgets)	\
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

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pyenvdir)
	echo ": PYTHONPATH=$(pygimpdir)" > $(DESTDIR)$(pyenvdir)/pygimp.env

snap:
	$(MAKE) dist distdir=$(PACKAGE)-SNAP-`date +"%Y%m%d"`

EXTRA_DIST = \
	$(pygimp_DATA)	\
	makefile.msc
