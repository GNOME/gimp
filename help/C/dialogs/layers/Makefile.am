## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/dialogs/layers

helpdata_DATA = \
        add_mask.html \
        apply_mask.html \
        edit_layer_attributes.html \
        index.html \
        layers.html \
	layer_boundary_size.html \
        merge_visible_layers.html \
        new_layer.html \
        scale_layer.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
