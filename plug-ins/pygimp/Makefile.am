SUBDIRS = doc plug-ins

INCLUDES = -I$(top_srcdir) $(PYTHON_INCLUDES) $(GLIB_CFLAGS)

pygimpdir = $(gimpplugindir)/python

pygimp_LTLIBRARIES = gimpmodule.la

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
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GLIB_LIBS)

pygimp_PYTHON = \
	gimpplugin.py	\
	gimpenums.py	\
	gimpshelf.py	\
	gimpui.py	\
	gimpfu.py

pyenvdir = $(gimpplugindir)/environ

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pyenvdir)
	echo ": PYTHONPATH=$(pygimpdir)" > $(DESTDIR)$(pyenvdir)/pygimp.env

snap:
	$(MAKE) dist distdir=$(PACKAGE)-SNAP-`date +"%Y%m%d"`

EXTRA_DIST = makefile.msc
