## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpthumb = $(top_builddir)/libgimpthumb/libgimpthumb-$(GIMP_API_VERSION).la

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

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Config\"	\
	@GTHREAD_CFLAGS@

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GLIB_CFLAGS)		\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)


EXTRA_PROGRAMS = test-config

EXTRA_DIST = makefile.msc

#
# unit tests for the GimpConfig system
#

TESTS = test-config

test_config_DEPENDENCIES = $(gimpconfig_libs)

test_config_LDFLAGS = \
	-u $(SYMPREFIX)xcf_init			\
	-u $(SYMPREFIX)internal_procs_init	\
	-u $(SYMPREFIX)gimp_coords_mix		\
	-u $(SYMPREFIX)gimp_plug_in_manager_restore

test_config_LDADD = \
	../widgets/widgets-enums.o		\
	../display/display-enums.o		\
	../display/gimpdisplayoptions.o		\
	../pdb/libappinternal-procs.a		\
	../xcf/libappxcf.a			\
	../file/libappfile.a			\
	../plug-in/libappplug-in.a		\
	../pdb/libapppdb.a			\
	../core/libappcore.a			\
	../vectors/libappvectors.a		\
	../paint/libapppaint.a			\
	../text/libapptext.a			\
	libappconfig.a				\
	../paint-funcs/libapppaint-funcs.a	\
	../composite/libappcomposite.a		\
	../base/libappbase.a			\
	$(libgimpmodule)			\
	$(libgimpcolor)				\
	$(libgimpthumb)				\
	$(libgimpmath)				\
	$(libgimpconfig)			\
	$(libgimpbase)				\
	$(PANGOFT2_LIBS)			\
	$(GDK_PIXBUF_LIBS)			\
	$(LIBART_LIBS)				\
	$(GLIB_LIBS)


CLEANFILES = $(EXTRA_PROGRAMS) foorc
