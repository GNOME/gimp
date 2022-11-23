## Process this file with automake to produce Makefile.in

# derived from  app/Makefile.am, another installed binary

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
framework_cocoa = -framework Cocoa
endif

if OS_WIN32
win32_ldflags = -mwindows -Wl,--tsaware $(WIN32_LARGE_ADDRESS_AWARE)

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
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
script_fu_interpreter_RC = script-fu-interpreter.rc.o
endif

libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la $(libm)

# link to libscriptfu
libligma_scriptfu = $(top_builddir)/plug-ins/script-fu/libscriptfu/libligma-scriptfu-$(LIGMA_API_VERSION).la

# include srcdir parent to find libscriptfu include files
AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	-I$(includedir)		\
	-I$(srcdir)/.. \
	-DG_LOG_DOMAIN=\"scriptfu\"

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_LDFLAGS = \
	$(munix)           \
	$(win32_ldflags)   \
	$(framework_cocoa) \
	$(xnone)

# interpreter is-a plugin but is-a bin_PROGRAM
# i.e. installs to usual place of executables e.g. /usr/bin so shebangs find it.
bin_PROGRAMS = ligma-script-fu-interpreter-@LIGMA_API_VERSION@

ligma_script_fu_interpreter_@LIGMA_API_VERSION@_SOURCES = \
	script-fu-interpreter.c       \
	script-fu-interpreter.h       \
	script-fu-interpreter-plugin.c

# link with libligma-scriptfu
# link with the usual ligma suspects

ligma_script_fu_interpreter_@LIGMA_API_VERSION@_LDADD = \
	$(libligma_scriptfu)  \
	$(libligmamath)       \
	$(libligma)           \
	$(libligmabase)       \
	$(libligmaui)         \
	$(libligmawidgets)    \
	$(libligmaconfig)     \
	$(libligmacolor)      \
	$(GTK_LIBS)          \
	$(GTK_MAC_INTEGRATION_LIBS) \
	$(RT_LIBS)           \
	$(INTLLIBS)          \
	$(exchndl)           \
	$(script_fu_interpreter_RC)

install-exec-hook:
if DEFAULT_BINARY
	cd $(DESTDIR)$(bindir) \
	&& rm -f ligma-script-fu-interpreter$(EXEEXT) \
	&& $(LN_S) ligma-script-fu-interpreter-$(LIGMA_APP_VERSION)$(EXEEXT) ligma-script-fu-interpreter$(EXEEXT)
endif

uninstall-local:
if DEFAULT_BINARY
	rm -f $(DESTDIR)$(bindir)/ligma-script-fu-interpreter$(EXEEXT)
endif
