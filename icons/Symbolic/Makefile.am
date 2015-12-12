## Process this file with automake to produce Makefile.in

iconsdir   = $(gimpdatadir)/icons/Symbolic/hicolor
icons12dir = $(gimpdatadir)/icons/Symbolic/hicolor/12x12/apps
icons16dir = $(gimpdatadir)/icons/Symbolic/hicolor/16x16/apps
icons18dir = $(gimpdatadir)/icons/Symbolic/hicolor/18x18/apps
icons20dir = $(gimpdatadir)/icons/Symbolic/hicolor/20x20/apps
icons22dir = $(gimpdatadir)/icons/Symbolic/hicolor/22x22/apps
icons24dir = $(gimpdatadir)/icons/Symbolic/hicolor/24x24/apps
icons32dir = $(gimpdatadir)/icons/Symbolic/hicolor/32x32/apps
icons48dir = $(gimpdatadir)/icons/Symbolic/hicolor/48x48/apps
icons64dir = $(gimpdatadir)/icons/Symbolic/hicolor/64x64/apps

icons_DATA = \
	index.theme

## The icon theme images are listed in a separate file to ensure the
## dark and light symbolic icon themes both have the same list.
include $(top_srcdir)/icons/Symbolic/icon-list.mk

## Compiled-in icons for both the core and libgimpwidgets

CORE_IMAGES = \
	64/gimp-question.png

ICON_IMAGES = \
	64/gimp-wilber-eek.png

EXTRA_DIST = \
	$(icons_DATA)		\
	$(icons12_DATA)		\
	$(icons16_DATA)		\
	$(icons18_DATA)		\
	$(icons20_DATA)		\
	$(icons22_DATA)		\
	$(icons24_DATA)		\
	$(icons32_DATA)		\
	$(icons48_DATA)		\
	$(icons64_DATA)

noinst_DATA = \
	gimp-core-pixbufs.c	\
	gimp-icon-pixbufs.c

CLEANFILES = \
	$(noinst_DATA)			\
	gimp-core-pixbufs.gresource.xml	\
	gimp-icon-pixbufs.gresource.xml


gimp-core-pixbufs.gresource.xml: $(CORE_IMAGES) Makefile.am
	$(AM_V_GEN) ( rm -f $@; \
	  echo '<?xml version="1.0" encoding="UTF-8"?>' > $@; \
	  echo '<gresources>' >> $@; \
	  echo '  <gresource prefix="/org/gimp/icons">' >> $@; \
	  for image in $(CORE_IMAGES); do \
	    echo "    <file preprocess=\"to-pixdata\">$$image</file>" >> $@; \
	  done; \
	  echo '  </gresource>' >> $@; \
	  echo '</gresources>' >> $@ )

gimp-core-pixbufs.c: gimp-core-pixbufs.gresource.xml
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-source \
	  --target=$@ gimp-core-pixbufs.gresource.xml


gimp-icon-pixbufs.gresource.xml: $(ICON_IMAGES) Makefile.am
	$(AM_V_GEN) ( rm -f $@; \
	  echo '<?xml version="1.0" encoding="UTF-8"?>' > $@; \
	  echo '<gresources>' >> $@; \
	  echo '  <gresource prefix="/org/gimp/icons">' >> $@; \
	  for image in $(ICON_IMAGES); do \
	    echo "    <file preprocess=\"to-pixdata\">$$image</file>" >> $@; \
	  done; \
	  echo '  </gresource>' >> $@; \
	  echo '</gresources>' >> $@ )

gimp-icon-pixbufs.c: gimp-icon-pixbufs.gresource.xml
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-source \
	  --target=$@ gimp-icon-pixbufs.gresource.xml
