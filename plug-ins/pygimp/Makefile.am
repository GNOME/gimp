SUBDIRS = doc plug-ins

INCLUDES = $(PYTHON_INCLUDES) $(GLIB_CFLAGS) -I$(top_srcdir)

pyexec_LTLIBRARIES = gimpmodule.la

gimpmodule_la_SOURCES = \
  gimpmodule.c \
  pygimp-image.c \
  pygimp-drawable.c \
  pygimp-tile.c \
  pygimp-display.c \
  pygimp-parasite.c \
  pygimp-pdb.c \
  pygimp.h
gimpmodule_la_LDFLAGS = -module -avoid-version \
  -export-symbols-regex 'initgimp|PLUG_IN_INFO'
gimpmodule_la_LIBADD = \
  $(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la \
  $(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la \
  $(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la \
  $(GLIB_LIBS)

python_PYTHON = \
  gimpplugin.py \
  gimpenums.py \
  gimpshelf.py \
  gimpui.py \
  gimpfu.py

EXTRA_DIST = pygimp.spec

snap:
	$(MAKE) dist distdir=$(PACKAGE)-SNAP-`date +"%Y%m%d"`
