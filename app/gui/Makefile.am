## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libappgui.la

libappgui_la_SOURCES = \
	about-dialog.c		\
	about-dialog.h		\
	brush-editor.c		\
	brush-editor.h		\
	brush-select.c		\
	brush-select.h		\
	channels-commands.c	\
	channels-commands.h	\
	channels-dialog.c	\
	channels-dialog.h	\
	color-area.c		\
	color-area.h		\
	color-notebook.c	\
	color-notebook.h	\
	color-select.c		\
	color-select.h		\
	colormap-dialog.c	\
	colormap-dialog.h	\
	commands.c		\
	commands.h		\
	convert-dialog.c	\
	convert-dialog.h	\
	dialogs.c		\
	dialogs.h		\
	dialogs-commands.c	\
	dialogs-commands.h	\
	dialogs-constructors.c	\
	dialogs-constructors.h	\
	file-dialog-utils.c	\
	file-dialog-utils.h	\
	file-new-dialog.c	\
	file-new-dialog.h	\
	file-open-dialog.c	\
	file-open-dialog.h	\
	file-save-dialog.c	\
	file-save-dialog.h	\
	gradient-editor.c	\
	gradient-editor.h	\
	gradient-select.h	\
	gradient-select.c	\
	gui.c			\
	gui.h			\
	indicator-area.c	\
	indicator-area.h	\
	info-dialog.c		\
	info-dialog.h		\
	info-window.c		\
	info-window.h		\
	layer-select.c		\
	layer-select.h		\
	layers-commands.c	\
	layers-commands.h	\
	layers-dialog.c		\
	layers-dialog.h		\
	menus.c			\
	menus.h			\
	offset-dialog.c		\
	offset-dialog.h		\
	palette-editor.c	\
	palette-editor.h	\
	palette-import-dialog.c	\
	palette-import-dialog.h	\
	palette-select.c	\
	palette-select.h	\
	paths-dialog.c		\
	paths-dialog.h		\
	pattern-select.c	\
	pattern-select.h	\
	preferences-dialog.c	\
	preferences-dialog.h	\
	session.c		\
	session.h		\
	splash.c		\
	splash.h		\
	test-commands.c		\
	test-commands.h		\
	tips-dialog.c		\
	tips-dialog.h		\
	toolbox.c		\
	toolbox.h

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-GUI\"

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GTK_CFLAGS)		\
	-I$(includedir)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
