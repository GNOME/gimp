## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	graphics/align_center.xpm		\
	graphics/align_left.xpm			\
	graphics/align_right.xpm		\
	graphics/antialias.xpm			\
	graphics/charmap.xpm			\
	graphics/font_preview.xpm		\
	graphics/font_preview_clear.xpm		\
	graphics/font_preview_default.xpm	\
	graphics/gdyntext_logo.xpm		\
	graphics/new_layer.xpm			\
	graphics/text_load.xpm			\
	ChangeLog				\
	README					\
	TODO

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = gdyntext

gdyntext_SOURCES = \
	charmap.c		\
	charmap.h		\
	charmap_window.c	\
	charmap_window.h	\
	font_selection.c	\
	font_selection.h	\
	gdyntext.c		\
	gdyntext.h		\
	gdyntext_ui.c		\
	gdyntext_ui.h		\
	message_window.c	\
	message_window.h

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

DEPS = \
	$(top_builddir)/libgimp/libgimp.la	

gdyntext_DEPENDENCIES = $(DEPS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done
