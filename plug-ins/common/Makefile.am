libexecdir = $(gimpplugindir)/plug-ins

EXTRA_DIST = \
	mkgen.pl	\
	plugin-defs.pl	\
	winclipboard.c	\
	winprint.c

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\"

INCLUDES = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

libexec_PROGRAMS = \
	CEL \
	CML_explorer \
	$(AA) \
	align_layers \
	animationplay \
	animoptimize \
	apply_lens \
	autocrop \
	autostretch_hsv \
	blinds \
	blur \
	bumpmap \
	bz2 \
	c_astretch \
	checkerboard \
	colorify \
	compose \
	convmatrix \
	csource \
	cubism \
	curve_bend \
	decompose \
	deinterlace \
	depthmerge \
	despeckle \
	destripe \
	diffraction \
	displace \
	edge \
	emboss \
	engrave \
	exchange \
	film \
	flarefx \
	fractaltrace \
	gauss_iir \
	gauss_rle \
	gbr \
	gee \
	gicon \
	gif \
	gifload \
	glasstile \
	gpb \
	gqbist \
	gradmap \
	grid \
	gtm \
	guillotine \
	gz \
	header \
	hot \
	hrz \
	illusion \
	iwarp \
	jigsaw \
	$(JPEG) \
	laplace \
	lic \
	mail \
	mapcolor \
	max_rgb \
	mblur \
	$(MPEG) \
	newsprint \
	nlfilt \
	noisify \
	normalize \
	nova \
	oilify \
	palette \
	papertile \
	pat \
	pcx \
	pix \
	pixelize \
	plasma \
	plugindetails \
	$(PNG) \
	pnm \
	polar \
	ps \
	psd \
	$(PSP) \
	randomize \
	ripple \
	rotate \
	rotators \
	sample_colorize \
	scatter_hsv \
	screenshot \
	semiflatten \
	sharpen \
	shift \
	smooth_palette \
	snoise \
	sobel \
	sparkle \
	spread \
	sunras \
	tga \
	threshold_alpha \
	$(TIFF) \
	tile \
	tileit \
	tiler \
	url \
	video \
	vinvert \
	vpropagate \
	waves \
	whirlpinch \
	wind \
	wmf \
	xbm \
	$(XPM) \
	xwd \
	zealouscrop

EXTRA_PROGRAMS = \
	aa \
	jpeg \
	mpeg \
	png \
	psp \
	tiff \
	xpm

install-%: %
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIRS)$(libexecdir)
	@if test -f $<; then \
	  echo " $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $< $(DESTDIR)$(libexecdir)/`echo $<|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	  $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $< $(DESTDIR)$(libexecdir)/`echo $<|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	else :; fi

CEL_SOURCES = \
	CEL.c

CEL_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

CML_explorer_SOURCES = \
	CML_explorer.c

CML_explorer_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

aa_SOURCES = \
	aa.c

aa_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(LIBAA)				\
	$(GTK_LIBS)				\
	$(INTLLIBS)

align_layers_SOURCES = \
	align_layers.c

align_layers_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

animationplay_SOURCES = \
	animationplay.c

animationplay_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

animoptimize_SOURCES = \
	animoptimize.c

animoptimize_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

apply_lens_SOURCES = \
	apply_lens.c

apply_lens_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

autocrop_SOURCES = \
	autocrop.c

autocrop_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

autostretch_hsv_SOURCES = \
	autostretch_hsv.c

autostretch_hsv_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

blinds_SOURCES = \
	blinds.c

blinds_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

blur_SOURCES = \
	blur.c

blur_LDADD = \
	$(top_builddir)/plug-ins/gpc/libgpc.a	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

bumpmap_SOURCES = \
	bumpmap.c

bumpmap_LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

bz2_SOURCES = \
	bz2.c

bz2_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

c_astretch_SOURCES = \
	c_astretch.c

c_astretch_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

checkerboard_SOURCES = \
	checkerboard.c

checkerboard_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

colorify_SOURCES = \
	colorify.c

colorify_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

compose_SOURCES = \
	compose.c

compose_LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

convmatrix_SOURCES = \
	convmatrix.c

convmatrix_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

csource_SOURCES = \
	csource.c

csource_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

cubism_SOURCES = \
	cubism.c

cubism_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

curve_bend_SOURCES = \
	curve_bend.c

curve_bend_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

decompose_SOURCES = \
	decompose.c

decompose_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

deinterlace_SOURCES = \
	deinterlace.c

deinterlace_LDADD = \
	$(top_builddir)/plug-ins/megawidget/libmegawidget.a	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

depthmerge_SOURCES = \
	depthmerge.c

depthmerge_LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

despeckle_SOURCES = \
	despeckle.c

despeckle_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

destripe_SOURCES = \
	destripe.c

destripe_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

diffraction_SOURCES = \
	diffraction.c

diffraction_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

displace_SOURCES = \
	displace.c

displace_LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

edge_SOURCES = \
	edge.c

edge_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

emboss_SOURCES = \
	emboss.c

emboss_LDADD = \
	$(top_builddir)/plug-ins/megawidget/libmegawidget.a	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

engrave_SOURCES = \
	engrave.c

engrave_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

exchange_SOURCES = \
	exchange.c

exchange_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

film_SOURCES = \
	film.c

film_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

flarefx_SOURCES = \
	flarefx.c

flarefx_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

fractaltrace_SOURCES = \
	fractaltrace.c

fractaltrace_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

gauss_iir_SOURCES = \
	gauss_iir.c

gauss_iir_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

gauss_rle_SOURCES = \
	gauss_rle.c

gauss_rle_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

gbr_SOURCES = \
	gbr.c

gbr_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

gee_SOURCES = \
	gee.c

gee_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

gicon_SOURCES = \
	gicon.c

gicon_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

gif_SOURCES = \
	gif.c

gif_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

gifload_SOURCES = \
	gifload.c

gifload_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

glasstile_SOURCES = \
	glasstile.c

glasstile_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

gpb_SOURCES = \
	gpb.c

gpb_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

gqbist_SOURCES = \
	gqbist.c

gqbist_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

gradmap_SOURCES = \
	gradmap.c

gradmap_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

grid_SOURCES = \
	grid.c

grid_LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

gtm_SOURCES = \
	gtm.c

gtm_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

guillotine_SOURCES = \
	guillotine.c

guillotine_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

gz_SOURCES = \
	gz.c

gz_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

header_SOURCES = \
	header.c

header_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

hot_SOURCES = \
	hot.c

hot_LDADD = \
	$(top_builddir)/plug-ins/megawidget/libmegawidget.a	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

hrz_SOURCES = \
	hrz.c

hrz_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

illusion_SOURCES = \
	illusion.c

illusion_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

iwarp_SOURCES = \
	iwarp.c

iwarp_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

jigsaw_SOURCES = \
	jigsaw.c

jigsaw_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

jpeg_SOURCES = \
	jpeg.c

jpeg_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(LIBJPEG)				\
	$(GTK_LIBS)				\
	$(INTLLIBS)

laplace_SOURCES = \
	laplace.c

laplace_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

lic_SOURCES = \
	lic.c

lic_LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

mail_SOURCES = \
	mail.c

mail_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

mapcolor_SOURCES = \
	mapcolor.c

mapcolor_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

max_rgb_SOURCES = \
	max_rgb.c

max_rgb_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

mblur_SOURCES = \
	mblur.c

mblur_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

mpeg_SOURCES = \
	mpeg.c

mpeg_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(LIBMPEG)				\
	$(GTK_LIBS)				\
	$(INTLLIBS)

newsprint_SOURCES = \
	newsprint.c

newsprint_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

nlfilt_SOURCES = \
	nlfilt.c

nlfilt_LDADD = \
	$(top_builddir)/plug-ins/megawidget/libmegawidget.a	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

noisify_SOURCES = \
	noisify.c

noisify_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

normalize_SOURCES = \
	normalize.c

normalize_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

nova_SOURCES = \
	nova.c

nova_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

oilify_SOURCES = \
	oilify.c

oilify_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

palette_SOURCES = \
	palette.c

palette_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

papertile_SOURCES = \
	papertile.c

papertile_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

pat_SOURCES = \
	pat.c

pat_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

pcx_SOURCES = \
	pcx.c

pcx_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

pix_SOURCES = \
	pix.c

pix_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

pixelize_SOURCES = \
	pixelize.c

pixelize_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

plasma_SOURCES = \
	plasma.c

plasma_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

plugindetails_SOURCES = \
	plugindetails.c

plugindetails_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

png_SOURCES = \
	png.c

png_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(LIBPNG)				\
	$(GTK_LIBS)				\
	$(INTLLIBS)

pnm_SOURCES = \
	pnm.c

pnm_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

polar_SOURCES = \
	polar.c

polar_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

ps_SOURCES = \
	ps.c

ps_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

psd_SOURCES = \
	psd.c

psd_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

psp_SOURCES = \
	psp.c

psp_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(LIBZ)				\
	$(GTK_LIBS)				\
	$(INTLLIBS)

randomize_SOURCES = \
	randomize.c

randomize_LDADD = \
	$(top_builddir)/plug-ins/gpc/libgpc.a	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

ripple_SOURCES = \
	ripple.c

ripple_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

rotate_SOURCES = \
	rotate.c

rotate_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

rotators_SOURCES = \
	rotators.c

rotators_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

sample_colorize_SOURCES = \
	sample_colorize.c

sample_colorize_LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

scatter_hsv_SOURCES = \
	scatter_hsv.c

scatter_hsv_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

screenshot_SOURCES = \
	screenshot.c

screenshot_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

semiflatten_SOURCES = \
	semiflatten.c

semiflatten_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

sharpen_SOURCES = \
	sharpen.c

sharpen_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

shift_SOURCES = \
	shift.c

shift_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

smooth_palette_SOURCES = \
	smooth_palette.c

smooth_palette_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

snoise_SOURCES = \
	snoise.c

snoise_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

sobel_SOURCES = \
	sobel.c

sobel_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

sparkle_SOURCES = \
	sparkle.c

sparkle_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

spread_SOURCES = \
	spread.c

spread_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

sunras_SOURCES = \
	sunras.c

sunras_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

tga_SOURCES = \
	tga.c

tga_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

threshold_alpha_SOURCES = \
	threshold_alpha.c

threshold_alpha_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

tiff_SOURCES = \
	tiff.c

tiff_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(LIBTIFF)				\
	$(GTK_LIBS)				\
	$(INTLLIBS)

tile_SOURCES = \
	tile.c

tile_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

tileit_SOURCES = \
	tileit.c

tileit_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

tiler_SOURCES = \
	tiler.c

tiler_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

url_SOURCES = \
	url.c

url_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

video_SOURCES = \
	video.c

video_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

vinvert_SOURCES = \
	vinvert.c

vinvert_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

vpropagate_SOURCES = \
	vpropagate.c

vpropagate_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

waves_SOURCES = \
	waves.c

waves_LDADD = \
	$(top_builddir)/plug-ins/megawidget/libmegawidget.a	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

whirlpinch_SOURCES = \
	whirlpinch.c

whirlpinch_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

wind_SOURCES = \
	wind.c

wind_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

wmf_SOURCES = \
	wmf.c

wmf_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

xbm_SOURCES = \
	xbm.c

xbm_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

xpm_SOURCES = \
	xpm.c

xpm_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(LIBXPM)				\
	$(GTK_LIBS)				\
	$(INTLLIBS)

xwd_SOURCES = \
	xwd.c

xwd_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

zealouscrop_SOURCES = \
	zealouscrop.c

zealouscrop_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)
