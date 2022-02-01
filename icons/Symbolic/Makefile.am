## Process this file with automake to produce Makefile.in

iconsdir = $(gimpdatadir)/icons/Symbolic

icons_DATA = \
	index.theme

## The icon theme images are listed in a separate file to ensure all
## official icon themes have the same list.
include $(top_srcdir)/icons/Symbolic/icon-list.mk

if ENABLE_VECTOR_ICONS
scalable_DATA = \
	$(scalable_images)
endif

# Vector icons are now committed into the repository.
# librsvg is just too buggy to handle build-time extraction with
# acceptable quality.
#scalable/%.svg: symbolic-scalable.svg ../../tools/compute-svg-viewbox
#	mkdir -p scalable && \
#	SVG=`$(top_srcdir)/tools/extract-vector-icon.sh $< $*` && \
#	echo $${SVG} > $@

# I wanted to make them in a single rule (since the dependency and the
# recipe are the same) but somehow it doesn't create all the images (in
# particular it creates only one size, when a single image is available
# in various sizes). If someone finds how to factorize these rules into
# one, be my guest!
12/%.symbolic.png: scalable/%.svg
	mkdir -p $(@D) && \
	$(GTK_ENCODE_SYMBOLIC_SVG) -o $(@D) $< $(@D)x$(@D)
16/%.symbolic.png: scalable/%.svg
	mkdir -p $(@D) && \
	$(GTK_ENCODE_SYMBOLIC_SVG) -o $(@D) $< $(@D)x$(@D)
18/%.symbolic.png: scalable/%.svg
	mkdir -p $(@D) && \
	$(GTK_ENCODE_SYMBOLIC_SVG) -o $(@D) $< $(@D)x$(@D)
22/%.symbolic.png: scalable/%.svg
	mkdir -p $(@D) && \
	$(GTK_ENCODE_SYMBOLIC_SVG) -o $(@D) $< $(@D)x$(@D)
24/%.symbolic.png: scalable/%.svg
	mkdir -p $(@D) && \
	$(GTK_ENCODE_SYMBOLIC_SVG) -o $(@D) $< $(@D)x$(@D)
32/%.symbolic.png: scalable/%.svg
	mkdir -p $(@D) && \
	$(GTK_ENCODE_SYMBOLIC_SVG) -o $(@D) $< $(@D)x$(@D)
48/%.symbolic.png: scalable/%.svg
	mkdir -p $(@D) && \
	$(GTK_ENCODE_SYMBOLIC_SVG) -o $(@D) $< $(@D)x$(@D)
64/%.symbolic.png: scalable/%.svg
	mkdir -p $(@D) && \
	$(GTK_ENCODE_SYMBOLIC_SVG) -o $(@D) $< $(@D)x$(@D)
96/%.symbolic.png: scalable/%.svg
	mkdir -p $(@D) && \
	$(GTK_ENCODE_SYMBOLIC_SVG) -o $(@D) $< $(@D)x$(@D)
128/%.symbolic.png: scalable/%.svg
	mkdir -p $(@D) && \
	$(GTK_ENCODE_SYMBOLIC_SVG) -o $(@D) $< $(@D)x$(@D)
192/%.symbolic.png: scalable/%.svg
	mkdir -p $(@D) && \
	$(GTK_ENCODE_SYMBOLIC_SVG) -o $(@D) $< $(@D)x$(@D)
256/%.symbolic.png: scalable/%.svg
	mkdir -p $(@D) && \
	$(GTK_ENCODE_SYMBOLIC_SVG) -o $(@D) $< $(@D)x$(@D)


EXTRA_DIST = \
	$(icons_DATA)		\
	64/gimp-frame.png	\
	64/gimp-texture.png	\
	$(scalable_images)	\
	symbolic-scalable.svg
