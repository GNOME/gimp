# Don't mess with user's dir
TESTS_ENVIRONMENT = \
	GIMP2_DIRECTORY=$(abs_top_srcdir)/app/tests/gimpdir

TESTS = \
	test-layer-grouping	\
	test-layers		\
	test-session-management	\
	test-window-management	\
	test-xcf		\
	test-ui

EXTRA_PROGRAMS = $(TESTS)

test_layers_SOURCES             = test-layers.c
test_layer_grouping_SOURCES     = test-layer-grouping.c
test_window_management_SOURCES  = test-window-management.c
test_session_management_SOURCES = test-session-management.c

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimpthumb = $(top_builddir)/libgimpthumb/libgimpthumb-$(GIMP_API_VERSION).la

AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(PANGOCAIRO_CFLAGS)	\
	$(GTK_CFLAGS)		\
	$(DBUS_GLIB_CFLAGS)	\
	$(GEGL_CFLAGS)		\
	-I$(includedir)

# We need this due to circular dependencies, see more detailed
# comments about it in app/Makefile.am
AM_LDFLAGS = \
	-u $(SYMPREFIX)xcf_init				\
	-u $(SYMPREFIX)base_init			\
	-u $(SYMPREFIX)internal_procs_init		\
	-u $(SYMPREFIX)gimp_plug_in_manager_restore	\
	-u $(SYMPREFIX)gimp_pdb_compat_param_spec	\
	-u $(SYMPREFIX)gui_init				\
	-u $(SYMPREFIX)plug_in_icc_profile_apply_rgb	\
	-u $(SYMPREFIX)gimp_image_map_config_get_type	\
	-u $(SYMPREFIX)gimp_vectors_undo_get_type	\
	-u $(SYMPREFIX)gimp_vectors_mod_undo_get_type	\
	-u $(SYMPREFIX)gimp_vectors_prop_undo_get_type	\
	-u $(SYMPREFIX)actions_init			\
	-u $(SYMPREFIX)gimp_error_dialog_new		\
	-u $(SYMPREFIX)menus_save			\
	-u $(SYMPREFIX)gimp_tools_save			\
	-u $(SYMPREFIX)gimp_curve_map_pixels

# Note that we have some duplicate entries here too to work around
# circular dependencies and systems on the same architectural layer as
# an alternative to LDFLAGS above
LDADD = \
	$(top_builddir)/app/dialogs/preferences-dialog.o	\
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
	$(top_builddir)/app/composite/libappcomposite.a		\
	$(top_builddir)/app/gegl/libappgegl.a			\
	$(top_builddir)/app/config/libappconfig.a		\
	$(top_builddir)/app/paint-funcs/libapppaint-funcs.a	\
	$(top_builddir)/app/base/libappbase.a			\
	$(top_builddir)/app/libapp.a				\
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
	$(CAIRO_LIBS)						\
	$(GEGL_LIBS)						\
	$(GLIB_LIBS)						\
	$(INTLLIBS)						\
	$(RT_LIBS)

