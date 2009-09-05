## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpthumb = $(top_builddir)/libgimpthumb/libgimpthumb-$(GIMP_API_VERSION).la

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Config\"	\
	-DGIMP_APP_VERSION_STRING=\"$(GIMP_APP_VERSION)\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappconfig.a

libappconfig_a_SOURCES = \
	config-types.h			\
	gimpconfig-dump.c		\
	gimpconfig-dump.h		\
	gimpconfig-file.c		\
	gimpconfig-file.h		\
	gimpconfig-utils.c		\
	gimpconfig-utils.h		\
	gimpbaseconfig.c		\
	gimpbaseconfig.h		\
	gimpcoreconfig.c		\
	gimpcoreconfig.h		\
	gimpdisplayconfig.c		\
	gimpdisplayconfig.h		\
	gimpguiconfig.c			\
	gimpguiconfig.h			\
	gimppluginconfig.c		\
	gimppluginconfig.h		\
	gimprc.c			\
	gimprc.h			\
	gimprc-blurbs.h			\
	gimprc-deserialize.c		\
	gimprc-deserialize.h		\
	gimprc-serialize.c		\
	gimprc-serialize.h		\
	gimprc-unknown.c		\
	gimprc-unknown.h		\
	gimpxmlparser.c			\
	gimpxmlparser.h

EXTRA_PROGRAMS = test-config

EXTRA_DIST = makefile.msc

#
# unit tests for the GimpConfig system
#

TESTS = test-config

test_config_DEPENDENCIES = $(gimpconfig_libs)

# We need this due to circular dependencies, see more detailed
# comments about it in app/Makefile.am
test_config_LDFLAGS = \
	-u $(SYMPREFIX)xcf_init				\
	-u $(SYMPREFIX)gimp_pdb_new			\
	-u $(SYMPREFIX)internal_procs_init		\
	-u $(SYMPREFIX)gimp_plug_in_manager_restore	\
	-u $(SYMPREFIX)plug_in_icc_profile_apply_rgb	\
	-u $(SYMPREFIX)gimp_vectors_undo_get_type	\
	-u $(SYMPREFIX)gimp_vectors_mod_undo_get_type	\
	-u $(SYMPREFIX)gimp_vectors_prop_undo_get_type	\
	-u $(SYMPREFIX)gimp_image_map_config_get_type

test_config_LDADD = \
	../display/gimpdisplayoptions.o		\
	../display/display-enums.o		\
	../widgets/widgets-enums.o		\
	../xcf/libappxcf.a			\
	../pdb/libappinternal-procs.a		\
	../pdb/libapppdb.a			\
	../plug-in/libappplug-in.a		\
	../vectors/libappvectors.a		\
	../core/libappcore.a			\
	../file/libappfile.a			\
	../text/libapptext.a			\
	../paint/libapppaint.a			\
	../composite/libappcomposite.a		\
	../gegl/libappgegl.a			\
	libappconfig.a				\
	../paint-funcs/libapppaint-funcs.a	\
	../base/libappbase.a			\
	../gimp-log.o				\
	$(libgimpmodule)			\
	$(libgimpcolor)				\
	$(libgimpthumb)				\
	$(libgimpmath)				\
	$(libgimpconfig)			\
	$(libgimpbase)				\
	$(PANGOCAIRO_LIBS)			\
	$(GDK_PIXBUF_LIBS)			\
	$(GEGL_LIBS)				\
	$(GLIB_LIBS)

CLEANFILES = $(EXTRA_PROGRAMS) foorc
