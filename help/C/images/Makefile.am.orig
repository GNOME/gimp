## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/images

helpdata_DATA = \
	image_menu.png		\
	image_window.png	\
	layers_dialog.png	\
	layers_menu.png		\
	tool_options.png	\
	toolbox.png		\
	\
	print_color.png		\
	print_main.png		\
	print_setup.png

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
