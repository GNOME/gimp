## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Display\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GTK_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libappdisplay.a

libappdisplay_a_sources = \
	display-enums.h				\
	display-types.h				\
	gimpcanvas.c				\
	gimpcanvas.h				\
	gimpdisplay.c				\
	gimpdisplay.h				\
	gimpdisplayoptions.c			\
	gimpdisplayoptions.h			\
	gimpdisplay-foreach.c			\
	gimpdisplay-foreach.h			\
	gimpdisplay-handlers.c			\
	gimpdisplay-handlers.h			\
	gimpdisplayshell.c			\
	gimpdisplayshell.h			\
	gimpdisplayshell-appearance.c		\
	gimpdisplayshell-appearance.h		\
	gimpdisplayshell-callbacks.c		\
	gimpdisplayshell-callbacks.h		\
	gimpdisplayshell-cursor.c		\
	gimpdisplayshell-cursor.h		\
	gimpdisplayshell-dnd.c			\
	gimpdisplayshell-dnd.h			\
	gimpdisplayshell-draw.c			\
	gimpdisplayshell-draw.h			\
	gimpdisplayshell-handlers.c		\
	gimpdisplayshell-handlers.h		\
	gimpdisplayshell-filter.c		\
	gimpdisplayshell-filter.h		\
	gimpdisplayshell-filter-dialog.c	\
	gimpdisplayshell-filter-dialog.h	\
	gimpdisplayshell-layer-select.c		\
	gimpdisplayshell-layer-select.h		\
	gimpdisplayshell-preview.c		\
	gimpdisplayshell-preview.h		\
	gimpdisplayshell-render.c		\
	gimpdisplayshell-render.h		\
	gimpdisplayshell-scale.c		\
	gimpdisplayshell-scale.h		\
	gimpdisplayshell-scroll.c		\
	gimpdisplayshell-scroll.h		\
	gimpdisplayshell-selection.c		\
	gimpdisplayshell-selection.h		\
	gimpdisplayshell-title.c		\
	gimpdisplayshell-title.h		\
	gimpdisplayshell-transform.c		\
	gimpdisplayshell-transform.h		\
	gimpnavigationview.c			\
	gimpnavigationview.h			\
	gimpprogress.c				\
	gimpprogress.h				\
	gimpscalecombobox.c			\
	gimpscalecombobox.h			\
	gimpstatusbar.c				\
	gimpstatusbar.h

libappdisplay_a_built_sources = display-enums.c

libappdisplay_a_SOURCES = \
	$(libappdisplay_a_built_sources)	\
	$(libappdisplay_a_sources)

EXTRA_DIST = makefile.msc

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-dec
CLEANFILES = $(gen_sources)

$(srcdir)/display-enums.c: $(srcdir)/display-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"display-enums.h\"\n#include\"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "\nstatic const GEnumValue @enum_name@_enum_values[] =\n{" \
		--vprod "  { @VALUENAME@, @valuedesc@, \"@valuenick@\" }," \
		--vtail "  { 0, NULL, NULL }\n};\n\nGType\n@enum_name@_get_type (void)\n{\n  static GType enum_type = 0;\n\n  if (!enum_type)\n    enum_type = g_enum_register_static (\"@EnumName@\", @enum_name@_enum_values);\n\n  return enum_type;\n}\n" \
		$(srcdir)/display-enums.h > xgen-dec \
	&& cp xgen-dec $(@F) \
	&& rm -f xgen-dec
