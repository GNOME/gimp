## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-PDB\"	\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/app		\
	-I$(top_srcdir)/app		\
	$(CAIRO_CFLAGS)			\
	$(GEGL_CFLAGS)			\
	$(GDK_PIXBUF_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libapppdb.a libappinternal-procs.a

libapppdb_a_SOURCES = \
	pdb-types.h			\
	\
	gimp-pdb-compat.c		\
	gimp-pdb-compat.h		\
	gimppdb.c			\
	gimppdb.h			\
	gimppdb-query.c			\
	gimppdb-query.h			\
	gimppdb-utils.c			\
	gimppdb-utils.h			\
	gimppdbcontext.c		\
	gimppdbcontext.h		\
	gimppdberror.c			\
	gimppdberror.h			\
	gimpprocedure.c			\
	gimpprocedure.h

libappinternal_procs_a_SOURCES = \
	internal-procs.c		\
	internal-procs.h		\
	brush-cmds.c			\
	brush-select-cmds.c		\
	brushes-cmds.c			\
	buffer-cmds.c			\
	channel-cmds.c			\
	context-cmds.c			\
	debug-cmds.c			\
	display-cmds.c			\
	drawable-cmds.c			\
	drawable-color-cmds.c		\
	drawable-edit-cmds.c		\
	dynamics-cmds.c			\
	edit-cmds.c			\
	fileops-cmds.c			\
	floating-sel-cmds.c		\
	font-select-cmds.c		\
	fonts-cmds.c			\
	gimp-cmds.c			\
	gimprc-cmds.c			\
	gradient-cmds.c			\
	gradient-select-cmds.c		\
	gradients-cmds.c		\
	help-cmds.c			\
	image-cmds.c			\
	image-color-profile-cmds.c	\
	image-convert-cmds.c		\
	image-grid-cmds.c		\
	image-guides-cmds.c		\
	image-sample-points-cmds.c	\
	image-select-cmds.c		\
	image-transform-cmds.c		\
	image-undo-cmds.c		\
	item-cmds.c			\
	item-transform-cmds.c		\
	layer-cmds.c			\
	message-cmds.c			\
	paint-tools-cmds.c		\
	palette-cmds.c			\
	palette-select-cmds.c		\
	palettes-cmds.c			\
	pattern-cmds.c			\
	pattern-select-cmds.c		\
	patterns-cmds.c			\
	plug-in-cmds.c			\
	plug-in-compat-cmds.c		\
	procedural-db-cmds.c		\
	progress-cmds.c			\
	selection-cmds.c		\
	text-layer-cmds.c		\
	text-tool-cmds.c		\
	unit-cmds.c			\
	vectors-cmds.c
