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

libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la $(libm)

# link to libscriptfu
libligma_scriptfu = $(top_builddir)/plug-ins/script-fu/libscriptfu/libligma-scriptfu-$(LIGMA_API_VERSION).la

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
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
script_fu_serverdir = $(ligmaplugindir)/plug-ins/script-fu-server

script_fu_server_PROGRAMS = script-fu-server

script_fu_server_SOURCES = \
	script-fu-server.c       \
	script-fu-server.h       \
	script-fu-server-plugin.c

# link with libligma-scriptfu and the usual ligma suspects
# Also link w socket libs
script_fu_server_LDADD = \
	$(libligma_scriptfu)  \
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligmamath)		\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmabase)		\
	$(GTK_LIBS)		\
	$(SOCKET_LIBS)		\
	$(WINSOCK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(script_fu_server_RC)
