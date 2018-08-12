## Process this file with automake to produce Makefile.in

iconsdir = $(gimpdatadir)/icons/Symbolic

icons_DATA = \
	index.theme

## The icon theme images are listed in a separate file to ensure all
## official icon themes have the same list.
include $(top_srcdir)/icons/icon-list.mk

# Vector icons are now committed into the repository.
# librsvg is just too buggy to handle build-time extraction with
# acceptable quality.
#scalable/%.svg: symbolic-scalable.svg ../../tools/compute-svg-viewbox
#	mkdir -p scalable && \
#	SVG=`$(top_srcdir)/tools/extract-vector-icon.sh $< $*` && \
#	echo $${SVG} > $@

## Compiled-in icons for both the core and libgimpwidgets

CORE_IMAGES = \
	64/gimp-question.png

ICON_IMAGES = \
	64/gimp-wilber-eek.png

EXTRA_DIST = \
	$(icons_DATA)			\
	$(icons12_images)		\
	$(icons16_images)		\
	$(icons18_images)		\
	$(icons20_images)		\
	$(icons22_images)		\
	$(icons24_images)		\
	$(icons32_images)		\
	$(icons48_images)		\
	$(icons64_images)		\
	$(icons64_system_images)	\
	$(icons96_images)		\
	$(icons128_images)		\
	$(icons192_images)		\
	$(icons256_images)		\
	$(scalable_images)		\
	$(vector24_images)		\
	symbolic-scalable.svg

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
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
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
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-source \
	  --target=$@ gimp-icon-pixbufs.gresource.xml
