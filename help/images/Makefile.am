## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/images

helpdata_DATA = \
	eek.png				\
	wilber.png			\
	\
	anchor_layer_icon.png		\
	delete_icon.png			\
	duplicate_icon.png		\
	lower_layer_icon.png		\
	new_icon.png			\
	path_to_selection_icon.png	\
	raise_layer_icon.png		\
	stroke_icon.png			\
	\
	layers_example.png		\
	layers_overview.png		\
	\
	note.gif			\
	tip.gif				\
	warning.gif

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
