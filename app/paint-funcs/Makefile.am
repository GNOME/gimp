## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libapppaint-funcs.a

if HAVE_ASM_MMX
mmx_sources = \
	paint-funcs-simd.S
else
mmx_sources =
endif

libapppaint_funcs_a_SOURCES = @STRIP_BEGIN@ \
	paint-funcs.c		\
	paint-funcs.h		\
	$(mmx_sources)		\
@STRIP_END@

EXTRA_DIST = \
	makefile.msc		\
	paint-funcs-simd.S

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-Paint-Funcs\"	\
	@GIMP_THREAD_FLAGS@ 			\
	@GIMP_MP_FLAGS@				\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GLIB_CFLAGS)		\
	-I$(includedir)		\
@STRIP_END@
