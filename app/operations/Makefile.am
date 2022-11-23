## Process this file with automake to produce Makefile.in

SUBDIRS = \
	layer-modes		\
	layer-modes-legacy	\
	tests

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Ligma-Operations\"	\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-I$(top_builddir)/app			\
	-I$(top_srcdir)/app			\
	$(CAIRO_CFLAGS)				\
	$(GEGL_CFLAGS)				\
	$(GDK_PIXBUF_CFLAGS)			\
	-I$(includedir)

noinst_LIBRARIES = \
	libappoperations.a

libappoperations_a_sources = \
	operations-types.h			\
	operations-enums.h			\
	ligma-operations.c			\
	ligma-operations.h			\
	\
	ligma-operation-config.c			\
	ligma-operation-config.h			\
	ligmaoperationsettings.c			\
	ligmaoperationsettings.h			\
	ligmabrightnesscontrastconfig.c		\
	ligmabrightnesscontrastconfig.h		\
	ligmacageconfig.c			\
	ligmacageconfig.h			\
	ligmacolorbalanceconfig.c		\
	ligmacolorbalanceconfig.h		\
	ligmacurvesconfig.c			\
	ligmacurvesconfig.h			\
	ligmahuesaturationconfig.c		\
	ligmahuesaturationconfig.h		\
	ligmalevelsconfig.c			\
	ligmalevelsconfig.h			\
	\
	ligmaoperationborder.c			\
	ligmaoperationborder.h			\
	ligmaoperationbuffersourcevalidate.c	\
	ligmaoperationbuffersourcevalidate.h	\
	ligmaoperationcagecoefcalc.c		\
	ligmaoperationcagecoefcalc.h		\
	ligmaoperationcagetransform.c		\
	ligmaoperationcagetransform.h		\
	ligmaoperationcomposecrop.c		\
	ligmaoperationcomposecrop.h		\
	ligmaoperationequalize.c			\
	ligmaoperationequalize.h			\
	ligmaoperationfillsource.c		\
	ligmaoperationfillsource.h		\
	ligmaoperationflood.c			\
	ligmaoperationflood.h			\
	ligmaoperationgradient.c			\
	ligmaoperationgradient.h			\
	ligmaoperationgrow.c			\
	ligmaoperationgrow.h			\
	ligmaoperationhistogramsink.c		\
	ligmaoperationhistogramsink.h		\
	ligmaoperationmaskcomponents.cc		\
	ligmaoperationmaskcomponents.h		\
	ligmaoperationoffset.c			\
	ligmaoperationoffset.h			\
	ligmaoperationprofiletransform.c		\
	ligmaoperationprofiletransform.h		\
	ligmaoperationscalarmultiply.c		\
	ligmaoperationscalarmultiply.h		\
	ligmaoperationsemiflatten.c		\
	ligmaoperationsemiflatten.h		\
	ligmaoperationsetalpha.c			\
	ligmaoperationsetalpha.h			\
	ligmaoperationshrink.c			\
	ligmaoperationshrink.h			\
	ligmaoperationthresholdalpha.c		\
	ligmaoperationthresholdalpha.h		\
	\
	ligmaoperationpointfilter.c		\
	ligmaoperationpointfilter.h		\
	ligmaoperationbrightnesscontrast.c	\
	ligmaoperationbrightnesscontrast.h	\
	ligmaoperationcolorbalance.c		\
	ligmaoperationcolorbalance.h		\
	ligmaoperationcolorize.c			\
	ligmaoperationcolorize.h			\
	ligmaoperationcurves.c			\
	ligmaoperationcurves.h			\
	ligmaoperationdesaturate.c		\
	ligmaoperationdesaturate.h		\
	ligmaoperationhuesaturation.c		\
	ligmaoperationhuesaturation.h		\
	ligmaoperationlevels.c			\
	ligmaoperationlevels.h			\
	ligmaoperationposterize.c		\
	ligmaoperationposterize.h		\
	ligmaoperationthreshold.c		\
	ligmaoperationthreshold.h

libappoperations_a_built_sources = operations-enums.c

libappoperations_a_SOURCES = \
	$(libappoperations_a_built_sources)	\
	$(libappoperations_a_sources)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = stamp-operations-enums.h xgen-oec
CLEANFILES = $(gen_sources)

xgen-oec: $(srcdir)/operations-enums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-operations-enums.h\"\n#include \"config.h\"\n#include <gio/gio.h>\n#include \"libligmabase/ligmabase.h\"\n#include \"operations-enums.h\"\n#include \"ligma-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-operations-enums.h: xgen-oec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/operations-enums.c: xgen-oec stamp-operations-enums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
