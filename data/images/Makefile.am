## Makefile.am for gimp/data/images

imagedatadir = $(gimpdatadir)/images

if GIMP_UNSTABLE
GIMP_LOGO = gimp-devel-logo.png
else
GIMP_LOGO = gimp-logo.png
endif

imagedata_DATA = \
	$(GIMP_LOGO)	\
	gimp-splash.png	\
	gimp-splash.xcf \
	wilber.png

EXTRA_DIST = $(imagedata_DATA)
