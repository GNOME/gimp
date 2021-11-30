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
script_fu_RC = script-fu.rc.o
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	$(INTROSPECTION_CFLAGS)	\
	-I$(includedir)		\
	-I$(srcdir)/libscriptfu \
	-DG_LOG_DOMAIN=\"scriptfu\"

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_LDFLAGS = \
	$(mwindows)		\
	$(framework_cocoa)	\
	$(xnone)

SUBDIRS = libscriptfu scripts server interpreter

# Each plugin must be installed in a dir of the same name as the executable
script_fudir = $(gimpplugindir)/plug-ins/script-fu

script_fu_PROGRAMS = script-fu

script_fu_SOURCES = \
	script-fu.c              \
	\
	script-fu-console.c      \
	script-fu-console.h      \
	script-fu-eval.c         \
	script-fu-eval.h         \
	script-fu-text-console.h \
	script-fu-text-console.c


# link with libgimp-scriptfu and the usual gimp suspects
script_fu_LDADD = \
	$(libgimp_scriptfu)  \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimpmath)		\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(INTROSPECTION_LIBS)	\
	$(script_fu_RC)


# Perform static analysis on all *.scm files and look for usage of
# deprecated pdb procedures
check-for-deprecated-procedures-in-script-fu:
	@echo "Looking for deprecated procedures in *.scm files"
	@scm_files=`find $(top_srcdir)/plug-ins/script-fu -name "*.scm"`;       \
	deprecated_procs=`$(top_builddir)/app/gimp-$(GIMP_APP_VERSION)$(EXEEXT) --dump-pdb-procedures-deprecated`; \
	for scm_file in $$scm_files; do                                         \
	  for proc in $$deprecated_procs; do                                    \
	    if grep -Eq "^([^;]*[[:blank:](])?$$proc([[:blank:])]|$$)" $$scm_file; then \
	      echo "$${scm_file} uses deprecated procedure '$${proc}'";         \
	    fi                                                                  \
	  done                                                                  \
	done
