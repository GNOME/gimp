## Makefile.am for ligma/data/images

imagedatadir = $(ligmadatadir)/images

if LIGMA_UNSTABLE
LIGMA_LOGO = ligma-devel-logo.png
else
LIGMA_LOGO = ligma-logo.png
endif

imagedata_DATA = \
	$(LIGMA_LOGO)	\
	ligma-splash.png	\
	wilber.png

EXTRA_DIST = $(imagedata_DATA)
