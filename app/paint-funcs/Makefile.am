## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libapppaint-funcs.a

if HAVE_ASM_MMX
mmx_sources = paint-funcs-simd.S
else
mmx_sources =
endif

libapppaint_funcs_a_SOURCES = \
	paint-funcs-types.h	\
	paint-funcs.c		\
	paint-funcs.h		\
	paint-funcs-generic.h	\
	paint-funcs-mmx.h	\
	$(mmx_sources)

EXTRA_DIST = \
	makefile.msc		\
	paint-funcs-simd.S

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Paint-Funcs\"	\
	@GIMP_THREAD_FLAGS@ 			\
	@GIMP_MP_FLAGS@

# for automake-1.5
AM_ASFLAGS = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app

# for automake-1.6
AM_CCASFLAGS = $(AM_ASFLAGS)

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GLIB_CFLAGS)		\
	-I$(includedir)
