## Process this file with automake to produce Makefile.in

SUBDIRS = 			\
	megawidget		\
	libgck 			\
	MapObject		\
	AlienMap		\
	Anamorphose		\
	CentralReflection	\
	Curtain			\
	Encrypt_Decrypt		\
	Julia			\
	Mandelbrot		\
	DigitalSignature	\
	dbbrowser		\
	fuse			\
	gflare			\
	@GIMPTCL@		\
	refract			\
	script-fu		\
	struc			\
	twist			\
	warp			\
	@TIFF@			\
	@JPEG@			\
	@PNG@			\
	@MPEG@			\
	@XD@			\
	@XPM@			\
	CML_explorer		\
	align_layers		\
	apply_lens		\
	autocrop		\
	animationplay		\
	autostretch_hsv		\
	blinds			\
	blur			\
	blur2			\
	bmp			\
	bumpmap			\
	bz2			\
	c_astretch		\
	CEL			\
	checkerboard		\
	colorify		\
	compose			\
	convmatrix		\
	coordmap		\
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
	figures			\
	film			\
	fits			\
	flame			\
	fp			\
	fractaltrace		\
	gauss_iir		\
	gauss_rle		\
	gbr			\
	gfli			\
	gicon			\
	gif			\
	glasstile		\
	gqbist			\
	gradmap			\
	grid			\
	gz			\
	header			\
	holes			\
	hot			\
	hrz			\
	ifscompose		\
	illusion		\
	iwarp			\
	laplace			\
	lic			\
	magiceye		\
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
	psd			\
	randomize		\
	ripple			\
	rotate			\
	rotators		\
	scatter_hsv		\
	sgi			\
	sharpen			\
	shift			\
	sinus			\
	smooth_palette		\
	snoise			\
	snp			\
	sobel			\
	sparkle			\
	spread			\
	stegano			\
	stereogram		\
	sunras			\
	tga			\
	threshold_alpha		\
	tile			\
	tileit			\
	tiler			\
	universal		\
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
