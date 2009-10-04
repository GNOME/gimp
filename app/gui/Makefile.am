## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-GUI\"	\
	-DGIMP_COMMAND=\"@GIMP_COMMAND@\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GEGL_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(DBUS_GLIB_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappgui.a

libappgui_a_sources = \
	color-history.c		\
	color-history.h		\
	gimpdbusservice.c	\
	gimpdbusservice.h	\
	gimpuiconfigurer.c	\
	gimpuiconfigurer.h	\
	gui.c			\
	gui.h			\
	gui-message.c		\
	gui-message.h		\
	gui-unique.c		\
	gui-unique.h		\
	gui-vtable.c		\
	gui-vtable.h		\
	gui-types.h		\
	session.c		\
	session.h		\
	splash.c		\
	splash.h		\
	themes.c		\
	themes.h		\
	ige-mac-menu.c		\
	ige-mac-menu.h

libappgui_a_built_sources = gimpdbusservice-glue.h 

libappgui_a_SOURCES = $(libappgui_a_built_sources) $(libappgui_a_sources)

EXTRA_DIST = \
	dbus-service.xml	\
	makefile.msc


#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = gimpdbusservice-glue.h
CLEANFILES = $(gen_sources)

$(srcdir)/gimpdbusservice.c: gimpdbusservice-glue.h

gimpdbusservice-glue.h: $(srcdir)/dbus-service.xml
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=gimp $< > $(@F)
