## Process this file with automake to produce Makefile.in

SUBDIRS = 			\
	libgck			\
	megawidget		\
	MapObject		\
	AlienMap		\
	dbbrowser		\
	script-fu		\
	struc			\
	@WEBBROWSER@		\
	@AA@			\
	@TIFF@			\
	@JPEG@			\
	@PNG@			\
	@MPEG@			\
	@XD@			\
	@XPM@			\
	CML_explorer		\
	align_layers		\
	animoptimize		\
	apply_lens		\
	autocrop		\
	animationplay		\
	autostretch_hsv		\
	blinds			\
	blur			\
	bmp			\
	bumpmap			\
	bz2			\
	c_astretch		\
	CEL			\
	checkerboard		\
	colorify		\
	compose			\
	convmatrix		\
	cubism			\
	decompose		\
	deinterlace		\
	depthmerge		\
	despeckle		\
	destripe		\
	diffraction		\
	displace		\
	edge			\
	emboss			\
	engrave			\
	exchange		\
	faxg3			\
	film			\
	fits			\
	flame			\
	flarefx			\
	fractaltrace		\
	gauss_iir		\
	gauss_rle		\
	gbr			\
	gfig			\
	gfli			\
	gicon			\
	gif			\
	glasstile		\
	gqbist			\
	gradmap			\
	grid			\
	gtm			\
	gz			\
	header			\
	hot			\
	hrz			\
	ifscompose		\
	illusion		\
	iwarp			\
	laplace			\
	mail			\
	max_rgb			\
	maze			\
	mblur			\
	mosaic			\
	nlfilt			\
	noisify			\
	normalize		\
	nova			\
	oilify			\
	pagecurl		\
	palette			\
	papertile		\
	pat			\
	pcx			\
	pix			\
	pixelize		\
	plasma			\
	pnm			\
	polar			\
	print			\
	ps			\
	randomize		\
	ripple			\
	rotate			\
	rotators		\
	scatter_hsv		\
	semiflatten		\
	sgi			\
	shift			\
	sinus			\
	smooth_palette		\
	snoise			\
	snp			\
	sobel			\
	sparkle			\
	spread			\
	sunras			\
	tga			\
	threshold_alpha		\
	tile			\
	tileit			\
	tiler			\
	url			\
	video			\
	vinvert			\
	vpropagate		\
	waves			\
	whirlpinch		\
	xwd			\
	zealouscrop

scriptdata =

pluginlibdir = $(gimpplugindir)/plug-ins

EXTRA_DIST = build.in

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

LIBGIMPUI = $(top_builddir)/libgimp/libgimpui.la

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done
