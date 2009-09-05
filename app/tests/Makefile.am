TESTS = test-layer-grouping
EXTRA_PROGRAMS = $(TESTS)

test_layer_grouping_SOURCES = \
	test-layer-grouping.c

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
	$(GDK_PIXBUF_CFLAGS)	\
	$(GEGL_CFLAGS)		\
	-I$(includedir)

# We need this due to circular dependencies, see more detailed
# comments about it in app/Makefile.am
AM_LDFLAGS = \
	-u $(SYMPREFIX)xcf_init				\
	-u $(SYMPREFIX)internal_procs_init		\
	-u $(SYMPREFIX)gimp_plug_in_manager_restore	\
	-u $(SYMPREFIX)gimp_pdb_compat_param_spec	\
	-u $(SYMPREFIX)plug_in_icc_profile_apply_rgb	\
	-u $(SYMPREFIX)gimp_image_map_config_get_type	\
	-u $(SYMPREFIX)gimp_vectors_undo_get_type	\
	-u $(SYMPREFIX)gimp_vectors_mod_undo_get_type	\
	-u $(SYMPREFIX)gimp_vectors_prop_undo_get_type	\
	-u $(SYMPREFIX)gimp_curve_map_pixels

LDADD = \
	$(top_builddir)/app/display/libappdisplay.a		\
	$(top_builddir)/app/widgets/widgets-enums.o		\
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
	$(top_builddir)/app/display/libappdisplay.a		\
	$(top_builddir)/app/paint-funcs/libapppaint-funcs.a	\
	$(top_builddir)/app/base/libappbase.a			\
	$(top_builddir)/app/gimp-log.o				\
	$(top_builddir)/app/tests.o				\
	$(top_builddir)/app/units.o				\
	$(libgimpconfig)					\
	$(libgimpmath)						\
	$(libgimpthumb)						\
	$(libgimpcolor)						\
	$(libgimpmodule)					\
	$(libgimpbase)						\
	$(GIMPICONRC)						\
	$(GDK_PIXBUF_LIBS)					\
	$(FREETYPE_LIBS)					\
	$(FONTCONFIG_LIBS)					\
	$(PANGOCAIRO_LIBS)					\
	$(CAIRO_LIBS)						\
	$(GEGL_LIBS)						\
	$(GLIB_LIBS)						\
	$(INTLLIBS)						\
	$(RT_LIBS)

