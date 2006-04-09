## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Plug-In\"	\
	@GTHREAD_CFLAGS@

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappplug-in.a

libappplug_in_a_SOURCES = \
	plug-in-enums.c 	\
	plug-in-enums.h 	\
	plug-in-types.h 	\
	plug-in.c		\
	plug-in.h		\
	plug-in-context.c	\
	plug-in-context.h	\
	plug-in-data.c		\
	plug-in-data.h		\
	plug-in-debug.c		\
	plug-in-debug.h		\
	plug-in-def.c		\
	plug-in-def.h		\
	plug-in-file.c		\
	plug-in-file.h		\
	plug-in-help-domain.c	\
	plug-in-help-domain.h	\
	plug-in-locale-domain.c	\
	plug-in-locale-domain.h	\
	plug-in-menu-branch.c	\
	plug-in-menu-branch.h	\
	plug-in-message.c	\
	plug-in-message.h	\
	plug-in-params.c	\
	plug-in-params.h	\
	plug-in-proc-frame.c	\
	plug-in-proc-frame.h	\
	plug-in-progress.c	\
	plug-in-progress.h	\
	plug-in-rc.c		\
	plug-in-rc.h		\
	plug-in-run.c		\
	plug-in-run.h		\
	plug-in-shm.c		\
	plug-in-shm.h		\
	plug-ins.c		\
	plug-ins.h		\
	plug-ins-query.c	\
	plug-ins-query.h

EXTRA_DIST = makefile.msc

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-bec
CLEANFILES = $(EXTRA_PROGRAMS) $(gen_sources)

$(srcdir)/plug-in-enums.c: $(srcdir)/plug-in-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"plug-in-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (! type)\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$(srcdir)/plug-in-enums.h > xgen-bec \
	&& cp xgen-bec $(@F) \
	&& rm -f xgen-bec
