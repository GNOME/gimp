## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libappgui.la

libappgui_la_SOURCES = \
	dialogs.c		\
	dialogs.h		\
	dialogs-commands.c	\
	dialogs-commands.h	\
	dialogs-constructors.c	\
	dialogs-constructors.h	\
	gimpdialogfactory.c	\
	gimpdialogfactory.h

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
