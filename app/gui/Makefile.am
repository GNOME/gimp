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
	file-open-menu.c		\
	file-open-menu.h		\
	file-save-menu.c		\
	file-save-menu.h		\
	image-menu.c			\
	image-menu.h			\
	menus.c				\
	menus.h				\
	plug-in-menus.c			\
	plug-in-menus.h			\
	tool-options-menu.c		\
	tool-options-menu.h		\
	toolbox-menu.c			\
	toolbox-menu.h

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
