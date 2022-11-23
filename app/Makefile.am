## Process this file with automake to produce Makefile.in

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
endif

libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la
libligmamodule = $(top_builddir)/libligmamodule/libligmamodule-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligmathumb = $(top_builddir)/libligmathumb/libligmathumb-$(LIGMA_API_VERSION).la

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

# Put the LIGMA core in a lib so we can conveniently link against that
# in test cases
noinst_LIBRARIES = libapp.a

if ENABLE_LIGMA_CONSOLE
bin_PROGRAMS = ligma-@LIGMA_APP_VERSION@ ligma-console-@LIGMA_APP_VERSION@
else
bin_PROGRAMS = ligma-@LIGMA_APP_VERSION@
endif

libapp_sources = \
	about.h			\
	app.c			\
	app.h			\
	errors.c		\
	errors.h		\
	ligmacoreapp.c		\
	ligmacoreapp.h		\
	ligmaconsoleapp.c	\
	ligmaconsoleapp.h	\
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
	ligma-debug.c		\
	ligma-debug.h		\
	ligma-intl.h		\
	ligma-log.c		\
	ligma-log.h		\
	ligma-priorities.h	\
	ligma-update.c		\
	ligma-update.h		\
	ligma-version.c		\
	ligma-version.h

libapp_a_SOURCES = $(libapp_sources)

ligma_@LIGMA_APP_VERSION@_SOURCES = $(libapp_sources) main.c


if PLATFORM_LINUX
libdl = -ldl
endif

if PLATFORM_OSX
framework_cocoa = -framework Cocoa
endif

if OS_WIN32
win32_ldflags = -mwindows -Wl,--tsaware $(WIN32_LARGE_ADDRESS_AWARE)

# for LigmaDashboard and LigmaBacktrace
psapi_cflags = -DPSAPI_VERSION=1
libpsapi = -lpsapi

# for LigmaBacktrace
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
include $(top_srcdir)/build/windows/ligmarc.rule
LIGMARC = ligma-$(LIGMA_APP_VERSION).rc.o
LIGMACONSOLERC = ligma-console-$(LIGMA_APP_VERSION).rc.o
endif

AM_CPPFLAGS = \
	-DLIGMADIR=\""$(ligmadir)"\"			\
	-DLIBEXECDIR=\""$(libexecdir)"\"		\
	-DLIGMA_USER_VERSION=\"$(LIGMA_USER_VERSION)\"	\
	-DLIGMA_TOOL_VERSION=\"$(LIGMA_TOOL_VERSION)\"	\
	-DG_LOG_DOMAIN=\"Ligma\"				\
	-DLIGMA_APP_GLUE_COMPILATION			\
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
	-Wl,-u,$(SYMPREFIX)ligma_vectors_undo_get_type		\
	-Wl,-u,$(SYMPREFIX)ligma_vectors_mod_undo_get_type	\
	-Wl,-u,$(SYMPREFIX)ligma_config_param_spec_duplicate		\
	-Wl,-u,$(SYMPREFIX)ligma_operations_init			\
	-Wl,-u,$(SYMPREFIX)xcf_init				\
	-Wl,-u,$(SYMPREFIX)internal_procs_init			\
	-Wl,-u,$(SYMPREFIX)ligma_pdb_compat_procs_register	\
	-Wl,-u,$(SYMPREFIX)ligma_plug_in_manager_restore		\
	-Wl,-u,$(SYMPREFIX)ligma_layer_mode_is_legacy		\
	-Wl,-u,$(SYMPREFIX)ligma_parallel_init			\
	-Wl,-u,$(SYMPREFIX)ligma_async_set_new			\
	-Wl,-u,$(SYMPREFIX)ligma_uncancelable_waitable_new

ligmaconsoleldadd = \
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
	$(libligmaconfig)					\
	$(libligmamath)						\
	$(libligmathumb)						\
	$(libligmacolor)						\
	$(libligmamodule)					\
	$(libligmabase)						\
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

ligma_@LIGMA_APP_VERSION@_LDFLAGS = \
	$(AM_LDFLAGS)						\
	$(win32_ldflags)					\
	$(framework_cocoa)					\
	-Wl,-u,$(SYMPREFIX)ligma_lebl_dialog			\
	-Wl,-u,$(SYMPREFIX)ligma_core_pixbufs_get_resource	\
	-Wl,-u,$(SYMPREFIX)ligma_tool_cursors_get_resource


ligma_@LIGMA_APP_VERSION@_LDADD = \
	gui/libappgui.a			\
	menus/libappmenus.a		\
	actions/libappactions.a		\
	dialogs/libappdialogs.a		\
	tools/libapptools.a		\
	display/libappdisplay.a		\
	propgui/libapppropgui.a		\
	widgets/libappwidgets.a		\
	$(libligmawidgets)		\
	$(GTK_LIBS)			\
	$(GTK_MAC_INTEGRATION_LIBS)	\
	$(ligmaconsoleldadd)		\
	$(exchndl)			\
	$(LIGMARC)


if ENABLE_LIGMA_CONSOLE

ligma_console_@LIGMA_APP_VERSION@_SOURCES = $(libapp_sources) main.c

ligma_console_@LIGMA_APP_VERSION@_CPPFLAGS = \
	$(AM_CPPFLAGS)			\
	-DLIGMA_CONSOLE_COMPILATION

ligma_console_@LIGMA_APP_VERSION@_LDADD = \
	$(ligmaconsoleldadd) \
	$(LIGMACONSOLERC)

endif


install-exec-hook:
if DEFAULT_BINARY
	cd $(DESTDIR)$(bindir) \
	&& rm -f ligma$(EXEEXT) \
	&& $(LN_S) ligma-$(LIGMA_APP_VERSION)$(EXEEXT) ligma$(EXEEXT)
if ENABLE_LIGMA_CONSOLE
	cd $(DESTDIR)$(bindir) \
	&& rm -f ligma-console$(EXEEXT) \
	&& $(LN_S) ligma-console-$(LIGMA_APP_VERSION)$(EXEEXT) ligma-console$(EXEEXT)
endif
endif

uninstall-local:
if DEFAULT_BINARY
	rm -f $(DESTDIR)$(bindir)/ligma$(EXEEXT)
if ENABLE_LIGMA_CONSOLE
	rm -f $(DESTDIR)$(bindir)/ligma-console$(EXEEXT)
endif
endif


# require ligma-console when making dist
#
if ENABLE_LIGMA_CONSOLE
dist-check-ligma-console:
else
dist-check-ligma-console:
	@echo "*** ligma-console must be enabled in order to make dist"
	@false
endif


# hook to assure that the system ligmarc and the ligmarc manpage are
# uptodate when a release is made
#
dist-dump-ligmarc: ligma-console-$(LIGMA_APP_VERSION)$(EXEEXT)
	./$< --dump-ligmarc-system > ligmarc.tmp	\
		&& (cmp -s ligmarc.tmp $(top_srcdir)/etc/ligmarc.in ||	\
		    cp ligmarc.tmp $(top_srcdir)/etc/ligmarc.in)		\
		&& rm ligmarc.tmp
	./$< --dump-ligmarc-manpage > ligmarc.tmp	\
		&& (cmp -s ligmarc.tmp $(top_srcdir)/docs/ligmarc.5.in ||\
		    cp ligmarc.tmp $(top_srcdir)/docs/ligmarc.5.in)	\
		&& rm ligmarc.tmp

dist-hook: dist-check-ligma-console dist-dump-ligmarc
