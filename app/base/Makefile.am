## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappbase.a

if HAVE_ASM_MMX
mmx_sources = detect-mmx.S
else
mmx_sources =
endif

libappbase_a_SOURCES = \
	base.c			\
	base.h			\
	base-enums.c		\
	base-enums.h		\
	base-types.h		\
	base-config.c		\
	base-config.h		\
	boundary.c		\
	boundary.h		\
	brush-scale.c		\
	brush-scale.h		\
	$(mmx_sources)		\
	detect-mmx.h		\
	gimphistogram.c		\
	gimphistogram.h		\
	gimplut.c		\
	gimplut.h		\
	lut-funcs.c		\
	lut-funcs.h		\
	pixel-processor.c	\
	pixel-processor.h	\
	pixel-region.c		\
	pixel-region.h		\
	pixel-surround.c	\
	pixel-surround.h	\
	temp-buf.c		\
	temp-buf.h		\
	tile.c			\
	tile.h			\
	tile-private.h		\
	tile-cache.c		\
	tile-cache.h		\
	tile-manager.c		\
	tile-manager.h		\
	tile-manager-private.h	\
	tile-manager-crop.c	\
	tile-manager-crop.h	\
	tile-swap.c		\
	tile-swap.h

EXTRA_DIST = makefile.msc

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Base\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

# for automake-1.5
AM_ASFLAGS = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app

# for automake-1.6
AM_CCASFLAGS = AM_ASFLAGS

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-bec
CLEANFILES = $(gen_sources)

$(srcdir)/base-enums.c: $(srcdir)/base-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"base-enums.h\"\n#include \"libgimp/gimpintl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "\nstatic const GEnumValue @enum_name@_enum_values[] =\n{" \
		--vprod "  { @VALUENAME@, @valuedesc@, \"@valuenick@\" }," \
		--vtail "  { 0, NULL, NULL }\n};\n\nGType\n@enum_name@_get_type (void)\n{\n  static GType enum_type = 0;\n\n  if (!enum_type)\n    enum_type = g_enum_register_static (\"@EnumName@\", @enum_name@_enum_values);\n\n  return enum_type;\n}\n" \
		$(srcdir)/base-enums.h > xgen-bec \
	&& cp xgen-bec $(@F) \
	&& rm -f xgen-bec
