## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappgui.a

dialogs_sources = \
	about-dialog.c			\
	about-dialog.h			\
	brush-select.c			\
	brush-select.h			\
	color-notebook.c		\
	color-notebook.h		\
	convert-dialog.c		\
	convert-dialog.h		\
	dialogs.c			\
	dialogs.h			\
	dialogs-constructors.c		\
	dialogs-constructors.h		\
	file-dialog-utils.c		\
	file-dialog-utils.h		\
	file-new-dialog.c		\
	file-new-dialog.h		\
	file-open-dialog.c		\
	file-open-dialog.h		\
	file-save-dialog.c		\
	file-save-dialog.h		\
	font-select.c			\
	font-select.h			\
	gradient-select.c		\
	gradient-select.h		\
	grid-dialog.h                   \
	grid-dialog.c                   \
	info-dialog.c			\
	info-dialog.h			\
	info-window.c			\
	info-window.h			\
	module-browser.c		\
	module-browser.h		\
	offset-dialog.c			\
	offset-dialog.h			\
	palette-import-dialog.c		\
	palette-import-dialog.h		\
	palette-select.c		\
	palette-select.h		\
	pattern-select.c		\
	pattern-select.h		\
	preferences-dialog.c		\
	preferences-dialog.h		\
	resize-dialog.c			\
	resize-dialog.h			\
	resolution-calibrate-dialog.c	\
	resolution-calibrate-dialog.h	\
	stroke-dialog.c			\
	stroke-dialog.h			\
	tips-dialog.c			\
	tips-dialog.h			\
	tips-parser.c			\
	tips-parser.h			\
	user-install-dialog.c		\
	user-install-dialog.h

menus_sources = \
	brushes-menu.c			\
	brushes-menu.h			\
	buffers-commands.c		\
	buffers-commands.h		\
	buffers-menu.c			\
	buffers-menu.h			\
	channels-commands.c		\
	channels-commands.h		\
	channels-menu.c			\
	channels-menu.h			\
	colormap-editor-commands.c	\
	colormap-editor-commands.h	\
	colormap-editor-menu.c		\
	colormap-editor-menu.h		\
	data-commands.c			\
	data-commands.h			\
	debug-commands.c		\
	debug-commands.h		\
	dialogs-commands.c		\
	dialogs-commands.h		\
	dialogs-menu.c			\
	dialogs-menu.h			\
	dockable-commands.c		\
	dockable-commands.h		\
	documents-commands.c		\
	documents-commands.h		\
	documents-menu.c		\
	documents-menu.h		\
	drawable-commands.c		\
	drawable-commands.h		\
	edit-commands.c			\
	edit-commands.h			\
	error-console-commands.c	\
	error-console-commands.h	\
	error-console-menu.c		\
	error-console-menu.h		\
	file-commands.c			\
	file-commands.h			\
	file-open-menu.c		\
	file-open-menu.h		\
	file-save-menu.c		\
	file-save-menu.h		\
	fonts-commands.c		\
	fonts-commands.h		\
	fonts-menu.c			\
	fonts-menu.h			\
	gradient-editor-commands.c	\
	gradient-editor-commands.h	\
	gradient-editor-menu.c		\
	gradient-editor-menu.h		\
	gradients-commands.c		\
	gradients-commands.h		\
	gradients-menu.c		\
	gradients-menu.h		\
	help-commands.c			\
	help-commands.h			\
	image-commands.c		\
	image-commands.h		\
	image-menu.c			\
	image-menu.h			\
	images-commands.c		\
	images-commands.h		\
	images-menu.c			\
	images-menu.h			\
	layers-commands.c		\
	layers-commands.h		\
	layers-menu.c			\
	layers-menu.h			\
	menus.c				\
	menus.h				\
	palette-editor-commands.c	\
	palette-editor-commands.h	\
	palette-editor-menu.c		\
	palette-editor-menu.h		\
	palettes-commands.c		\
	palettes-commands.h		\
	palettes-menu.c			\
	palettes-menu.h			\
	patterns-menu.c			\
	patterns-menu.h			\
	plug-in-commands.c		\
	plug-in-commands.h		\
	plug-in-menus.c			\
	plug-in-menus.h			\
	qmask-commands.c		\
	qmask-commands.h		\
	qmask-menu.c			\
	qmask-menu.h			\
	select-commands.c		\
	select-commands.h		\
	templates-commands.c		\
	templates-commands.h		\
	templates-menu.c		\
	templates-menu.h		\
	tool-options-menu.c		\
	tool-options-menu.h		\
	tool-options-commands.c		\
	tool-options-commands.h		\
	toolbox-menu.c			\
	toolbox-menu.h			\
	tools-commands.c		\
	tools-commands.h		\
	vectors-commands.c		\
	vectors-commands.h		\
	vectors-menu.c			\
	vectors-menu.h			\
	view-commands.c			\
	view-commands.h

libappgui_a_SOURCES = \
	$(dialogs_sources)		\
	$(menus_sources)		\
	authors.h			\
	color-history.c			\
	color-history.h			\
	gui.c				\
	gui.h				\
	gui-vtable.c			\
	gui-vtable.h			\
	gui-types.h			\
	session.c			\
	session.h			\
	splash.c			\
	splash.h			\
	themes.c			\
	themes.h

EXTRA_DIST = makefile.msc

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-GUI\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GTK_CFLAGS)		\
	-I$(includedir)
