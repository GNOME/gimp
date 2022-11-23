#TESTS = test-operations

EXTRA_PROGRAMS = $(TESTS)
CLEANFILES = $(EXTRA_PROGRAMS)

$(TESTS): output-dir

libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la
libligmamodule = $(top_builddir)/libligmamodule/libligmamodule-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligmathumb = $(top_builddir)/libligmathumb/libligmathumb-$(LIGMA_API_VERSION).la

if OS_WIN32
else
libm = -lm
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GEGL_CFLAGS)		\
	-I$(includedir)

# We need this due to circular dependencies, see more detailed
# comments about it in app/Makefile.am
AM_LDFLAGS = \
	-Wl,-u,$(SYMPREFIX)xcf_init				\
	-Wl,-u,$(SYMPREFIX)internal_procs_init			\
	-Wl,-u,$(SYMPREFIX)ligma_plug_in_manager_restore		\
	-Wl,-u,$(SYMPREFIX)ligma_vectors_undo_get_type		\
	-Wl,-u,$(SYMPREFIX)ligma_vectors_mod_undo_get_type	\
	-Wl,-u,$(SYMPREFIX)ligma_vectors_prop_undo_get_type

# Note that we have some duplicate entries here too to work around
# circular dependencies and systems on the same architectural layer as
# an alternative to LDFLAGS above
LDADD = \
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
	$(libligmaconfig)					\
	$(libligmamath)						\
	$(libligmathumb)						\
	$(libligmacolor)						\
	$(libligmamodule)					\
	$(libligmabase)						\
	$(GDK_PIXBUF_LIBS)					\
	$(PANGOCAIRO_LIBS)					\
	$(GEGL_LIBS)						\
	$(GLIB_LIBS)						\
	$(libm)

output-dir:
	mkdir -p output

clean-local:
	rm -rf output
