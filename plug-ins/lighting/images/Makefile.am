## Process this file with automake to produce Makefile.in

STOCK_IMAGES = \
	lighting-intensity-ambient-high.png		\
	lighting-intensity-ambient-low.png		\
	lighting-intensity-diffuse-high.png		\
	lighting-intensity-diffuse-low.png		\
	lighting-reflectivity-diffuse-high.png		\
	lighting-reflectivity-diffuse-low.png		\
	lighting-reflectivity-specular-high.png		\
	lighting-reflectivity-specular-low.png		\
	lighting-reflectivity-highlight-high.png	\
	lighting-reflectivity-highlight-low.png

EXTRA_DIST = \
	$(STOCK_IMAGES)		\
	lighting-icon-images.gresource.xml

noinst_DATA = \
	lighting-icon-images.c	\
	lighting-icon-images.h

CLEANFILES = $(noinst_DATA)

lighting-icon-images.h: lighting-icon-images.gresource.xml
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-header \
	  --target=$@ $(srcdir)/lighting-icon-images.gresource.xml

lighting-icon-images.c: lighting-icon-images.h
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-source \
	  --target=$@ $(srcdir)/lighting-icon-images.gresource.xml
