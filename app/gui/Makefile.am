## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappgui.a

libappgui_a_SOURCES = @STRIP_BEGIN@ \
	about-dialog.c			\
	about-dialog.h			\
	authors.h			\
	brush-editor.c			\
	brush-editor.h			\
	brush-select.c			\
	brush-select.h			\
	brushes-commands.c		\
	brushes-commands.h		\
	buffers-commands.c		\
	buffers-commands.h		\
	channels-commands.c		\
	channels-commands.h		\
	color-area.c			\
	color-area.h			\
	color-notebook.c		\
	color-notebook.h		\
	color-select.c			\
	color-select.h			\
	colormap-dialog.c		\
	colormap-dialog.h		\
	commands.c			\
	commands.h			\
	convert-dialog.c		\
	convert-dialog.h		\
	data-commands.c			\
	data-commands.h			\
	dialogs.c			\
	dialogs.h			\
	dialogs-commands.c		\
	dialogs-commands.h		\
	dialogs-constructors.c		\
	dialogs-constructors.h		\
	documents-commands.c		\
	documents-commands.h		\
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
	gradient-editor.c		\
	gradient-editor.h		\
	gradient-select.h		\
	gradient-select.c		\
	gradients-commands.c		\
	gradients-commands.h		\
	gui.c				\
	gui.h				\
	gui-types.h			\
	image-commands.c		\
	image-commands.h		\
	indicator-area.c		\
	indicator-area.h		\
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
	palette-editor.c		\
	palette-editor.h		\
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
	preferences-dialog.c		\
	preferences-dialog.h		\
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
	tool-options-dialog.c		\
	tool-options-dialog.h		\
	toolbox.c			\
	toolbox.h			\
	tools-commands.c		\
	tools-commands.h		\
	user-install-dialog.c		\
	user-install-dialog.h		\
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
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
