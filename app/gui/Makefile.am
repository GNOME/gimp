## Process this file with automake to produce Makefile.in

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
endif

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-GUI\"		\
	-DGIMP_COMMAND=\"@GIMP_COMMAND@\"	\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-I$(top_builddir)/app			\
	-I$(top_srcdir)/app			\
	$(GIO_UNIX_CFLAGS)			\
	$(GEGL_CFLAGS)				\
	$(GTK_CFLAGS)				\
	$(GTK_MAC_INTEGRATION_CFLAGS)		\
	-I$(includedir)

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_LDFLAGS = \
	$(xnone)

noinst_LIBRARIES = libappgui.a

libappgui_a_sources = \
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
	icon-themes.c		\
	icon-themes.h		\
	session.c		\
	session.h		\
	splash.c		\
	splash.h		\
	themes.c		\
	themes.h

libappgui_a_built_sources = \
	gimpdbusservice-generated.c	\
	gimpdbusservice-generated.h

libappgui_a_SOURCES = $(libappgui_a_built_sources) $(libappgui_a_sources)

BUILT_SOURCES = $(libappgui_a_built_sources)

EXTRA_DIST = \
	dbus-service.xml

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = $(libappgui_a_built_sources)
CLEANFILES = $(gen_sources)

$(srcdir)/gimpdbusservice.c: $(libappgui_a_built_sources)

$(libappgui_a_built_sources): $(srcdir)/dbus-service.xml
	$(GDBUS_CODEGEN) --interface-prefix org.gimp.GIMP. \
			 --generate-c-code gimpdbusservice-generated \
			 --c-namespace GimpDBusService \
			 $(srcdir)/dbus-service.xml
