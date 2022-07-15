## Process this file with automake to produce Makefile.in

# Derived from libgimp/Makefile.am and modules/Makefile.am

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
framework_cocoa = -framework Cocoa
endif

if OS_WIN32
mwindows = -mwindows
else
libm = -lm
endif

if OS_WIN32
if HAVE_EXCHNDL
exchndl = -lexchndl
endif

scriptfu_def = script-fu.def
libgimp_scriptfu_export_symbols = -export-symbols $(srcdir)/script-fu.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimp-scriptfu-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/script-fu.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimp-scriptfu-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/script-fu.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimp-scriptfu-$(GIMP_API_VERSION).lib

install-ms-lib:
	$(INSTALL) gimp-scriptfu-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimp-scriptfu-$(GIMP_API_VERSION).lib

gimp-scriptfu-@GIMP_API_VERSION@.lib: script-fu.def
	lib -name:libgimp-scriptfu-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:script-fu.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif


AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	-I$(includedir)		\
	-DG_LOG_DOMAIN=\"scriptfu\"	\
	-DSTANDALONE=0		\
	-DUSE_INTERFACE=1	\
	-DUSE_STRLWR=0

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)   \
	$(no_undefined)                    \
	$(libgimp_scriptfu_export_symbols) \
	$(mwindows)		\
	$(framework_cocoa)	\
	$(xnone)

# make convenience libraries
SUBDIRS = tinyscheme ftx

libtinyscheme = tinyscheme/libtinyscheme.la $(libm)
libftx = ftx/libftx.la

# FUTURE: install to private dir "lib" near other GIMP plugin executables
# Not doing this because it complicates packaging
# Instead, this library installs in same place as libgimp
# libdir = $(gimpplugindir)/lib

# Declare target, this library
lib_LTLIBRARIES = libgimp-scriptfu-@GIMP_API_VERSION@.la

# declare sources, independent of static versus shared
libgimp_scriptfu_@GIMP_API_VERSION@_la_SOURCES = \
	script-fu-types.h   \
	script-fu-enums.h   \
	script-fu-interface.c \
	script-fu-interface.h \
	script-fu-intl.h    \
	script-fu-regex.c   \
	script-fu-regex.h   \
	script-fu-script.c  \
	script-fu-script.h  \
	script-fu-scripts.c \
	script-fu-scripts.h \
	script-fu-utils.c   \
	script-fu-utils.h   \
	script-fu-errors.c  \
	script-fu-errors.h  \
	script-fu-compat.c  \
	script-fu-compat.h  \
	scheme-marshal.c    \
	scheme-marshal.h    \
	scheme-wrapper.c    \
	scheme-wrapper.h    \
	script-fu-lib.c     \
	script-fu-lib.h     \
	script-fu-proc-factory.h \
	script-fu-proc-factory.c \
	script-fu-arg.c     \
	script-fu-arg.h     \
	script-fu-command.h  \
	script-fu-command.c  \
	script-fu-dialog.h   \
	script-fu-dialog.c   \
	script-fu-register.h \
	script-fu-register.c \
	script-fu-run-func.h \
	script-fu-run-func.c

EXTRA_libgimp_scriptfu_@GIMP_API_VERSION@_la_DEPENDENCIES = $(scriptfu_def)

libgimp_scriptfu_@GIMP_API_VERSION@_la_LIBADD = \
	$(libtinyscheme)  \
	$(libftx)         \
	$(exchndl)

# libgimp-scriptfu depends on many other libraries, but we don't declare deps
# until exist more than just the script-fu executable using the lib

EXTRA_DIST = \
	script-fu.def

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
