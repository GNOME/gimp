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
        imap_about.c                    \
        imap_browse.c                   \
        imap_circle.c                   \
        imap_cern_lex.c                 \
        imap_cern_parse.c               \
        imap_cmd_clear.c                \
        imap_cmd_copy.c                 \
        imap_cmd_copy_object.c          \
        imap_cmd_create.c               \
        imap_cmd_cut.c                  \
        imap_cmd_cut_object.c           \
        imap_cmd_delete.c               \
        imap_cmd_delete_point.c         \
        imap_cmd_edit_object.c          \
        imap_cmd_guides.c               \
        imap_cmd_insert_point.c         \
        imap_cmd_move.c                 \
        imap_cmd_move_down.c            \
        imap_cmd_move_sash.c            \
        imap_cmd_move_to_front.c        \
        imap_cmd_move_up.c              \
        imap_cmd_object_down.c          \
        imap_cmd_object_up.c            \
        imap_cmd_paste.c                \
        imap_cmd_select.c               \
        imap_cmd_select_all.c           \
        imap_cmd_select_region.c        \
        imap_cmd_send_to_back.c         \
        imap_cmd_unselect.c             \
        imap_cmd_unselect_all.c         \
        imap_command.c                  \
        imap_csim_lex.c                 \
        imap_csim_parse.c               \
        imap_default_dialog.c           \
        imap_edit_area_info.c           \
        imap_file.c                     \
        imap_grid.c                     \
        imap_main.c                     \
        imap_menu.c                     \
        imap_menu_funcs.c               \
        imap_misc.c                     \
        imap_mru.c                      \
        imap_ncsa_lex.c                 \
        imap_ncsa_parse.c               \
        imap_object.c                   \
        imap_object_popup.c             \
        imap_polygon.c                  \
        imap_popup.c                    \
        imap_preferences.c              \
        imap_preview.c                  \
        imap_rectangle.c                \
        imap_selection.c                \
        imap_settings.c                 \
        imap_source.c                   \
        imap_statusbar.c                \
        imap_string.c                   \
        imap_table.c                    \
        imap_toolbar.c                  \
        imap_tools.c


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
