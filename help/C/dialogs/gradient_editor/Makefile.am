## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/dialogs/gradient_editor

helpdata_DATA = \
        copy_gradient.html \
        delete_gradient.html \
        gradient_editor.html \
        index.html \
        new_gradient.html \
        rename_gradient.html \
        replicate_segment.html \
        save_as_pov_ray.html \
        split_segments_uniformly.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
