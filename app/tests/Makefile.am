# Prevent parallel builds for the tests, as e.g. done by make -j check
# The tests must not be run in parallel or in a different order as specified
.NOTPARALLEL: check

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
endif

SUBDIRS = \
	files		\
	ligmadir		\
	ligmadir-empty

# Don't mess with user's ligmadir. Pass in the abs top srcdir to the
# tests through an environment variable so they can set the ligmadir
# they want to use
TESTS_ENVIRONMENT = \
	LIGMA_TESTING_ABS_TOP_SRCDIR=@abs_top_srcdir@ \
	LIGMA_TESTING_ABS_TOP_BUILDDIR=@abs_top_builddir@ \
	LIGMA_TESTING_PLUGINDIRS=@abs_top_builddir@/plug-ins/common \
	LIGMA_TESTING_PLUGINDIRS_BASENAME_IGNORES=mkgen.pl

# Run tests with xvfb-run if available
if HAVE_XVFB_RUN
TESTS_ENVIRONMENT += $(XVFB_RUN) --auto-servernum --server-args="-screen 0 1280x1024x24"
endif


TESTS = \
	test-core					\
	test-ligmaidtable				\
	test-save-and-export				\
	test-session-2-8-compatibility-multi-window	\
	test-session-2-8-compatibility-single-window	\
	test-single-window-mode				\
	test-tools					\
	test-ui						\
	test-xcf

EXTRA_PROGRAMS = $(TESTS)
CLEANFILES = $(EXTRA_PROGRAMS)

$(TESTS): ligmadir-output ligma-test-icon-theme

noinst_LIBRARIES = libligmaapptestutils.a
libligmaapptestutils_a_SOURCES = \
	ligma-app-test-utils.c		\
	ligma-app-test-utils.h		\
	ligma-test-session-utils.c	\
	ligma-test-session-utils.h

libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la
libligmamodule = $(top_builddir)/libligmamodule/libligmamodule-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligmathumb = $(top_builddir)/libligmathumb/libligmathumb-$(LIGMA_API_VERSION).la

if PLATFORM_LINUX
libdl = -ldl
endif

if OS_WIN32
else
libm = -lm
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(PANGOCAIRO_CFLAGS)	\
	$(GTK_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	$(xobjective_c)		\
	-I$(includedir)

# We need this due to circular dependencies
AM_LDFLAGS = \
	-Wl,-u,$(SYMPREFIX)ligma_vectors_undo_get_type		\
	-Wl,-u,$(SYMPREFIX)ligma_vectors_mod_undo_get_type	\
	-Wl,-u,$(SYMPREFIX)ligma_config_param_spec_duplicate		\
	-Wl,-u,$(SYMPREFIX)ligma_operations_init			\
	-Wl,-u,$(SYMPREFIX)xcf_init				\
	-Wl,-u,$(SYMPREFIX)internal_procs_init			\
	-Wl,-u,$(SYMPREFIX)ligma_pdb_compat_procs_register	\
	-Wl,-u,$(SYMPREFIX)ligma_plug_in_manager_restore		\
	-Wl,-u,$(SYMPREFIX)ligma_layer_mode_is_legacy		\
	-Wl,-u,$(SYMPREFIX)gui_init				\
	-Wl,-u,$(SYMPREFIX)ligma_tool_cursors_get_resource	\
	-Wl,-u,$(SYMPREFIX)ligma_lebl_dialog

# Note that we have some duplicate entries here too to work around
# circular dependencies and systems on the same architectural layer as
# an alternative to LDFLAGS above
LDADD = \
	../gui/libappgui.a						\
	../tools/libapptools.a						\
	../dialogs/libappdialogs.a					\
	../menus/libappmenus.a						\
	../actions/libappactions.a					\
	../dialogs/libappdialogs.a					\
	../display/libappdisplay.a					\
	../propgui/libapppropgui.a					\
	../widgets/libappwidgets.a					\
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
	../config/libappconfig.a					\
	../libapp.a							\
	../gegl/libappgegl.a						\
	../operations/libappoperations.a				\
	../operations/layer-modes/libapplayermodes.a			\
	../operations/layer-modes-legacy/libapplayermodeslegacy.a	\
	libligmaapptestutils.a						\
	$(libligmawidgets)						\
	$(libligmaconfig)						\
	$(libligmamath)							\
	$(libligmathumb)							\
	$(libligmacolor)							\
	$(libligmamodule)						\
	$(libligmabase)							\
	$(LIGMAICONRC)							\
	$(GTK_LIBS)							\
	$(APPSTREAM_GLIB_LIBS)						\
	$(GTK_MAC_INTEGRATION_LIBS)					\
	$(GDK_PIXBUF_LIBS)						\
	$(FREETYPE_LIBS)						\
	$(FONTCONFIG_LIBS)						\
	$(PANGOCAIRO_LIBS)						\
	$(HARFBUZZ_LIBS)						\
	$(CAIRO_LIBS)							\
	$(GEGL_LIBS)							\
	$(GIO_LIBS)							\
	$(GEXIV2_LIBS)							\
	$(Z_LIBS)							\
	$(JSON_C_LIBS)							\
	$(LIBARCHIVE_LIBS)						\
	$(LIBMYPAINT_LIBS)						\
	$(LIBBACKTRACE_LIBS)						\
	$(LIBUNWIND_LIBS)						\
	$(INTLLIBS)							\
	$(RT_LIBS)							\
	$(libm)								\
	$(libdl)

ligmadir-output:
	mkdir -p ligmadir-output
	mkdir -p ligmadir-output/brushes
	mkdir -p ligmadir-output/patterns
	mkdir -p ligmadir-output/gradients

ligma-test-icon-theme:
	mkdir -p $$(echo $$(find $(top_srcdir)/icons/Color -name [0-9][0-9] -type d | sed 's@.*/\([0-9][0-9]\)$$@ligma-test-icon-theme/hicolor/\1x\1@'))
	for dir in $$(echo $$(find $(top_srcdir)/icons/Color/ -name [0-9][0-9] -type d | sed 's@.*/\([0-9][0-9]\)$$@\1@')); do \
		(cd ligma-test-icon-theme/hicolor/$${dir}x$${dir}/ && \
		$(LN_S) $(abs_top_srcdir)/icons/Color/$${dir} apps); \
	done
	(cd ligma-test-icon-theme/hicolor && $(LN_S) $(abs_top_srcdir)/icons/Color/index.theme index.theme)

clean-local:
	rm -rf ligmadir-output
	rm -fr ligma-test-icon-theme
