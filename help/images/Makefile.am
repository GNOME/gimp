## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/images

helpdata_DATA = \
	add_point.png			\
	anchor_layer.png		\
	channel_to_selection.png	\
	delete_channel.png		\
	delete_directory.png		\
	delete_layer.png		\
	delete_path.png			\
	dotted_square.png		\
	duplicate_channel.png		\
	duplicate_layer.png		\
	duplicate_path.png		\
	edit_point.png			\
	eek.png				\
	geezoom.png			\
	layers_example.png		\
	layers_overview.png		\
	lower_channel.png		\
	lower_directory.png		\
	lower_layer.png			\
	new_channel.png			\
	new_directory.png		\
	new_path.png			\
	new_point.png			\
	note.png			\
	path_to_selection.png		\
	preview_box.png			\
	raise_channel.png		\
	raise_directory.png		\
	raise_layer.png			\
	red_square.png			\
	remove_point.png		\
	selection_to_path.png		\
	stroke_path.png			\
	tip.png				\
	warning.png			\
	wilber.png

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
