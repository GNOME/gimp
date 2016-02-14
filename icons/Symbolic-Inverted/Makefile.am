## Process this file with automake to produce Makefile.in

iconsdir    = $(gimpdatadir)/icons/Symbolic-Inverted/hicolor

## The icon theme images

icons_DATA = \
	index.theme

include $(top_srcdir)/icons/Symbolic/icon-list.mk

# As an exception, we do not want to invert gimp-default-colors.
# Just use it as-is.
12/gimp-default-colors.png: $(top_srcdir)/icons/Symbolic/12/gimp-default-colors.png
	mkdir -p `dirname $<`
	cp -f $< $@

# We are basically repeating the same rule for every subdirectory, which
# is very dirty. But this is the only way we found to have a rule
# dependency correctly tied to the image with same name in Symbolic/.
# I would expect the following to match as a single rule:
#     %.png: $(top_srcdir)/icons/Symbolic/%.png
# But it doesn't. If someone knows how, feel free to fix it back into
# a single rule.

MAKE_ICON = mkdir -p `dirname $@`; \
	$(GEGL) -x \
	"<gegl> \
	  <node operation='gegl:invert-gamma'></node> \
	  <node operation='gegl:load'> \
	    <params> \
	      <param name='path'>$(top_srcdir)/icons/Symbolic/$@</param> \
	    </params> \
	  </node> \
	</gegl>" -o $@

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
	$(icons12_DATA)		\
	$(icons16_DATA)		\
	$(icons18_DATA)		\
	$(icons20_DATA)		\
	$(icons22_DATA)		\
	$(icons24_DATA)		\
	$(icons32_DATA)		\
	$(icons48_DATA)		\
	$(icons64_DATA)		\
	$(icons96_DATA)		\
	$(icons128_DATA)	\
	$(icons192_DATA)	\
	$(icons256_DATA)

EXTRA_DIST = \
	$(icons_DATA)		\
	$(all_icons)

DISTCLEANFILES = \
	$(all_icons)
