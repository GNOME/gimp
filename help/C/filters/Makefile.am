## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/filters

helpdata_DATA = \
	undocumented_filter.html	\
	repeat_last.html		\
	reshow_last.html		\
	\
	applycanvas.html		\
        blinds.html			\
        blur.html			\
        bz2.html			\
        cubism.html			\
        fits.html			\
        gauss_iir.html			\
        gauss_rle.html			\
        gpb.html			\
        gqbist.html			\
        gtm.html			\
        gz.html				\
        hrz.html			\
        index.html			\
        jpeg.html			\
	miff.html			\
        oilify.html			\
        pat.html			\
        pixelize.html			\
        png.html			\
        pnm.html			\
        print.html			\
        psd.html			\
        psp.html			\
        ripple.html			\
        scatter_hsv.html		\
        semiflatten.html		\
        sgi.html			\
        sharpen.html			\
        shift.html			\
        sunras.html			\
        tga.html			\
        tiff.html			\
        tile.html			\
        url.html			\
        vinvert.html			\
        wmf.html			\
        xwd.html

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
