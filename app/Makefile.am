## Process this file with automake to produce Makefile.in

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
endif

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimpthumb = $(top_builddir)/libgimpthumb/libgimpthumb-$(GIMP_API_VERSION).la

# Sort this by architectural dependencies, lowest level at the top,
# so that when e.g. changing a header-file the subdirs are built in
# the right order
SUBDIRS = \
	config		\
	core		\
	operations	\
	gegl		\
	text		\
	vectors		\
	paint		\
	plug-in		\
	xcf		\
	file		\
	file-data	\
	pdb		\
	widgets		\
	propgui		\
	display		\
	tools		\
	dialogs		\
	actions		\
	menus		\
	gui		\
	.		\
	tests

# Put the GIMP core in a lib so we can conveniently link against that
# in test cases
noinst_LIBRARIES = libapp.a

if ENABLE_GIMP_CONSOLE
bin_PROGRAMS = gimp-@GIMP_APP_VERSION@ gimp-console-@GIMP_APP_VERSION@
else
bin_PROGRAMS = gimp-@GIMP_APP_VERSION@
endif

libapp_sources = \
	about.h			\
	app.c			\
	app.h			\
	errors.c		\
	errors.h		\
	language.c		\
	language.h		\
	sanity.c		\
	sanity.h		\
	signals.c		\
	signals.h		\
	tests.c			\
	tests.h			\
	unique.c		\
	unique.h		\
	gimp-debug.c		\
	gimp-debug.h		\
	gimp-intl.h		\
	gimp-log.c		\
	gimp-log.h		\
	gimp-priorities.h	\
	gimp-update.c		\
	gimp-update.h		\
	gimp-version.c		\
	gimp-version.h

libapp_a_SOURCES = $(libapp_sources)

gimp_@GIMP_APP_VERSION@_SOURCES = $(libapp_sources) main.c


if PLATFORM_LINUX
libdl = -ldl
endif

if PLATFORM_OSX
framework_cocoa = -framework Cocoa
endif

if OS_WIN32
win32_ldflags = -mwindows -Wl,--tsaware $(WIN32_LARGE_ADDRESS_AWARE)

# for GimpDashboard and GimpBacktrace
psapi_cflags = -DPSAPI_VERSION=1
libpsapi = -lpsapi

# for GimpBacktrace
libdbghelp = -ldbghelp

# for I_RpcExceptionFilter()
librpcrt4 = -lrpcrt4

if HAVE_EXCHNDL
exchndl = -lexchndl
endif

else
libm = -lm
endif

if ENABLE_RELOCATABLE_RESOURCES
munix = -Wl,-rpath '-Wl,$$ORIGIN/../lib'
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc.rule
GIMPRC = gimp-$(GIMP_APP_VERSION).rc.o
GIMPCONSOLERC = gimp-console-$(GIMP_APP_VERSION).rc.o
endif

AM_CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"			\
	-DLIBEXECDIR=\""$(libexecdir)"\"		\
	-DGIMP_USER_VERSION=\"$(GIMP_USER_VERSION)\"	\
	-DGIMP_TOOL_VERSION=\"$(GIMP_TOOL_VERSION)\"	\
	-DG_LOG_DOMAIN=\"Gimp\"				\
	-DGIMP_APP_GLUE_COMPILATION			\
	-DCC_VERSION=\""$(CC_VERSION)"\"		\
	-I$(top_srcdir)					\
	$(GTK_CFLAGS)					\
	$(PANGOCAIRO_CFLAGS)				\
	$(GEGL_CFLAGS)					\
	$(LCMS_CFLAGS)					\
	$(GEXIV2_CFLAGS)				\
	$(psapi_cflags)					\
	$(xobjective_c)					\
	-I$(includedir)					\
	-I$(builddir)/gui

# We need this due to circular dependencies
AM_LDFLAGS = \
	$(munix)						\
	-Wl,-u,$(SYMPREFIX)gimp_vectors_undo_get_type		\
	-Wl,-u,$(SYMPREFIX)gimp_vectors_mod_undo_get_type	\
	-Wl,-u,$(SYMPREFIX)gimp_config_param_spec_duplicate		\
	-Wl,-u,$(SYMPREFIX)gimp_operations_init			\
	-Wl,-u,$(SYMPREFIX)xcf_init				\
	-Wl,-u,$(SYMPREFIX)internal_procs_init			\
	-Wl,-u,$(SYMPREFIX)gimp_pdb_compat_procs_register	\
	-Wl,-u,$(SYMPREFIX)gimp_plug_in_manager_restore		\
	-Wl,-u,$(SYMPREFIX)gimp_layer_mode_is_legacy		\
	-Wl,-u,$(SYMPREFIX)gimp_parallel_init			\
	-Wl,-u,$(SYMPREFIX)gimp_async_set_new			\
	-Wl,-u,$(SYMPREFIX)gimp_uncancelable_waitable_new

gimpconsoleldadd = \
	xcf/libappxcf.a						\
	pdb/libappinternal-procs.a				\
	pdb/libapppdb.a						\
	plug-in/libappplug-in.a					\
	vectors/libappvectors.a					\
	core/libappcore.a					\
	file/libappfile.a					\
	file-data/libappfile-data.a				\
	text/libapptext.a					\
	paint/libapppaint.a					\
	operations/libappoperations.a				\
	operations/layer-modes/libapplayermodes.a		\
	operations/layer-modes-legacy/libapplayermodeslegacy.a	\
	gegl/libappgegl.a					\
	config/libappconfig.a					\
	$(libgimpconfig)					\
	$(libgimpmath)						\
	$(libgimpthumb)						\
	$(libgimpcolor)						\
	$(libgimpmodule)					\
	$(libgimpbase)						\
	$(APPSTREAM_GLIB_LIBS)					\
	$(GDK_PIXBUF_LIBS)					\
	$(FREETYPE_LIBS)					\
	$(FONTCONFIG_LIBS)					\
	$(PANGOCAIRO_LIBS)					\
	$(HARFBUZZ_LIBS)					\
	$(CAIRO_LIBS)						\
	$(GIO_UNIX_LIBS)					\
	$(GIO_WINDOWS_LIBS)					\
	$(GEGL_LIBS)						\
	$(GLIB_LIBS)						\
	$(LCMS_LIBS)						\
	$(GEXIV2_LIBS)						\
	$(Z_LIBS)						\
	$(JSON_C_LIBS)						\
	$(LIBARCHIVE_LIBS)					\
	$(LIBMYPAINT_LIBS)					\
	$(LIBBACKTRACE_LIBS)					\
	$(LIBUNWIND_LIBS)					\
	$(INTLLIBS)						\
	$(RT_LIBS)						\
	$(libm)							\
	$(libdl)						\
	$(libpsapi)						\
	$(libdbghelp)						\
	$(librpcrt4)

gimp_@GIMP_APP_VERSION@_LDFLAGS = \
	$(AM_LDFLAGS)						\
	$(win32_ldflags)					\
	$(framework_cocoa)					\
	-Wl,-u,$(SYMPREFIX)gimp_lebl_dialog			\
	-Wl,-u,$(SYMPREFIX)gimp_core_pixbufs_get_resource	\
	-Wl,-u,$(SYMPREFIX)gimp_tool_cursors_get_resource


gimp_@GIMP_APP_VERSION@_LDADD = \
	gui/libappgui.a			\
	menus/libappmenus.a		\
	actions/libappactions.a		\
	dialogs/libappdialogs.a		\
	tools/libapptools.a		\
	display/libappdisplay.a		\
	propgui/libapppropgui.a		\
	widgets/libappwidgets.a		\
	$(libgimpwidgets)		\
	$(GTK_LIBS)			\
	$(GTK_MAC_INTEGRATION_LIBS)	\
	$(gimpconsoleldadd)		\
	$(exchndl)			\
	$(GIMPRC)


if ENABLE_GIMP_CONSOLE

gimp_console_@GIMP_APP_VERSION@_SOURCES = $(libapp_sources) main.c

gimp_console_@GIMP_APP_VERSION@_CPPFLAGS = \
	$(AM_CPPFLAGS)			\
	-DGIMP_CONSOLE_COMPILATION

gimp_console_@GIMP_APP_VERSION@_LDADD = \
	$(gimpconsoleldadd) \
	$(GIMPCONSOLERC)

endif


install-exec-hook:
if DEFAULT_BINARY
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp$(EXEEXT) \
	&& $(LN_S) gimp-$(GIMP_APP_VERSION)$(EXEEXT) gimp$(EXEEXT)
if ENABLE_GIMP_CONSOLE
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp-console$(EXEEXT) \
	&& $(LN_S) gimp-console-$(GIMP_APP_VERSION)$(EXEEXT) gimp-console$(EXEEXT)
endif
endif

uninstall-local:
if DEFAULT_BINARY
	rm -f $(DESTDIR)$(bindir)/gimp$(EXEEXT)
if ENABLE_GIMP_CONSOLE
	rm -f $(DESTDIR)$(bindir)/gimp-console$(EXEEXT)
endif
endif


# require gimp-console when making dist
#
if ENABLE_GIMP_CONSOLE
dist-check-gimp-console:
else
dist-check-gimp-console:
	@echo "*** gimp-console must be enabled in order to make dist"
	@false
endif


# hook to assure that the system gimprc and the gimprc manpage are
# uptodate when a release is made
#
dist-dump-gimprc: gimp-console-$(GIMP_APP_VERSION)$(EXEEXT)
	./$< --dump-gimprc-system > gimprc.tmp	\
		&& (cmp -s gimprc.tmp $(top_srcdir)/etc/gimprc.in ||	\
		    cp gimprc.tmp $(top_srcdir)/etc/gimprc.in)		\
		&& rm gimprc.tmp
	./$< --dump-gimprc-manpage > gimprc.tmp	\
		&& (cmp -s gimprc.tmp $(top_srcdir)/docs/gimprc.5.in ||\
		    cp gimprc.tmp $(top_srcdir)/docs/gimprc.5.in)	\
		&& rm gimprc.tmp

dist-hook: dist-check-gimp-console dist-dump-gimprc
