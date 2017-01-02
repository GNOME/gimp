## Process this file with automake to produce Makefile.in

iconsdir = $(gimpdatadir)/icons/Color

icons_DATA = \
	index.theme

include $(top_srcdir)/icons/icon-list.mk

# Vector icons are now committed into the repository.
# librsvg is just too buggy to handle build-time extraction with
# acceptable quality.
#scalable/%.svg: color-scalable.svg ../../tools/compute-svg-viewbox
#	mkdir -p scalable && \
#	SVG=`$(top_srcdir)/tools/extract-vector-icon.sh $< $*` && \
#	echo $${SVG} > $@

# The blending mode icons are auto-generated from the Symbolic icons, by
# overlaying a white version of the corresponding symbolic icon on top of the
# blending-mode icon backdrop (gimp-mode-backdrop.{svg,png} in this theme).
MAKE_SVG_MODE_ICON = \
	mkdir -p `dirname $@` && \
	(sed    '/<\/metadata>/q' $< && \
	 sed    '1,/<\/metadata>/d;/<\/svg>/Q' `dirname $< | sed 's/\bSymbolic\b/Color/'`/gimp-mode-backdrop.svg && \
	 sed -r '1,/<\/metadata>/d;s/\#[[:xdigit:]]{6}\b/\#ffffff/g' $<) > $@

scalable/gimp-%-mode.svg: ../Symbolic/scalable/gimp-%-mode.svg scalable/gimp-mode-backdrop.svg Makefile.am
	$(AM_V_GEN) $(MAKE_SVG_MODE_ICON)

MAKE_MODE_ICON = \
	mkdir -p `dirname $@` && \
	GEGL_USE_OPENCL=no GEGL_SWAP=ram $(GEGL) -o $@ -- \
	over input=[ png-load path=`dirname $< | sed 's/\bSymbolic\b/Color/'`/gimp-mode-backdrop.png ] \
	     aux=[ load path=$< levels out-low=1 ] \
	     srgb=true

16/gimp-%-mode.png: ../Symbolic/16/gimp-%-mode.png 16/gimp-mode-backdrop.png Makefile.am
	$(AM_V_GEN) $(MAKE_MODE_ICON)

## Compiled-in icons for both the core and libgimpwidgets

CORE_IMAGES = \
	64/gimp-question.png

ICON_IMAGES = \
	64/gimp-wilber-eek.png

EXTRA_DIST = \
	$(icons_DATA)		\
	$(icons12_images)	\
	$(icons16_images)	\
	$(icons18_images)	\
	$(icons20_images)	\
	$(icons22_images)	\
	$(icons24_images)	\
	$(icons32_images)	\
	$(icons48_images)	\
	$(icons64_images)	\
	$(icons96_images)	\
	$(icons128_images)	\
	$(icons192_images)	\
	$(icons256_images)	\
	$(scalable_images)	\
	$(vector24_images)	\
	color-scalable.svg

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
