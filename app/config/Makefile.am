## Process this file with automake to produce Makefile.in

libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la
libligmamodule = $(top_builddir)/libligmamodule/libligmamodule-$(LIGMA_API_VERSION).la
libligmathumb = $(top_builddir)/libligmathumb/libligmathumb-$(LIGMA_API_VERSION).la

if OS_WIN32
else
libm = -lm
endif

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Ligma-Config\"				\
	-DLIGMA_APP_VERSION_STRING=\"$(LIGMA_APP_VERSION)\"	\
	-I$(top_builddir)					\
	-I$(top_srcdir)						\
	-I$(top_builddir)/app					\
	-I$(top_srcdir)/app					\
	$(GIO_UNIX_CFLAGS)					\
	$(GIO_WINDOWS_CFLAGS)					\
	$(GEGL_CFLAGS)						\
	$(CAIRO_CFLAGS)						\
	$(GDK_PIXBUF_CFLAGS)					\
	$(MYPAINT_BRUSHES_CFLAGS)				\
	-I$(includedir)

noinst_LIBRARIES = libappconfig.a

libappconfig_a_sources = \
	config-enums.h			\
	config-types.h			\
	ligmaconfig-dump.c		\
	ligmaconfig-dump.h		\
	ligmaconfig-file.c		\
	ligmaconfig-file.h		\
	ligmaconfig-utils.c		\
	ligmaconfig-utils.h		\
	ligmacoreconfig.c		\
	ligmacoreconfig.h		\
	ligmadialogconfig.c		\
	ligmadialogconfig.h		\
	ligmadisplayconfig.c		\
	ligmadisplayconfig.h		\
	ligmadisplayoptions.c		\
	ligmadisplayoptions.h		\
	ligmaearlyrc.c			\
	ligmaearlyrc.h			\
	ligmageglconfig.c		\
	ligmageglconfig.h		\
	ligmaguiconfig.c			\
	ligmaguiconfig.h			\
	ligmapluginconfig.c		\
	ligmapluginconfig.h		\
	ligmarc.c			\
	ligmarc.h			\
	ligmarc-blurbs.h			\
	ligmarc-deserialize.c		\
	ligmarc-deserialize.h		\
	ligmarc-serialize.c		\
	ligmarc-serialize.h		\
	ligmarc-unknown.c		\
	ligmarc-unknown.h		\
	ligmaxmlparser.c			\
	ligmaxmlparser.h

libappconfig_a_built_sources = \
	config-enums.c

libappconfig_a_SOURCES = \
	$(libappconfig_a_built_sources)	\
	$(libappconfig_a_sources)

EXTRA_PROGRAMS = test-config

#
# unit tests for the LigmaConfig system
#

TESTS = test-config

test_config_DEPENDENCIES = $(ligmaconfig_libs)

# We need this due to circular dependencies
test_config_LDFLAGS = \
	-Wl,-u,$(SYMPREFIX)ligma_vectors_undo_get_type		\
	-Wl,-u,$(SYMPREFIX)ligma_vectors_mod_undo_get_type	\
	-Wl,-u,$(SYMPREFIX)ligma_config_param_spec_duplicate		\
	-Wl,-u,$(SYMPREFIX)xcf_init				\
	-Wl,-u,$(SYMPREFIX)internal_procs_init			\
	-Wl,-u,$(SYMPREFIX)ligma_pdb_compat_procs_register	\
	-Wl,-u,$(SYMPREFIX)ligma_plug_in_manager_restore		\
	-Wl,-u,$(SYMPREFIX)ligma_layer_mode_is_legacy		\
	-Wl,-u,$(SYMPREFIX)ligma_async_set_new			\
	-Wl,-u,$(SYMPREFIX)ligma_uncancelable_waitable_new

test_config_LDADD = \
	../xcf/libappxcf.a						\
	../pdb/libappinternal-procs.a					\
	../pdb/libapppdb.a						\
	../plug-in/libappplug-in.a					\
	../vectors/libappvectors.a					\
	../core/libappcore.a						\
	../file/libappfile.a						\
	../file-data/libappfile-data.a					\
	../text/libapptext.a						\
	../paint/libapppaint.a						\
	../gegl/libappgegl.a						\
	../operations/libappoperations.a				\
	../operations/layer-modes/libapplayermodes.a			\
	../operations/layer-modes-legacy/libapplayermodeslegacy.a	\
	libappconfig.a							\
	../ligma-debug.o							\
	../ligma-log.o							\
	$(libligmamodule)						\
	$(libligmacolor)							\
	$(libligmathumb)							\
	$(libligmamath)							\
	$(libligmaconfig)						\
	$(libligmabase)							\
	$(APPSTREAM_GLIB_LIBS)						\
	$(PANGOCAIRO_LIBS)						\
	$(HARFBUZZ_LIBS)						\
	$(GDK_PIXBUF_LIBS)						\
	$(GEGL_LIBS)							\
	$(GIO_LIBS)							\
	$(GEXIV2_LIBS)							\
	$(Z_LIBS)							\
	$(JSON_C_LIBS)							\
	$(LIBARCHIVE_LIBS)						\
	$(LIBMYPAINT_LIBS)						\
	$(libm)

CLEANFILES = $(EXTRA_PROGRAMS) foorc

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = stamp-config-enums.h xgen-cec
CLEANFILES += $(gen_sources)

xgen-cec: $(srcdir)/config-enums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-config-enums.h\"\n#include \"config.h\"\n#include <gio/gio.h>\n#include \"libligmabase/ligmabase.h\"\n#include \"config-enums.h\"\n#include \"ligma-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-config-enums.h: xgen-cec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/config-enums.c: xgen-cec stamp-config-enums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
