## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappactions.a

libappactions_a_SOURCES = \
	actions-types.h			\
	\
	brushes-actions.c		\
	brushes-actions.h		\
	buffers-actions.c		\
	buffers-actions.h		\
	channels-actions.c		\
	channels-actions.h		\
	colormap-editor-actions.c	\
	colormap-editor-actions.h	\
	debug-actions.c			\
	debug-actions.h			\
	dialogs-actions.c		\
	dialogs-actions.h		\
	dockable-actions.c		\
	dockable-actions.h		\
	documents-actions.c		\
	documents-actions.h		\
	drawable-actions.c		\
	drawable-actions.h		\
	edit-actions.c			\
	edit-actions.h			\
	error-console-actions.c		\
	error-console-actions.h		\
	file-actions.c			\
	file-actions.h			\
	fonts-actions.c			\
	fonts-actions.h			\
	gradient-editor-actions.c	\
	gradient-editor-actions.h	\
	gradients-actions.c		\
	gradients-actions.h		\
	help-actions.c			\
	help-actions.h			\
	image-actions.c			\
	image-actions.h			\
	images-actions.c		\
	images-actions.h		\
	layers-actions.c		\
	layers-actions.h		\
	palette-editor-actions.c	\
	palette-editor-actions.h	\
	palettes-actions.c		\
	palettes-actions.h		\
	patterns-actions.c		\
	patterns-actions.h		\
	qmask-actions.c			\
	qmask-actions.h			\
	select-actions.c		\
	select-actions.h		\
	templates-actions.c		\
	templates-actions.h		\
	tool-options-actions.c		\
	tool-options-actions.h		\
	vectors-actions.c		\
	vectors-actions.h		\
	view-actions.c			\
	view-actions.h


EXTRA_DIST = makefile.msc

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Actions\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GTK_CFLAGS)		\
	-I$(includedir)
