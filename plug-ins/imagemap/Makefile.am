## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = imagemap

EXTRA_DIST = \
	arrow.xpm	\
	arrow_down.xpm	\
	arrow_up.xpm	\
	circle.xpm	\
	coord.xpm	\
	copy.xpm	\
	cut.xpm		\
	delete.xpm	\
	dimension.xpm	\
	edit.xpm	\
	grid.xpm	\
	map_info.xpm	\
	open.xpm	\
	paste.xpm	\
	polygon.xpm	\
	preferences.xpm	\
	rectangle.xpm	\
	redo.xpm	\
	save.xpm	\
	to_back.xpm	\
	to_front.xpm	\
	undo.xpm	\
	zoom_in.xpm	\
	zoom_out.xpm	\
	ChangeLog	\
	AUTHORS		\
	NEWS		\
	README		\
	TODO

imagemap_SOURCES = \
	imap_about.c			\
	imap_about.h			\
	imap_browse.c			\
	imap_browse.h			\
	imap_cern_lex.c			\
	imap_cern_parse.c		\
	imap_cern_parse.h		\
	imap_circle.c			\
	imap_circle.h			\
	imap_cmd_clear.c		\
	imap_cmd_clear.h		\
	imap_cmd_copy.c			\
	imap_cmd_copy.h			\
	imap_cmd_copy_object.c		\
	imap_cmd_copy_object.h		\
	imap_cmd_create.c		\
	imap_cmd_create.h		\
	imap_cmd_cut.c			\
	imap_cmd_cut.h			\
	imap_cmd_cut_object.c		\
	imap_cmd_cut_object.h		\
	imap_cmd_delete.c		\
	imap_cmd_delete.h		\
	imap_cmd_delete_point.c		\
	imap_cmd_delete_point.h		\
	imap_cmd_edit_object.c		\
	imap_cmd_edit_object.h		\
	imap_cmd_guides.c		\
	imap_cmd_guides.h		\
	imap_cmd_insert_point.c		\
	imap_cmd_insert_point.h		\
	imap_cmd_move.c			\
	imap_cmd_move.h			\
	imap_cmd_move_down.c		\
	imap_cmd_move_down.h		\
	imap_cmd_move_sash.c		\
	imap_cmd_move_sash.h		\
	imap_cmd_move_selected.c	\
	imap_cmd_move_selected.h	\
	imap_cmd_move_to_front.c	\
	imap_cmd_move_to_front.h	\
	imap_cmd_move_up.c		\
	imap_cmd_move_up.h		\
	imap_cmd_object_down.c		\
	imap_cmd_object_down.h		\
	imap_cmd_object_move.c		\
	imap_cmd_object_move.h		\
	imap_cmd_object_up.c		\
	imap_cmd_object_up.h		\
	imap_cmd_paste.c		\
	imap_cmd_paste.h		\
	imap_cmd_select.c		\
	imap_cmd_select.h		\
	imap_cmd_select_all.c		\
	imap_cmd_select_all.h		\
	imap_cmd_select_next.c		\
	imap_cmd_select_next.h		\
	imap_cmd_select_prev.c		\
	imap_cmd_select_prev.h		\
	imap_cmd_select_region.c	\
	imap_cmd_select_region.h	\
	imap_cmd_send_to_back.c		\
	imap_cmd_send_to_back.h		\
	imap_cmd_unselect.c		\
	imap_cmd_unselect.h		\
	imap_cmd_unselect_all.c		\
	imap_cmd_unselect_all.h		\
	imap_command.c			\
	imap_command.h			\
	imap_csim_lex.c			\
	imap_csim_parse.c		\
	imap_csim_parse.h		\
	imap_default_dialog.c		\
	imap_default_dialog.h		\
	imap_edit_area_info.c		\
	imap_edit_area_info.h		\
	imap_file.c			\
	imap_file.h			\
	imap_grid.c			\
	imap_grid.h			\
	imap_main.c			\
	imap_main.h			\
	imap_menu.c			\
	imap_menu.h			\
	imap_menu_funcs.c		\
	imap_menu_funcs.h		\
	imap_misc.c			\
	imap_misc.h			\
	imap_mru.c			\
	imap_mru.h			\
	imap_ncsa_lex.c			\
	imap_ncsa_parse.c		\
	imap_ncsa_parse.h		\
	imap_object.c			\
	imap_object.h			\
	imap_object_popup.c		\
	imap_object_popup.h		\
	imap_polygon.c			\
	imap_polygon.h			\
	imap_popup.c			\
	imap_popup.h			\
	imap_preferences.c		\
	imap_preferences.h		\
	imap_preview.c			\
	imap_preview.h			\
	imap_rectangle.c		\
	imap_rectangle.h		\
	imap_selection.c		\
	imap_selection.h		\
	imap_settings.c			\
	imap_settings.h			\
	imap_source.c			\
	imap_source.h			\
	imap_statusbar.c		\
	imap_statusbar.h		\
	imap_string.c			\
	imap_string.h			\
	imap_table.c			\
	imap_table.h			\
	imap_taglist.c			\
	imap_taglist.h			\
	imap_toolbar.c			\
	imap_toolbar.h			\
	imap_tools.c			\
	imap_tools.h


INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
