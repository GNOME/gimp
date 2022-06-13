## Process this file with automake to produce Makefile.in

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
framework_cocoa = -framework Cocoa
endif

if OS_WIN32
mwindows = -mwindows
WINSOCK_LIBS = -lws2_32
else
libm = -lm
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

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
script_fu_server_RC = script-fu-server.rc.o
endif

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
	$(mwindows)		\
	$(framework_cocoa)	\
	$(xnone)

# Each plugin must be installed in a dir of the same name as the executable
script_fu_serverdir = $(gimpplugindir)/plug-ins/script-fu-server

script_fu_server_PROGRAMS = script-fu-server

script_fu_server_SOURCES = \
	script-fu-server.c       \
	script-fu-server.h       \
	script-fu-server-plugin.c

# link with libgimp-scriptfu and the usual gimp suspects
# Also link w socket libs
script_fu_server_LDADD = \
	$(libgimp_scriptfu)  \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimpmath)		\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(SOCKET_LIBS)		\
	$(WINSOCK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(script_fu_server_RC)
