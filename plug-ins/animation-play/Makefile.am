AUTOMAKE_OPTIONS = subdir-objects

if OS_WIN32
mwindows = -mwindows
else
libm = -lm
endif

libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la $(libm)
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = animation-play

AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS) \
	-I$(includedir)

LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpmodule)	\
	$(libgimp)		\
	$(libgimpmath)		\
	$(libgimpconfig)	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(animation_play_RC)

animation_play_SOURCES = \
	core/animation.h				\
	core/animation.c				\
	core/animationanimatic.h		\
	core/animationanimatic.c		\
	core/animation-celanimation.h	\
	core/animation-celanimation.c	\
	widgets/animation-dialog.h		\
	widgets/animation-dialog.c		\
	widgets/animation-storyboard.h	\
	widgets/animation-storyboard.c	\
	animation-utils.h				\
	animation-utils.c				\
	animation-play.c
