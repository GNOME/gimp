## Process this file with automake to produce Makefile.in

iconsdir = $(gimpdatadir)/icons/Symbolic-Inverted

icons_DATA = \
	index.theme

include $(top_srcdir)/icons/icon-list.mk

# As exceptions, we do not want to invert some icons.
# Just use these as-is: gimp-default-colors, gimp-toilet-paper.png.
12/gimp-default-colors.png: $(top_srcdir)/icons/Symbolic/12/gimp-default-colors.png
	mkdir -p `dirname $@` && cp -f $< $@
scalable/gimp-default-colors.svg: ../Symbolic/scalable/gimp-default-colors.svg
	mkdir -p `dirname $@` && cp -f $< $@

%/gimp-toilet-paper.png: $(top_srcdir)/icons/Symbolic/$(@D)/gimp-toilet-paper.png
	mkdir -p `dirname $@` && cp -f $< $@
scalable/gimp-toilet-paper.svg: ../Symbolic/scalable/gimp-toilet-paper.svg
	mkdir -p `dirname $@` && cp -f $< $@

# Other exceptions: inverted gimp-color-picker-white|black are generated
# from each other.
18/gimp-color-picker-black.png: $(top_srcdir)/icons/Symbolic/18/gimp-color-picker-white.png
	$(MAKE_ICON)
scalable/gimp-color-picker-black.svg: $(top_builddir)/icons/Symbolic/scalable/gimp-color-picker-white.svg
	mkdir -p scalable && $(top_builddir)/tools/invert-svg $< $@

18/gimp-color-picker-white.png: $(top_srcdir)/icons/Symbolic/18/gimp-color-picker-black.png
	$(MAKE_ICON)
scalable/gimp-color-picker-white.svg: $(top_builddir)/icons/Symbolic/scalable/gimp-color-picker-black.svg
	mkdir -p scalable && $(top_builddir)/tools/invert-svg $< $@

# Rule for all other scalable icons.
scalable/%.svg: ../Symbolic/scalable/%.svg $(top_builddir)/tools/invert-svg$(BUILD_EXEEXT)
	mkdir -p scalable && \
	$(top_builddir)/tools/invert-svg$(BUILD_EXEEXT) $< $@

24/%.svg: $(top_srcdir)/icons/Symbolic/24/%.svg $(top_builddir)/tools/invert-svg$(BUILD_EXEEXT)
	mkdir -p 24 && \
	$(top_builddir)/tools/invert-svg$(BUILD_EXEEXT) $< $@

# We are basically repeating the same rule for every subdirectory, which
# is very dirty. But this is the only way we found to have a rule
# dependency correctly tied to the image with same name in Symbolic/.
# I would expect the following to match as a single rule:
#     %.png: $(top_srcdir)/icons/Symbolic/%.png
# But it doesn't. If someone knows how, feel free to fix it back into
# a single rule.

MAKE_ICON = mkdir -p `dirname $@`; GEGL_USE_OPENCL=no GEGL_SWAP=ram $(GEGL) $< -o $@ -- gegl:invert-gamma

12/%.png: $(top_srcdir)/icons/Symbolic/12/%.png
	$(MAKE_ICON)

16/%.png: $(top_srcdir)/icons/Symbolic/16/%.png
	$(MAKE_ICON)

18/%.png: $(top_srcdir)/icons/Symbolic/18/%.png
	$(MAKE_ICON)

20/%.png: $(top_srcdir)/icons/Symbolic/20/%.png
	$(MAKE_ICON)

22/%.png: $(top_srcdir)/icons/Symbolic/22/%.png
	$(MAKE_ICON)

24/%.png: $(top_srcdir)/icons/Symbolic/24/%.png
	$(MAKE_ICON)

32/%.png: $(top_srcdir)/icons/Symbolic/32/%.png
	$(MAKE_ICON)

48/%.png: $(top_srcdir)/icons/Symbolic/48/%.png
	$(MAKE_ICON)

64/%.png: $(top_srcdir)/icons/Symbolic/64/%.png
	$(MAKE_ICON)

96/%.png: $(top_srcdir)/icons/Symbolic/96/%.png
	$(MAKE_ICON)

128/%.png: $(top_srcdir)/icons/Symbolic/128/%.png
	$(MAKE_ICON)

192/%.png: $(top_srcdir)/icons/Symbolic/192/%.png
	$(MAKE_ICON)

256/%.png: $(top_srcdir)/icons/Symbolic/256/%.png
	$(MAKE_ICON)

all_icons = \
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
	$(vector24_images)

EXTRA_DIST = \
	$(icons_DATA)

DISTCLEANFILES = \
	$(all_icons)
