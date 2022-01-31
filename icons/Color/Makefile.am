## Process this file with automake to produce Makefile.in

iconsdir = $(gimpdatadir)/icons/Color

icons_DATA = \
	index.theme

include $(top_srcdir)/icons/Color/icon-list.mk

if ENABLE_VECTOR_ICONS
scalable_DATA = \
	$(scalable_images)
endif

# Vector icons are now committed into the repository.
# librsvg is just too buggy to handle build-time extraction with
# acceptable quality.
#scalable/%.svg: color-scalable.svg ../../tools/compute-svg-viewbox
#	mkdir -p scalable && \
#	SVG=`$(top_srcdir)/tools/extract-vector-icon.sh $< $*` && \
#	echo $${SVG} > $@

12/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)
16/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)
18/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)
20/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)
22/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)
24/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)
32/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)
48/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)
64/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)
96/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)
128/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)
192/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)
256/%.png: scalable/%.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)

## Compiled-in icons for both the core and libgimpwidgets

CORE_IMAGES = \
	64/dialog-question.png

ICON_IMAGES = \
	64/gimp-wilber-eek.png

EXTRA_DIST = \
	$(icons_DATA)		\
	64/gimp-frame.png	\
	64/gimp-texture.png	\
	$(scalable_images)	\
	color-scalable.svg

noinst_DATA = \
	gimp-core-pixbufs.gresource.xml	\
	gimp-icon-pixbufs.gresource.xml

if ENABLE_VECTOR_ICONS
noinst_DATA += $(CORE_IMAGES) $(ICON_IMAGES)
endif

CLEANFILES = $(noinst_DATA)


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
