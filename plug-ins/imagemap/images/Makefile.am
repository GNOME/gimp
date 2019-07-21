## Process this file with automake to produce Makefile.in

ICON_IMAGES = \
	imagemap-circle.png	\
	imagemap-coord.png	\
	imagemap-dimension.png	\
	imagemap-java.png	\
	imagemap-polygon.png	\
	imagemap-rectangle.png	\
	imagemap-to-back.png	\
	imagemap-to-front.png

EXTRA_DIST = \
	$(ICON_IMAGES)	\
	imagemap-icon-images.gresource.xml

noinst_DATA = \
	imagemap-icon-images.c	\
	imagemap-icon-images.h

CLEANFILES = $(noinst_DATA)

imagemap-icon-images.h: imagemap-icon-images.gresource.xml
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-header \
	  --target=$@ $(srcdir)/imagemap-icon-images.gresource.xml

imagemap-icon-images.c: imagemap-icon-images.h
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-source \
	  --target=$@ $(srcdir)/imagemap-icon-images.gresource.xml
