## Process this file with automake to produce Makefile.in

iconsdir = $(gimpdatadir)/icons/Symbolic-Inverted-High-Contrast

icons_DATA = \
	index.theme

include $(top_srcdir)/icons/icon-list.mk

CONTRAST = -1.5

MAKE_ICON = \
	mkdir -p `dirname $@` && \
	GEGL_USE_OPENCL=no GEGL_SWAP=ram $(GEGL) $< -o $@ -- \
		cast-format input-format="R'G'B'A float" output-format="RGBA float" \
		brightness-contrast contrast=$(CONTRAST) \
		cast-format input-format="RGBA float" output-format="R'G'B'A float"

MAKE_SVG_ICON = \
	mkdir -p `dirname $@` && \
	$(top_builddir)/tools/svg-contrast$(BUILD_EXEEXT) $< $@ $(CONTRAST)

# As exceptions, we do not want to invert some icons.
# Just use these as-is: gimp-default-colors, gimp-toilet-paper.png.
12/gimp-default-colors.png: $(top_srcdir)/icons/Symbolic/12/gimp-default-colors.png Makefile.am
	mkdir -p `dirname $@` && cp -f $< $@
scalable/gimp-default-colors.svg: ../Symbolic/scalable/gimp-default-colors.svg Makefile.am
	mkdir -p `dirname $@` && cp -f $< $@

%/gimp-toilet-paper.png: $(top_srcdir)/icons/Symbolic/$(@D)/gimp-toilet-paper.png Makefile.am
	mkdir -p `dirname $@` && cp -f $< $@
scalable/gimp-toilet-paper.svg: ../Symbolic/scalable/gimp-toilet-paper.svg Makefile.am
	mkdir -p `dirname $@` && cp -f $< $@

# Other exceptions: inverted gimp-color-picker-white|black are generated
# from each other.
18/gimp-color-picker-black.png: $(top_srcdir)/icons/Symbolic/18/gimp-color-picker-white.png Makefile.am
	$(MAKE_ICON)
scalable/gimp-color-picker-black.svg: $(top_builddir)/icons/Symbolic/scalable/gimp-color-picker-white.svg $(top_builddir)/tools/svg-contrast$(BUILD_EXEEXT) Makefile.am
	$(MAKE_SVG_ICON)

18/gimp-color-picker-white.png: $(top_srcdir)/icons/Symbolic/18/gimp-color-picker-black.png Makefile.am
	$(MAKE_ICON)
scalable/gimp-color-picker-white.svg: $(top_builddir)/icons/Symbolic/scalable/gimp-color-picker-black.svg $(top_builddir)/tools/svg-contrast$(BUILD_EXEEXT) Makefile.am
	$(MAKE_SVG_ICON)

# Rule for all other scalable icons.
scalable/%.svg: ../Symbolic/scalable/%.svg $(top_builddir)/tools/svg-contrast$(BUILD_EXEEXT) Makefile.am
	$(MAKE_SVG_ICON)

24/%.svg: $(top_srcdir)/icons/Symbolic/24/%.svg $(top_builddir)/tools/svg-contrast$(BUILD_EXEEXT) Makefile.am
	$(MAKE_SVG_ICON)

# We are basically repeating the same rule for every subdirectory, which
# is very dirty. But this is the only way we found to have a rule
# dependency correctly tied to the image with same name in Symbolic/.
# I would expect the following to match as a single rule:
#     %.png: $(top_srcdir)/icons/Symbolic/%.png
# But it doesn't. If someone knows how, feel free to fix it back into
# a single rule.

12/%.png: $(top_srcdir)/icons/Symbolic/12/%.png Makefile.am
	$(MAKE_ICON)

16/%.png: $(top_srcdir)/icons/Symbolic/16/%.png Makefile.am
	$(MAKE_ICON)

18/%.png: $(top_srcdir)/icons/Symbolic/18/%.png Makefile.am
	$(MAKE_ICON)

20/%.png: $(top_srcdir)/icons/Symbolic/20/%.png Makefile.am
	$(MAKE_ICON)

22/%.png: $(top_srcdir)/icons/Symbolic/22/%.png Makefile.am
	$(MAKE_ICON)

24/%.png: $(top_srcdir)/icons/Symbolic/24/%.png Makefile.am
	$(MAKE_ICON)

32/%.png: $(top_srcdir)/icons/Symbolic/32/%.png Makefile.am
	$(MAKE_ICON)

48/%.png: $(top_srcdir)/icons/Symbolic/48/%.png Makefile.am
	$(MAKE_ICON)

64/%.png: $(top_srcdir)/icons/Symbolic/64/%.png Makefile.am
	$(MAKE_ICON)

96/%.png: $(top_srcdir)/icons/Symbolic/96/%.png Makefile.am
	$(MAKE_ICON)

128/%.png: $(top_srcdir)/icons/Symbolic/128/%.png Makefile.am
	$(MAKE_ICON)

192/%.png: $(top_srcdir)/icons/Symbolic/192/%.png Makefile.am
	$(MAKE_ICON)

256/%.png: $(top_srcdir)/icons/Symbolic/256/%.png Makefile.am
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
