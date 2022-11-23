## Process this file with automake to produce Makefile.in

iconsdir = $(ligmadatadir)/icons/Color

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

## Compiled-in icons for both the core and libligmawidgets

CORE_IMAGES = \
	64/dialog-question.png

ICON_IMAGES = \
	64/ligma-wilber-eek.png

EXTRA_DIST = \
	$(icons_DATA)		\
	64/ligma-frame.png	\
	64/ligma-texture.png	\
	$(scalable_images)	\
	color-scalable.svg

noinst_DATA = \
	ligma-core-pixbufs.gresource.xml	\
	ligma-icon-pixbufs.gresource.xml

if ENABLE_VECTOR_ICONS
noinst_DATA += $(CORE_IMAGES) $(ICON_IMAGES)
endif

CLEANFILES = $(noinst_DATA)

# This is basically the same rule as the catch all 64/%.png rule above,
# because somehow the native Windows build fail without it (no problem
# on cross-build for Windows or native Linux builds). When trying to
# build ligma-core-pixbufs.gresource.xml, we were getting an error
# because of "No rule to make target '64/dialog-question.png'".
# Interestingly, the problem doesn't happen for the other XML, but maybe
# it's because it's not needed from within icons/Makefile.am.
64/dialog-question.png: scalable/dialog-question.svg ../../tools/colorsvg2png
	mkdir -p $(@D) && \
	$(top_builddir)/tools/colorsvg2png $< $@ $(@D)

ligma-core-pixbufs.gresource.xml: $(CORE_IMAGES) Makefile.am
	$(AM_V_GEN) ( rm -f $@; \
	  echo '<?xml version="1.0" encoding="UTF-8"?>' > $@; \
	  echo '<gresources>' >> $@; \
	  echo '  <gresource prefix="/org/ligma/icons">' >> $@; \
	  for image in $(CORE_IMAGES); do \
	    echo "    <file preprocess=\"to-pixdata\">$$image</file>" >> $@; \
	  done; \
	  echo '  </gresource>' >> $@; \
	  echo '</gresources>' >> $@ )

ligma-icon-pixbufs.gresource.xml: $(ICON_IMAGES) Makefile.am
	$(AM_V_GEN) ( rm -f $@; \
	  echo '<?xml version="1.0" encoding="UTF-8"?>' > $@; \
	  echo '<gresources>' >> $@; \
	  echo '  <gresource prefix="/org/ligma/icons">' >> $@; \
	  for image in $(ICON_IMAGES); do \
	    echo "    <file preprocess=\"to-pixdata\">$$image</file>" >> $@; \
	  done; \
	  echo '  </gresource>' >> $@; \
	  echo '</gresources>' >> $@ )
