## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	   plug_in_CML_explorer_iter_ALT.inc	\
	   plug_in_alpha2color_iter_ALT.inc	\
	   plug_in_blinds_iter_ALT.inc		\
	   plug_in_borderaverage_iter_ALT.inc	\
	   plug_in_bump_map_iter_ALT.inc	\
	   plug_in_checkerboard_iter_ALT.inc	\
	   plug_in_color_map_iter_ALT.inc	\
	   plug_in_colorify_iter_ALT.inc	\
	   plug_in_convmatrix_iter_ALT.inc	\
	   plug_in_cubism_iter_ALT.inc		\
	   plug_in_destripe_iter_ALT.inc	\
	   plug_in_diffraction_iter_ALT.inc	\
	   plug_in_displace_iter_ALT.inc	\
	   plug_in_edge_iter_ALT.inc		\
	   plug_in_engrave_iter_ALT.inc		\
	   plug_in_flame_iter_ALT.inc		\
	   plug_in_flarefx_iter_ALT.inc		\
	   plug_in_fractal_trace_iter_ALT.inc	\
	   plug_in_gauss_iir_iter_ALT.inc	\
	   plug_in_gauss_rle_iter_ALT.inc	\
	   plug_in_gfig_iter_ALT.inc		\
	   plug_in_glasstile_iter_ALT.inc	\
	   plug_in_grid_iter_ALT.inc		\
	   plug_in_jigsaw_iter_ALT.inc		\
	   plug_in_make_seamless_iter_ALT.inc	\
	   plug_in_mblur_iter_ALT.inc		\
	   plug_in_mosaic_iter_ALT.inc		\
	   plug_in_newsprint_iter_ALT.inc	\
	   plug_in_noisify_iter_ALT.inc		\
	   plug_in_paper_tile_iter_ALT.inc	\
	   plug_in_pixelize_iter_ALT.inc	\
	   plug_in_randomize_hurl_iter_ALT.inc	\
	   plug_in_randomize_pick_iter_ALT.inc	\
	   plug_in_randomize_slur_iter_ALT.inc	\
	   plug_in_ripple_iter_ALT.inc		\
	   plug_in_rotate_iter_ALT.inc		\
	   plug_in_scatter_hsv_iter_ALT.inc	\
	   plug_in_sharpen_iter_ALT.inc		\
	   plug_in_shift_iter_ALT.inc		\
	   plug_in_spread_iter_ALT.inc		\
	   plug_in_video_iter_ALT.inc		\
	   plug_in_vpropagate_iter_ALT.inc	\
	   plug_in_waves_iter_ALT.inc		\
	   plug_in_whirl_pinch_iter_ALT.inc	\
	   plug_in_wind_iter_ALT.inc		\
	   plug_in_zealouscrop_iter_ALT.inc


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

