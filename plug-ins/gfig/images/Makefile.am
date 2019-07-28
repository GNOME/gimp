## Process this file with automake to produce Makefile.in

STOCK_IMAGES = \
	gfig-bezier.png		\
	gfig-circle.png		\
	gfig-copy-object.png	\
	gfig-curve.png		\
	gfig-delete-object.png	\
	gfig-ellipse.png	\
	gfig-line.png		\
	gfig-move-object.png	\
	gfig-move-point.png	\
	gfig-polygon.png	\
	gfig-rectangle.png	\
	gfig-select-object.png	\
	gfig-show-all.png	\
	gfig-spiral.png		\
	gfig-star.png		\
	gfig-logo.png

EXTRA_DIST = \
	$(STOCK_IMAGES)		\
	gfig-icon-images.gresource.xml

noinst_DATA = \
	gfig-icon-images.c	\
	gfig-icon-images.h

CLEANFILES = $(noinst_DATA)

gfig-icon-images.h: gfig-icon-images.gresource.xml
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-header \
	  --target=$@ $(srcdir)/gfig-icon-images.gresource.xml

gfig-icon-images.c: gfig-icon-images.h
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-source \
	  --target=$@ $(srcdir)/gfig-icon-images.gresource.xml
