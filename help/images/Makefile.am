## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/images

helpdata_DATA = \
	eek.png				\
	wilber.png			\
	\
	anchor_icon.png			\
	delete_icon.png			\
	duplicate_icon.png		\
	lower_icon.png			\
	new_icon.png			\
	pen_add_icon.png		\
	pen_delete_icon.png		\
	pen_edit_icon.png		\
	pen_normal_icon.png		\
	to_selection_icon.png		\
	raise_icon.png			\
	selection_to_path_icon.png	\
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
