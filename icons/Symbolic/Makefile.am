## Process this file with automake to produce Makefile.in

iconsdir = $(gimpdatadir)/icons/Symbolic

icons_DATA = \
	index.theme

## The icon theme images are listed in a separate file to ensure all
## official icon themes have the same list.
include $(top_srcdir)/icons/icon-list.mk

if ENABLE_VECTOR_ICONS
icons24_DATA = \
	$(symbolic_vector24_images)

scalable_DATA = \
	$(symbolic_scalable_images)
endif

# Vector icons are now committed into the repository.
# librsvg is just too buggy to handle build-time extraction with
# acceptable quality.
#scalable/%.svg: symbolic-scalable.svg ../../tools/compute-svg-viewbox
#	mkdir -p scalable && \
#	SVG=`$(top_srcdir)/tools/extract-vector-icon.sh $< $*` && \
#	echo $${SVG} > $@

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
	$(symbolic_scalable_images)	\
	$(symbolic_vector24_images)	\
	symbolic-scalable.svg
