## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	   plug_in_Twist_iter_ALT.inc		\
	   plug_in_alienmap_iter_ALT.inc	\
	   plug_in_applylens_iter_ALT.inc	\
	   plug_in_depth_merge_iter_ALT.inc	\
	   plug_in_despeckle_iter_ALT.inc	\
	   plug_in_emboss_iter_ALT.inc		\
	   plug_in_exchange_iter_ALT.inc	\
	   plug_in_map_object_iter_ALT.inc	\
	   plug_in_maze_iter_ALT.inc		\
	   plug_in_nlfilt_iter_ALT.inc		\
	   plug_in_nova_iter_ALT.inc		\
	   plug_in_oilify_iter_ALT.inc		\
	   plug_in_polar_coords_iter_ALT.inc	\
	   plug_in_sinus_iter_ALT.inc		\
	   plug_in_sparkle_iter_ALT.inc		\
	   plug_in_blur_iter_ALT.inc		\
	   plug_in_pagecurl_iter_ALT.inc	\
	   plug_in_plasma_iter_ALT.inc		\
	   plug_in_sample_colorize_iter_ALT.inc	\
	   plug_in_solid_noise_iter_ALT.inc	\
	   plug_in_lighting_iter_ALT.inc


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

