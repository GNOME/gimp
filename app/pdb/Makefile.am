## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libapppdb.a

libapppdb_a_SOURCES = @STRIP_BEGIN@ \
	internal_procs.c	\
	internal_procs.h	\
	procedural_db.c		\
	procedural_db.h		\
	brush_select_cmds.c	\
	brushes_cmds.c		\
	channel_cmds.c		\
	color_cmds.c		\
	convert_cmds.c		\
	display_cmds.c		\
	drawable_cmds.c		\
	edit_cmds.c		\
	fileops_cmds.c		\
	floating_sel_cmds.c	\
	gimprc_cmds.c		\
	gradient_select_cmds.c	\
	gradients_cmds.c	\
	guides_cmds.c		\
	help_cmds.c		\
	image_cmds.c		\
	layer_cmds.c		\
	message_cmds.c		\
	misc_cmds.c		\
	palette_cmds.c		\
	parasite_cmds.c		\
	paths_cmds.c		\
	pattern_select_cmds.c	\
	patterns_cmds.c		\
	pdb_glue.h		\
	pdb-types.h		\
	plug_in_cmds.c		\
	procedural_db_cmds.c	\
	selection_cmds.c	\
	selection_tools_cmds.c	\
	text_tool_cmds.c	\
	tools_cmds.c		\
	transform_tools_cmds.c	\
	undo_cmds.c		\
	unit_cmds.c		\
@STRIP_END@

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-PDB\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@
