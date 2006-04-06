## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libapppdb.a

libapppdb_a_SOURCES = \
	pdb-types.h			\
	\
	gimp-pdb.c			\
	gimp-pdb.h			\
	gimp-pdb-compat.c		\
	gimp-pdb-compat.h		\
	gimp-pdb-query.c		\
	gimp-pdb-query.h		\
	gimpprocedure.c			\
	gimpprocedure.h			\
	gimppluginprocedure.c		\
	gimppluginprocedure.h		\
	gimptemporaryprocedure.c	\
	gimptemporaryprocedure.h	\
	\
	internal_procs.c		\
	internal_procs.h		\
	brush_cmds.c			\
	brush_select_cmds.c		\
	brushes_cmds.c			\
	buffer_cmds.c			\
	channel_cmds.c			\
	color_cmds.c			\
	context_cmds.c			\
	convert_cmds.c			\
	display_cmds.c			\
	drawable_cmds.c			\
	drawable_transform_cmds.c	\
	edit_cmds.c			\
	fileops_cmds.c			\
	floating_sel_cmds.c		\
	font_select_cmds.c		\
	fonts_cmds.c			\
	gimprc_cmds.c			\
	gradient_cmds.c			\
	gradient_select_cmds.c		\
	gradients_cmds.c		\
	grid_cmds.c			\
	guides_cmds.c			\
	help_cmds.c			\
	image_cmds.c			\
	layer_cmds.c			\
	message_cmds.c			\
	misc_cmds.c			\
	paint_tools_cmds.c		\
	palette_cmds.c			\
	palette_select_cmds.c		\
	palettes_cmds.c			\
	parasite_cmds.c			\
	paths_cmds.c			\
	pattern_cmds.c			\
	pattern_select_cmds.c		\
	patterns_cmds.c			\
	plug_in_cmds.c			\
	procedural_db_cmds.c		\
	progress_cmds.c			\
	selection_cmds.c		\
	selection_tools_cmds.c		\
	text_tool_cmds.c		\
	transform_tools_cmds.c		\
	undo_cmds.c			\
	unit_cmds.c			\
	vectors_cmds.c

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-PDB\"	\
	@GTHREAD_CFLAGS@

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

EXTRA_DIST = makefile.msc
