## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappgui.a

libappgui_a_SOURCES = @STRIP_BEGIN@ \
	about-dialog.c			\
	about-dialog.h			\
	authors.h			\
	brush-select.c			\
	brush-select.h			\
	brushes-commands.c		\
	brushes-commands.h		\
	buffers-commands.c		\
	buffers-commands.h		\
	channels-commands.c		\
	channels-commands.h		\
	color-notebook.c		\
	color-notebook.h		\
	color-select.c			\
	color-select.h			\
	colormap-editor-commands.c	\
	colormap-editor-commands.h	\
	convert-dialog.c		\
	convert-dialog.h		\
	data-commands.c			\
	data-commands.h			\
	device-status-dialog.c		\
	device-status-dialog.h		\
	dialogs.c			\
	dialogs.h			\
	dialogs-commands.c		\
	dialogs-commands.h		\
	dialogs-constructors.c		\
	dialogs-constructors.h		\
	documents-commands.c		\
	documents-commands.h		\
	drawable-commands.c		\
	drawable-commands.h		\
	edit-commands.c			\
	edit-commands.h			\
	error-console-dialog.c		\
	error-console-dialog.h		\
	file-commands.c			\
	file-commands.h			\
	file-dialog-utils.c		\
	file-dialog-utils.h		\
	file-new-dialog.c		\
	file-new-dialog.h		\
	file-open-dialog.c		\
	file-open-dialog.h		\
	file-save-dialog.c		\
	file-save-dialog.h		\
	gradient-editor-commands.c	\
	gradient-editor-commands.h	\
	gradient-select.h		\
	gradient-select.c		\
	gradients-commands.c		\
	gradients-commands.h		\
	gui.c				\
	gui.h				\
	gui-types.h			\
	help-commands.c			\
	help-commands.h			\
	image-commands.c		\
	image-commands.h		\
	info-dialog.c			\
	info-dialog.h			\
	info-window.c			\
	info-window.h			\
	layers-commands.c		\
	layers-commands.h		\
	menus.c				\
	menus.h				\
	module-browser.c		\
	module-browser.h		\
	offset-dialog.c			\
	offset-dialog.h			\
	palette-editor-commands.c	\
	palette-editor-commands.h	\
	palette-import-dialog.c		\
	palette-import-dialog.h		\
	palette-select.c		\
	palette-select.h		\
	palettes-commands.c		\
	palettes-commands.h		\
	paths-dialog.c			\
	paths-dialog.h			\
	pattern-select.c		\
	pattern-select.h		\
	patterns-commands.c		\
	patterns-commands.h		\
	plug-in-commands.c		\
	plug-in-commands.h		\
	plug-in-menus.c			\
	plug-in-menus.h			\
	preferences-dialog.c		\
	preferences-dialog.h		\
	qmask-commands.c		\
	qmask-commands.h		\
	resize-dialog.c			\
	resize-dialog.h			\
	resolution-calibrate-dialog.c	\
	resolution-calibrate-dialog.h	\
	select-commands.c		\
	select-commands.h		\
	session.c			\
	session.h			\
	splash.c			\
	splash.h			\
	test-commands.c			\
	test-commands.h			\
	tips-dialog.c			\
	tips-dialog.h			\
	tips-parser.c			\
	tips-parser.h			\
	tool-options-dialog.c		\
	tool-options-dialog.h		\
	tools-commands.c		\
	tools-commands.h		\
	user-install-dialog.c		\
	user-install-dialog.h		\
	vectors-commands.c		\
	vectors-commands.h		\
	view-commands.c			\
	view-commands.h			\
@STRIP_END@

EXTRA_DIST = @STRIP_BEGIN@ \
	makefile.msc			\
@STRIP_END@

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-GUI\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/app		\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@
