## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
else
libm = -lm
endif

libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
pagecurl_RC = pagecurl.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(ligmaplugindir)/plug-ins/pagecurl

libexec_PROGRAMS = pagecurl

pagecurl_built_sources = \
	pagecurl-icons.c	\
	pagecurl-icons.h

pagecurl_SOURCES = \
	$(pagecurl_built_sources)	\
	pagecurl.c

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libm)			\
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(pagecurl_RC)


PAGECURL_IMAGES = \
	curl0.png	\
	curl1.png	\
	curl2.png	\
	curl3.png	\
	curl4.png	\
	curl5.png	\
	curl6.png	\
	curl7.png

EXTRA_DIST = $(PAGECURL_IMAGES)

gen_sources = \
	pagecurl-icons.c	\
	pagecurl-icons.h

CLEANFILES = \
	$(gen_sources)	\
	pagecurl-icons.gresource.xml

pagecurl-icons.gresource.xml: $(PAGECURL_IMAGES) Makefile.am
	$(AM_V_GEN) ( rm -f $@; \
	  echo '<?xml version="1.0" encoding="UTF-8"?>' > $@; \
	  echo '<gresources>' >> $@; \
	  echo '  <gresource prefix="/org/ligma/pagecurl-icons">' >> $@; \
	  for image in $(PAGECURL_IMAGES); do \
	    echo "    <file preprocess=\"to-pixdata\">$$image</file>" >> $@; \
	  done; \
	  echo '  </gresource>' >> $@; \
	  echo '</gresources>' >> $@ )

pagecurl-icons.h: pagecurl-icons.gresource.xml
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-header \
	  --target=$@ pagecurl-icons.gresource.xml

pagecurl-icons.c: pagecurl-icons.h
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-source \
	  --target=$@ pagecurl-icons.gresource.xml
