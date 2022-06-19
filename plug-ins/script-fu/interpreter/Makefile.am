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
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
script_fu_interpreter_RC = script-fu-interpreter.rc.o
endif

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la $(libm)

# link to libscriptfu
libgimp_scriptfu = $(top_builddir)/plug-ins/script-fu/libscriptfu/libgimp-scriptfu-$(GIMP_API_VERSION).la

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
bin_PROGRAMS = gimp-script-fu-interpreter-@GIMP_API_VERSION@

gimp_script_fu_interpreter_@GIMP_API_VERSION@_SOURCES = \
	script-fu-interpreter.c       \
	script-fu-interpreter.h       \
	script-fu-interpreter-plugin.c

# link with libgimp-scriptfu
# link with the usual gimp suspects

gimp_script_fu_interpreter_@GIMP_API_VERSION@_LDADD = \
	$(libgimp_scriptfu)  \
	$(libgimpmath)       \
	$(libgimp)           \
	$(libgimpbase)       \
	$(libgimpui)         \
	$(libgimpwidgets)    \
	$(libgimpconfig)     \
	$(libgimpcolor)      \
	$(GTK_LIBS)          \
	$(GTK_MAC_INTEGRATION_LIBS) \
	$(RT_LIBS)           \
	$(INTLLIBS)          \
	$(exchndl)           \
	$(script_fu_interpreter_RC)

install-exec-hook:
if DEFAULT_BINARY
	cd $(DESTDIR)$(bindir) \
	&& rm -f gimp-script-fu-interpreter$(EXEEXT) \
	&& $(LN_S) gimp-script-fu-interpreter-$(GIMP_APP_VERSION)$(EXEEXT) gimp-script-fu-interpreter$(EXEEXT)
endif

uninstall-local:
if DEFAULT_BINARY
	rm -f $(DESTDIR)$(bindir)/gimp-script-fu-interpreter$(EXEEXT)
endif
