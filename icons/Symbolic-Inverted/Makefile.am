## Process this file with automake to produce Makefile.in

iconsdir   = $(gimpdatadir)/icons/Symbolic-Inverted/hicolor
icons12dir = $(gimpdatadir)/icons/Symbolic-Inverted/hicolor/12x12/apps
icons16dir = $(gimpdatadir)/icons/Symbolic-Inverted/hicolor/16x16/apps
icons18dir = $(gimpdatadir)/icons/Symbolic-Inverted/hicolor/18x18/apps
icons20dir = $(gimpdatadir)/icons/Symbolic-Inverted/hicolor/20x20/apps
icons22dir = $(gimpdatadir)/icons/Symbolic-Inverted/hicolor/22x22/apps
icons24dir = $(gimpdatadir)/icons/Symbolic-Inverted/hicolor/24x24/apps
icons32dir = $(gimpdatadir)/icons/Symbolic-Inverted/hicolor/32x32/apps
icons48dir = $(gimpdatadir)/icons/Symbolic-Inverted/hicolor/48x48/apps
icons64dir = $(gimpdatadir)/icons/Symbolic-Inverted/hicolor/64x64/apps

## The icon theme images

icons_DATA = \
	index.theme

include $(top_srcdir)/icons/Symbolic/icon-list.mk

%.png:
	mkdir -p `dirname $@`;
	$(GEGL) -x \
	"<gegl> \
	  <node operation='gegl:invert-linear'></node> \
	  <node operation='gegl:load'> \
	    <params> \
	      <param name='path'>$(top_srcdir)/icons/Symbolic/$@</param> \
	    </params> \
	  </node> \
	</gegl>" -o $@

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
