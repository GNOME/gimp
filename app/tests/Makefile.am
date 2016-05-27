SUBDIRS = \
	files		\
	gimpdir		\
	gimpdir-empty

# Don't mess with user's gimpdir. Pass in the abs top srcdir to the
# tests through an environment variable so they can set the gimpdir
# they want to use
TESTS_ENVIRONMENT = \
	GIMP_TESTING_ABS_TOP_SRCDIR=@abs_top_srcdir@ \
	GIMP_TESTING_ABS_TOP_BUILDDIR=@abs_top_builddir@ \
	GIMP_TESTING_PLUGINDIRS=@abs_top_builddir@/plug-ins/common \
	GIMP_TESTING_PLUGINDIRS_BASENAME_IGNORES=mkgen.pl

# Run tests with xvfb-run if available
if HAVE_XVFB_RUN
TESTS_ENVIRONMENT += $(XVFB_RUN) --auto-servernum --server-args="-screen 0 1280x1024x24"
endif


TESTS = \
	test-core					\
	test-gimpidtable				\
	test-save-and-export				\
	test-session-2-6-compatibility			\
	test-session-2-8-compatibility-multi-window	\
	test-session-2-8-compatibility-single-window	\
	test-single-window-mode				\
	test-tools					\
	test-ui						\
	test-xcf

EXTRA_PROGRAMS = $(TESTS)
CLEANFILES = $(EXTRA_PROGRAMS)

$(TESTS): gimpdir-output gimp-test-icon-theme

noinst_LIBRARIES = libgimpapptestutils.a
libgimpapptestutils_a_SOURCES = \
	gimp-app-test-utils.c		\
	gimp-app-test-utils.h		\
	gimp-test-session-utils.c	\
	gimp-test-session-utils.h

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimpthumb = $(top_builddir)/libgimpthumb/libgimpthumb-$(GIMP_API_VERSION).la

if OS_WIN32
else
libm = -lm
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(PANGOCAIRO_CFLAGS)	\
	$(GTK_CFLAGS)		\
	$(DBUS_GLIB_CFLAGS)	\
	$(GEGL_CFLAGS)		\
	-I$(includedir)

# We need this due to circular dependencies
AM_LDFLAGS = \
	-Wl,-u,$(SYMPREFIX)gimp_vectors_undo_get_type		\
	-Wl,-u,$(SYMPREFIX)gimp_vectors_mod_undo_get_type	\
	-Wl,-u,$(SYMPREFIX)gimp_param_spec_duplicate		\
	-Wl,-u,$(SYMPREFIX)gimp_operations_init			\
	-Wl,-u,$(SYMPREFIX)xcf_init				\
	-Wl,-u,$(SYMPREFIX)internal_procs_init			\
	-Wl,-u,$(SYMPREFIX)gimp_plug_in_manager_restore		\
	-Wl,-u,$(SYMPREFIX)gimp_pdb_compat_param_spec		\
	-Wl,-u,$(SYMPREFIX)gui_init				\
	-Wl,-u,$(SYMPREFIX)gimp_lebl_dialog

# Note that we have some duplicate entries here too to work around
# circular dependencies and systems on the same architectural layer as
# an alternative to LDFLAGS above
LDADD = \
	$(top_builddir)/app/gui/libappgui.a			\
	$(top_builddir)/app/tools/libapptools.a			\
	$(top_builddir)/app/dialogs/libappdialogs.a		\
	$(top_builddir)/app/menus/libappmenus.a			\
	$(top_builddir)/app/actions/libappactions.a		\
	$(top_builddir)/app/dialogs/libappdialogs.a		\
	$(top_builddir)/app/display/libappdisplay.a		\
	$(top_builddir)/app/widgets/libappwidgets.a		\
	$(top_builddir)/app/xcf/libappxcf.a			\
	$(top_builddir)/app/pdb/libappinternal-procs.a		\
	$(top_builddir)/app/pdb/libapppdb.a			\
	$(top_builddir)/app/plug-in/libappplug-in.a		\
	$(top_builddir)/app/vectors/libappvectors.a		\
	$(top_builddir)/app/core/libappcore.a			\
	$(top_builddir)/app/file/libappfile.a			\
	$(top_builddir)/app/text/libapptext.a			\
	$(top_builddir)/app/paint/libapppaint.a			\
	$(top_builddir)/app/config/libappconfig.a		\
	$(top_builddir)/app/libapp.a				\
	$(top_builddir)/app/gegl/libappgegl.a			\
	$(top_builddir)/app/operations/libappoperations.a	\
	libgimpapptestutils.a					\
	$(libgimpwidgets)					\
	$(libgimpconfig)					\
	$(libgimpmath)						\
	$(libgimpthumb)						\
	$(libgimpcolor)						\
	$(libgimpmodule)					\
	$(libgimpbase)						\
	$(GIMPICONRC)						\
	$(GTK_LIBS)						\
	$(DBUS_GLIB_LIBS)					\
	$(GDK_PIXBUF_LIBS)					\
	$(FREETYPE_LIBS)					\
	$(FONTCONFIG_LIBS)					\
	$(PANGOCAIRO_LIBS)					\
	$(HARFBUZZ_LIBS)					\
	$(CAIRO_LIBS)						\
	$(GEGL_LIBS)						\
	$(GIO_LIBS)						\
	$(GEXIV2_LIBS)						\
	$(Z_LIBS)						\
	$(JSON_C_LIBS)						\
	$(LIBMYPAINT_LIBS)					\
	$(INTLLIBS)						\
	$(RT_LIBS)						\
	$(libm)

gimpdir-output:
	mkdir -p gimpdir-output
	mkdir -p gimpdir-output/brushes
	mkdir -p gimpdir-output/patterns
	mkdir -p gimpdir-output/gradients

gimp-test-icon-theme:
	mkdir -p $$(echo $$(find $(top_srcdir)/icons/Color -name [0-9][0-9] -type d | sed 's@.*/\([0-9][0-9]\)$$@gimp-test-icon-theme/hicolor/\1x\1@'))
	for dir in $$(echo $$(find $(top_srcdir)/icons/Color/ -name [0-9][0-9] -type d | sed 's@.*/\([0-9][0-9]\)$$@\1@')); do \
		(cd gimp-test-icon-theme/hicolor/$${dir}x$${dir}/ && \
		$(LN_S) $(abs_top_srcdir)/icons/Color/$${dir} apps); \
	done
	(cd gimp-test-icon-theme/hicolor && $(LN_S) $(abs_top_srcdir)/icons/Color/index.theme index.theme)

clean-local:
	rm -rf gimpdir-output
	rm -fr gimp-test-icon-theme
