## Process this file with automake to produce Makefile.in

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
endif

AM_CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"			\
	-DGIMP_APP_VERSION=\"$(GIMP_APP_VERSION)\"	\
	-DGIMP_USER_VERSION=\"$(GIMP_USER_VERSION)\"	\
	-DG_LOG_DOMAIN=\"Gimp-Core\"			\
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_builddir)/app				\
	-I$(top_srcdir)/app				\
	$(CAIRO_CFLAGS)					\
	$(GEGL_CFLAGS)					\
	$(GDK_PIXBUF_CFLAGS)				\
	$(LIBMYPAINT_CFLAGS)			\
	$(GEXIV2_CFLAGS)				\
	$(LCMS_CFLAGS)					\
	-I$(includedir)					\
	$(xobjective_c)

noinst_LIBRARIES = libappcore.a

libappcore_a_sources = \
	core-enums.h				\
	core-types.h				\
	gimp.c					\
	gimp.h					\
	gimp-batch.c				\
	gimp-batch.h				\
	gimp-cairo.c				\
	gimp-cairo.h				\
	gimp-contexts.c				\
	gimp-contexts.h				\
	gimp-edit.c				\
	gimp-edit.h				\
	gimp-filter-history.c			\
	gimp-filter-history.h			\
	gimp-gradients.c			\
	gimp-gradients.h			\
	gimp-gui.c				\
	gimp-gui.h				\
	gimp-memsize.c				\
	gimp-memsize.h				\
	gimp-modules.c				\
	gimp-modules.h				\
	gimp-palettes.c				\
	gimp-palettes.h				\
	gimp-parasites.c			\
	gimp-parasites.h			\
	gimp-tags.c				\
	gimp-tags.h				\
	gimp-templates.c			\
	gimp-templates.h			\
	gimp-transform-resize.c			\
	gimp-transform-resize.h			\
	gimp-transform-utils.c			\
	gimp-transform-utils.h			\
	gimp-units.c				\
	gimp-units.h				\
	gimp-user-install.c			\
	gimp-user-install.h			\
	gimp-utils.c				\
	gimp-utils.h				\
	gimpbezierdesc.h			\
	gimpbezierdesc.c			\
	gimpboundary.c				\
	gimpboundary.h				\
	gimpbrush.c				\
	gimpbrush.h				\
	gimpbrush-boundary.c			\
	gimpbrush-boundary.h			\
	gimpbrush-header.h			\
	gimpbrush-load.c			\
	gimpbrush-load.h			\
	gimpbrush-private.h			\
	gimpbrush-transform.c			\
	gimpbrush-transform.h			\
	gimpbrushcache.c			\
	gimpbrushcache.h			\
	gimpbrushclipboard.c			\
	gimpbrushclipboard.h			\
	gimpbrushgenerated.c			\
	gimpbrushgenerated.h			\
	gimpbrushgenerated-load.c		\
	gimpbrushgenerated-load.h		\
	gimpbrushgenerated-save.c		\
	gimpbrushgenerated-save.h		\
	gimpbrushpipe.c				\
	gimpbrushpipe.h				\
	gimpbrushpipe-load.c			\
	gimpbrushpipe-load.h			\
	gimpbuffer.c				\
	gimpbuffer.h				\
	gimpchannel.c				\
	gimpchannel.h				\
	gimpchannel-combine.c			\
	gimpchannel-combine.h			\
	gimpchannel-select.c			\
	gimpchannel-select.h			\
	gimpchannelpropundo.c			\
	gimpchannelpropundo.h			\
	gimpchannelundo.c			\
	gimpchannelundo.h			\
	gimpcontainer.c				\
	gimpcontainer.h				\
	gimpcontainer-filter.c			\
	gimpcontainer-filter.h			\
	gimpcontext.c				\
	gimpcontext.h				\
	gimpcoords.c				\
	gimpcoords.h				\
	gimpcoords-interpolate.c		\
	gimpcoords-interpolate.h		\
	gimpcurve.c				\
	gimpcurve.h				\
	gimpcurve-load.c			\
	gimpcurve-load.h			\
	gimpcurve-map.c				\
	gimpcurve-map.h				\
	gimpcurve-save.c			\
	gimpcurve-save.h			\
	gimpdashpattern.c			\
	gimpdashpattern.h			\
	gimpdata.c				\
	gimpdata.h				\
	gimpdatafactory.c			\
	gimpdatafactory.h			\
	gimpdocumentlist.c			\
	gimpdocumentlist.h			\
	gimpdrawable.c				\
	gimpdrawable.h				\
	gimpdrawable-blend.c			\
	gimpdrawable-blend.h			\
	gimpdrawable-bucket-fill.c		\
	gimpdrawable-bucket-fill.h		\
	gimpdrawable-combine.c			\
	gimpdrawable-combine.h			\
	gimpdrawable-equalize.c			\
	gimpdrawable-equalize.h			\
	gimpdrawable-fill.c			\
	gimpdrawable-fill.h			\
	gimpdrawable-filters.c			\
	gimpdrawable-filters.h			\
	gimpdrawable-floating-selection.c	\
	gimpdrawable-floating-selection.h	\
	gimpdrawable-foreground-extract.c	\
	gimpdrawable-foreground-extract.h	\
	gimpdrawable-histogram.c		\
	gimpdrawable-histogram.h		\
	gimpdrawable-levels.c			\
	gimpdrawable-levels.h			\
	gimpdrawable-offset.c			\
	gimpdrawable-offset.h			\
	gimpdrawable-operation.c		\
	gimpdrawable-operation.h		\
	gimpdrawable-preview.c			\
	gimpdrawable-preview.h			\
	gimpdrawable-private.h			\
	gimpdrawable-shadow.c			\
	gimpdrawable-shadow.h			\
	gimpdrawable-stroke.c			\
	gimpdrawable-stroke.h			\
	gimpdrawable-transform.c		\
	gimpdrawable-transform.h		\
	gimpdrawablefilter.c			\
	gimpdrawablefilter.h			\
	gimpdrawablemodundo.c			\
	gimpdrawablemodundo.h			\
	gimpdrawablestack.c			\
	gimpdrawablestack.h			\
	gimpdrawableundo.c			\
	gimpdrawableundo.h			\
	gimpdynamics.c				\
	gimpdynamics.h				\
	gimpdynamics-load.c			\
	gimpdynamics-load.h			\
	gimpdynamics-save.c			\
	gimpdynamics-save.h			\
	gimpdynamicsoutput.c			\
	gimpdynamicsoutput.h			\
	gimperror.c				\
	gimperror.h				\
	gimpfilloptions.c			\
	gimpfilloptions.h			\
	gimpfilter.c				\
	gimpfilter.h				\
	gimpfilteredcontainer.c			\
	gimpfilteredcontainer.h			\
	gimpfilterstack.c			\
	gimpfilterstack.h			\
	gimpfloatingselectionundo.c		\
	gimpfloatingselectionundo.h		\
	gimpgradient.c				\
	gimpgradient.h				\
	gimpgradient-load.c			\
	gimpgradient-load.h			\
	gimpgradient-save.c			\
	gimpgradient-save.h			\
	gimpgrid.c				\
	gimpgrid.h				\
	gimpgrouplayer.c			\
	gimpgrouplayer.h			\
	gimpgrouplayerundo.c			\
	gimpgrouplayerundo.h			\
	gimpguide.c				\
	gimpguide.h				\
	gimpguideundo.c				\
	gimpguideundo.h				\
	gimphistogram.c				\
	gimphistogram.h				\
	gimpidtable.c				\
	gimpidtable.h				\
	gimpimage.c				\
	gimpimage.h				\
	gimpimage-arrange.c			\
	gimpimage-arrange.h			\
	gimpimage-color-profile.c		\
	gimpimage-color-profile.h		\
	gimpimage-colormap.c			\
	gimpimage-colormap.h			\
	gimpimage-convert-indexed.c		\
	gimpimage-convert-indexed.h		\
	gimpimage-convert-fsdither.h		\
	gimpimage-convert-data.h		\
	gimpimage-convert-precision.c		\
	gimpimage-convert-precision.h		\
	gimpimage-convert-type.c		\
	gimpimage-convert-type.h		\
	gimpimage-crop.c			\
	gimpimage-crop.h			\
	gimpimage-duplicate.c			\
	gimpimage-duplicate.h			\
	gimpimage-flip.c			\
	gimpimage-flip.h			\
	gimpimage-grid.h                	\
	gimpimage-grid.c                	\
	gimpimage-guides.c			\
	gimpimage-guides.h			\
	gimpimage-item-list.c			\
	gimpimage-item-list.h			\
	gimpimage-merge.c			\
	gimpimage-merge.h			\
	gimpimage-metadata.c			\
	gimpimage-metadata.h			\
	gimpimage-new.c				\
	gimpimage-new.h				\
	gimpimage-pick-color.c			\
	gimpimage-pick-color.h			\
	gimpimage-pick-layer.c			\
	gimpimage-pick-layer.h			\
	gimpimage-preview.c			\
	gimpimage-preview.h			\
	gimpimage-private.h			\
	gimpimage-quick-mask.c			\
	gimpimage-quick-mask.h			\
	gimpimage-resize.c			\
	gimpimage-resize.h			\
	gimpimage-rotate.c			\
	gimpimage-rotate.h			\
	gimpimage-sample-points.c		\
	gimpimage-sample-points.h		\
	gimpimage-scale.c			\
	gimpimage-scale.h			\
	gimpimage-snap.c			\
	gimpimage-snap.h			\
	gimpimage-symmetry.c			\
	gimpimage-symmetry.h			\
	gimpimage-undo.c			\
	gimpimage-undo.h			\
	gimpimage-undo-push.c			\
	gimpimage-undo-push.h			\
	gimpimageundo.c				\
	gimpimageundo.h				\
	gimpimagefile.c				\
	gimpimagefile.h				\
	gimpitem.c				\
	gimpitem.h				\
	gimpitem-exclusive.c			\
	gimpitem-exclusive.h			\
	gimpitem-linked.c			\
	gimpitem-linked.h			\
	gimpitem-preview.c			\
	gimpitem-preview.h			\
	gimpitempropundo.c			\
	gimpitempropundo.h			\
	gimpitemstack.c				\
	gimpitemstack.h				\
	gimpitemtree.c				\
	gimpitemtree.h				\
	gimpitemundo.c				\
	gimpitemundo.h				\
	gimplayer.c				\
	gimplayer.h				\
	gimplayer-floating-selection.c		\
	gimplayer-floating-selection.h		\
	gimplayer-new.c				\
	gimplayer-new.h				\
	gimplayermask.c				\
	gimplayermask.h				\
	gimplayermaskpropundo.c			\
	gimplayermaskpropundo.h			\
	gimplayermaskundo.c			\
	gimplayermaskundo.h			\
	gimplayerpropundo.c			\
	gimplayerpropundo.h			\
	gimplayerundo.c				\
	gimplayerundo.h				\
	gimplist.c				\
	gimplist.h				\
	gimpmaskundo.c				\
	gimpmaskundo.h				\
	gimpmybrush.c				\
	gimpmybrush.h				\
	gimpmybrush-load.c			\
	gimpmybrush-load.h			\
	gimpmybrush-private.h			\
	gimpobject.c				\
	gimpobject.h				\
	gimppaintinfo.c				\
	gimppaintinfo.h				\
	gimppattern.c				\
	gimppattern.h				\
	gimppattern-header.h			\
	gimppattern-load.c			\
	gimppattern-load.h			\
	gimppatternclipboard.c			\
	gimppatternclipboard.h			\
	gimppalette.c				\
	gimppalette.h				\
	gimppalette-import.c			\
	gimppalette-import.h			\
	gimppalette-load.c			\
	gimppalette-load.h			\
	gimppalette-save.c			\
	gimppalette-save.h			\
	gimppalettemru.c			\
	gimppalettemru.h			\
	gimpparamspecs.c			\
	gimpparamspecs.h			\
	gimpparamspecs-desc.c			\
	gimpparamspecs-desc.h			\
	gimpparamspecs-duplicate.c		\
	gimpparamspecs-duplicate.h		\
	gimpparasitelist.c			\
	gimpparasitelist.h			\
	gimppdbprogress.c			\
	gimppdbprogress.h			\
	gimppickable.c				\
	gimppickable.h				\
	gimppickable-auto-shrink.c		\
	gimppickable-auto-shrink.h		\
	gimppickable-contiguous-region.c	\
	gimppickable-contiguous-region.h	\
	gimpprogress.c				\
	gimpprogress.h				\
	gimpprojectable.c			\
	gimpprojectable.h			\
	gimpprojection.c			\
	gimpprojection.h			\
	gimpsamplepoint.c			\
	gimpsamplepoint.h			\
	gimpsamplepointundo.c			\
	gimpsamplepointundo.h			\
	gimpscanconvert.c			\
	gimpscanconvert.h			\
	gimpselection.c				\
	gimpselection.h				\
	gimpsettings.c				\
	gimpsettings.h				\
	gimpstrokeoptions.c			\
	gimpstrokeoptions.h			\
	gimpsubprogress.c			\
	gimpsubprogress.h			\
	gimpsymmetry.c				\
	gimpsymmetry.h				\
	gimpsymmetry-mandala.c			\
	gimpsymmetry-mandala.h			\
	gimpsymmetry-mirror.c			\
	gimpsymmetry-mirror.h			\
	gimpsymmetry-tiling.c			\
	gimpsymmetry-tiling.h			\
	gimptag.c				\
	gimptag.h				\
	gimptagcache.c				\
	gimptagcache.h				\
	gimptagged.c				\
	gimptagged.h				\
	gimptaggedcontainer.c			\
	gimptaggedcontainer.h			\
	gimptempbuf.c				\
	gimptempbuf.h				\
	gimptemplate.c				\
	gimptemplate.h				\
	gimptoolinfo.c				\
	gimptoolinfo.h				\
	gimptooloptions.c			\
	gimptooloptions.h			\
	gimptoolpreset.c			\
	gimptoolpreset.h			\
	gimptoolpreset-load.c			\
	gimptoolpreset-load.h			\
	gimptoolpreset-save.c			\
	gimptoolpreset-save.h			\
	gimptreehandler.c			\
	gimptreehandler.h			\
	gimpunit.c				\
	gimpunit.h				\
	gimpundo.c				\
	gimpundo.h				\
	gimpundostack.c				\
	gimpundostack.h				\
	gimpviewable.c				\
	gimpviewable.h

libappcore_a_built_sources = \
	core-enums.c	\
	gimpmarshal.c	\
	gimpmarshal.h

libappcore_a_extra_sources = \
	gimpmarshal.list

libappcore_a_SOURCES = $(libappcore_a_built_sources) $(libappcore_a_sources)

EXTRA_DIST = \
	$(libappcore_a_extra_sources)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-gmh xgen-gmc xgen-cec
CLEANFILES = $(gen_sources)

gimpmarshal.h: $(srcdir)/gimpmarshal.list
	$(GLIB_GENMARSHAL) --prefix=gimp_marshal $(srcdir)/gimpmarshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh $(@F) || cp xgen-gmh $(@F)) \
	&& rm -f xgen-gmh xgen-gmh~

gimpmarshal.c: gimpmarshal.h
	echo "#include \"gimpmarshal.h\"" >> xgen-gmc \
	&& $(GLIB_GENMARSHAL) --prefix=gimp_marshal $(srcdir)/gimpmarshal.list --body >> xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc xgen-gmc~

core-enums.c: $(srcdir)/core-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gio/gio.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"core-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$(srcdir)/core-enums.h > xgen-cec \
	&& cp xgen-cec $(@F) \
	&& rm -f xgen-cec
